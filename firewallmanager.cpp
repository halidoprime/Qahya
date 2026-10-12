#include "firewallmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
// Runs inside pkexec (root). Optional ufw action first, then the reads, so a single
// authentication covers both. ufw accepts only one of "verbose" or "numbered" per call,
// so both are run: verbose gives policies and logging, numbered gives rule numbers.
// The config lines give policies and logging when ufw is inactive (it reports nothing then).
// LC_ALL=C keeps the output parseable.
const char kPrivilegedScript[] =
    "LC_ALL=C; export LC_ALL; ufw=$1; shift; "
    "if [ $# -gt 0 ]; then \"$ufw\" \"$@\" || exit $?; fi; "
    "\"$ufw\" status verbose || exit $?; "
    "\"$ufw\" status numbered || exit $?; "
    "grep -hE '^(DEFAULT_(INPUT|OUTPUT|FORWARD)_POLICY|LOGLEVEL)=' /etc/default/ufw /etc/ufw/ufw.conf 2>/dev/null; "
    "exit 0";

// ufw config stores DROP/ACCEPT/REJECT; the UI uses allow/deny/reject.
QString policyFromConfig(const QString &value)
{
    if (value == QLatin1String("ACCEPT"))
        return QStringLiteral("allow");
    if (value == QLatin1String("DROP"))
        return QStringLiteral("deny");
    if (value == QLatin1String("REJECT"))
        return QStringLiteral("reject");
    return {};
}
} // namespace

FirewallManager::FirewallManager(QObject *parent)
    : QObject(parent)
    , m_ufwPath(QStandardPaths::findExecutable(QStringLiteral("ufw")))
    , m_pkexecPath(QStandardPaths::findExecutable(QStringLiteral("pkexec")))
    , m_shPath(QStandardPaths::findExecutable(QStringLiteral("sh")))
{
    if (m_ufwPath.isEmpty())
        m_availabilityError = tr("ufw could not be found.");
    else if (m_pkexecPath.isEmpty())
        m_availabilityError = tr("pkexec could not be found. Firewall management is unavailable.");

    loadCache();

    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                const QByteArray output = m_process.readAllStandardOutput();
                const QByteArray error = m_process.readAllStandardError();
                const bool success = exitStatus == QProcess::NormalExit && exitCode == 0
                                     && output.contains("Status:");
                QString message;
                if (success) {
                    parseStatusOutput(output);
                    m_loaded = true;
                    m_lastRead = QDateTime::currentDateTime();
                    saveCache();
                    message = m_currentSuccess;
                } else {
                    message = failureMessage(error, output, exitCode, tr("Firewall operation failed."));
                }
                m_actionRunning = false;
                emit actionRunningChanged(false);
                emit stateChanged();
                emit operationFinished(message, success);
            });

    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || !m_actionRunning)
            return;
        m_actionRunning = false;
        emit actionRunningChanged(false);
        emit operationFinished(tr("Could not start pkexec: %1").arg(m_process.errorString()), false);
    });
}

const FirewallStatus &FirewallManager::status() const
{
    return m_status;
}

const QVector<FirewallRule> &FirewallManager::rules() const
{
    return m_rules;
}

bool FirewallManager::isLoaded() const
{
    return m_loaded;
}

QDateTime FirewallManager::lastRead() const
{
    return m_lastRead;
}

bool FirewallManager::isActionRunning() const
{
    return m_actionRunning;
}

bool FirewallManager::canManage() const
{
    return !m_ufwPath.isEmpty() && !m_pkexecPath.isEmpty() && !m_shPath.isEmpty();
}

QString FirewallManager::availabilityError() const
{
    return m_availabilityError;
}

QString FirewallManager::validateRule(const FirewallRuleSpec &spec)
{
    if (spec.action != QLatin1String("allow") && spec.action != QLatin1String("deny")
        && spec.action != QLatin1String("reject"))
        return tr("Choose allow, deny, or reject.");

    if (!spec.protocol.isEmpty() && spec.protocol != QLatin1String("tcp")
        && spec.protocol != QLatin1String("udp"))
        return tr("Protocol must be tcp, udp, or any.");

    if (!spec.port.isEmpty()) {
        const bool multi = spec.port.contains(QLatin1Char(':')) || spec.port.contains(QLatin1Char(','));
        if (multi && spec.protocol.isEmpty())
            return tr("Port ranges and lists require a protocol (tcp or udp).");

        static const QRegularExpression portRe(QStringLiteral("^\\d{1,5}(:\\d{1,5})?$"));
        for (const QString &part : spec.port.split(QLatin1Char(','))) {
            if (!portRe.match(part).hasMatch())
                return tr("Invalid port \"%1\". Use 22, 6000:6007, or 80,443.").arg(spec.port);
            const QStringList bounds = part.split(QLatin1Char(':'));
            for (const QString &bound : bounds) {
                const int value = bound.toInt();
                if (value < 1 || value > 65535)
                    return tr("Ports must be between 1 and 65535.");
            }
            if (bounds.size() == 2 && bounds.at(0).toInt() > bounds.at(1).toInt())
                return tr("Port range start must not be greater than its end.");
        }
    }

    const QString from = spec.from.trimmed();
    if (!from.isEmpty() && from.compare(QLatin1String("any"), Qt::CaseInsensitive) != 0) {
        const QStringList parts = from.split(QLatin1Char('/'));
        if (parts.size() > 2)
            return tr("Invalid source address \"%1\".").arg(from);
        // Qt Network is not linked, so validate addresses with patterns instead of QHostAddress.
        const QString address = parts.first();
        static const QRegularExpression ipv4Re(
            QStringLiteral("^(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}$"));
        static const QRegularExpression ipv6Re(QStringLiteral("^[0-9A-Fa-f:.]+$"));
        const bool isIPv6 = address.contains(QLatin1Char(':'));
        const bool valid = isIPv6 ? (ipv6Re.match(address).hasMatch() && address.count(QLatin1Char(':')) >= 2)
                                  : ipv4Re.match(address).hasMatch();
        if (!valid)
            return tr("Invalid source address \"%1\".").arg(from);
        if (parts.size() == 2) {
            bool ok = false;
            const int prefix = parts.at(1).toInt(&ok);
            const int maxPrefix = isIPv6 ? 128 : 32;
            if (!ok || prefix < 0 || prefix > maxPrefix)
                return tr("Invalid prefix length in \"%1\".").arg(from);
        }
    }

    if (spec.comment.size() > 80)
        return tr("Comment must be 80 characters or fewer.");
    if (spec.comment.contains(QRegularExpression(QStringLiteral("[\"'\\\\\\r\\n]"))))
        return tr("Comment cannot contain quotes, backslashes, or line breaks.");

    return {};
}

QString FirewallManager::failureMessage(const QByteArray &error, const QByteArray &output, int exitCode,
                                        const QString &fallback) const
{
    const QString details = QString::fromLocal8Bit(error.isEmpty() ? output : error).trimmed();
    return details.isEmpty() ? tr("%1 (exit code %2)").arg(fallback).arg(exitCode) : details;
}

void FirewallManager::parseStatusOutput(const QByteArray &output)
{
    static const QRegularExpression ruleRe(QStringLiteral("^\\[\\s*(\\d+)\\]\\s+(.*)$"));
    static const QRegularExpression policyRe(QStringLiteral("(\\w+)\\s+\\((\\w+)\\)"));
    static const QRegularExpression levelRe(QStringLiteral("\\((\\w+)\\)"));
    static const QRegularExpression gapRe(QStringLiteral("\\s{2,}"));

    FirewallStatus status;
    QVector<FirewallRule> rules;
    QHash<QString, QString> config;

    const QStringList lines = QString::fromLocal8Bit(output).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();

        if (line.startsWith(QLatin1String("DEFAULT_")) || line.startsWith(QLatin1String("LOGLEVEL="))) {
            const int eq = line.indexOf(QLatin1Char('='));
            config.insert(line.left(eq), line.mid(eq + 1).remove(QLatin1Char('"')).trimmed());
        } else if (line.startsWith(QLatin1String("Status:"))) {
            status.active = line.mid(7).trimmed() == QLatin1String("active");
        } else if (line.startsWith(QLatin1String("Logging:"))) {
            const QString value = line.mid(8).trimmed();
            if (value.startsWith(QLatin1String("on"))) {
                const QRegularExpressionMatch match = levelRe.match(value);
                status.logging = match.hasMatch() ? match.captured(1) : QStringLiteral("low");
            } else {
                status.logging = QStringLiteral("off");
            }
        } else if (line.startsWith(QLatin1String("Default:"))) {
            QRegularExpressionMatchIterator it = policyRe.globalMatch(line);
            while (it.hasNext()) {
                const QRegularExpressionMatch match = it.next();
                const QString policy = match.captured(1);
                const QString direction = match.captured(2);
                if (direction == QLatin1String("incoming"))
                    status.incomingPolicy = policy;
                else if (direction == QLatin1String("outgoing"))
                    status.outgoingPolicy = policy;
                else if (direction == QLatin1String("routed"))
                    status.routedPolicy = policy;
            }
        } else {
            const QRegularExpressionMatch match = ruleRe.match(line);
            if (!match.hasMatch())
                continue;

            // Columns are: To, Action, From, then an optional "# comment".
            QString body = match.captured(2);
            QString comment;
            const int hash = body.indexOf(QLatin1Char('#'));
            if (hash >= 0) {
                comment = body.mid(hash + 1).trimmed();
                body = body.left(hash);
            }

            const QStringList fields = body.split(gapRe, Qt::SkipEmptyParts);
            if (fields.size() < 3)
                continue;

            const QStringList actionParts = fields.at(1).split(QLatin1Char(' '), Qt::SkipEmptyParts);
            FirewallRule rule;
            rule.number = match.captured(1).toInt();
            rule.to = fields.at(0);
            rule.action = actionParts.value(0);
            rule.direction = actionParts.value(1);
            rule.from = fields.at(2);
            rule.comment = comment;
            rules.append(rule);
        }
    }

    // Inactive firewall: ufw prints no Default/Logging lines, so show the configured values.
    if (!status.active) {
        if (status.incomingPolicy.isEmpty())
            status.incomingPolicy = policyFromConfig(config.value(QStringLiteral("DEFAULT_INPUT_POLICY")));
        if (status.outgoingPolicy.isEmpty())
            status.outgoingPolicy = policyFromConfig(config.value(QStringLiteral("DEFAULT_OUTPUT_POLICY")));
        if (status.routedPolicy.isEmpty())
            status.routedPolicy = policyFromConfig(config.value(QStringLiteral("DEFAULT_FORWARD_POLICY")));
        if (status.logging.isEmpty() && config.contains(QStringLiteral("LOGLEVEL")))
            status.logging = config.value(QStringLiteral("LOGLEVEL"));
    }

    m_status = status;
    m_rules = rules;
}

QString FirewallManager::cachePath() const
{
    // GenericCacheLocation avoids depending on QCoreApplication organization/app names.
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
           + QStringLiteral("/QControl/ufw-rules.json");
}

void FirewallManager::saveCache() const
{
    const QString path = cachePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonObject statusJson;
    statusJson.insert(QStringLiteral("active"), m_status.active);
    statusJson.insert(QStringLiteral("logging"), m_status.logging);
    statusJson.insert(QStringLiteral("incomingPolicy"), m_status.incomingPolicy);
    statusJson.insert(QStringLiteral("outgoingPolicy"), m_status.outgoingPolicy);
    statusJson.insert(QStringLiteral("routedPolicy"), m_status.routedPolicy);

    QJsonArray rulesJson;
    for (const FirewallRule &rule : m_rules) {
        QJsonObject ruleJson;
        ruleJson.insert(QStringLiteral("number"), rule.number);
        ruleJson.insert(QStringLiteral("action"), rule.action);
        ruleJson.insert(QStringLiteral("direction"), rule.direction);
        ruleJson.insert(QStringLiteral("from"), rule.from);
        ruleJson.insert(QStringLiteral("to"), rule.to);
        ruleJson.insert(QStringLiteral("comment"), rule.comment);
        rulesJson.append(ruleJson);
    }

    QJsonObject root;
    root.insert(QStringLiteral("readAt"), m_lastRead.toString(Qt::ISODate));
    root.insert(QStringLiteral("status"), statusJson);
    root.insert(QStringLiteral("rules"), rulesJson);

    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.commit();
    }
}

void FirewallManager::loadCache()
{
    QFile file(cachePath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return;

    const QJsonObject root = doc.object();
    const QDateTime readAt = QDateTime::fromString(root.value(QStringLiteral("readAt")).toString(), Qt::ISODate);
    if (!readAt.isValid())
        return;

    const QJsonObject statusJson = root.value(QStringLiteral("status")).toObject();
    m_status.active = statusJson.value(QStringLiteral("active")).toBool();
    m_status.logging = statusJson.value(QStringLiteral("logging")).toString();
    m_status.incomingPolicy = statusJson.value(QStringLiteral("incomingPolicy")).toString();
    m_status.outgoingPolicy = statusJson.value(QStringLiteral("outgoingPolicy")).toString();
    m_status.routedPolicy = statusJson.value(QStringLiteral("routedPolicy")).toString();

    m_rules.clear();
    for (const QJsonValue &value : root.value(QStringLiteral("rules")).toArray()) {
        const QJsonObject ruleJson = value.toObject();
        FirewallRule rule;
        rule.number = ruleJson.value(QStringLiteral("number")).toInt();
        rule.action = ruleJson.value(QStringLiteral("action")).toString();
        rule.direction = ruleJson.value(QStringLiteral("direction")).toString();
        rule.from = ruleJson.value(QStringLiteral("from")).toString();
        rule.to = ruleJson.value(QStringLiteral("to")).toString();
        rule.comment = ruleJson.value(QStringLiteral("comment")).toString();
        m_rules.append(rule);
    }

    m_lastRead = readAt;
    m_loaded = true;
}

void FirewallManager::read()
{
    if (m_actionRunning)
        return;
    runPrivileged({}, tr("Firewall rules read and cached."));
}

void FirewallManager::setEnabled(bool enabled)
{
    if (enabled)
        runPrivileged({QStringLiteral("--force"), QStringLiteral("enable")}, tr("Firewall enabled."));
    else
        runPrivileged({QStringLiteral("disable")}, tr("Firewall disabled."));
}

void FirewallManager::setDefaultPolicy(const QString &direction, const QString &policy)
{
    const bool validDirection = direction == QLatin1String("incoming") || direction == QLatin1String("outgoing")
                                || direction == QLatin1String("routed");
    const bool validPolicy = policy == QLatin1String("allow") || policy == QLatin1String("deny")
                             || policy == QLatin1String("reject");
    if (!validDirection || !validPolicy) {
        emit operationFinished(tr("Invalid default policy."), false);
        return;
    }
    runPrivileged({QStringLiteral("default"), policy, direction},
                  tr("Default %1 policy set to %2.").arg(direction, policy));
}

void FirewallManager::setLogging(const QString &level)
{
    static const QStringList levels = {QStringLiteral("off"), QStringLiteral("low"), QStringLiteral("medium"),
                                       QStringLiteral("high"), QStringLiteral("full")};
    if (!levels.contains(level)) {
        emit operationFinished(tr("Invalid logging level."), false);
        return;
    }
    runPrivileged({QStringLiteral("logging"), level}, tr("Logging set to %1.").arg(level));
}

void FirewallManager::addRule(const FirewallRuleSpec &spec)
{
    const QString error = validateRule(spec);
    if (!error.isEmpty()) {
        emit operationFinished(error, false);
        return;
    }

    const QString from = spec.from.trimmed();
    QStringList args = {spec.action};
    if (!spec.protocol.isEmpty())
        args << QStringLiteral("proto") << spec.protocol;
    args << QStringLiteral("from")
         << (from.isEmpty() || from.compare(QLatin1String("any"), Qt::CaseInsensitive) == 0
                 ? QStringLiteral("any")
                 : from);
    if (!spec.port.isEmpty())
        args << QStringLiteral("to") << QStringLiteral("any") << QStringLiteral("port") << spec.port;
    if (!spec.comment.isEmpty())
        args << QStringLiteral("comment") << spec.comment;

    runPrivileged(args, tr("Added %1 rule.").arg(spec.action));
}

void FirewallManager::deleteRule(int number)
{
    if (number < 1) {
        emit operationFinished(tr("Select a rule to delete."), false);
        return;
    }
    runPrivileged({QStringLiteral("--force"), QStringLiteral("delete"), QString::number(number)},
                  tr("Deleted rule #%1.").arg(number));
}

void FirewallManager::runPrivileged(const QStringList &ufwArgs, const QString &successMessage)
{
    if (!canManage()) {
        emit operationFinished(m_availabilityError, false);
        return;
    }
    if (m_actionRunning)
        return;

    m_actionRunning = true;
    m_currentSuccess = successMessage;
    emit actionRunningChanged(true);

    // pkexec sh -c SCRIPT sh <ufw path> [ufw args...]
    QStringList args = {m_shPath, QStringLiteral("-c"), QLatin1String(kPrivilegedScript),
                        QStringLiteral("sh"), m_ufwPath};
    args << ufwArgs;
    m_process.setProgram(m_pkexecPath);
    m_process.setArguments(args);
    m_process.start();
}
