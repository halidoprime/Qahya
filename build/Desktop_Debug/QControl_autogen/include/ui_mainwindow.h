/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *buttonBarLayout;
    QPushButton *btnNetwork;
    QPushButton *btnServices;
    QPushButton *btnOther;
    QStackedWidget *stackedWidget;
    QWidget *pageNetwork;
    QVBoxLayout *verticalLayoutNetwork;
    QTabWidget *networkTabs;
    QWidget *tabFirewall;
    QVBoxLayout *verticalLayoutFirewall;
    QHBoxLayout *firewallTopLayout;
    QFrame *firewallStatusFrame;
    QVBoxLayout *firewallStatusLayout;
    QLabel *firewallStatusLabel;
    QHBoxLayout *firewallEnableLayout;
    QPushButton *btnFirewallEnable;
    QPushButton *btnFirewallDisable;
    QGridLayout *firewallSettingsLayout;
    QLabel *labelPolicyIncoming;
    QComboBox *comboPolicyIncoming;
    QLabel *labelPolicyOutgoing;
    QComboBox *comboPolicyOutgoing;
    QLabel *labelPolicyRouted;
    QComboBox *comboPolicyRouted;
    QLabel *labelLogging;
    QComboBox *comboLogging;
    QSpacerItem *firewallTopSpacer;
    QTableView *rulesTable;
    QHBoxLayout *firewallRuleButtonsLayout;
    QPushButton *btnAddRule;
    QPushButton *btnDeleteRule;
    QPushButton *btnReadFirewall;
    QLabel *firewallLastReadLabel;
    QSpacerItem *firewallRuleButtonsSpacer;
    QWidget *tabInterfaces;
    QVBoxLayout *verticalLayoutInterfaces;
    QTableView *interfacesTable;
    QWidget *tabConnections;
    QVBoxLayout *verticalLayoutConnections;
    QTableView *connectionsTable;
    QHBoxLayout *connectionButtonsLayout;
    QPushButton *btnActivateConnection;
    QPushButton *btnDeactivateConnection;
    QPushButton *btnRefreshConnections;
    QSpacerItem *connectionButtonsSpacer;
    QWidget *tabLocals;
    QVBoxLayout *verticalLayoutLocals;
    QTableView *localsTable;
    QLabel *localsWarningLabel;
    QHBoxLayout *localsButtonsLayout;
    QLabel *labelLocalsScope;
    QComboBox *comboLocalsScope;
    QPushButton *btnScanLocals;
    QLabel *localsLastScanLabel;
    QSpacerItem *localsButtonsSpacer;
    QWidget *pageServices;
    QVBoxLayout *verticalLayoutServices;
    QHBoxLayout *servicesTopLayout;
    QVBoxLayout *servicesListLayout;
    QTableView *servicesTable;
    QHBoxLayout *serviceActionsLayout;
    QPushButton *btnStartService;
    QPushButton *btnStopService;
    QPushButton *btnEnableService;
    QPushButton *btnDisableService;
    QPushButton *btnRefreshServices;
    QSpacerItem *serviceActionsSpacer;
    QFrame *criticalWarningFrame;
    QVBoxLayout *criticalWarningLayout;
    QLabel *criticalStatusLabel;
    QWidget *servicesBottomPlaceholder;
    QWidget *pageOther;
    QVBoxLayout *verticalLayoutOther;
    QLabel *labelOther;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setSpacing(14);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(16, 16, 16, 16);
        buttonBarLayout = new QHBoxLayout();
        buttonBarLayout->setSpacing(10);
        buttonBarLayout->setObjectName("buttonBarLayout");
        btnNetwork = new QPushButton(centralwidget);
        btnNetwork->setObjectName("btnNetwork");
        btnNetwork->setCheckable(true);
        btnNetwork->setChecked(true);
        btnNetwork->setAutoExclusive(true);

        buttonBarLayout->addWidget(btnNetwork);

        btnServices = new QPushButton(centralwidget);
        btnServices->setObjectName("btnServices");
        btnServices->setCheckable(true);
        btnServices->setAutoExclusive(true);

        buttonBarLayout->addWidget(btnServices);

        btnOther = new QPushButton(centralwidget);
        btnOther->setObjectName("btnOther");
        btnOther->setCheckable(true);
        btnOther->setAutoExclusive(true);

        buttonBarLayout->addWidget(btnOther);


        verticalLayout->addLayout(buttonBarLayout);

        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName("stackedWidget");
        pageNetwork = new QWidget();
        pageNetwork->setObjectName("pageNetwork");
        verticalLayoutNetwork = new QVBoxLayout(pageNetwork);
        verticalLayoutNetwork->setObjectName("verticalLayoutNetwork");
        networkTabs = new QTabWidget(pageNetwork);
        networkTabs->setObjectName("networkTabs");
        tabFirewall = new QWidget();
        tabFirewall->setObjectName("tabFirewall");
        verticalLayoutFirewall = new QVBoxLayout(tabFirewall);
        verticalLayoutFirewall->setSpacing(12);
        verticalLayoutFirewall->setObjectName("verticalLayoutFirewall");
        firewallTopLayout = new QHBoxLayout();
        firewallTopLayout->setSpacing(16);
        firewallTopLayout->setObjectName("firewallTopLayout");
        firewallStatusFrame = new QFrame(tabFirewall);
        firewallStatusFrame->setObjectName("firewallStatusFrame");
        firewallStatusFrame->setMinimumSize(QSize(220, 110));
        firewallStatusFrame->setMaximumSize(QSize(260, 16777215));
        firewallStatusFrame->setFrameShape(QFrame::Shape::StyledPanel);
        firewallStatusFrame->setFrameShadow(QFrame::Shadow::Plain);
        firewallStatusLayout = new QVBoxLayout(firewallStatusFrame);
        firewallStatusLayout->setObjectName("firewallStatusLayout");
        firewallStatusLabel = new QLabel(firewallStatusFrame);
        firewallStatusLabel->setObjectName("firewallStatusLabel");
        firewallStatusLabel->setAlignment(Qt::AlignmentFlag::AlignTop);
        firewallStatusLabel->setWordWrap(true);

        firewallStatusLayout->addWidget(firewallStatusLabel);

        firewallEnableLayout = new QHBoxLayout();
        firewallEnableLayout->setSpacing(8);
        firewallEnableLayout->setObjectName("firewallEnableLayout");
        btnFirewallEnable = new QPushButton(firewallStatusFrame);
        btnFirewallEnable->setObjectName("btnFirewallEnable");

        firewallEnableLayout->addWidget(btnFirewallEnable);

        btnFirewallDisable = new QPushButton(firewallStatusFrame);
        btnFirewallDisable->setObjectName("btnFirewallDisable");

        firewallEnableLayout->addWidget(btnFirewallDisable);


        firewallStatusLayout->addLayout(firewallEnableLayout);


        firewallTopLayout->addWidget(firewallStatusFrame);

        firewallSettingsLayout = new QGridLayout();
        firewallSettingsLayout->setObjectName("firewallSettingsLayout");
        firewallSettingsLayout->setHorizontalSpacing(12);
        firewallSettingsLayout->setVerticalSpacing(10);
        labelPolicyIncoming = new QLabel(tabFirewall);
        labelPolicyIncoming->setObjectName("labelPolicyIncoming");

        firewallSettingsLayout->addWidget(labelPolicyIncoming, 0, 0, 1, 1);

        comboPolicyIncoming = new QComboBox(tabFirewall);
        comboPolicyIncoming->setObjectName("comboPolicyIncoming");

        firewallSettingsLayout->addWidget(comboPolicyIncoming, 0, 1, 1, 1);

        labelPolicyOutgoing = new QLabel(tabFirewall);
        labelPolicyOutgoing->setObjectName("labelPolicyOutgoing");

        firewallSettingsLayout->addWidget(labelPolicyOutgoing, 1, 0, 1, 1);

        comboPolicyOutgoing = new QComboBox(tabFirewall);
        comboPolicyOutgoing->setObjectName("comboPolicyOutgoing");

        firewallSettingsLayout->addWidget(comboPolicyOutgoing, 1, 1, 1, 1);

        labelPolicyRouted = new QLabel(tabFirewall);
        labelPolicyRouted->setObjectName("labelPolicyRouted");

        firewallSettingsLayout->addWidget(labelPolicyRouted, 2, 0, 1, 1);

        comboPolicyRouted = new QComboBox(tabFirewall);
        comboPolicyRouted->setObjectName("comboPolicyRouted");

        firewallSettingsLayout->addWidget(comboPolicyRouted, 2, 1, 1, 1);

        labelLogging = new QLabel(tabFirewall);
        labelLogging->setObjectName("labelLogging");

        firewallSettingsLayout->addWidget(labelLogging, 3, 0, 1, 1);

        comboLogging = new QComboBox(tabFirewall);
        comboLogging->setObjectName("comboLogging");

        firewallSettingsLayout->addWidget(comboLogging, 3, 1, 1, 1);


        firewallTopLayout->addLayout(firewallSettingsLayout);

        firewallTopSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        firewallTopLayout->addItem(firewallTopSpacer);


        verticalLayoutFirewall->addLayout(firewallTopLayout);

        rulesTable = new QTableView(tabFirewall);
        rulesTable->setObjectName("rulesTable");
        rulesTable->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
        rulesTable->setAlternatingRowColors(true);
        rulesTable->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
        rulesTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        rulesTable->setShowGrid(false);
        rulesTable->setSortingEnabled(false);

        verticalLayoutFirewall->addWidget(rulesTable);

        firewallRuleButtonsLayout = new QHBoxLayout();
        firewallRuleButtonsLayout->setSpacing(8);
        firewallRuleButtonsLayout->setObjectName("firewallRuleButtonsLayout");
        btnAddRule = new QPushButton(tabFirewall);
        btnAddRule->setObjectName("btnAddRule");

        firewallRuleButtonsLayout->addWidget(btnAddRule);

        btnDeleteRule = new QPushButton(tabFirewall);
        btnDeleteRule->setObjectName("btnDeleteRule");

        firewallRuleButtonsLayout->addWidget(btnDeleteRule);

        btnReadFirewall = new QPushButton(tabFirewall);
        btnReadFirewall->setObjectName("btnReadFirewall");

        firewallRuleButtonsLayout->addWidget(btnReadFirewall);

        firewallLastReadLabel = new QLabel(tabFirewall);
        firewallLastReadLabel->setObjectName("firewallLastReadLabel");

        firewallRuleButtonsLayout->addWidget(firewallLastReadLabel);

        firewallRuleButtonsSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        firewallRuleButtonsLayout->addItem(firewallRuleButtonsSpacer);


        verticalLayoutFirewall->addLayout(firewallRuleButtonsLayout);

        networkTabs->addTab(tabFirewall, QString());
        tabInterfaces = new QWidget();
        tabInterfaces->setObjectName("tabInterfaces");
        verticalLayoutInterfaces = new QVBoxLayout(tabInterfaces);
        verticalLayoutInterfaces->setSpacing(12);
        verticalLayoutInterfaces->setObjectName("verticalLayoutInterfaces");
        interfacesTable = new QTableView(tabInterfaces);
        interfacesTable->setObjectName("interfacesTable");
        interfacesTable->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
        interfacesTable->setAlternatingRowColors(true);
        interfacesTable->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
        interfacesTable->setShowGrid(false);
        interfacesTable->setSortingEnabled(true);

        verticalLayoutInterfaces->addWidget(interfacesTable);

        networkTabs->addTab(tabInterfaces, QString());
        tabConnections = new QWidget();
        tabConnections->setObjectName("tabConnections");
        verticalLayoutConnections = new QVBoxLayout(tabConnections);
        verticalLayoutConnections->setSpacing(12);
        verticalLayoutConnections->setObjectName("verticalLayoutConnections");
        connectionsTable = new QTableView(tabConnections);
        connectionsTable->setObjectName("connectionsTable");
        connectionsTable->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
        connectionsTable->setAlternatingRowColors(true);
        connectionsTable->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
        connectionsTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        connectionsTable->setShowGrid(false);
        connectionsTable->setSortingEnabled(false);

        verticalLayoutConnections->addWidget(connectionsTable);

        connectionButtonsLayout = new QHBoxLayout();
        connectionButtonsLayout->setSpacing(8);
        connectionButtonsLayout->setObjectName("connectionButtonsLayout");
        btnActivateConnection = new QPushButton(tabConnections);
        btnActivateConnection->setObjectName("btnActivateConnection");

        connectionButtonsLayout->addWidget(btnActivateConnection);

        btnDeactivateConnection = new QPushButton(tabConnections);
        btnDeactivateConnection->setObjectName("btnDeactivateConnection");

        connectionButtonsLayout->addWidget(btnDeactivateConnection);

        btnRefreshConnections = new QPushButton(tabConnections);
        btnRefreshConnections->setObjectName("btnRefreshConnections");

        connectionButtonsLayout->addWidget(btnRefreshConnections);

        connectionButtonsSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        connectionButtonsLayout->addItem(connectionButtonsSpacer);


        verticalLayoutConnections->addLayout(connectionButtonsLayout);

        networkTabs->addTab(tabConnections, QString());
        tabLocals = new QWidget();
        tabLocals->setObjectName("tabLocals");
        verticalLayoutLocals = new QVBoxLayout(tabLocals);
        verticalLayoutLocals->setSpacing(12);
        verticalLayoutLocals->setObjectName("verticalLayoutLocals");
        localsTable = new QTableView(tabLocals);
        localsTable->setObjectName("localsTable");
        localsTable->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
        localsTable->setAlternatingRowColors(true);
        localsTable->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
        localsTable->setShowGrid(false);
        localsTable->setSortingEnabled(false);

        verticalLayoutLocals->addWidget(localsTable);

        localsWarningLabel = new QLabel(tabLocals);
        localsWarningLabel->setObjectName("localsWarningLabel");
        localsWarningLabel->setVisible(false);
        localsWarningLabel->setWordWrap(true);

        verticalLayoutLocals->addWidget(localsWarningLabel);

        localsButtonsLayout = new QHBoxLayout();
        localsButtonsLayout->setSpacing(8);
        localsButtonsLayout->setObjectName("localsButtonsLayout");
        labelLocalsScope = new QLabel(tabLocals);
        labelLocalsScope->setObjectName("labelLocalsScope");

        localsButtonsLayout->addWidget(labelLocalsScope);

        comboLocalsScope = new QComboBox(tabLocals);
        comboLocalsScope->setObjectName("comboLocalsScope");

        localsButtonsLayout->addWidget(comboLocalsScope);

        btnScanLocals = new QPushButton(tabLocals);
        btnScanLocals->setObjectName("btnScanLocals");

        localsButtonsLayout->addWidget(btnScanLocals);

        localsLastScanLabel = new QLabel(tabLocals);
        localsLastScanLabel->setObjectName("localsLastScanLabel");

        localsButtonsLayout->addWidget(localsLastScanLabel);

        localsButtonsSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        localsButtonsLayout->addItem(localsButtonsSpacer);


        verticalLayoutLocals->addLayout(localsButtonsLayout);

        networkTabs->addTab(tabLocals, QString());

        verticalLayoutNetwork->addWidget(networkTabs);

        stackedWidget->addWidget(pageNetwork);
        pageServices = new QWidget();
        pageServices->setObjectName("pageServices");
        verticalLayoutServices = new QVBoxLayout(pageServices);
        verticalLayoutServices->setSpacing(12);
        verticalLayoutServices->setObjectName("verticalLayoutServices");
        servicesTopLayout = new QHBoxLayout();
        servicesTopLayout->setSpacing(16);
        servicesTopLayout->setObjectName("servicesTopLayout");
        servicesListLayout = new QVBoxLayout();
        servicesListLayout->setSpacing(10);
        servicesListLayout->setObjectName("servicesListLayout");
        servicesTable = new QTableView(pageServices);
        servicesTable->setObjectName("servicesTable");
        servicesTable->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
        servicesTable->setAlternatingRowColors(true);
        servicesTable->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
        servicesTable->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        servicesTable->setShowGrid(false);
        servicesTable->setSortingEnabled(true);

        servicesListLayout->addWidget(servicesTable);

        serviceActionsLayout = new QHBoxLayout();
        serviceActionsLayout->setSpacing(8);
        serviceActionsLayout->setObjectName("serviceActionsLayout");
        btnStartService = new QPushButton(pageServices);
        btnStartService->setObjectName("btnStartService");

        serviceActionsLayout->addWidget(btnStartService);

        btnStopService = new QPushButton(pageServices);
        btnStopService->setObjectName("btnStopService");

        serviceActionsLayout->addWidget(btnStopService);

        btnEnableService = new QPushButton(pageServices);
        btnEnableService->setObjectName("btnEnableService");

        serviceActionsLayout->addWidget(btnEnableService);

        btnDisableService = new QPushButton(pageServices);
        btnDisableService->setObjectName("btnDisableService");

        serviceActionsLayout->addWidget(btnDisableService);

        btnRefreshServices = new QPushButton(pageServices);
        btnRefreshServices->setObjectName("btnRefreshServices");

        serviceActionsLayout->addWidget(btnRefreshServices);

        serviceActionsSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        serviceActionsLayout->addItem(serviceActionsSpacer);


        servicesListLayout->addLayout(serviceActionsLayout);


        servicesTopLayout->addLayout(servicesListLayout);

        criticalWarningFrame = new QFrame(pageServices);
        criticalWarningFrame->setObjectName("criticalWarningFrame");
        criticalWarningFrame->setMinimumSize(QSize(220, 110));
        criticalWarningFrame->setMaximumSize(QSize(260, 16777215));
        criticalWarningFrame->setFrameShape(QFrame::Shape::StyledPanel);
        criticalWarningFrame->setFrameShadow(QFrame::Shadow::Plain);
        criticalWarningLayout = new QVBoxLayout(criticalWarningFrame);
        criticalWarningLayout->setObjectName("criticalWarningLayout");
        criticalStatusLabel = new QLabel(criticalWarningFrame);
        criticalStatusLabel->setObjectName("criticalStatusLabel");
        criticalStatusLabel->setAlignment(Qt::AlignmentFlag::AlignTop);
        criticalStatusLabel->setWordWrap(true);

        criticalWarningLayout->addWidget(criticalStatusLabel);


        servicesTopLayout->addWidget(criticalWarningFrame);


        verticalLayoutServices->addLayout(servicesTopLayout);

        servicesBottomPlaceholder = new QWidget(pageServices);
        servicesBottomPlaceholder->setObjectName("servicesBottomPlaceholder");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(servicesBottomPlaceholder->sizePolicy().hasHeightForWidth());
        servicesBottomPlaceholder->setSizePolicy(sizePolicy);
        servicesBottomPlaceholder->setMinimumSize(QSize(0, 70));

        verticalLayoutServices->addWidget(servicesBottomPlaceholder);

        stackedWidget->addWidget(pageServices);
        pageOther = new QWidget();
        pageOther->setObjectName("pageOther");
        verticalLayoutOther = new QVBoxLayout(pageOther);
        verticalLayoutOther->setObjectName("verticalLayoutOther");
        labelOther = new QLabel(pageOther);
        labelOther->setObjectName("labelOther");
        labelOther->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayoutOther->addWidget(labelOther);

        stackedWidget->addWidget(pageOther);

        verticalLayout->addWidget(stackedWidget);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 22));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        stackedWidget->setCurrentIndex(0);
        networkTabs->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "QControl", nullptr));
        btnNetwork->setText(QCoreApplication::translate("MainWindow", "Network", nullptr));
        btnServices->setText(QCoreApplication::translate("MainWindow", "Services", nullptr));
        btnOther->setText(QCoreApplication::translate("MainWindow", "Other", nullptr));
        firewallStatusLabel->setText(QCoreApplication::translate("MainWindow", "Checking firewall\342\200\246", nullptr));
#if QT_CONFIG(tooltip)
        btnFirewallEnable->setToolTip(QCoreApplication::translate("MainWindow", "Enable ufw at boot and start filtering now.", nullptr));
#endif // QT_CONFIG(tooltip)
        btnFirewallEnable->setText(QCoreApplication::translate("MainWindow", "Enable", nullptr));
#if QT_CONFIG(tooltip)
        btnFirewallDisable->setToolTip(QCoreApplication::translate("MainWindow", "Stop ufw filtering. All traffic becomes unfiltered.", nullptr));
#endif // QT_CONFIG(tooltip)
        btnFirewallDisable->setText(QCoreApplication::translate("MainWindow", "Disable", nullptr));
        labelPolicyIncoming->setText(QCoreApplication::translate("MainWindow", "Incoming policy", nullptr));
        labelPolicyOutgoing->setText(QCoreApplication::translate("MainWindow", "Outgoing policy", nullptr));
        labelPolicyRouted->setText(QCoreApplication::translate("MainWindow", "Routed policy", nullptr));
        labelLogging->setText(QCoreApplication::translate("MainWindow", "Logging", nullptr));
        btnAddRule->setText(QCoreApplication::translate("MainWindow", "Add Rule\342\200\246", nullptr));
        btnDeleteRule->setText(QCoreApplication::translate("MainWindow", "Delete", nullptr));
#if QT_CONFIG(tooltip)
        btnReadFirewall->setToolTip(QCoreApplication::translate("MainWindow", "Read ufw rules with admin rights and save them to the local cache.", nullptr));
#endif // QT_CONFIG(tooltip)
        btnReadFirewall->setText(QCoreApplication::translate("MainWindow", "Read", nullptr));
        firewallLastReadLabel->setText(QCoreApplication::translate("MainWindow", "Last read: never", nullptr));
        networkTabs->setTabText(networkTabs->indexOf(tabFirewall), QCoreApplication::translate("MainWindow", "Firewall", nullptr));
        networkTabs->setTabText(networkTabs->indexOf(tabInterfaces), QCoreApplication::translate("MainWindow", "Interfaces", nullptr));
#if QT_CONFIG(tooltip)
        btnActivateConnection->setToolTip(QCoreApplication::translate("MainWindow", "Bring the selected connection up. Asks for the admin password only if NetworkManager requires it.", nullptr));
#endif // QT_CONFIG(tooltip)
        btnActivateConnection->setText(QCoreApplication::translate("MainWindow", "Activate", nullptr));
#if QT_CONFIG(tooltip)
        btnDeactivateConnection->setToolTip(QCoreApplication::translate("MainWindow", "Bring the selected connection down.", nullptr));
#endif // QT_CONFIG(tooltip)
        btnDeactivateConnection->setText(QCoreApplication::translate("MainWindow", "Deactivate", nullptr));
        btnRefreshConnections->setText(QCoreApplication::translate("MainWindow", "Refresh", nullptr));
        networkTabs->setTabText(networkTabs->indexOf(tabConnections), QCoreApplication::translate("MainWindow", "Connections", nullptr));
        labelLocalsScope->setText(QCoreApplication::translate("MainWindow", "Scope", nullptr));
#if QT_CONFIG(tooltip)
        comboLocalsScope->setToolTip(QCoreApplication::translate("MainWindow", "Routed subnet scans the whole subnet from the kernel route (can be large). Local /24 scans only the /24 that contains this computer's address.", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        btnScanLocals->setToolTip(QCoreApplication::translate("MainWindow", "Scan the local network with arp-scan and nmap. Asks for the admin password.", nullptr));
#endif // QT_CONFIG(tooltip)
        btnScanLocals->setText(QCoreApplication::translate("MainWindow", "Scan", nullptr));
        localsLastScanLabel->setText(QCoreApplication::translate("MainWindow", "Last scan: never", nullptr));
        networkTabs->setTabText(networkTabs->indexOf(tabLocals), QCoreApplication::translate("MainWindow", "Locals", nullptr));
        btnStartService->setText(QCoreApplication::translate("MainWindow", "Start", nullptr));
        btnStopService->setText(QCoreApplication::translate("MainWindow", "Stop", nullptr));
#if QT_CONFIG(tooltip)
        btnEnableService->setToolTip(QCoreApplication::translate("MainWindow", "Enable this service at boot; this does not start it now.", nullptr));
#endif // QT_CONFIG(tooltip)
        btnEnableService->setText(QCoreApplication::translate("MainWindow", "Enable", nullptr));
#if QT_CONFIG(tooltip)
        btnDisableService->setToolTip(QCoreApplication::translate("MainWindow", "Disable this service at boot; this does not stop it now.", nullptr));
#endif // QT_CONFIG(tooltip)
        btnDisableService->setText(QCoreApplication::translate("MainWindow", "Disable", nullptr));
        btnRefreshServices->setText(QCoreApplication::translate("MainWindow", "Refresh", nullptr));
        criticalStatusLabel->setText(QCoreApplication::translate("MainWindow", "Checking critical services\342\200\246", nullptr));
        labelOther->setText(QCoreApplication::translate("MainWindow", "Welcome to the Other page", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
