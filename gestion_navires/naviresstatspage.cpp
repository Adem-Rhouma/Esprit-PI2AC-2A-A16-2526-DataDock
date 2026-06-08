#include "naviresstatspage.h"
#include "ui_naviresstatspage.h"
#include "connection.h"
#include "NavireDatabaseHelper.h"
#include "navire.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QMenu>
#include <QAction>

#include <QDate>
#include <QDateTime>

#include "naviredetailpage.h"
#include "navirepredictivepage.h"
#include <QSqlRecord>
#include <QHeaderView>
#include <QLocale>
#include <QVariant>

#include <QDate>
#include <QDateTime>
#include <QTimer>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QDateTime>
#include <QLabel>
#include <QVBoxLayout>

#include "meteoglobalservice.h"
#include "meteostripwidget.h"
#include "meteowidget.h"


NaviresStatsPage::NaviresStatsPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NaviresStatsPage)
{
    ui->setupUi(this);

    MeteoStripWidget *meteoStrip = new MeteoStripWidget(this);
    ui->scrollLayout->insertWidget(1, meteoStrip);
    
    connect(&MeteoGlobalService::instance(), &MeteoGlobalService::dataUpdated, 
            meteoStrip, &MeteoStripWidget::updateData);
            
    connect(meteoStrip, &MeteoStripWidget::clicked, this, &NaviresStatsPage::requestMeteoTab);
    
    MeteoGlobalService::instance().refresh();

    lineChart = new QChart();
    donutChart = new QChart();
    barChart = new QChart();

    applyStyling();
    
    // Ensure database schema is ready for this module
    NavireDatabaseHelper::initializeSchema();

    loadStatistics();

    connect(ui->btnRefresh, &QToolButton::clicked, this, &NaviresStatsPage::loadStatistics);
    
    // Connect ledger double-click to detail page
    connect(ui->tableRecent, &QTableWidget::cellDoubleClicked, [this](int row, int col){
        Q_UNUSED(col);
        int shipId = ui->tableRecent->item(row, 0)->text().toInt();
        NavireDetailPage *detail = new NavireDetailPage(shipId);
        detail->setWindowTitle("Détails du Navire - " + ui->tableRecent->item(row, 1)->text());
        detail->show();
    });

    ui->tableRecent->verticalHeader()->setDefaultSectionSize(50);
    ui->tableRecent->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableRecent, &QTableWidget::customContextMenuRequested, this, &NaviresStatsPage::showContextMenu);

    QTimer *refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &NaviresStatsPage::loadStatistics);
    refreshTimer->start(60000); 
}

NaviresStatsPage::~NaviresStatsPage() {
    delete ui;
}

void NaviresStatsPage::applyStyling() {
    setAttribute(Qt::WA_StyledBackground, true);
    
    // Identifiers for CSS (Main Branch Pattern)
    const QList<QFrame*> cards = {
        ui->cardKpiTotalNavires, ui->cardKpiHours, ui->cardKpiFuel, ui->cardKpiAlerts,
        ui->cardKpiUtilization, ui->cardKpiLoad, ui->cardKpiExposure, ui->cardKpiStrain,
        ui->cardChartLine, ui->cardChartDonut, ui->cardChartBar, ui->cardRecentTable
    };
    for(auto* card : cards) {
        card->setProperty("card", "stats");
        card->setAttribute(Qt::WA_StyledBackground, true);
    }

    ui->lblTitle->setProperty("role", "pageTitle");
    ui->lblSubtitle->setProperty("role", "pageSubtitle");

    const QString style = R"(
        QWidget { color: #1e293b; background: transparent; }
        QLabel[role="pageTitle"] { font-size: 28px; font-weight: 800; color: #0f172a; }
        QLabel[role="pageSubtitle"] { font-size: 15px; color: #5390D9; font-weight: 600; }
        QFrame[card="stats"] { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 16px; font-size: 12px; }
        QLabel[name^="lblKpi"][name$="Title"] { color: #64748b; font-weight: 700; font-size: 10px; text-transform: uppercase; }
        QLabel[name^="lblKpi"][name$="Value"] { color: #020617; font-size: 22px; font-weight: 800; }
        QLabel[name^="lblKpi"][name$="Badge"] { color: #5390D9; font-weight: 700; font-size: 11px; }
        QTableWidget { background: #ffffff; color: #1e293b; font-size: 14px; border: none; gridline-color: #f1f5f9; }
        QHeaderView::section { background: #f8fafc; color: #475569; font-weight: 700; border: none; padding: 12px; font-size: 14px; }
    )";
    setStyleSheet(style);
}

void NaviresStatsPage::loadStatistics() {
    QSqlQuery q(Connection::createInstance().db);
    // Helper to check if any data exists
    bool hasData = false;
    if (q.exec("SELECT COUNT(*) FROM VESSEL_LOGS") && q.next()) {
        hasData = (q.value(0).toInt() > 0);
    }

    // 1) KPIs - TOP ROW
    // Heures Mer (30j)
    if (hasData && q.exec("SELECT NVL(SUM((RETURN_TIME - DEPARTURE_TIME) * 24), 0) FROM VESSEL_LOGS WHERE RETURN_TIME > SYSDATE - 30") && q.next()) {
        ui->lblKpiHoursValue->setText(QString::number(q.value(0).toDouble(), 'f', 1) + "h");
    } else {
        ui->lblKpiHoursValue->setText("0.0h");
    }

    // Efficacité Fuel (L/nm)
    if (hasData && q.exec("SELECT NVL(SUM(FUEL_CONSUMED) / NULLIF(SUM(DISTANCE_NM), 0), 0) FROM VESSEL_LOGS WHERE RETURN_TIME > SYSDATE - 30") && q.next()) {
        ui->lblKpiFuelValue->setText(QString::number(q.value(0).toDouble(), 'f', 2) + " L/nm");
    } else {
        ui->lblKpiFuelValue->setText("N/A");
    }

    // Maintenance Imminente (Real data)
    if (q.exec("SELECT COUNT(*) FROM MAINTENANCE_TASKS WHERE SCHEDULED_DATE BETWEEN SYSDATE AND SYSDATE + 7 AND TASK_STATUS != 'COMPLETED'") && q.next()) {
        ui->lblKpiAlertsValue->setText(q.value(0).toString());
    } else {
        ui->lblKpiAlertsValue->setText("0");
    }

    // 2) KPIs - MIDDLE ROW
    // Utilisation Flotte (%)
    if (q.exec("SELECT COUNT(DISTINCT IDNAVIRE) * 100.0 / NULLIF((SELECT COUNT(*) FROM NAVIRES), 0) FROM VESSEL_LOGS WHERE RETURN_TIME > SYSDATE - 30") && q.next()) {
        double val = q.value(0).toDouble();
        ui->lblKpiUtilizationValue->setText(QString::number(val, 'f', 1) + "%");
    } else {
        ui->lblKpiUtilizationValue->setText("0.0%");
    }

    // Efficacité Cargo (%)
    if (hasData && q.exec("SELECT NVL(SUM(L.CARGO_LOAD_TONS) * 100.0 / NULLIF(SUM(N.MAX_CARGO_CAPACITY), 0), 0) "
               "FROM VESSEL_LOGS L JOIN NAVIRES N ON L.IDNAVIRE = N.IDNAVIRE "
               "WHERE L.RETURN_TIME > SYSDATE - 30") && q.next()) {
        ui->lblKpiLoadValue->setText(QString::number(q.value(0).toDouble(), 'f', 1) + "%");
    } else {
        ui->lblKpiLoadValue->setText("0.0%");
    }

    // Static Fleet KPIs
    if (q.exec("SELECT COUNT(*) FROM NAVIRES") && q.next()) {
        ui->lblKpiTotalNaviresValue->setText(q.value(0).toString());
    }
    
    if (q.exec("SELECT NVL(SUM(CASE WHEN TASK_STATUS = 'COMPLETED' THEN 1 ELSE 0 END) * 100.0 / NULLIF(COUNT(*), 0), 0) FROM MAINTENANCE_TASKS") && q.next()) {
         ui->lblKpiTotalNaviresBadge->setText(QString("Readiness: %1%").arg(QString::number(q.value(0).toDouble(), 'f', 0)));
    }

    // Clear removed indices
    ui->lblKpiExposureValue->setText("--");
    ui->lblKpiStrainValue->setText("--");

    // 3) LINE CHART: Activity (Distance over time)
    lineChart->removeAllSeries();
    QLineSeries *distSeries = new QLineSeries();
    distSeries->setName("Distance parcourue (nm)");
    if (hasData && q.exec("SELECT TRUNC(RETURN_TIME), SUM(DISTANCE_NM) FROM VESSEL_LOGS "
               "WHERE RETURN_TIME > SYSDATE - 30 GROUP BY TRUNC(RETURN_TIME) ORDER BY 1 ASC")) {
        while (q.next()) {
            distSeries->append(q.value(0).toDateTime().toMSecsSinceEpoch(), q.value(1).toDouble());
        }
    }
    lineChart->addSeries(distSeries);
    lineChart->setTitle("Activité Flotte (Distance nm / 30j)");
    lineChart->setBackgroundVisible(false);

    if (lineChart->axes().isEmpty()) {
        QDateTimeAxis *ax = new QDateTimeAxis();
        ax->setFormat("dd/MM");
        lineChart->addAxis(ax, Qt::AlignBottom);
        distSeries->attachAxis(ax);
        QValueAxis *ay = new QValueAxis();
        ay->setTitleText("nm");
        lineChart->addAxis(ay, Qt::AlignLeft);
        distSeries->attachAxis(ay);
    }

    if (ui->chartLineContainer->layout() == nullptr) {
        QVBoxLayout *layout = new QVBoxLayout(ui->chartLineContainer);
        QChartView *chartView = new QChartView(lineChart);
        chartView->setRenderHint(QPainter::Antialiasing);
        layout->addWidget(chartView);
    }

    // 4) DONUT CHART: Distribution by Type
    donutChart->removeAllSeries();
    QPieSeries *pieSeries = new QPieSeries();
    if (q.exec("SELECT TYPENAVIRE, COUNT(*) FROM NAVIRES GROUP BY TYPENAVIRE")) {
        while (q.next()) {
            pieSeries->append(q.value(0).toString(), q.value(1).toInt());
        }
    }
    pieSeries->setHoleSize(0.6);
    donutChart->addSeries(pieSeries);
    donutChart->setTitle("Répartition Flotte");
    donutChart->setBackgroundVisible(false);
    donutChart->legend()->setAlignment(Qt::AlignBottom);

    if (ui->chartDonutContainer->layout() == nullptr) {
        QVBoxLayout *layout = new QVBoxLayout(ui->chartDonutContainer);
        QChartView *chartView = new QChartView(donutChart);
        chartView->setRenderHint(QPainter::Antialiasing);
        layout->addWidget(chartView);
    }

    // 5) BAR CHART: Top Performers (Most nautical miles)
    barChart->removeAllSeries();
    const auto axes = barChart->axes();
    for (auto axis : axes) barChart->removeAxis(axis);

    QBarSet *distSet = new QBarSet("Distance Totale (nm)");
    QStringList categories;
    if (hasData && q.exec("SELECT * FROM ("
               "  SELECT N.NOMNAVIRE, SUM(L.DISTANCE_NM) as TOTAL_DIST "
               "  FROM NAVIRES N JOIN VESSEL_LOGS L ON N.IDNAVIRE = L.IDNAVIRE "
               "  GROUP BY N.NOMNAVIRE ORDER BY TOTAL_DIST DESC"
               ") WHERE ROWNUM <= 3")) {
        while (q.next()) {
            categories << q.value(0).toString();
            distSet->append(q.value(1).toDouble());
        }
    }

    QBarSeries *barSeries = new QBarSeries();
    barSeries->append(distSet);
    barChart->addSeries(barSeries);
    barChart->setTitle("Top 3 Navires (Activité nm)");
    barChart->setBackgroundVisible(false);

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    barChart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    barChart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);

    if (ui->chartBarContainer->layout() == nullptr) {
        QVBoxLayout *layout = new QVBoxLayout(ui->chartBarContainer);
        QChartView *chartView = new QChartView(barChart);
        chartView->setRenderHint(QPainter::Antialiasing);
        layout->addWidget(chartView);
    }

    // 6) TABLE: Ledger
    Navire n;
    QSqlQueryModel *model = n.afficher();
    ui->tableRecent->setRowCount(0);
    ui->tableRecent->setColumnCount(model->columnCount());
    QStringList headers;
    for (int i = 0; i < model->columnCount(); ++i) {
        headers << model->headerData(i, Qt::Horizontal).toString();
    }
    ui->tableRecent->setHorizontalHeaderLabels(headers);
    ui->tableRecent->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableRecent->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableRecent->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    for (int row = 0; row < model->rowCount(); ++row) {
        ui->tableRecent->insertRow(row);
        for (int col = 0; col < model->columnCount(); ++col) {
            ui->tableRecent->setItem(row, col, new QTableWidgetItem(model->record(row).value(col).toString()));
        }
    }
    delete model;
}

void NaviresStatsPage::showContextMenu(const QPoint &pos) {
    int row = ui->tableRecent->rowAt(pos.y());
    if (row < 0) return;

    int shipId = ui->tableRecent->item(row, 0)->text().toInt();
    QString shipName = ui->tableRecent->item(row, 1)->text();

    QMenu menu(this);
    menu.setStyleSheet("QMenu { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; padding: 4px; }"
                       "QMenu::item { padding: 8px 16px; border-radius: 4px; }"
                       "QMenu::item:selected { background-color: #f1f5f9; }");

    QAction *actionNormal = menu.addAction("🚢 Détails Standards");
    QAction *actionPredictive = menu.addAction("🧠 Analyse Prédictive & Maintenance");

    QAction *selectedAction = menu.exec(ui->tableRecent->viewport()->mapToGlobal(pos));

    if (selectedAction == actionNormal) {
        NavireDetailPage *detail = new NavireDetailPage(shipId);
        detail->setWindowTitle("Détails du Navire - " + shipName);
        detail->show();
    } else if (selectedAction == actionPredictive) {
        openPredictiveAnalysis(shipId, shipName);
    }
}

void NaviresStatsPage::openPredictiveAnalysis(int shipId, const QString &shipName) {
    qDebug() << "Opening predictive analysis for ship" << shipId << shipName;
    NavirePredictivePage *predPage = new NavirePredictivePage(shipId);
    predPage->setWindowTitle("Analyse Prédictive - " + shipName);
    predPage->show();
}

