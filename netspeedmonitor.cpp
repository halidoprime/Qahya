#include "netspeedmonitor.h"

#include <QFile>
#include <QStringList>

NetSpeedMonitor::NetSpeedMonitor(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &NetSpeedMonitor::poll);
}

void NetSpeedMonitor::start()
{
    m_lastRxBytes = -1;
    m_lastTxBytes = -1;
    poll();
    m_timer.start();
}

void NetSpeedMonitor::stop()
{
    m_timer.stop();
}

bool NetSpeedMonitor::isActive() const
{
    return m_timer.isActive();
}

void NetSpeedMonitor::poll()
{
    QFile file(QStringLiteral("/proc/net/dev"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    qint64 rxBytes = 0;
    qint64 txBytes = 0;
    const QStringList lines = QString::fromLatin1(file.readAll()).split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const qsizetype colon = line.indexOf(QLatin1Char(':'));
        if (colon < 0)
            continue;
        const QString name = line.left(colon).trimmed();
        if (name.isEmpty() || name == QLatin1String("lo"))
            continue;
        const QStringList fields = line.mid(colon + 1).split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (fields.size() < 16)
            continue;
        rxBytes += fields.at(0).toLongLong();
        txBytes += fields.at(8).toLongLong();
    }

    if (m_lastRxBytes < 0 || m_lastTxBytes < 0) {
        m_lastRxBytes = rxBytes;
        m_lastTxBytes = txBytes;
        m_elapsed.start();
        emit speedsUpdated(0.0, 0.0);
        return;
    }

    const double seconds = qMax<qint64>(m_elapsed.elapsed(), 1) / 1000.0;
    m_elapsed.restart();
    const qint64 rxDelta = rxBytes - m_lastRxBytes;
    const qint64 txDelta = txBytes - m_lastTxBytes;
    m_lastRxBytes = rxBytes;
    m_lastTxBytes = txBytes;
    emit speedsUpdated(rxDelta > 0 ? rxDelta / seconds : 0.0,
                       txDelta > 0 ? txDelta / seconds : 0.0);
}
