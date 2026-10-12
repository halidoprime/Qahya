#ifndef FIREWALLMANAGER_H
#define FIREWALLMANAGER_H

#include <QDateTime>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVector>

struct FirewallStatus
{
    bool active = false;
    QString logging;          // off, low, medium, high, full
    QString incomingPolicy;   // allow, deny, reject
    QString outgoingPolicy;
    QString routedPolicy;
};

struct FirewallRule
{
    int number = 0;
    QString action;      // ALLOW, DENY, REJECT, LIMIT
    QString direction;   // IN, OUT, FWD (may be empty)
    QString from;
    QString to;
    QString comment;
};

struct FirewallRuleSpec
{
    QString action;      // allow, deny, reject
    QString protocol;    // empty (any), tcp, udp
    QString port;        // empty, "22", "6000:6007", "80,443" (ranges/lists need a protocol)
    QString from;        // empty or "any", IP address, or CIDR
    QString comment;
};

class FirewallManager : public QObject
{
    Q_OBJECT

public:
    explicit FirewallManager(QObject *parent = nullptr);

    const FirewallStatus &status() const;
    const QVector<FirewallRule> &rules() const;
    bool isLoaded() const;
    QDateTime lastRead() const;
    bool isActionRunning() const;
    bool canManage() const;
    QString availabilityError() const;

    static QString validateRule(const FirewallRuleSpec &spec);

    // Reads ufw state through pkexec and stores it in the cache. Only called on user request.
    void read();
    void setEnabled(bool enabled);
    void setDefaultPolicy(const QString &direction, const QString &policy);
    void setLogging(const QString &level);
    void addRule(const FirewallRuleSpec &spec);
    void deleteRule(int number);

signals:
    void stateChanged();
    void actionRunningChanged(bool running);
    void operationFinished(const QString &message, bool success);

private:
    void runPrivileged(const QStringList &ufwArgs, const QString &successMessage);
    void loadCache();
    void saveCache() const;
    QString cachePath() const;
    QString failureMessage(const QByteArray &error, const QByteArray &output, int exitCode,
                           const QString &fallback) const;
    void parseStatusOutput(const QByteArray &output);

    QString m_ufwPath;
    QString m_pkexecPath;
    QString m_shPath;
    QString m_availabilityError;
    QProcess m_process;
    FirewallStatus m_status;
    QVector<FirewallRule> m_rules;
    QDateTime m_lastRead;
    QString m_currentSuccess;
    bool m_loaded = false;
    bool m_actionRunning = false;
};

#endif // FIREWALLMANAGER_H
