#ifndef ADDRULEDIALOG_H
#define ADDRULEDIALOG_H

#include <QDialog>

#include "firewallmanager.h"

class QComboBox;
class QLineEdit;

class AddRuleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddRuleDialog(QWidget *parent = nullptr);

    FirewallRuleSpec ruleSpec() const;

public slots:
    void accept() override;

private:
    QComboBox *m_actionCombo;
    QComboBox *m_protocolCombo;
    QLineEdit *m_portEdit;
    QLineEdit *m_fromEdit;
    QLineEdit *m_commentEdit;
};

#endif // ADDRULEDIALOG_H
