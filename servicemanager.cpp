#include "servicemanager.h"

#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QHash>
#include <algorithm>
#include <utility>

ServiceManager::ServiceManager(QObject *parent)
    : QObject(parent)
    , m_systemctlPath(QStandardPaths::findExecutable(QStringLiteral("systemctl")))
    , m_pkexecPath(QStandardPaths::findExecutable(QStringLiteral("pkexec")))
{
    if (m_systemctlPath.isEmpty()) {
        m_availabilityError = tr("systemctl could not be found.");
    } else if (m_pkexecPath.isEmpty()) {
        m_availabilityError = tr("pkexec could not be found. Service management is unavailable.");
    }

    connect(&m_refreshProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                const QByteArray output = m_refreshProcess.readAllStandardOutput();
                const QByteArray error = m_refreshProcess.readAllStandardError();

                if (m_refreshStage == RefreshStage::UnitFiles) {
                    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                        m_unitFilesSucceeded = true;
                        const QStringList lines = parseLines(output);
                        for (const QString &line : lines) {
                            const QStringList fields = line.split(QRegularExpression(QStringLiteral("\\s+")),
                                                                  Qt::SkipEmptyParts);
                            if (fields.size() >= 2 && fields.first().endsWith(QStringLiteral(".service"))) {
                                m_installedUnits.append(fields.first());
                                m_unitFileStates.insert(fields.first(), fields.at(1));
                            }
                        }
                    } else {
                        emit operationFinished(error.isEmpty() ? tr("Could not list installed service units.")
                                                               : QString::fromLocal8Bit(error).trimmed(),
                                               false);
                    }
                    startRefreshStage(RefreshStage::Units);
                    return;
                }

                if (m_refreshStage == RefreshStage::Units) {
                    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                        m_unitsSucceeded = true;
                        QHash<QString, ServiceInfo> runtimeInfo;
                        const QStringList lines = parseLines(output);
                        for (const QString &line : lines) {
                            const QStringList fields = line.split(QRegularExpression(QStringLiteral("\\s+")),
                                                                  Qt::SkipEmptyParts);
                            if (fields.size() < 4 || !fields.first().endsWith(QStringLiteral(".service")))
                                continue;

                            ServiceInfo info;
                            info.unit = fields.at(0);
                            info.load = fields.at(1);
                            info.active = fields.at(2);
                            info.sub = fields.at(3);
                            info.startupState = m_unitFileStates.value(info.unit, QStringLiteral("transient"));
                            info.description = fields.mid(4).join(QLatin1Char(' '));
                            runtimeInfo.insert(info.unit, info);
                        }

                        m_services.clear();
                        for (const QString &unit : std::as_const(m_installedUnits)) {
                            ServiceInfo info = runtimeInfo.value(unit);
                            if (info.unit.isEmpty()) {
                                info.unit = unit;
                                info.load = QStringLiteral("not-loaded");
                                info.active = QStringLiteral("inactive");
                                info.sub = QStringLiteral("dead");
                                info.startupState = m_unitFileStates.value(unit, QStringLiteral("unknown"));
                                info.description = unit;
                            }
                            m_services.append(info);
                        }

                        // Include transient or runtime-only service units too.
                        for (auto it = runtimeInfo.cbegin(); it != runtimeInfo.cend(); ++it) {
                            if (!m_installedUnits.contains(it.key()))
                                m_services.append(it.value());
                        }
                    } else {
                        emit operationFinished(error.isEmpty() ? tr("Could not read service states.")
                                                               : QString::fromLocal8Bit(error).trimmed(),
                                               false);
                    }
                    finishRefresh();
                }
            });

    connect(&m_refreshProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || m_refreshStage == RefreshStage::Idle)
            return;

        emit operationFinished(tr("Could not start systemctl: %1").arg(m_refreshProcess.errorString()), false);
        if (m_refreshStage == RefreshStage::UnitFiles)
            m_unitFilesSucceeded = false;
        else
            m_unitsSucceeded = false;
        finishRefresh();
    });

    connect(&m_actionProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                const QByteArray error = m_actionProcess.readAllStandardError();
                const QByteArray output = m_actionProcess.readAllStandardOutput();
                const bool success = exitStatus == QProcess::NormalExit && exitCode == 0;
                const QString details = QString::fromLocal8Bit(error.isEmpty() ? output : error).trimmed();
                QString successMessage;
                if (m_currentAction == QLatin1String("start"))
                    successMessage = tr("Started %1.").arg(m_currentUnit);
                else if (m_currentAction == QLatin1String("stop"))
                    successMessage = tr("Stopped %1.").arg(m_currentUnit);
                else if (m_currentAction == QLatin1String("enable"))
                    successMessage = tr("Enabled %1 for startup.").arg(m_currentUnit);
                else if (m_currentAction == QLatin1String("disable"))
                    successMessage = tr("Disabled %1 for startup.").arg(m_currentUnit);
                const QString message = success
                                            ? successMessage
                                            : (details.isEmpty() ? tr("Service operation failed (exit code %1).").arg(exitCode)
                                                                 : details);
                m_actionRunning = false;
                emit actionRunningChanged(false);
                emit operationFinished(message, success);
                refresh();
            });

    connect(&m_actionProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || !m_actionRunning)
            return;

        const QString message = tr("Could not start pkexec: %1").arg(m_actionProcess.errorString());
        m_actionRunning = false;
        emit actionRunningChanged(false);
        emit operationFinished(message, false);
    });
}

const QVector<ServiceInfo> &ServiceManager::services() const
{
    return m_services;
}

bool ServiceManager::isActionRunning() const
{
    return m_actionRunning;
}

bool ServiceManager::canManageServices() const
{
    return !m_systemctlPath.isEmpty() && !m_pkexecPath.isEmpty();
}

bool ServiceManager::canRefreshServices() const
{
    return !m_systemctlPath.isEmpty();
}

QString ServiceManager::availabilityError() const
{
    return m_availabilityError;
}

QStringList ServiceManager::parseLines(const QByteArray &output)
{
    QStringList lines = QString::fromLocal8Bit(output).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (QString &line : lines)
        line = line.trimmed();
    lines.removeAll(QString());
    return lines;
}

void ServiceManager::refresh()
{
    if (m_systemctlPath.isEmpty()) {
        emit operationFinished(m_availabilityError, false);
        return;
    }
    if (m_refreshStage != RefreshStage::Idle) {
        m_refreshPending = true;
        return;
    }

    m_installedUnits.clear();
    m_unitFileStates.clear();
    m_unitFilesSucceeded = false;
    m_unitsSucceeded = false;
    startRefreshStage(RefreshStage::UnitFiles);
}

void ServiceManager::startRefreshStage(RefreshStage stage)
{
    m_refreshStage = stage;
    m_refreshProcess.setProgram(m_systemctlPath);
    if (stage == RefreshStage::UnitFiles) {
        m_refreshProcess.setArguments({QStringLiteral("list-unit-files"),
                                       QStringLiteral("--type=service"),
                                       QStringLiteral("--no-legend"),
                                       QStringLiteral("--no-pager"),
                                       QStringLiteral("--full")});
    } else {
        m_refreshProcess.setArguments({QStringLiteral("list-units"),
                                       QStringLiteral("--type=service"),
                                       QStringLiteral("--all"),
                                       QStringLiteral("--plain"),
                                       QStringLiteral("--no-legend"),
                                       QStringLiteral("--no-pager"),
                                       QStringLiteral("--full")});
    }
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    m_refreshProcess.setProcessEnvironment(environment);
    m_refreshProcess.start();
}

void ServiceManager::finishRefresh()
{
    m_refreshStage = RefreshStage::Idle;
    emit servicesChanged();
    updateCriticalStatus();

    if (m_refreshPending) {
        m_refreshPending = false;
        refresh();
    }
}

void ServiceManager::start(const QString &unit)
{
    runAction(QStringLiteral("start"), unit);
}

void ServiceManager::stop(const QString &unit)
{
    runAction(QStringLiteral("stop"), unit);
}

void ServiceManager::enable(const QString &unit)
{
    runAction(QStringLiteral("enable"), unit);
}

void ServiceManager::disable(const QString &unit)
{
    runAction(QStringLiteral("disable"), unit);
}

void ServiceManager::runAction(const QString &action, const QString &unit)
{
    if (!canManageServices()) {
        emit operationFinished(m_availabilityError, false);
        return;
    }
    if (m_actionRunning || (action != QLatin1String("start") && action != QLatin1String("stop")
                            && action != QLatin1String("enable") && action != QLatin1String("disable")))
        return;
    if (!isKnownService(unit)) {
        emit operationFinished(tr("The selected service unit is not in the current systemd inventory."), false);
        return;
    }

    m_actionRunning = true;
    m_currentAction = action;
    m_currentUnit = unit;
    emit actionRunningChanged(true);
    m_actionProcess.setProgram(m_pkexecPath);
    m_actionProcess.setArguments({m_systemctlPath, action, unit});
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    m_actionProcess.setProcessEnvironment(environment);
    m_actionProcess.start();
}

bool ServiceManager::isKnownService(const QString &unit) const
{
    return unit.endsWith(QStringLiteral(".service")) && !unit.startsWith(QLatin1Char('-'))
           && !unit.contains(QLatin1Char('/')) && !unit.contains(QRegularExpression(QStringLiteral("\\s")))
           && (m_installedUnits.contains(unit)
               || std::any_of(m_services.cbegin(), m_services.cend(), [&unit](const ServiceInfo &info) {
                      return info.unit == unit;
                  }));
}

void ServiceManager::updateCriticalStatus()
{
    if (!m_unitFilesSucceeded || !m_unitsSucceeded) {
        emit criticalStatusChanged(false, {}, false);
        return;
    }

    struct CriticalGroup {
        QString label;
        QStringList candidates;
    };

    const QVector<CriticalGroup> groups = {
        {QStringLiteral("NetworkManager"), {QStringLiteral("NetworkManager.service")}},
        {QStringLiteral("SSH"), {QStringLiteral("sshd.service"), QStringLiteral("ssh.service")}},
        {QStringLiteral("UFW"), {QStringLiteral("ufw.service")}},
        {QStringLiteral("Cron"), {QStringLiteral("cron.service"), QStringLiteral("crond.service")}},
        {QStringLiteral("systemd-journald"), {QStringLiteral("systemd-journald.service")}}
    };

    QHash<QString, QString> activeByUnit;
    for (const ServiceInfo &service : std::as_const(m_services))
        activeByUnit.insert(service.unit, service.active);

    QStringList inactive;
    for (const CriticalGroup &group : groups) {
        QStringList installedCandidates;
        bool active = false;
        for (const QString &candidate : group.candidates) {
            if (!m_installedUnits.contains(candidate))
                continue;
            installedCandidates.append(candidate);
            if (activeByUnit.value(candidate) == QLatin1String("active"))
                active = true;
        }
        if (!installedCandidates.isEmpty() && !active)
            inactive.append(group.label);
    }

    emit criticalStatusChanged(inactive.isEmpty(), inactive, true);
}
