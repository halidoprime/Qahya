#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QDateTime>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVector>

struct NetworkConnectionInfo
{
    QString name;
    QString type;     // nmcli type, e.g. 802-3-ethernet, 802-11-wireless, vpn
    QString device;   // empty when the connection is not active
    bool active = false;
};

class ConnectionManager : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionManager(QObject *parent = nullptr);

    const QVector<NetworkConnectionInfo> &connections() const;
    bool isLoaded() const;
    QDateTime lastUpdate() const;
    bool isActionRunning() const;
    bool canManage() const;
    QString availabilityError() const;

    // Unprivileged list of NetworkManager connections. Ignored while a list is already running.
    void refresh();
    void activate(const QString &name);
    void deactivate(const QString &name);

signals:
    void connectionsChanged();
    void actionRunningChanged(bool running);
    void operationFinished(const QString &message, bool success);

private:
    void runAction(const QString &verb, const QString &name);
    void startAction(bool privileged);
    static QStringList splitEscaped(const QString &line);
    void parseOutput(const QByteArray &output);

    QString m_nmcliPath;
    QString m_pkexecPath;
    QString m_availabilityError;
    QProcess m_listProcess;
    QProcess m_actionProcess;
    QVector<NetworkConnectionInfo> m_connections;
    QDateTime m_lastUpdate;
    QStringList m_actionArgs;
    QString m_currentSuccess;
    bool m_loaded = false;
    bool m_listRunning = false;
    bool m_actionRunning = false;
    bool m_actionPrivileged = false;
};

#endif // CONNECTIONMANAGER_H
