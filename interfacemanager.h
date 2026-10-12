#ifndef INTERFACEMANAGER_H
#define INTERFACEMANAGER_H

#include <QDateTime>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QVector>

struct NetworkInterfaceInfo
{
    QString name;
    QString state;   // operstate from ip: UP, DOWN, UNKNOWN, ...
    QString ipv4;    // comma-separated addresses with prefix length
    QString ipv6;
    QString mac;
};

class InterfaceManager : public QObject
{
    Q_OBJECT

public:
    explicit InterfaceManager(QObject *parent = nullptr);

    const QVector<NetworkInterfaceInfo> &interfaces() const;
    bool isLoaded() const;
    QDateTime lastUpdate() const;
    bool canRefresh() const;
    QString availabilityError() const;

    // Unprivileged read of network interfaces. Ignored while a read is already running.
    void refresh();

signals:
    void interfacesChanged();
    void operationFinished(const QString &message, bool success);

private:
    void parseOutput(const QByteArray &output);

    QString m_ipPath;
    QString m_availabilityError;
    QProcess m_process;
    QVector<NetworkInterfaceInfo> m_interfaces;
    QDateTime m_lastUpdate;
    bool m_loaded = false;
    bool m_refreshRunning = false;
};

#endif // INTERFACEMANAGER_H
