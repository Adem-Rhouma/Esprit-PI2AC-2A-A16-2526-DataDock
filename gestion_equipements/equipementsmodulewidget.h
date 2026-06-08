#ifndef EQUIPEMENTSMODULEWIDGET_H
#define EQUIPEMENTSMODULEWIDGET_H

#include <QWidget>

class QTabWidget;
class EquipementsStatsPage;
class EquipementsQrCodePage;
class EquipementsSmsAlertsPage;

class EquipementsModuleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EquipementsModuleWidget(QWidget *parent = nullptr);

private:
    QTabWidget *tabWidget;
    EquipementsStatsPage *statsPage;
    EquipementsQrCodePage *qrCodePage;
    EquipementsSmsAlertsPage *smsAlertsPage;
};

#endif // EQUIPEMENTSMODULEWIDGET_H
