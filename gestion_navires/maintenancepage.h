#ifndef MAINTENANCEPAGE_H
#define MAINTENANCEPAGE_H

#include <QWidget>
#include "maintenancemodel.h"
#include "maintenancegantt.h"
#include "meteo_data.h"

namespace Ui {
class MaintenancePage;
}

class MaintenancePage : public QWidget {
    Q_OBJECT
public:
    explicit MaintenancePage(QWidget *parent = nullptr);
    ~MaintenancePage();

    void refreshData(const MeteoData::MeteoReport &weather);

private:
    Ui::MaintenancePage *ui;
    MaintenanceModel *model;
    MaintenanceGantt *gantt;
};

#endif // MAINTENANCEPAGE_H
