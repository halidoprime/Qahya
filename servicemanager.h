#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>
#include <QHash>
#include <QProcess>
#include <QStringList>
#include <QVector>

struct ServiceInfo
{
    QString unit;
    QString load;
    QString active;
    QString sub;
    QString startupState;
    QString description;
};

class ServiceManager : public QObject
{
    Q_OBJECT

public:
    explicit ServiceManager(QObject *parent = nullptr);

    const QVector<ServiceInfo> &services() const;
    bool isActionRunning() const;
    bool canManageServices() const;
    bool canRefreshServices() const;
    QString availabilityError() const;

    void refresh();
    void start(const QString &unit);
    void stop(const QString &unit);
    void enable(const QString &unit);
    void disable(const QString &unit);

signals:
    void servicesChanged();
    void criticalStatusChanged(bool allOkay, const QStringList &inactiveServices, bool available);
    void actionRunningChanged(bool running);
    void operationFinished(const QString &message, bool success);

private:
    enum class RefreshStage { Idle, UnitFiles, Units };

    void startRefreshStage(RefreshStage stage);
    void finishRefresh();
    void runAction(const QString &action, const QString &unit);
    void updateCriticalStatus();
    bool isKnownService(const QString &unit) const;
    static QStringList parseLines(const QByteArray &output);

    QString m_systemctlPath;
    QString m_pkexecPath;
    QString m_availabilityError;
    QProcess m_refreshProcess;
    QProcess m_actionProcess;
    RefreshStage m_refreshStage = RefreshStage::Idle;
    QVector<ServiceInfo> m_services;
    QStringList m_installedUnits;
    QHash<QString, QString> m_unitFileStates;
    QString m_currentAction;
    QString m_currentUnit;
    bool m_unitFilesSucceeded = false;
    bool m_unitsSucceeded = false;
    bool m_refreshPending = false;
    bool m_actionRunning = false;
};

#endif // SERVICEMANAGER_H
