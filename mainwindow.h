#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "servicemanager.h"

class FirewallManager;
class InterfaceManager;
class ConnectionManager;
class LocalDeviceManager;
class NetSpeedMonitor;
class NetGraphWidget;
class QComboBox;
class QStandardItemModel;
class QTimer;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void applyDarkTheme();
    void populateServicesTable();
    void updateServiceControls();
    QString selectedServiceUnit() const;

    void setupFirewallTab();
    void populatePolicyCombo(QComboBox *combo) const;
    void updateFirewallView();
    void updateFirewallControls();
    void populateFirewallRules();
    int selectedFirewallRuleNumber() const;

    void setupInterfacesTab();
    void populateInterfacesTable();
    void setupConnectionsTab();
    void populateConnectionsTable();
    void updateConnectionControls();
    QString selectedConnectionName() const;

    void setupLocalsTab();
    void populateLocalsTable();
    void updateLocalsControls();

    void setupNetworkGraph();

    Ui::MainWindow *ui;
    ServiceManager *m_serviceManager;
    QStandardItemModel *m_servicesModel;
    QTimer *m_serviceRefreshTimer;
    FirewallManager *m_firewallManager;
    QStandardItemModel *m_rulesModel;
    InterfaceManager *m_interfaceManager;
    QStandardItemModel *m_interfacesModel;
    QTimer *m_interfaceRefreshTimer;
    ConnectionManager *m_connectionManager;
    QStandardItemModel *m_connectionsModel;
    QTimer *m_connectionRefreshTimer;
    LocalDeviceManager *m_localDeviceManager;
    QStandardItemModel *m_localsModel;
    NetSpeedMonitor *m_netSpeedMonitor;
    NetGraphWidget *m_netGraph;
};
#endif // MAINWINDOW_H
