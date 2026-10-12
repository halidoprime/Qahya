#ifndef NETSPEEDMONITOR_H
#define NETSPEEDMONITOR_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

class NetSpeedMonitor : public QObject
{
    Q_OBJECT

public:
    explicit NetSpeedMonitor(QObject *parent = nullptr);

    void start();
    void stop();
    bool isActive() const;

signals:
    void speedsUpdated(double downBytesPerSecond, double upBytesPerSecond);

private:
    void poll();

    QTimer m_timer;
    QElapsedTimer m_elapsed;
    qint64 m_lastRxBytes = -1;
    qint64 m_lastTxBytes = -1;
};

#endif // NETSPEEDMONITOR_H
