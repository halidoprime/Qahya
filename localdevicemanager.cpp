#include "localdevicemanager.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>

namespace {
const char kArpMarker[] = "@@QCONTROL-ARP-SCAN@@";
const char kNmapMarker[] = "@@QCONTROL-NMAP@@";
const char kEndMarker[] = "@@QCONTROL-END@@";

// A routed /19 is about 8190 hosts, which takes many minutes per tool. Partial results are kept on timeout.
const int kScanTimeoutMs = 600000;
// After the timeout closes the script's stdin, wait this long for it to stop its tools before killing pkexec.
const int kStopGraceMs = 15000;

// Runs inside pkexec (root). Every argument after the tool paths is "<interface>=<cidr>".
// arp-scan and nmap each run once per subnet on that subnet's interface. A failing run does
// not stop the others. Markers separate the two tools' output. LC_ALL=C keeps output parseable.
//
// Stop handling: the script copies its stdin, and a watcher waits for it to close. The manager
// closes that pipe on timeout. The watcher then signals the script, whose trap kills the running
// tool (its children) and exits. pkill is from procps.
const char kScanScript[] =
    "LC_ALL=C; export LC_ALL; "
    "arp=$1; nmap=$2; shift 2; "
    "exec 3<&0; "
    "trap 'pkill -TERM -P $$ >/dev/null 2>&1; exit 143' TERM; "
    "( cat <&3 >/dev/null 2>&1; kill -TERM $$ >/dev/null 2>&1 ) & "
    "watcher=$!; "
    "echo '@@QCONTROL-ARP-SCAN@@'; "
    "for spec in \"$@\"; do "
    "  dev=${spec%%=*}; cidr=${spec#*=}; "
    "  if [ -n \"$arp\" ]; then \"$arp\" --interface=\"$dev\" \"$cidr\" & wait $!; fi; "
    "done; "
    "echo '@@QCONTROL-NMAP@@'; "
    "for spec in \"$@\"; do "
    "  dev=${spec%%=*}; cidr=${spec#*=}; "
    "  if [ -n \"$nmap\" ]; then \"$nmap\" -sn -T4 -e \"$dev\" \"$cidr\" & wait $!; fi; "
    "done; "
    "echo '@@QCONTROL-END@@'; "
    "kill $watcher >/dev/null 2>&1; "
    "exit 0";

const char kScopeKey[] = "locals/scope";

// Searches PATH plus the sbin directories, which are often missing from a user's PATH
// but hold arp-scan on some distributions.
QString findTool(const QString &name)
{
    QStringList dirs = qEnvironmentVariable("PATH").split(QLatin1Char(':'), Qt::SkipEmptyParts);
    dirs << QStringLiteral("/usr/local/sbin") << QStringLiteral("/usr/sbin") << QStringLiteral("/sbin");
    return QStandardPaths::findExecutable(name, dirs);
}

QString describeFailure(const QByteArray &error, int exitCode, const QString &fallback)
{
    const QString details = QString::fromLocal8Bit(error).trimmed();
    return details.isEmpty() ? QStringLiteral("%1 (exit code %2)").arg(fallback).arg(exitCode) : details;
}

// Parses `ip -j route show` and returns "<interface>=<cidr>" for each directly connected IPv4
// subnet. Nothing is hardcoded: the subnets are whatever the kernel reports at scan time.
// RoutedSubnet uses the route's own prefix (/16 to /30 only, so a huge route cannot trigger a
// huge scan). Local24 uses the /24 containing the route's preferred source address.
QStringList subnetSpecs(const QByteArray &output, LocalDeviceManager::ScanScope scope)
{
    static const QRegularExpression cidrRe(QStringLiteral("^\\d{1,3}(?:\\.\\d{1,3}){3}/(\\d{1,2})$"));
    static const QRegularExpression ipv4Re(QStringLiteral("^\\d{1,3}(?:\\.\\d{1,3}){3}$"));
    static const QRegularExpression devRe(QStringLiteral("^[A-Za-z0-9_.:@-]+$"));

    QStringList specs;
    const QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isArray())
        return specs;

    for (const QJsonValue &value : doc.array()) {
        const QJsonObject route = value.toObject();
        const QString dst = route.value(QStringLiteral("dst")).toString();
        const QString dev = route.value(QStringLiteral("dev")).toString();
        const QRegularExpressionMatch match = cidrRe.match(dst);
        if (!match.hasMatch() || route.value(QStringLiteral("scope")).toString() != QLatin1String("link")
            || dev == QLatin1String("lo") || !devRe.match(dev).hasMatch())
            continue;

        QString network;
        if (scope == LocalDeviceManager::ScanScope::Local24) {
            QString address = route.value(QStringLiteral("prefsrc")).toString();
            if (!ipv4Re.match(address).hasMatch())
                address = dst.section(QLatin1Char('/'), 0, 0);
            network = address.section(QLatin1Char('.'), 0, 2) + QStringLiteral(".0/24");
        } else {
            const int prefix = match.captured(1).toInt();
            if (prefix < 16 || prefix > 30)
                continue;
            network = dst;
        }

        const QString spec = dev + QLatin1Char('=') + network;
        if (!specs.contains(spec))
            specs.append(spec);
    }
    return specs;
}

void addMethod(LocalDeviceInfo &device, const QString &tool)
{
    if (device.method.isEmpty())
        device.method = tool;
    else if (!device.method.split(QStringLiteral(" + ")).contains(tool))
        device.method += QStringLiteral(" + ") + tool;
}

quint32 ipToNumber(const QString &ip)
{
    quint32 value = 0;
    for (const QString &octet : ip.split(QLatin1Char('.')))
        value = (value << 8) | octet.toUInt();
    return value;
}

// Parses the output of arp-scan (tab-separated "IP<TAB>MAC<TAB>vendor") and nmap -sn
// ("Nmap scan report for host (ip)" followed by an optional "MAC Address: ..." line).
// Results are merged by IP and sorted numerically.
QVector<LocalDeviceInfo> parseScanOutput(const QByteArray &output)
{
    static const QRegularExpression ipRe(QStringLiteral("^\\d{1,3}(?:\\.\\d{1,3}){3}$"));
    static const QRegularExpression macRe(QStringLiteral("^[0-9A-Fa-f]{2}(?::[0-9A-Fa-f]{2}){5}$"));
    static const QRegularExpression dupRe(QStringLiteral("\\s*\\(DUP:[^)]*\\)$"));
    static const QRegularExpression nmapReportRe(
        QStringLiteral("^Nmap scan report for (.+?)(?: \\((\\d{1,3}(?:\\.\\d{1,3}){3})\\))?$"));
    static const QRegularExpression nmapMacRe(
        QStringLiteral("^MAC Address: ([0-9A-Fa-f:]{17})(?: \\((.*)\\))?$"));

    enum class Section { None, Arp, Nmap };
    Section section = Section::None;
    QString currentNmapIp;
    QHash<QString, LocalDeviceInfo> devices;

    const QStringList lines = QString::fromLocal8Bit(output).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &rawLine : lines) {
        const QString trimmed = rawLine.trimmed();
        if (trimmed == QLatin1String(kArpMarker)) {
            section = Section::Arp;
            continue;
        }
        if (trimmed == QLatin1String(kNmapMarker)) {
            section = Section::Nmap;
            continue;
        }
        if (trimmed == QLatin1String(kEndMarker)) {
            section = Section::None;
            continue;
        }

        if (section == Section::Arp) {
            // Split the raw line: the vendor column may be empty, and trimming would hide that.
            const QStringList fields = rawLine.split(QLatin1Char('\t'));
            if (fields.size() < 2)
                continue;
            const QString ip = fields.at(0).trimmed();
            const QString mac = fields.at(1).trimmed();
            if (!ipRe.match(ip).hasMatch() || !macRe.match(mac).hasMatch())
                continue;
            const QString vendor = fields.value(2).trimmed().remove(dupRe);

            LocalDeviceInfo &device = devices[ip];
            device.ip = ip;
            if (device.mac.isEmpty())
                device.mac = mac;
            if (device.vendor.isEmpty())
                device.vendor = vendor;
            addMethod(device, QStringLiteral("arp-scan"));
        } else if (section == Section::Nmap) {
            const QRegularExpressionMatch report = nmapReportRe.match(trimmed);
            if (report.hasMatch()) {
                const bool hasParenIp = !report.captured(2).isEmpty();
                const QString ip = hasParenIp ? report.captured(2) : report.captured(1);
                currentNmapIp.clear();
                if (!ipRe.match(ip).hasMatch())
                    continue;

                currentNmapIp = ip;
                LocalDeviceInfo &device = devices[ip];
                device.ip = ip;
                if (hasParenIp && device.hostname.isEmpty())
                    device.hostname = report.captured(1);
                addMethod(device, QStringLiteral("nmap"));
                continue;
            }

            const QRegularExpressionMatch mac = nmapMacRe.match(trimmed);
            if (mac.hasMatch() && !currentNmapIp.isEmpty()) {
                LocalDeviceInfo &device = devices[currentNmapIp];
                if (device.mac.isEmpty())
                    device.mac = mac.captured(1);
                if (device.vendor.isEmpty())
                    device.vendor = mac.captured(2);
            }
        }
    }

    const QList<LocalDeviceInfo> values = devices.values();
    QVector<LocalDeviceInfo> result(values.cbegin(), values.cend());
    std::sort(result.begin(), result.end(), [](const LocalDeviceInfo &a, const LocalDeviceInfo &b) {
        return ipToNumber(a.ip) < ipToNumber(b.ip);
    });
    return result;
}
} // namespace

LocalDeviceManager::LocalDeviceManager(QObject *parent)
    : QObject(parent)
    , m_ipPath(QStandardPaths::findExecutable(QStringLiteral("ip")))
    , m_pkexecPath(QStandardPaths::findExecutable(QStringLiteral("pkexec")))
    , m_shPath(QStandardPaths::findExecutable(QStringLiteral("sh")))
    , m_arpScanPath(findTool(QStringLiteral("arp-scan")))
    , m_nmapPath(findTool(QStringLiteral("nmap")))
    , m_scanTimer(new QTimer(this))
    , m_stopTimer(new QTimer(this))
{
    if (m_ipPath.isEmpty())
        m_availabilityError = tr("ip could not be found. Local network scanning is unavailable.");
    else if (m_pkexecPath.isEmpty())
        m_availabilityError = tr("pkexec could not be found. Local network scanning is unavailable.");
    else if (m_arpScanPath.isEmpty() && m_nmapPath.isEmpty())
        m_availabilityError = tr("Neither arp-scan nor nmap could be found. Install one of them to scan the local network.");

    QSettings settings(QStringLiteral("QControl"), QStringLiteral("QControl"));
    m_scope = settings.value(QLatin1String(kScopeKey), 0).toInt() == 1 ? ScanScope::Local24
                                                                         : ScanScope::RoutedSubnet;

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    m_routeProcess.setProcessEnvironment(environment);

    m_scanTimer->setSingleShot(true);
    m_scanTimer->setInterval(kScanTimeoutMs);
    connect(m_scanTimer, &QTimer::timeout, this, [this]() {
        m_timedOut = true;
        if (m_routeProcess.state() != QProcess::NotRunning)
            m_routeProcess.kill();
        if (m_scanProcess.state() != QProcess::NotRunning) {
            // Closing stdin tells the script to stop its tools. Kill pkexec only if it does not exit.
            m_scanProcess.closeWriteChannel();
            m_stopTimer->start();
        }
    });

    m_stopTimer->setSingleShot(true);
    m_stopTimer->setInterval(kStopGraceMs);
    connect(m_stopTimer, &QTimer::timeout, this, [this]() {
        if (m_scanProcess.state() != QProcess::NotRunning)
            m_scanProcess.kill();
    });

    // Step 1: unprivileged route listing to find the subnets to scan.
    connect(&m_routeProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                if (m_timedOut) {
                    finishScan(false, tr("Scan timed out and was stopped."));
                    return;
                }
                const QByteArray output = m_routeProcess.readAllStandardOutput();
                const QByteArray error = m_routeProcess.readAllStandardError();
                if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                    finishScan(false, describeFailure(error, exitCode, tr("Could not read the routing table.")));
                    return;
                }

                const QStringList specs = subnetSpecs(output, m_scope);
                if (specs.isEmpty()) {
                    finishScan(false, tr("No local IPv4 subnet found to scan."));
                    return;
                }
                startPrivilegedScan(specs);
            });

    connect(&m_routeProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && m_scanRunning)
            finishScan(false, tr("Could not start ip: %1").arg(m_routeProcess.errorString()));
    });

    // Step 2: one pkexec call (one password prompt) runs both tools.
    connect(&m_scanProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                const QByteArray output = m_scanProcess.readAllStandardOutput();
                const QString errorText = QString::fromLocal8Bit(m_scanProcess.readAllStandardError()).trimmed();
                m_warnings = errorText;

                if (m_timedOut) {
                    // Keep whatever finished before the timeout instead of discarding it.
                    m_devices = parseScanOutput(output);
                    m_lastScan = QDateTime::currentDateTime();
                    m_loaded = true;
                    finishScan(false, tr("Scan timed out after 10 minutes and was stopped. Showing %1 partial device(s).")
                                          .arg(m_devices.size()));
                    return;
                }
                if (exitStatus != QProcess::NormalExit || exitCode != 0 || !output.contains(kEndMarker)) {
                    finishScan(false, describeFailure(errorText.toLocal8Bit(), exitCode, tr("Network scan failed.")));
                    return;
                }

                const QVector<LocalDeviceInfo> found = parseScanOutput(output);
                if (found.isEmpty() && !errorText.isEmpty()) {
                    finishScan(false, errorText);
                    return;
                }

                m_devices = found;
                m_lastScan = QDateTime::currentDateTime();
                m_loaded = true;
                finishScan(true, tr("Scan complete: %1 device(s) found.").arg(found.size()));
            });

    connect(&m_scanProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && m_scanRunning)
            finishScan(false, tr("Could not start pkexec: %1").arg(m_scanProcess.errorString()));
    });
}

const QVector<LocalDeviceInfo> &LocalDeviceManager::devices() const
{
    return m_devices;
}

bool LocalDeviceManager::isLoaded() const
{
    return m_loaded;
}

QDateTime LocalDeviceManager::lastScan() const
{
    return m_lastScan;
}

bool LocalDeviceManager::isScanRunning() const
{
    return m_scanRunning;
}

bool LocalDeviceManager::canScan() const
{
    return !m_ipPath.isEmpty() && !m_pkexecPath.isEmpty() && !m_shPath.isEmpty()
           && (!m_arpScanPath.isEmpty() || !m_nmapPath.isEmpty());
}

QString LocalDeviceManager::availabilityError() const
{
    return m_availabilityError;
}

QString LocalDeviceManager::warnings() const
{
    return m_warnings;
}

LocalDeviceManager::ScanScope LocalDeviceManager::scope() const
{
    return m_scope;
}

void LocalDeviceManager::setScope(ScanScope scope)
{
    if (scope == m_scope)
        return;
    m_scope = scope;
    QSettings settings(QStringLiteral("QControl"), QStringLiteral("QControl"));
    settings.setValue(QLatin1String(kScopeKey), scope == ScanScope::Local24 ? 1 : 0);
}

void LocalDeviceManager::scan()
{
    if (!canScan()) {
        emit operationFinished(m_availabilityError, false);
        return;
    }
    if (m_scanRunning)
        return;

    m_scanRunning = true;
    m_timedOut = false;
    m_warnings.clear();
    emit scanRunningChanged(true);
    m_scanTimer->start();

    m_routeProcess.setProgram(m_ipPath);
    m_routeProcess.setArguments({QStringLiteral("-j"), QStringLiteral("route"), QStringLiteral("show")});
    m_routeProcess.start();
}

void LocalDeviceManager::startPrivilegedScan(const QStringList &specs)
{
    // pkexec sh -c SCRIPT sh <arp-scan> <nmap> <interface=cidr...>
    // Empty tool paths are passed through and skipped by the script.
    QStringList args = {m_shPath, QStringLiteral("-c"), QLatin1String(kScanScript), QStringLiteral("sh"),
                        m_arpScanPath, m_nmapPath};
    args << specs;
    m_scanProcess.setProgram(m_pkexecPath);
    m_scanProcess.setArguments(args);
    m_scanProcess.start();
}

void LocalDeviceManager::finishScan(bool success, const QString &message)
{
    m_scanRunning = false;
    m_scanTimer->stop();
    m_stopTimer->stop();
    emit scanRunningChanged(false);
    emit devicesChanged();
    emit operationFinished(message, success);
}
