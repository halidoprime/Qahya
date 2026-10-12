#ifndef LOCALDEVICEMANAGER_H
#define LOCALDEVICEMANAGER_H

#include <QDateTime>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVector>

class QTimer;

struct LocalDeviceInfo
{
    QString ip;
    QString mac;
    QString hostname;   // reported by nmap (reverse DNS); empty when unknown
    QString vendor;     // MAC vendor (OUI) name; empty when unknown
    QString method;     // arp-scan, nmap, or "arp-scan + nmap"
};

class LocalDeviceManager : public QObject
{
    Q_OBJECT

public:
    enum class ScanScope {
        RoutedSubnet,   // the whole subnet from the kernel route (/16 to /30)
        Local24         // only the /24 that contains this machine's address
    };

    explicit LocalDeviceManager(QObject *parent = nullptr);

    const QVector<LocalDeviceInfo> &devices() const;
    bool isLoaded() const;
    QDateTime lastScan() const;
    bool isScanRunning() const;
    bool canScan() const;
    QString availabilityError() const;
    // Stderr from the last scan (for example arp-scan "Resource temporarily unavailable"). Empty when none.
    QString warnings() const;

    ScanScope scope() const;
    void setScope(ScanScope scope);

    // Scans the local IPv4 subnets with arp-scan and nmap through pkexec. Only called on user request.
    void scan();

signals:
    void devicesChanged();
    void scanRunningChanged(bool running);
    void operationFinished(const QString &message, bool success);

private:
    void startPrivilegedScan(const QStringList &specs);
    void finishScan(bool success, const QString &message);

    QString m_ipPath;
    QString m_pkexecPath;
    QString m_shPath;
    QString m_arpScanPath;
    QString m_nmapPath;
    QString m_availabilityError;
    QString m_warnings;
    QProcess m_routeProcess;
    QProcess m_scanProcess;
    QTimer *m_scanTimer;
    QTimer *m_stopTimer;
    QVector<LocalDeviceInfo> m_devices;
    QDateTime m_lastScan;
    ScanScope m_scope = ScanScope::RoutedSubnet;
    bool m_loaded = false;
    bool m_scanRunning = false;
    bool m_timedOut = false;
};

#endif // LOCALDEVICEMANAGER_H
