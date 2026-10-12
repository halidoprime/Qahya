#include "connectionmanager.h"

#include <QProcessEnvironment>
#include <QStandardPaths>
#include <utility>

ConnectionManager::ConnectionManager(QObject *parent)
    : QObject(parent)
    , m_nmcliPath(QStandardPaths::findExecutable(QStringLiteral("nmcli")))
    , m_pkexecPath(QStandardPaths::findExecutable(QStringLiteral("pkexec")))
{
    if (m_nmcliPath.isEmpty())
        m_availabilityError = tr("nmcli could not be found. NetworkManager connections are unavailable.");

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    m_listProcess.setProcessEnvironment(environment);
    m_actionProcess.setProcessEnvironment(environment);

    connect(&m_listProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                m_listRunning = false;
                const QByteArray output = m_listProcess.readAllStandardOutput();
                const QByteArray error = m_listProcess.readAllStandardError();
                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    parseOutput(output);
                    m_loaded = true;
                    m_lastUpdate = QDateTime::currentDateTime();
                    emit connectionsChanged();
                } else {
                    const QString details = QString::fromLocal8Bit(error.isEmpty() ? output : error).trimmed();
                    emit operationFinished(details.isEmpty()
                                               ? tr("Could not list connections (exit code %1).").arg(exitCode)
                                               : details,
                                           false);
                }
            });

    connect(&m_listProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || !m_listRunning)
            return;
        m_listRunning = false;
        emit operationFinished(tr("Could not start nmcli: %1").arg(m_listProcess.errorString()), false);
    });

    connect(&m_actionProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                const QByteArray output = m_actionProcess.readAllStandardOutput();
                const QByteArray error = m_actionProcess.readAllStandardError();
                const bool success = exitStatus == QProcess::NormalExit && exitCode == 0;
                const QString details = QString::fromLocal8Bit(error.isEmpty() ? output : error).trimmed();

                // Most users may control networking without a password (polkit). If not, retry once
                // through pkexec, which asks for the admin password.
                const bool notAuthorized = !success && !m_actionPrivileged && !m_pkexecPath.isEmpty()
                                           && details.contains(QLatin1String("not authorized"), Qt::CaseInsensitive);
                if (notAuthorized) {
                    startAction(true);
                    return;
                }

                m_actionRunning = false;
                emit actionRunningChanged(false);
                const QString message = success
                                            ? m_currentSuccess
                                            : (details.isEmpty()
                                                   ? tr("Connection operation failed (exit code %1).").arg(exitCode)
                                                   : details);
                emit operationFinished(message, success);
                refresh();
            });

    connect(&m_actionProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || !m_actionRunning)
            return;
        m_actionRunning = false;
        emit actionRunningChanged(false);
        emit operationFinished(tr("Could not start nmcli: %1").arg(m_actionProcess.errorString()), false);
    });
}

const QVector<NetworkConnectionInfo> &ConnectionManager::connections() const
{
    return m_connections;
}

bool ConnectionManager::isLoaded() const
{
    return m_loaded;
}

QDateTime ConnectionManager::lastUpdate() const
{
    return m_lastUpdate;
}

bool ConnectionManager::isActionRunning() const
{
    return m_actionRunning;
}

bool ConnectionManager::canManage() const
{
    return !m_nmcliPath.isEmpty();
}

QString ConnectionManager::availabilityError() const
{
    return m_availabilityError;
}

void ConnectionManager::refresh()
{
    if (!canManage()) {
        emit operationFinished(m_availabilityError, false);
        return;
    }
    if (m_listRunning)
        return;

    m_listRunning = true;
    m_listProcess.setProgram(m_nmcliPath);
    // Terse output: fields separated by ':', with ':' and '\' escaped by a backslash.
    m_listProcess.setArguments({QStringLiteral("-t"), QStringLiteral("-f"),
                                QStringLiteral("NAME,TYPE,DEVICE,ACTIVE"),
                                QStringLiteral("connection"), QStringLiteral("show")});
    m_listProcess.start();
}

void ConnectionManager::activate(const QString &name)
{
    runAction(QStringLiteral("up"), name);
}

void ConnectionManager::deactivate(const QString &name)
{
    runAction(QStringLiteral("down"), name);
}

void ConnectionManager::runAction(const QString &verb, const QString &name)
{
    if (!canManage()) {
        emit operationFinished(m_availabilityError, false);
        return;
    }
    if (m_actionRunning)
        return;

    // Only act on names from the last list, so arbitrary input never reaches nmcli.
    bool known = false;
    for (const NetworkConnectionInfo &info : std::as_const(m_connections)) {
        if (info.name == name) {
            known = true;
            break;
        }
    }
    if (!known) {
        emit operationFinished(tr("The selected connection is no longer in the list. Refresh and try again."), false);
        return;
    }

    m_actionArgs = {QStringLiteral("connection"), verb, name};
    m_currentSuccess = verb == QLatin1String("up") ? tr("Activated %1.").arg(name)
                                                   : tr("Deactivated %1.").arg(name);
    m_actionRunning = true;
    emit actionRunningChanged(true);
    startAction(false);
}

void ConnectionManager::startAction(bool privileged)
{
    m_actionPrivileged = privileged;
    if (privileged) {
        m_actionProcess.setProgram(m_pkexecPath);
        m_actionProcess.setArguments(QStringList{m_nmcliPath} + m_actionArgs);
    } else {
        m_actionProcess.setProgram(m_nmcliPath);
        m_actionProcess.setArguments(m_actionArgs);
    }
    m_actionProcess.start();
}

QStringList ConnectionManager::splitEscaped(const QString &line)
{
    QStringList fields;
    QString current;
    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line.at(i);
        if (c == QLatin1Char('\\') && i + 1 < line.size()) {
            current.append(line.at(++i));
        } else if (c == QLatin1Char(':')) {
            fields.append(current);
            current.clear();
        } else {
            current.append(c);
        }
    }
    fields.append(current);
    return fields;
}

void ConnectionManager::parseOutput(const QByteArray &output)
{
    QVector<NetworkConnectionInfo> connections;
    const QStringList lines = QString::fromLocal8Bit(output).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &rawLine : lines) {
        const QStringList fields = splitEscaped(rawLine.trimmed());
        if (fields.size() < 4 || fields.at(0).isEmpty())
            continue;

        NetworkConnectionInfo info;
        info.name = fields.at(0);
        info.type = fields.at(1);
        info.device = fields.at(2);
        info.active = fields.at(3) == QLatin1String("yes");
        connections.append(info);
    }
    m_connections = connections;
}
