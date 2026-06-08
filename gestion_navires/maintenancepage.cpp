#include "maintenancepage.h"
#include "ui_maintenancepage.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include "maintenanceengine.h"

MaintenancePage::MaintenancePage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MaintenancePage)
{
    ui->setupUi(this);

    model = new MaintenanceModel(this);
    ui->tableView->setModel(model);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    gantt = new MaintenanceGantt(this);
    ui->ganttCard->layout()->addWidget(gantt);

    model->refresh();

    // Fix White-on-White text visibility
    const QString style = R"(
        QWidget { color: #1c232b; }
        QLabel { color: #1c232b; }
        QHeaderView::section { background: #f7f9fc; color: #000000; font-weight: 600; border: none; padding: 4px; }
        QTableView { background: #ffffff; color: #1c232b; selection-background-color: #eaf3ff; selection-color: #1c232b; gridline-color: #f1f5f9; border-radius: 10px; }
    )";
    setStyleSheet(style);
}

MaintenancePage::~MaintenancePage() {
    delete ui;
}

void MaintenancePage::refreshData(const MeteoData::MeteoReport &weather) {
    model->refresh();
    
    QMap<QString, QDate> maintenanceDates;
    QSqlQuery q("SELECT IDNAVIRE, NOMNAVIRE FROM NAVIRES");
    while (q.next()) {
        int navId = q.value("IDNAVIRE").toInt();
        QString name = q.value("NOMNAVIRE").toString();
        
        PredictionResult pred = MaintenanceEngine::predictNextMaintenance(navId, weather);
        maintenanceDates.insert(name, pred.predictedDate);
    }
    gantt->setData(maintenanceDates);
}
