#ifndef CHAMBRESFROIDESALERTINBOX_H
#define CHAMBRESFROIDESALERTINBOX_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QMap>

class ChambresFroidesAlertInbox : public QDialog
{
    Q_OBJECT

public:
    explicit ChambresFroidesAlertInbox(QWidget *parent = nullptr);

private slots:
    void refreshAlerts();

private:
    void setupUi();
    void applyStyles();
    QWidget* createAlertItem(const QMap<QString, QString> &alertData);

    QPushButton *m_btnRefresh;
    QVBoxLayout *m_alertsLayout;
    QWidget *m_scrollContent;
};

#endif // CHAMBRESFROIDESALERTINBOX_H
