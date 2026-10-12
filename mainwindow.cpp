#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "addruledialog.h"
#include "firewallmanager.h"
#include "interfacemanager.h"
#include "connectionmanager.h"
#include "localdevicemanager.h"
#include "netgraphwidget.h"
#include "netspeedmonitor.h"

#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyle>
#include <QTableView>
#include <QTabWidget>
#include <QTimer>

namespace {
void setComboValue(QComboBox *combo, const QString &value)
{
    if (value.isEmpty()) {
        combo->setCurrentIndex(-1);
        return;
    }
    int index = combo->findData(value);
    if (index < 0) {
        combo->addItem(value, value);
        index = combo->count() - 1;
    }
    combo->setCurrentIndex(index);
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serviceManager(new ServiceManager(this))
    , m_servicesModel(new QStandardItemModel(this))
    , m_serviceRefreshTimer(new QTimer(this))
    , m_firewallManager(new FirewallManager(this))
    , m_rulesModel(new QStandardItemModel(this))
    , m_interfaceManager(new InterfaceManager(this))
    , m_interfacesModel(new QStandardItemModel(this))
    , m_interfaceRefreshTimer(new QTimer(this))
    , m_connectionManager(new ConnectionManager(this))
    , m_connectionsModel(new QStandardItemModel(this))
    , m_connectionRefreshTimer(new QTimer(this))
    , m_localDeviceManager(new LocalDeviceManager(this))
    , m_localsModel(new QStandardItemModel(this))
    , m_netSpeedMonitor(new NetSpeedMonitor(this))
    , m_netGraph(new NetGraphWidget(this))
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->pageNetwork);
    ui->btnNetwork->setChecked(true);
    applyDarkTheme();

    m_servicesModel->setHorizontalHeaderLabels({tr("Unit"), tr("Load"), tr("Active"), tr("Sub"),
                                                tr("Startup"), tr("Description")});
    ui->servicesTable->setModel(m_servicesModel);
    ui->servicesTable->setSortingEnabled(true);
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->servicesTable->horizontalHeader()->setStretchLastSection(false);
    ui->servicesTable->setColumnWidth(0, 220);
    ui->servicesTable->setColumnWidth(1, 90);
    ui->servicesTable->setColumnWidth(2, 90);
    ui->servicesTable->setColumnWidth(3, 90);
    ui->servicesTable->setColumnWidth(4, 110);
    ui->servicesTable->setColumnWidth(5, 320);

    m_serviceRefreshTimer->setInterval(10000);
    connect(m_serviceRefreshTimer, &QTimer::timeout, m_serviceManager, &ServiceManager::refresh);
    m_serviceRefreshTimer->start();

    connect(m_serviceManager, &ServiceManager::servicesChanged, this, &MainWindow::populateServicesTable);
    connect(m_serviceManager, &ServiceManager::actionRunningChanged, this, &MainWindow::updateServiceControls);
    connect(m_serviceManager, &ServiceManager::operationFinished, this, [this](const QString &message, bool success) {
        ui->statusbar->showMessage(message, success ? 5000 : 10000);
    });
    connect(m_serviceManager, &ServiceManager::criticalStatusChanged, this,
            [this](bool allOkay, const QStringList &inactiveServices, bool available) {
                QLabel *label = ui->criticalStatusLabel;
                if (!available) {
                    label->setText(tr("Critical service status unavailable"));
                    label->setToolTip(tr("QControl could not read the systemd service inventory."));
                    label->setProperty("state", QStringLiteral("alert"));
                } else if (allOkay) {
                    label->setText(tr("Everything is fine"));
                    label->setToolTip(tr("All installed critical services are active."));
                    label->setProperty("state", QStringLiteral("ok"));
                } else {
                    label->setText(tr("Critical services down: %1").arg(inactiveServices.join(QStringLiteral(", "))));
                    label->setToolTip(tr("Installed critical services that are not active: %1")
                                           .arg(inactiveServices.join(QStringLiteral(", "))));
                    label->setProperty("state", QStringLiteral("alert"));
                }
                label->style()->unpolish(label);
                label->style()->polish(label);
            });

    connect(ui->servicesTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::updateServiceControls);
    connect(ui->btnStartService, &QPushButton::clicked, this, [this]() {
        m_serviceManager->start(selectedServiceUnit());
    });
    connect(ui->btnStopService, &QPushButton::clicked, this, [this]() {
        m_serviceManager->stop(selectedServiceUnit());
    });
    connect(ui->btnEnableService, &QPushButton::clicked, this, [this]() {
        m_serviceManager->enable(selectedServiceUnit());
    });
    connect(ui->btnDisableService, &QPushButton::clicked, this, [this]() {
        m_serviceManager->disable(selectedServiceUnit());
    });
    connect(ui->btnRefreshServices, &QPushButton::clicked, m_serviceManager, &ServiceManager::refresh);

    connect(ui->btnNetwork, &QPushButton::clicked, this, [this]() {
        ui->stackedWidget->setCurrentWidget(ui->pageNetwork);
        m_netSpeedMonitor->start();
    });
    connect(ui->btnServices, &QPushButton::clicked, this, [this]() {
        ui->stackedWidget->setCurrentWidget(ui->pageServices);
        m_serviceManager->refresh();
        m_netSpeedMonitor->stop();
    });
    connect(ui->btnOther, &QPushButton::clicked, this, [this]() {
        ui->stackedWidget->setCurrentWidget(ui->pageOther);
        m_netSpeedMonitor->stop();
    });

    setupFirewallTab();
    setupInterfacesTab();
    setupConnectionsTab();
    setupLocalsTab();
    setupNetworkGraph();

    if (!m_serviceManager->canRefreshServices()) {
        ui->criticalStatusLabel->setText(tr("Critical service status unavailable"));
        ui->criticalStatusLabel->setToolTip(m_serviceManager->availabilityError());
        ui->criticalStatusLabel->setProperty("state", QStringLiteral("alert"));
        ui->criticalStatusLabel->style()->unpolish(ui->criticalStatusLabel);
        ui->criticalStatusLabel->style()->polish(ui->criticalStatusLabel);
    }
    if (!m_serviceManager->availabilityError().isEmpty())
        ui->statusbar->showMessage(m_serviceManager->availabilityError());
    updateServiceControls();
    m_serviceManager->refresh();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::applyDarkTheme()
{
    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget#pageNetwork, QWidget#pageServices, QWidget#pageOther {"
        "    background-color: #1e1f24;"
        "}"
        "QLabel {"
        "    color: #e6e7ea;"
        "    font-size: 24pt;"
        "    font-weight: 600;"
        "}"
        "QLabel#criticalStatusLabel {"
        "    font-size: 12pt;"
        "    font-weight: 600;"
        "}"
        "QLabel#criticalStatusLabel[state=\"ok\"] { color: #63d297; }"
        "QLabel#criticalStatusLabel[state=\"alert\"] { color: #ff7777; }"
        "QFrame#criticalWarningFrame {"
        "    background-color: #25262d;"
        "    border: 1px solid #3d3e48;"
        "    border-radius: 8px;"
        "}"
        "QFrame#firewallStatusFrame {"
        "    background-color: #25262d;"
        "    border: 1px solid #3d3e48;"
        "    border-radius: 8px;"
        "}"
        "QLabel#firewallStatusLabel {"
        "    font-size: 12pt;"
        "    font-weight: 600;"
        "}"
        "QLabel#firewallStatusLabel[state=\"ok\"] { color: #63d297; }"
        "QLabel#firewallStatusLabel[state=\"alert\"] { color: #ff7777; }"
        "QWidget#tabFirewall QLabel, QWidget#tabInterfaces QLabel, QWidget#tabConnections QLabel, QWidget#tabLocals QLabel {"
        "    font-size: 11pt;"
        "    font-weight: 600;"
        "}"
        "QLabel#localsWarningLabel {"
        "    color: #e0b15a;"
        "    font-size: 10pt;"
        "    font-weight: 600;"
        "}"
        "QLabel#netGraphLegend {"
        "    font-size: 9pt;"
        "    font-weight: 600;"
        "}"
        "QTabWidget::pane {"
        "    background-color: #1e1f24;"
        "    border: 0;"
        "}"
        "QTabBar::tab {"
        "    background-color: #2e2f36;"
        "    color: #c9cbd3;"
        "    border: 1px solid #3d3e48;"
        "    border-bottom: 0;"
        "    border-top-left-radius: 6px;"
        "    border-top-right-radius: 6px;"
        "    margin-right: 4px;"
        "    padding: 8px 18px;"
        "    font-size: 11pt;"
        "    font-weight: 600;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: #4f9cf9;"
        "    border-color: #6bb0ff;"
        "    color: #ffffff;"
        "}"
        "QComboBox, QLineEdit {"
        "    background-color: #25262d;"
        "    color: #e6e7ea;"
        "    border: 1px solid #3d3e48;"
        "    border-radius: 6px;"
        "    padding: 6px 8px;"
        "    min-height: 22px;"
        "    font-size: 10pt;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #25262d;"
        "    color: #e6e7ea;"
        "    selection-background-color: #3e6fa8;"
        "}"
        "QDialog#addRuleDialog {"
        "    background-color: #1e1f24;"
        "}"
        "QDialog#addRuleDialog QLabel {"
        "    font-size: 11pt;"
        "    font-weight: 600;"
        "}"
        "QTableView {"
        "    background-color: #25262d;"
        "    alternate-background-color: #2b2c33;"
        "    color: #e6e7ea;"
        "    border: 1px solid #3d3e48;"
        "    border-radius: 6px;"
        "    selection-background-color: #3e6fa8;"
        "    selection-color: #ffffff;"
        "    font-size: 10pt;"
        "    outline: 0;"
        "}"
        "QHeaderView::section {"
        "    background-color: #303139;"
        "    color: #c9cbd3;"
        "    border: 0;"
        "    border-right: 1px solid #3d3e48;"
        "    padding: 7px;"
        "    font-weight: 600;"
        "}"
        "QPushButton#btnStartService, QPushButton#btnStopService, QPushButton#btnEnableService, QPushButton#btnDisableService, QPushButton#btnRefreshServices {"
        "    min-width: 64px;"
        "    padding: 6px 8px;"
        "}"
        "QPushButton {"
        "    background-color: #2e2f36;"
        "    color: #c9cbd3;"
        "    border: 1px solid #3d3e48;"
        "    border-radius: 8px;"
        "    padding: 9px 22px;"
        "    min-width: 130px;"
        "    font-size: 11pt;"
        "    font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "    background-color: #383941;"
        "    color: #e6e7ea;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #282930;"
        "}"
        "QPushButton:checked {"
        "    background-color: #4f9cf9;"
        "    border-color: #6bb0ff;"
        "    color: #ffffff;"
        "}"
        "QPushButton:checked:hover {"
        "    background-color: #5ea6fb;"
        "}"
        "QMenuBar {"
        "    background-color: #191a1e;"
        "    color: #c9cbd3;"
        "}"
        "QMenuBar::item:selected {"
        "    background-color: #2e2f36;"
        "}"
        "QStatusBar {"
        "    background-color: #191a1e;"
        "    color: #8b8d96;"
        "}"
    ));
}

void MainWindow::populateServicesTable()
{
    const bool sortingEnabled = ui->servicesTable->isSortingEnabled();
    ui->servicesTable->setSortingEnabled(false);
    m_servicesModel->removeRows(0, m_servicesModel->rowCount());

    for (const ServiceInfo &service : m_serviceManager->services()) {
        QList<QStandardItem *> row;
        const QStringList values = {service.unit, service.load, service.active, service.sub,
                                    service.startupState, service.description};
        for (const QString &value : values) {
            auto *item = new QStandardItem(value);
            item->setEditable(false);
            row.append(item);
        }
        row.first()->setData(service.unit, Qt::UserRole);
        m_servicesModel->appendRow(row);
    }

    ui->servicesTable->setSortingEnabled(sortingEnabled);
    updateServiceControls();
}

void MainWindow::updateServiceControls()
{
    const bool hasSelection = !ui->servicesTable->selectionModel()->selectedRows().isEmpty();
    const bool canAct = hasSelection && m_serviceManager->canManageServices()
                        && !m_serviceManager->isActionRunning();
    ui->btnStartService->setEnabled(canAct);
    ui->btnStopService->setEnabled(canAct);
    ui->btnEnableService->setEnabled(canAct);
    ui->btnDisableService->setEnabled(canAct);
    ui->btnRefreshServices->setEnabled(m_serviceManager->canRefreshServices());
    if (!m_serviceManager->canManageServices()) {
        ui->btnStartService->setToolTip(m_serviceManager->availabilityError());
        ui->btnStopService->setToolTip(m_serviceManager->availabilityError());
        ui->btnEnableService->setToolTip(m_serviceManager->availabilityError());
        ui->btnDisableService->setToolTip(m_serviceManager->availabilityError());
    }
}

QString MainWindow::selectedServiceUnit() const
{
    const QModelIndexList selectedRows = ui->servicesTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return {};
    return m_servicesModel->item(selectedRows.first().row(), 0)->data(Qt::UserRole).toString();
}

void MainWindow::setupFirewallTab()
{
    m_rulesModel->setHorizontalHeaderLabels({tr("#"), tr("Action"), tr("Direction"), tr("From"), tr("To"),
                                             tr("Comment")});
    ui->rulesTable->setModel(m_rulesModel);
    ui->rulesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->rulesTable->horizontalHeader()->setStretchLastSection(true);
    ui->rulesTable->setColumnWidth(0, 50);
    ui->rulesTable->setColumnWidth(1, 90);
    ui->rulesTable->setColumnWidth(2, 90);
    ui->rulesTable->setColumnWidth(3, 200);
    ui->rulesTable->setColumnWidth(4, 200);

    populatePolicyCombo(ui->comboPolicyIncoming);
    populatePolicyCombo(ui->comboPolicyOutgoing);
    populatePolicyCombo(ui->comboPolicyRouted);
    ui->comboLogging->addItem(tr("Off"), QStringLiteral("off"));
    ui->comboLogging->addItem(tr("Low"), QStringLiteral("low"));
    ui->comboLogging->addItem(tr("Medium"), QStringLiteral("medium"));
    ui->comboLogging->addItem(tr("High"), QStringLiteral("high"));
    ui->comboLogging->addItem(tr("Full"), QStringLiteral("full"));

    connect(m_firewallManager, &FirewallManager::stateChanged, this, &MainWindow::updateFirewallView);
    connect(m_firewallManager, &FirewallManager::actionRunningChanged, this, &MainWindow::updateFirewallControls);
    connect(m_firewallManager, &FirewallManager::operationFinished, this,
            [this](const QString &message, bool success) {
                ui->statusbar->showMessage(message, success ? 5000 : 10000);
            });
    connect(ui->rulesTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::updateFirewallControls);

    connect(ui->btnFirewallEnable, &QPushButton::clicked, this, [this]() {
        m_firewallManager->setEnabled(true);
    });
    connect(ui->btnFirewallDisable, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(this, tr("Disable firewall"),
                                  tr("Disabling ufw removes all firewall filtering. Continue?"))
            == QMessageBox::Yes)
            m_firewallManager->setEnabled(false);
    });
    connect(ui->comboPolicyIncoming, &QComboBox::activated, this, [this](int index) {
        m_firewallManager->setDefaultPolicy(QStringLiteral("incoming"),
                                            ui->comboPolicyIncoming->itemData(index).toString());
    });
    connect(ui->comboPolicyOutgoing, &QComboBox::activated, this, [this](int index) {
        m_firewallManager->setDefaultPolicy(QStringLiteral("outgoing"),
                                            ui->comboPolicyOutgoing->itemData(index).toString());
    });
    connect(ui->comboPolicyRouted, &QComboBox::activated, this, [this](int index) {
        m_firewallManager->setDefaultPolicy(QStringLiteral("routed"),
                                            ui->comboPolicyRouted->itemData(index).toString());
    });
    connect(ui->comboLogging, &QComboBox::activated, this, [this](int index) {
        m_firewallManager->setLogging(ui->comboLogging->itemData(index).toString());
    });
    connect(ui->btnAddRule, &QPushButton::clicked, this, [this]() {
        AddRuleDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted)
            m_firewallManager->addRule(dialog.ruleSpec());
    });
    connect(ui->btnDeleteRule, &QPushButton::clicked, this, [this]() {
        const int number = selectedFirewallRuleNumber();
        if (number < 1)
            return;
        if (QMessageBox::question(this, tr("Delete firewall rule"),
                                  tr("Delete rule #%1? This takes effect immediately.").arg(number))
            == QMessageBox::Yes)
            m_firewallManager->deleteRule(number);
    });
    connect(ui->btnReadFirewall, &QPushButton::clicked, m_firewallManager, &FirewallManager::read);

    updateFirewallView();
    updateFirewallControls();
}

void MainWindow::populatePolicyCombo(QComboBox *combo) const
{
    combo->addItem(tr("Allow"), QStringLiteral("allow"));
    combo->addItem(tr("Deny"), QStringLiteral("deny"));
    combo->addItem(tr("Reject"), QStringLiteral("reject"));
}

void MainWindow::updateFirewallView()
{
    const FirewallStatus &status = m_firewallManager->status();
    const bool loaded = m_firewallManager->isLoaded();

    QString text;
    QString tooltip;
    QString state;
    if (!m_firewallManager->canManage()) {
        text = tr("Firewall unavailable");
        tooltip = m_firewallManager->availabilityError();
        state = QStringLiteral("alert");
    } else if (!loaded) {
        text = tr("Firewall not read yet");
        tooltip = tr("Press Read to load ufw status. This requires your admin password.");
        state = QStringLiteral("alert");
    } else if (status.active) {
        text = tr("Firewall is active");
        tooltip = tr("ufw is filtering traffic.");
        state = QStringLiteral("ok");
    } else {
        text = tr("Firewall is inactive");
        tooltip = tr("ufw is not filtering traffic.");
        state = QStringLiteral("alert");
    }
    QLabel *label = ui->firewallStatusLabel;
    label->setText(text);
    label->setToolTip(tooltip);
    label->setProperty("state", state);
    label->style()->unpolish(label);
    label->style()->polish(label);

    if (loaded) {
        setComboValue(ui->comboPolicyIncoming, status.incomingPolicy);
        setComboValue(ui->comboPolicyOutgoing, status.outgoingPolicy);
        setComboValue(ui->comboPolicyRouted, status.routedPolicy);
        setComboValue(ui->comboLogging, status.logging);
    }

    populateFirewallRules();

    const QDateTime lastRead = m_firewallManager->lastRead();
    ui->firewallLastReadLabel->setText(
        lastRead.isValid() ? tr("Last read: %1").arg(lastRead.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")))
                           : tr("Last read: never"));
    updateFirewallControls();
}

void MainWindow::updateFirewallControls()
{
    const bool canManage = m_firewallManager->canManage();
    const bool busy = m_firewallManager->isActionRunning();
    const bool loaded = m_firewallManager->isLoaded();
    const bool active = loaded && m_firewallManager->status().active;
    const bool hasSelection = ui->rulesTable->selectionModel()->hasSelection();
    const bool editable = canManage && loaded && !busy;

    ui->btnFirewallEnable->setEnabled(editable && !active);
    ui->btnFirewallDisable->setEnabled(editable && active);
    ui->comboPolicyIncoming->setEnabled(editable);
    ui->comboPolicyOutgoing->setEnabled(editable);
    ui->comboPolicyRouted->setEnabled(editable);
    ui->comboLogging->setEnabled(editable);
    ui->btnAddRule->setEnabled(canManage && !busy);
    ui->btnDeleteRule->setEnabled(editable && hasSelection);
    ui->btnReadFirewall->setEnabled(canManage && !busy);

    if (!canManage) {
        const QString error = m_firewallManager->availabilityError();
        ui->btnFirewallEnable->setToolTip(error);
        ui->btnFirewallDisable->setToolTip(error);
        ui->btnAddRule->setToolTip(error);
        ui->btnDeleteRule->setToolTip(error);
        ui->btnReadFirewall->setToolTip(error);
    }
}

void MainWindow::populateFirewallRules()
{
    m_rulesModel->removeRows(0, m_rulesModel->rowCount());
    if (!m_firewallManager->isLoaded())
        return;

    for (const FirewallRule &rule : m_firewallManager->rules()) {
        const QStringList values = {QString::number(rule.number), rule.action, rule.direction,
                                    rule.from, rule.to, rule.comment};
        QList<QStandardItem *> row;
        for (const QString &value : values) {
            auto *item = new QStandardItem(value);
            item->setEditable(false);
            row.append(item);
        }
        row.first()->setData(rule.number, Qt::UserRole);
        m_rulesModel->appendRow(row);
    }
}

int MainWindow::selectedFirewallRuleNumber() const
{
    const QModelIndexList selectedRows = ui->rulesTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return -1;
    return m_rulesModel->item(selectedRows.first().row(), 0)->data(Qt::UserRole).toInt();
}

void MainWindow::setupInterfacesTab()
{
    m_interfacesModel->setHorizontalHeaderLabels({tr("Interface"), tr("State"), tr("IPv4"), tr("IPv6"),
                                                  tr("MAC")});
    ui->interfacesTable->setModel(m_interfacesModel);
    ui->interfacesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->interfacesTable->horizontalHeader()->setStretchLastSection(true);
    ui->interfacesTable->setColumnWidth(0, 130);
    ui->interfacesTable->setColumnWidth(1, 90);
    ui->interfacesTable->setColumnWidth(2, 260);
    ui->interfacesTable->setColumnWidth(3, 340);

    // Interface data needs no privileges, so it refreshes automatically while the tab is visible.
    m_interfaceRefreshTimer->setInterval(10000);
    connect(m_interfaceRefreshTimer, &QTimer::timeout, m_interfaceManager, &InterfaceManager::refresh);

    connect(m_interfaceManager, &InterfaceManager::interfacesChanged, this, &MainWindow::populateInterfacesTable);
    connect(m_interfaceManager, &InterfaceManager::operationFinished, this,
            [this](const QString &message, bool success) {
                ui->statusbar->showMessage(message, success ? 5000 : 10000);
            });

    connect(ui->networkTabs, &QTabWidget::currentChanged, this, [this](int index) {
        const bool onInterfaces = ui->networkTabs->widget(index) == ui->tabInterfaces;
        if (onInterfaces) {
            m_interfaceManager->refresh();
            m_interfaceRefreshTimer->start();
        } else {
            m_interfaceRefreshTimer->stop();
        }
    });

    if (!m_interfaceManager->canRefresh())
        ui->interfacesTable->setToolTip(m_interfaceManager->availabilityError());
}

void MainWindow::populateInterfacesTable()
{
    m_interfacesModel->removeRows(0, m_interfacesModel->rowCount());
    for (const NetworkInterfaceInfo &info : m_interfaceManager->interfaces()) {
        const QStringList values = {info.name, info.state, info.ipv4, info.ipv6, info.mac};
        QList<QStandardItem *> row;
        for (const QString &value : values) {
            auto *item = new QStandardItem(value.isEmpty() ? QStringLiteral("-") : value);
            item->setEditable(false);
            row.append(item);
        }
        m_interfacesModel->appendRow(row);
    }
}

void MainWindow::setupConnectionsTab()
{
    m_connectionsModel->setHorizontalHeaderLabels({tr("Connection"), tr("Type"), tr("Device"), tr("Active")});
    ui->connectionsTable->setModel(m_connectionsModel);
    ui->connectionsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->connectionsTable->horizontalHeader()->setStretchLastSection(true);
    ui->connectionsTable->setColumnWidth(0, 300);
    ui->connectionsTable->setColumnWidth(1, 170);
    ui->connectionsTable->setColumnWidth(2, 140);
    ui->connectionsTable->setColumnWidth(3, 90);

    // Connection list needs no privileges, so it refreshes automatically while the tab is visible.
    m_connectionRefreshTimer->setInterval(10000);
    connect(m_connectionRefreshTimer, &QTimer::timeout, m_connectionManager, &ConnectionManager::refresh);

    connect(m_connectionManager, &ConnectionManager::connectionsChanged, this, &MainWindow::populateConnectionsTable);
    connect(m_connectionManager, &ConnectionManager::actionRunningChanged, this, &MainWindow::updateConnectionControls);
    connect(m_connectionManager, &ConnectionManager::operationFinished, this,
            [this](const QString &message, bool success) {
                ui->statusbar->showMessage(message, success ? 5000 : 10000);
            });
    connect(ui->connectionsTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::updateConnectionControls);

    connect(ui->btnActivateConnection, &QPushButton::clicked, this, [this]() {
        m_connectionManager->activate(selectedConnectionName());
    });
    connect(ui->btnDeactivateConnection, &QPushButton::clicked, this, [this]() {
        m_connectionManager->deactivate(selectedConnectionName());
    });
    connect(ui->btnRefreshConnections, &QPushButton::clicked, m_connectionManager, &ConnectionManager::refresh);

    connect(ui->networkTabs, &QTabWidget::currentChanged, this, [this](int index) {
        if (ui->networkTabs->widget(index) == ui->tabConnections) {
            m_connectionManager->refresh();
            m_connectionRefreshTimer->start();
        } else {
            m_connectionRefreshTimer->stop();
        }
    });

    if (!m_connectionManager->canManage())
        ui->btnRefreshConnections->setToolTip(m_connectionManager->availabilityError());
    updateConnectionControls();
}

void MainWindow::populateConnectionsTable()
{
    m_connectionsModel->removeRows(0, m_connectionsModel->rowCount());
    for (const NetworkConnectionInfo &info : m_connectionManager->connections()) {
        const QStringList values = {info.name, info.type, info.device.isEmpty() ? QStringLiteral("-") : info.device,
                                    info.active ? tr("Yes") : tr("No")};
        QList<QStandardItem *> row;
        for (const QString &value : values) {
            auto *item = new QStandardItem(value);
            item->setEditable(false);
            row.append(item);
        }
        row.first()->setData(info.name, Qt::UserRole);
        row.first()->setData(info.active, Qt::UserRole + 1);
        m_connectionsModel->appendRow(row);
    }
    updateConnectionControls();
}

void MainWindow::updateConnectionControls()
{
    const bool canManage = m_connectionManager->canManage();
    const bool busy = m_connectionManager->isActionRunning();
    const QModelIndexList selectedRows = ui->connectionsTable->selectionModel()->selectedRows();
    const bool hasSelection = !selectedRows.isEmpty();
    const bool active = hasSelection
                        && m_connectionsModel->item(selectedRows.first().row(), 0)->data(Qt::UserRole + 1).toBool();
    const bool enabled = canManage && !busy && hasSelection;

    ui->btnActivateConnection->setEnabled(enabled && !active);
    ui->btnDeactivateConnection->setEnabled(enabled && active);
    ui->btnRefreshConnections->setEnabled(canManage && !busy);
}

QString MainWindow::selectedConnectionName() const
{
    const QModelIndexList selectedRows = ui->connectionsTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return {};
    return m_connectionsModel->item(selectedRows.first().row(), 0)->data(Qt::UserRole).toString();
}

void MainWindow::setupLocalsTab()
{
    m_localsModel->setHorizontalHeaderLabels({tr("IP"), tr("MAC"), tr("Hostname"), tr("Vendor"), tr("Method")});
    ui->localsTable->setModel(m_localsModel);
    ui->localsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->localsTable->horizontalHeader()->setStretchLastSection(true);
    ui->localsTable->setColumnWidth(0, 130);
    ui->localsTable->setColumnWidth(1, 150);
    ui->localsTable->setColumnWidth(2, 220);
    ui->localsTable->setColumnWidth(3, 240);

    // Scanning sends packets and asks for the admin password, so it only runs on the Scan button.
    connect(m_localDeviceManager, &LocalDeviceManager::devicesChanged, this, &MainWindow::populateLocalsTable);
    connect(m_localDeviceManager, &LocalDeviceManager::scanRunningChanged, this, &MainWindow::updateLocalsControls);
    connect(m_localDeviceManager, &LocalDeviceManager::operationFinished, this,
            [this](const QString &message, bool success) {
                ui->statusbar->showMessage(message, success ? 5000 : 10000);
            });
    connect(ui->btnScanLocals, &QPushButton::clicked, m_localDeviceManager, &LocalDeviceManager::scan);

    ui->comboLocalsScope->addItem(tr("Routed subnet"), static_cast<int>(LocalDeviceManager::ScanScope::RoutedSubnet));
    ui->comboLocalsScope->addItem(tr("Local /24"), static_cast<int>(LocalDeviceManager::ScanScope::Local24));
    ui->comboLocalsScope->setCurrentIndex(
        ui->comboLocalsScope->findData(static_cast<int>(m_localDeviceManager->scope())));
    connect(ui->comboLocalsScope, &QComboBox::activated, this, [this](int index) {
        m_localDeviceManager->setScope(
            static_cast<LocalDeviceManager::ScanScope>(ui->comboLocalsScope->itemData(index).toInt()));
    });

    if (!m_localDeviceManager->canScan())
        ui->btnScanLocals->setToolTip(m_localDeviceManager->availabilityError());
    populateLocalsTable();
}

void MainWindow::populateLocalsTable()
{
    m_localsModel->removeRows(0, m_localsModel->rowCount());
    for (const LocalDeviceInfo &device : m_localDeviceManager->devices()) {
        const QStringList values = {device.ip, device.mac, device.hostname, device.vendor, device.method};
        QList<QStandardItem *> row;
        for (const QString &value : values) {
            auto *item = new QStandardItem(value.isEmpty() ? QStringLiteral("-") : value);
            item->setEditable(false);
            row.append(item);
        }
        m_localsModel->appendRow(row);
    }

    const QDateTime lastScan = m_localDeviceManager->lastScan();
    ui->localsLastScanLabel->setText(
        lastScan.isValid() ? tr("Last scan: %1").arg(lastScan.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")))
                           : tr("Last scan: never"));

    const QString warnings = m_localDeviceManager->warnings();
    ui->localsWarningLabel->setText(tr("Scan warnings: %1").arg(warnings));
    ui->localsWarningLabel->setToolTip(warnings);
    ui->localsWarningLabel->setVisible(!warnings.isEmpty());
    updateLocalsControls();
}

void MainWindow::updateLocalsControls()
{
    const bool busy = m_localDeviceManager->isScanRunning();
    ui->btnScanLocals->setEnabled(m_localDeviceManager->canScan() && !busy);
    ui->btnScanLocals->setText(busy ? tr("Scanning…") : tr("Scan"));
    ui->comboLocalsScope->setEnabled(!busy);
}

void MainWindow::setupNetworkGraph()
{
    ui->networkTabs->setCornerWidget(m_netGraph, Qt::TopRightCorner);
    connect(m_netSpeedMonitor, &NetSpeedMonitor::speedsUpdated, m_netGraph, &NetGraphWidget::addSample);
    m_netSpeedMonitor->start();
}
