#include "interfacemanager.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QStringList>

InterfaceManager::InterfaceManager(QObject *parent)
    : QObject(parent)
    , m_ipPath(QStandardPaths::findExecutable(QStringLiteral("ip")))
{
    if (m_ipPath.isEmpty())
        m_availabilityError = tr("ip could not be found. Interface information is unavailable.");

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    m_process.setProcessEnvironment(environment);

    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                m_refreshRunning = false;
                const QByteArray output = m_process.readAllStandardOutput();
                const QByteArray error = m_process.readAllStandardError();
                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    parseOutput(output);
                    m_loaded = true;
                    m_lastUpdate = QDateTime::currentDateTime();
                    emit interfacesChanged();
                } else {
                    const QString details = QString::fromLocal8Bit(error.isEmpty() ? output : error).trimmed();
                    emit operationFinished(details.isEmpty()
                                               ? tr("Could not read network interfaces (exit code %1).").arg(exitCode)
                                               : details,
                                           false);
                }
            });

    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || !m_refreshRunning)
            return;
        m_refreshRunning = false;
        emit operationFinished(tr("Could not start ip: %1").arg(m_process.errorString()), false);
    });
}

const QVector<NetworkInterfaceInfo> &InterfaceManager::interfaces() const
{
    return m_interfaces;
}

bool InterfaceManager::isLoaded() const
{
    return m_loaded;
}

QDateTime InterfaceManager::lastUpdate() const
{
    return m_lastUpdate;
}

bool InterfaceManager::canRefresh() const
{
    return !m_ipPath.isEmpty();
}

QString InterfaceManager::availabilityError() const
{
    return m_availabilityError;
}

void InterfaceManager::refresh()
{
    if (!canRefresh()) {
        emit operationFinished(m_availabilityError, false);
        return;
    }
    if (m_refreshRunning)
        return;

    m_refreshRunning = true;
    m_process.setProgram(m_ipPath);
    m_process.setArguments({QStringLiteral("-j"), QStringLiteral("addr"), QStringLiteral("show")});
    m_process.start();
}

void InterfaceManager::parseOutput(const QByteArray &output)
{
    const QJsonDocument doc = QJsonDocument::fromJson(output);
    if (!doc.isArray()) {
        emit operationFinished(tr("Unexpected output from ip."), false);
        return;
    }

    QVector<NetworkInterfaceInfo> interfaces;
    for (const QJsonValue &value : doc.array()) {
        const QJsonObject link = value.toObject();

        NetworkInterfaceInfo info;
        info.name = link.value(QStringLiteral("ifname")).toString();
        info.state = link.value(QStringLiteral("operstate")).toString();
        info.mac = link.value(QStringLiteral("address")).toString();

        QStringList ipv4;
        QStringList ipv6;
        for (const QJsonValue &addrValue : link.value(QStringLiteral("addr_info")).toArray()) {
            const QJsonObject addr = addrValue.toObject();
            const QString text = QStringLiteral("%1/%2")
                                     .arg(addr.value(QStringLiteral("local")).toString())
                                     .arg(addr.value(QStringLiteral("prefixlen")).toInt());
            const QString family = addr.value(QStringLiteral("family")).toString();
            if (family == QLatin1String("inet"))
                ipv4.append(text);
            else if (family == QLatin1String("inet6"))
                ipv6.append(text);
        }
        info.ipv4 = ipv4.join(QStringLiteral(", "));
        info.ipv6 = ipv6.join(QStringLiteral(", "));

        if (!info.name.isEmpty())
            interfaces.append(info);
    }

    m_interfaces = interfaces;
}
