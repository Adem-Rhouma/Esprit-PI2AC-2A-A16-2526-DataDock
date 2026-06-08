#ifndef EQUIPEMENTSSMSALERTSPAGE_H
#define EQUIPEMENTSSMSALERTSPAGE_H

#include <QWidget>

#include "materiel.h"

class QLineEdit;
class QPlainTextEdit;
class QTableWidget;
class QLabel;
class QPushButton;

class EquipementsSmsAlertsPage : public QWidget
{
    Q_OBJECT

public:
    explicit EquipementsSmsAlertsPage(QWidget *parent = nullptr);

    void refreshData();

private:
    void applyStyling();
    void buildUi();
    void loadFaultyEquipments();
    void selectFaultyEquipment(int row, int column);
    QString buildSmsMessage(const materiel &value) const;
    static QString normalizePhoneNumber(const QString &phone);
    static bool isValidE164Phone(const QString &phone);
    void prepareSms();
    void sendSms();
    static bool isFaultyEtat(const QString &etat);

    QTableWidget *tableAlerts;
    QLabel *lblCountValue;
    QLabel *lblSelectedIdValue;
    QLabel *lblSelectedLabelValue;
    QLabel *lblSelectedEtatValue;
    QLineEdit *inputPhoneNumber;
    QPlainTextEdit *messagePreview;
    QPlainTextEdit *sentLog;
    QPushButton *btnRefresh;
    QPushButton *btnPrepare;
    QPushButton *btnSend;

    QList<materiel> m_faultyEquipments;
    int m_selectedRow;
};

#endif // EQUIPEMENTSSMSALERTSPAGE_H
