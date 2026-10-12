#include "addruledialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

AddRuleDialog::AddRuleDialog(QWidget *parent)
    : QDialog(parent)
    , m_actionCombo(new QComboBox(this))
    , m_protocolCombo(new QComboBox(this))
    , m_portEdit(new QLineEdit(this))
    , m_fromEdit(new QLineEdit(this))
    , m_commentEdit(new QLineEdit(this))
{
    setObjectName(QStringLiteral("addRuleDialog"));
    setWindowTitle(tr("Add Firewall Rule"));
    setMinimumWidth(440);

    m_actionCombo->addItem(tr("Allow"), QStringLiteral("allow"));
    m_actionCombo->addItem(tr("Deny"), QStringLiteral("deny"));
    m_actionCombo->addItem(tr("Reject"), QStringLiteral("reject"));

    m_protocolCombo->addItem(tr("Any"), QString());
    m_protocolCombo->addItem(QStringLiteral("TCP"), QStringLiteral("tcp"));
    m_protocolCombo->addItem(QStringLiteral("UDP"), QStringLiteral("udp"));

    m_portEdit->setPlaceholderText(tr("e.g. 22, 6000:6007, or 80,443 (empty = any)"));
    m_fromEdit->setPlaceholderText(tr("any, 192.168.1.10, or 10.0.0.0/8 (empty = any)"));
    m_commentEdit->setPlaceholderText(tr("Optional, up to 80 characters"));
    m_commentEdit->setMaxLength(80);

    auto *form = new QFormLayout;
    form->addRow(tr("Action:"), m_actionCombo);
    form->addRow(tr("Protocol:"), m_protocolCombo);
    form->addRow(tr("Port:"), m_portEdit);
    form->addRow(tr("From:"), m_fromEdit);
    form->addRow(tr("Comment:"), m_commentEdit);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &AddRuleDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

FirewallRuleSpec AddRuleDialog::ruleSpec() const
{
    FirewallRuleSpec spec;
    spec.action = m_actionCombo->currentData().toString();
    spec.protocol = m_protocolCombo->currentData().toString();
    spec.port = m_portEdit->text().trimmed();
    spec.from = m_fromEdit->text().trimmed();
    spec.comment = m_commentEdit->text().trimmed();
    return spec;
}

void AddRuleDialog::accept()
{
    const QString error = FirewallManager::validateRule(ruleSpec());
    if (!error.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), error);
        return;
    }
    QDialog::accept();
}
