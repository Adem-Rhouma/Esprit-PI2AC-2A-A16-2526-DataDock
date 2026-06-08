#include "logistiquestatspage.h"
#include "ui_logistiquestatspage.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QAreaSeries>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QMap>
#include <QTimer>
#include <QToolTip>
#include <QCursor>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDate>
#include <QChart>
#include <QLineSeries>
#include <QChartView>
#include <QCategoryAxis>
#include <QValueAxis>

namespace {
void ensureChartLayout(QWidget *container)
{
    if (!container || container->layout()) {
        return;
    }
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

QString resolveIconPath(const QString &resourcePath)
{
    if (QFile::exists(resourcePath)) {
        return resourcePath;
    }

    const QString fileName = QFileInfo(resourcePath).fileName();
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + "/../assets/img/" + fileName,
        appDir + "/../../assets/img/" + fileName,
        appDir + "/../../../assets/img/" + fileName,
        appDir + "/../../../../assets/img/" + fileName,
        "assets/img/" + fileName,
        "../assets/img/" + fileName,
        "../../assets/img/" + fileName
    };

    for (const QString &candidate : candidates) {
        const QString cleaned = QDir::cleanPath(candidate);
        if (QFile::exists(cleaned)) {
            return cleaned;
        }
    }

    return resourcePath;
}

void setLabelIcon(QLabel *label, const QString &path, int size)
{
    if (!label) {
        return;
    }
    QPixmap pixmap(resolveIconPath(path));
    if (!pixmap.isNull()) {
        const QBitmap mask = pixmap.createMaskFromColor(Qt::white);
        pixmap.setMask(mask);
    }
    label->setPixmap(pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    label->setFixedSize(size, size);
}

void setToolButtonIcon(QToolButton *button, const QString &path, int size)
{
    if (!button) {
        return;
    }
    QPixmap pixmap(resolveIconPath(path));
    if (!pixmap.isNull()) {
        const QBitmap mask = pixmap.createMaskFromColor(Qt::white);
        pixmap.setMask(mask);
    }
    button->setIcon(QIcon(pixmap));
    button->setIconSize(QSize(size, size));
}

// Helper: clear all widgets from a layout, then add new chart view
QChartView* replaceChartInContainer(QWidget *container, QChart *chart)
{
    ensureChartLayout(container);
    QLayout *lay = container->layout();
    while (QLayoutItem *item = lay->takeAt(0)) { delete item->widget(); delete item; }
    auto *view = new QChartView(chart, container);
    view->setRenderHint(QPainter::Antialiasing);
    lay->addWidget(view);
    return view;
}

// Helper: create a coloured bar chart from a SQL grouped query (label col, count col)
QChart* makeBarChartFromQuery(QSqlQuery &q, const QString &setName)
{
    auto *barSet = new QBarSet(setName);
    QStringList labels;
    int maxVal = 0;
    while (q.next()) {
        int cnt = q.value(1).toInt();
        *barSet << cnt;
        labels << q.value(0).toString();
        if (cnt > maxVal) maxVal = cnt;
    }

    auto *series = new QBarSeries();
    series->append(barSet);
    series->setLabelsVisible(true);
    series->setLabelsPosition(QAbstractBarSeries::LabelsInsideEnd);
    series->setLabelsFormat("@value");

    QObject::connect(series, &QBarSeries::hovered, [labels](bool status, int index, QBarSet *barset) {
        if (status) {
            QToolTip::showText(QCursor::pos(), QString("%1: %2").arg(labels.value(index)).arg(barset->at(index)));
        } else {
            QToolTip::hideText();
        }
    });

    auto *chart = new QChart();
    chart->addSeries(series);
    chart->legend()->setVisible(false);
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(10, 10, 10, 10)); // Increased margins to avoid label clipping

    auto *axisX = new QBarCategoryAxis();
    axisX->append(labels);
    auto *axisY = new QValueAxis();
    
    // Set Y axis to mod 10/20/50/100 to avoid crowding while keeping clean increments
    int interval = 10;
    if (maxVal > 100) interval = 20;
    if (maxVal > 200) interval = 50;
    if (maxVal > 500) interval = 100;
    if (maxVal > 1000) interval = 200;

    int rangeMax = ((maxVal + interval) / interval) * interval; // Add at least one full interval of headroom
    if (rangeMax < interval) rangeMax = interval;
    
    axisY->setRange(0, rangeMax);
    axisY->setTickCount(rangeMax / interval + 1);
    axisY->setLabelFormat("%d");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    return chart;
}

// Helper: create a donut (pie) chart from a SQL grouped query (label col, count col)
QChart* makeDonutChartFromQuery(QSqlQuery &q, double holeSize = 0.45)
{
    auto *pieSeries = new QPieSeries();
    while (q.next()) {
        QString label = q.value(0).toString();
        int val = q.value(1).toInt();
        auto *slice = pieSeries->append(QString("%1: %2").arg(label).arg(val), val);
        slice->setLabelVisible(true);
    }
    pieSeries->setHoleSize(holeSize);

    QObject::connect(pieSeries, &QPieSeries::hovered, [](QPieSlice *slice, bool state) {
        if (state) {
            QToolTip::showText(QCursor::pos(), slice->label());
        } else {
            QToolTip::hideText();
        }
    });

    auto *chart = new QChart();
    chart->addSeries(pieSeries);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(0, 0, 0, 0));

    return chart;
}
}

LogistiqueStatsPage::LogistiqueStatsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LogistiqueStatsPage)
{
    ui->setupUi(this);
    setObjectName("LogistiqueStatsPage");

    ui->cardRecentTable->hide();
    
    // Reposition refresh button to the header
    auto *topLayout = new QHBoxLayout();
    ui->headerLayout->removeWidget(ui->lblTitle);
    ui->headerLayout->removeWidget(ui->lblSubtitle);
    
    auto *titleLayout = new QVBoxLayout();
    titleLayout->addWidget(ui->lblTitle);
    titleLayout->addWidget(ui->lblSubtitle);
    
    topLayout->addLayout(titleLayout);
    topLayout->addStretch();
    topLayout->addWidget(ui->btnRefresh);
    
    ui->headerLayout->addLayout(topLayout);

    if (ui->chartBottomBarContainer) {
        ui->chartBottomBarContainer->setMinimumHeight(400); // More height for the main timeline
    }

    // ─── Arrange Charts side-by-side ──────────────────────────────────────────
    // We group the first two cards into a horizontal layout
    if (ui->cardChartDonut && ui->cardChartBar) {
        auto *sideBySideLayout = new QHBoxLayout();
        sideBySideLayout->setSpacing(20);
        sideBySideLayout->setContentsMargins(0, 0, 0, 0);

        // Find the parent layout (likely vertical)
        QWidget *parent = ui->cardChartDonut->parentWidget();
        if (parent && parent->layout()) {
            QLayout *lay = parent->layout();
            // Remove them from current layout position
            lay->removeWidget(ui->cardChartDonut);
            lay->removeWidget(ui->cardChartBar);
            
            // Move them to the horizontal layout
            sideBySideLayout->addWidget(ui->cardChartDonut);
            sideBySideLayout->addWidget(ui->cardChartBar);
            
            // Add the horizontal layout at the end (footer)
            if (auto *vbox = qobject_cast<QVBoxLayout*>(lay)) {
                vbox->addLayout(sideBySideLayout); // addLayout puts them at the very bottom
            } else {
                lay->addItem(sideBySideLayout);
            }
        }
    }

    applyStyling();
    setupIcons();
    setupCharts();
    setupTable();

    ui->btnRefresh->setText(" Rafraîchir");
    ui->btnRefresh->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    connect(ui->btnRefresh, &QToolButton::clicked, this, &LogistiqueStatsPage::refreshData);
    
    // Auto-refresh every 60 seconds
    auto *refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &LogistiqueStatsPage::refreshData);
    refreshTimer->start(60000); 

    setupPredictionsChart();
    refreshData();
}

LogistiqueStatsPage::~LogistiqueStatsPage()
{
    delete ui;
}

void LogistiqueStatsPage::applyStyling()
{
    setAttribute(Qt::WA_StyledBackground, true);

    const QList<QFrame *> kpiCards = {
        ui->cardKpiTotalCamions,
        ui->cardKpiFreqEntree
    };
    for (QFrame *card : kpiCards) {
        if (card) {
            card->setProperty("card", "kpi");
            card->setAttribute(Qt::WA_StyledBackground, true);
        }
    }

    const QList<QFrame *> chartCards = {
        ui->cardChartDonut,
        ui->cardChartBar,
        ui->cardBottomBar,
        ui->cardChartCamionStatut,
        ui->cardChartTypeOperation,
        ui->cardChartZoneOperation,
        ui->cardChartStatutOperation
    };
    for (QFrame *card : chartCards) {
        if (card) {
            card->setProperty("card", "chart");
            card->setAttribute(Qt::WA_StyledBackground, true);
        }
    }

    if (ui->cardRecentTable) {
        ui->cardRecentTable->setProperty("card", "table");
        ui->cardRecentTable->setAttribute(Qt::WA_StyledBackground, true);
    }

    if (ui->lblTitle) {
        ui->lblTitle->setProperty("role", "pageTitle");
    }
    if (ui->lblSubtitle) {
        ui->lblSubtitle->setProperty("role", "pageSubtitle");
    }

    const QList<QLabel *> kpiValues = {
        ui->lblKpiTotalCamionsValue,
        ui->lblKpiFreqEntreeValue
    };
    for (QLabel *label : kpiValues) {
        if (label) {
            label->setProperty("role", "kpiValue");
        }
    }



    const QString style = QStringLiteral(R"(
QWidget#LogistiqueStatsPage {
    background: transparent;
}
QLabel {
    color: #1c232b;
}
QLabel[role="pageTitle"] {
    font-family: 'Segoe UI', 'Helvetica Neue', Helvetica, Arial, sans-serif;
    font-size: 28px;
    font-weight: 900;
    letter-spacing: 1px;
}
QLabel[role="pageSubtitle"] {
    color: #5f6b7a;
    font-size: 14px;
}
QFrame[card="kpi"],
QFrame[card="chart"],
QFrame[card="table"] {
    background: #ffffff;
    border: 1px solid #e3e8ee;
    border-radius: 14px;
}
QLabel[role="kpiValue"] {
    font-size: 20px;
    font-weight: 700;
}
QToolButton {
    background: #5390D9;
    color: #ffffff;
    border: none;
    border-radius: 8px;
    padding: 6px 12px;
    font-weight: 600;
}
QToolButton:hover {
    background: #48BFE3;
}
QTableWidget {
    background: #ffffff;
    border: 1px solid #e3e8ee;
    border-radius: 10px;
    gridline-color: #edf1f6;
}
QHeaderView::section {
    background: #f7f9fc;
    padding: 6px 8px;
    border: none;
    font-weight: 600;
    color: #3b4654;
}
QPushButton#btnVoirPlus {
    background: #1f2c38;
    color: #ffffff;
    border-radius: 10px;
    padding: 6px 14px;
}
QPushButton#btnVoirPlus:hover {
    background: #243342;
}
)" );

    setStyleSheet(style);
}

void LogistiqueStatsPage::setupIcons()
{
    const QString base = QStringLiteral(":/peche/icons/"); // Reusing Peche icons for now

    setLabelIcon(ui->lblKpiTotalCamionsIcon, base + "ic_kpi_total_peche.png", 48);
    setLabelIcon(ui->lblKpiFreqEntreeIcon, base + "ic_kpi_total_tonnes.png", 48);

    setLabelIcon(ui->lblChartDonutIcon, base + "ic_chart_donut.png", 24);
    setLabelIcon(ui->lblChartBarIcon, base + "ic_chart_bar.png", 24);
    setLabelIcon(ui->lblBottomBarIcon, base + "ic_chart_bar.png", 24);

    setLabelIcon(ui->lblRecentIcon, base + "ic_table_recent.png", 24);

    // New chart section icons
    setLabelIcon(ui->lblChartCamionStatutIcon,   base + "ic_chart_bar.png",   24);
    setLabelIcon(ui->lblChartTypeOperationIcon,  base + "ic_chart_donut.png", 24);
    setLabelIcon(ui->lblChartZoneOperationIcon,  base + "ic_chart_bar.png",   24);
    setLabelIcon(ui->lblChartStatutOperationIcon,base + "ic_chart_donut.png", 24);

    setToolButtonIcon(ui->btnFilter, base + "ic_filter.png", 20);
    setToolButtonIcon(ui->btnCalendar, base + "ic_calendar.png", 20);
    setToolButtonIcon(ui->btnRefresh, base + "ic_refresh.png", 20);
    setToolButtonIcon(ui->btnExport, base + "ic_export.png", 20);
}

void LogistiqueStatsPage::refreshData()
{
    // 1. KPI - Total Camions
    QSqlQuery q;
    if (q.exec("SELECT COUNT(*) FROM Camions")) {
        if (q.next()) ui->lblKpiTotalCamionsValue->setText(q.value(0).toString());
    }

    // 2. KPI - Opérations ce mois-ci
    if (q.exec("SELECT COUNT(*) FROM OperationsCamions WHERE EXTRACT(MONTH FROM DateDebut) = EXTRACT(MONTH FROM SYSDATE) AND EXTRACT(YEAR FROM DateDebut) = EXTRACT(YEAR FROM SYSDATE)")) {
        if (q.next()) {
            ui->lblKpiFreqEntreeValue->setText(q.value(0).toString());
        }
    }

    // Update charts and table with fresh data
    setupCharts();
    setupTable();
    updatePredictions();
}

void LogistiqueStatsPage::setupCharts()
{
    // ─── Line Chart: Operations per date (last 30 days - scrollable) ──────────
    // Build a map for the last 30 days initialised to 0
    QMap<QString, int> dateCounts;
    QStringList dateOrder;
    for (int i = 29; i >= 0; --i) {
        QString d = QDateTime::currentDateTime().addDays(-i).toString("dd/MM");
        dateCounts[d] = 0;
        dateOrder << d;
    }

    QSqlQuery q("SELECT TO_CHAR(DateDebut, 'DD/MM') FROM OperationsCamions WHERE DateDebut >= SYSDATE - 30");
    while (q.next()) {
        QString d = q.value(0).toString();
        if (dateCounts.contains(d)) dateCounts[d]++;
    }

    // ─── Donut Chart: Truck Types (bigger circle = smaller hole) ─────────────
    if (q.exec("SELECT TypeCamion, COUNT(*) FROM Camions GROUP BY TypeCamion")) {
        QChart *donutChart = makeDonutChartFromQuery(q, 0.35); // smaller hole → bigger ring
        replaceChartInContainer(ui->chartDonutContainer, donutChart);
    }

    // ─── Bar Chart: Trucks per Parking Zone ───────────────────────────────────
    if (q.exec("SELECT ZoneStationnement, COUNT(*) FROM Camions GROUP BY ZoneStationnement")) {
        QChart *barChart = makeBarChartFromQuery(q, "Camions");
        replaceChartInContainer(ui->chartBarContainer, barChart);
    }

    // ─── Bottom Bar: Operations per Date (last 7 days - same data as line) ────
    {
        auto *opSeries = new QLineSeries();
        opSeries->setPointLabelsVisible(true);
        opSeries->setPointLabelsFormat("@yPoint");
        opSeries->setPointLabelsClipping(false);
        int opMax = 0;
        int idx = 0;
        for (const QString &d : dateOrder) {
            int cnt = dateCounts.value(d, 0);
            opSeries->append(idx, cnt);
            if (cnt > opMax) opMax = cnt;
            idx++;
        }

        QObject::connect(opSeries, &QLineSeries::hovered, [dateOrder](const QPointF &point, bool state) {
            if (state) {
                int index = qRound(point.x());
                if (index >= 0 && index < dateOrder.size()) {
                    QToolTip::showText(QCursor::pos(), QString("%1: %2").arg(dateOrder.value(index)).arg(point.y()));
                }
            } else {
                QToolTip::hideText();
            }
        });

        auto *opChart = new QChart();
        opChart->addSeries(opSeries);
        opChart->legend()->setVisible(false);
        opChart->setBackgroundVisible(false);
        opChart->setMargins(QMargins(15, 10, 15, 50));

        auto *opAxisX = new QCategoryAxis();
        for (int i = 0; i < dateOrder.size(); ++i) {
            opAxisX->append(dateOrder[i], i);
        }
        opAxisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
        opAxisX->setLabelsAngle(-90); // Vertical rotation is more space-efficient for dates
        
        auto *opAxisY = new QValueAxis();
        // Set Y axis to mod 10/20... to avoid crowding
        int intervalY = 10;
        if (opMax > 100) intervalY = 20;
        if (opMax > 200) intervalY = 50;
        if (opMax > 500) intervalY = 100;

        int rangeMaxY = ((opMax + (intervalY - 1)) / intervalY) * intervalY;
        if (rangeMaxY < intervalY) rangeMaxY = intervalY;
        
        opAxisY->setRange(0, rangeMaxY);
        opAxisY->setTickCount(rangeMaxY / intervalY + 1);
        opAxisY->setLabelFormat("%d");

        opChart->addAxis(opAxisX, Qt::AlignBottom);
        opChart->addAxis(opAxisY, Qt::AlignLeft);
        opSeries->attachAxis(opAxisX);
        opSeries->attachAxis(opAxisY);

        // Set initial window to last 15 days to show more data at once (scrollable up to 30)
        if (dateOrder.size() > 15) {
            opAxisX->setRange(dateOrder.size() - 15, dateOrder.size() - 1);
        }

        auto *view = replaceChartInContainer(ui->chartBottomBarContainer, opChart);
        view->setDragMode(QChartView::ScrollHandDrag);
        view->setRubberBand(QChartView::HorizontalRubberBand);
    }

    // ─── CAMION STATISTIQUES ──────────────────────────────────────────────────

    // Camion Statut (pie/donut)
    if (q.exec("SELECT Statut, COUNT(*) FROM Camions GROUP BY Statut")) {
        QChart *chart = makeDonutChartFromQuery(q, 0.40);
        replaceChartInContainer(ui->chartCamionStatutContainer, chart);
    }



    // ─── OPERATION STATISTIQUES ───────────────────────────────────────────────

    // Type d'Opération (donut)
    if (q.exec("SELECT TypeOperation, COUNT(*) FROM OperationsCamions GROUP BY TypeOperation")) {
        QChart *chart = makeDonutChartFromQuery(q, 0.40);
        replaceChartInContainer(ui->chartTypeOperationContainer, chart);
    }

    // Zone d'Opération (bar)
    if (q.exec("SELECT ZoneOperation, COUNT(*) FROM OperationsCamions GROUP BY ZoneOperation")) {
        QChart *chart = makeBarChartFromQuery(q, "Opérations");
        replaceChartInContainer(ui->chartZoneOperationContainer, chart);
    }

    // Statut d'Opération (donut)
    if (q.exec("SELECT Statut, COUNT(*) FROM OperationsCamions GROUP BY Statut")) {
        QChart *chart = makeDonutChartFromQuery(q, 0.40);
        replaceChartInContainer(ui->chartStatutOperationContainer, chart);
    }
}

void LogistiqueStatsPage::setupTable()
{
    if (!ui->tableRecent) return;

    ui->tableRecent->setColumnCount(5);
    ui->tableRecent->setHorizontalHeaderLabels({"ID", "Camion", "Zone", "Date", "Statut"});
    ui->tableRecent->setRowCount(0);
    ui->tableRecent->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableRecent->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QSqlQuery q("SELECT o.OpCamId, c.Immatriculation, o.ZoneOperation, "
                "TO_CHAR(o.DateDebut, 'DD/MM HH24:MI'), o.Statut "
                "FROM OperationsCamions o "
                "JOIN Camions c ON o.CamionId = c.CamionId "
                "ORDER BY o.DateDebut DESC FETCH FIRST 5 ROWS ONLY");
    
    int row = 0;
    while (q.next()) {
        ui->tableRecent->insertRow(row);
        for (int i = 0; i < 5; ++i) {
            auto *item = new QTableWidgetItem(q.value(i).toString());
            item->setTextAlignment(Qt::AlignCenter);
            ui->tableRecent->setItem(row, i, item);
        }
        row++;
    }
}

void LogistiqueStatsPage::setupPredictionsChart()
{
    // A container widget to hold everything related to predictions
    auto *cardPred = new QFrame(this);
    cardPred->setObjectName("cardPredictions");
    cardPred->setProperty("card", "chart");
    cardPred->setAttribute(Qt::WA_StyledBackground, true);
    cardPred->setMinimumHeight(350);

    auto *cardLayout = new QVBoxLayout(cardPred);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(12);

    // Header part
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(10);

    auto *iconLabel = new QLabel(cardPred);
    setLabelIcon(iconLabel, ":/peche/icons/ic_chart_bar.png", 24); 
    headerLayout->addWidget(iconLabel);

    auto *titleLabel = new QLabel("Prévisions des Opérations (3 Prochains Mois)", cardPred);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #1c232b;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    cardLayout->addLayout(headerLayout);

    // Container for the chart
    auto *chartContainer = new QWidget(cardPred);
    chartContainer->setObjectName("chartPredictionsContainer");
    chartContainer->setMinimumHeight(250);
    ensureChartLayout(chartContainer);
    cardLayout->addWidget(chartContainer);

    // Add this card to the main layout's bottom (footer)
    if (auto *mainVbox = qobject_cast<QVBoxLayout*>(layout())) {
        mainVbox->addWidget(cardPred);
    }
}

void LogistiqueStatsPage::updatePredictions()
{
    // Look for our container
    QWidget *container = findChild<QWidget*>("chartPredictionsContainer");
    if (!container) return;

    // Show loading
    auto *loading = new QLabel("Chargement des prévisions AI...", container);
    loading->setStyleSheet("color: #6B7280; font-style: italic;");
    loading->setAlignment(Qt::AlignCenter);
    replaceChartInContainer(container, nullptr);
    container->layout()->addWidget(loading);

    auto *process = new QProcess(this);
    
    static QString cachedPythonPath;
    if (cachedPythonPath.isEmpty()) {
        const QString customPython = QString::fromLocal8Bit(qgetenv("DATA_DOCK_PYTHON"));
        QDir appDir(QCoreApplication::applicationDirPath());

        QStringList pythonCandidates;
        if (!customPython.isEmpty()) {
            pythonCandidates << customPython;
        }
        pythonCandidates << "python3" << "python" << "python.exe" << "python3.exe";

        QDir searchDir(appDir);
        for (int i = 0; i < 6; ++i) {
            pythonCandidates << searchDir.absoluteFilePath("venv/bin/python");
            pythonCandidates << searchDir.absoluteFilePath("venv/bin/python3");
            pythonCandidates << searchDir.absoluteFilePath("venv/Scripts/python.exe");
            if (!searchDir.cdUp()) break;
        }

        pythonCandidates << QDir::currentPath() + "/venv/bin/python";
        pythonCandidates << QDir::currentPath() + "/venv/Scripts/python.exe";
        pythonCandidates << QDir::homePath() + "/.local/bin/python";
        pythonCandidates << QDir::homePath() + "/.local/bin/python3";

        for (const QString &cmd : pythonCandidates) {
            #ifdef Q_OS_WIN
                // Skip non-Windows paths on Windows and vice versa if needed
            #endif
            
            if (cmd.startsWith("/") || cmd.contains(":/")) {
                if (!QFile::exists(cmd)) continue;
            }

            QProcess check;
            check.start(cmd, {"-c", "import pandas, catboost, numpy"});
            if (check.waitForFinished(3000)) { // 3s timeout for heavier environments
                if (check.exitCode() == 0) {
                    cachedPythonPath = cmd;
                    break;
                }
            }
        }
        if (cachedPythonPath.isEmpty()) cachedPythonPath = "python"; // Fallback to PATH search
    }
    QString pythonPath = cachedPythonPath;
    qDebug() << "[AI] Selected Python:" << pythonPath;

    // Robust search for the script
    QString binPath = QCoreApplication::applicationDirPath();
    QString scriptPath;
    bool scriptFound = false;
    QDir search(binPath);
    for (int i = 0; i < 5; ++i) {
        QString testPath = search.absoluteFilePath("gestion_logistique/ai_operations_predictor/predict_3months.py");
        if (QFile::exists(testPath)) { scriptPath = testPath; scriptFound = true; break; }
        if (!search.cdUp()) break;
    }
    if (!scriptFound && QFile::exists("gestion_logistique/ai_operations_predictor/predict_3months.py")) {
        scriptPath = "gestion_logistique/ai_operations_predictor/predict_3months.py";
        scriptFound = true;
    }

    if (!scriptFound) {
        replaceChartInContainer(container, nullptr);
        container->layout()->addWidget(new QLabel("Moteur AI non trouvé (predict_3months.py manquant)."));
        process->deleteLater();
        return;
    }

    qDebug() << "[AI] Running command:" << pythonPath << scriptPath;
    process->start(pythonPath, QStringList() << scriptPath);

    connect(process, &QProcess::readyReadStandardOutput, [process](){
        qDebug() << "[AI STDOUT]" << process->readAllStandardOutput().trimmed();
    });
    connect(process, &QProcess::readyReadStandardError, [process](){
        qDebug() << "[AI STDERR]" << process->readAllStandardError().trimmed();
    });
    
    // We need to capture the script's directory for reading the JSON later
    QString scriptDir = QFileInfo(scriptPath).absolutePath();
    QString jsonPath = scriptDir + "/predictions_3months.json";

    connect(process, &QProcess::errorOccurred, [this, container](QProcess::ProcessError error){
        QString errStr = "Erreur de démarrage du processus.";
        if (error == QProcess::FailedToStart) errStr = "Impossible de lancer l'interpréteur Python. Vérifiez qu'il est dans le PATH.";
        auto *lbl = new QLabel("Défaut de lancement AI: " + errStr, container);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color: #ef4444;");
        replaceChartInContainer(container, nullptr);
        container->layout()->addWidget(lbl);
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this, process, container, jsonPath](int exitCode, QProcess::ExitStatus exitStatus){
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            
            QFile file(jsonPath);
            if (!file.open(QIODevice::ReadOnly)) {
                auto *lbl = new QLabel("Échec de lecture du fichier de prédiction.", container);
                lbl->setAlignment(Qt::AlignCenter);
                replaceChartInContainer(container, nullptr);
                container->layout()->addWidget(lbl);
                process->deleteLater();
                return;
            }

            QByteArray output = file.readAll();
            file.close();

            QJsonDocument doc = QJsonDocument::fromJson(output);

            if (!doc.isNull() && doc.isObject()) {
                QJsonObject root = doc.object();
                if (root.contains("error")) {
                    auto *lbl = new QLabel("Détail Erreur AI: " + root["error"].toString(), container);
                    lbl->setAlignment(Qt::AlignCenter);
                    replaceChartInContainer(container, nullptr);
                    container->layout()->addWidget(lbl);
                    process->deleteLater();
                    return;
                }

                if (root.isEmpty()) {
                    auto *lbl = new QLabel("Aucune donnée de prévision disponible.", container);
                    lbl->setAlignment(Qt::AlignCenter);
                    replaceChartInContainer(container, nullptr);
                    container->layout()->addWidget(lbl);
                    process->deleteLater();
                    return;
                }

                auto *chart = new QChart();
                chart->setBackgroundVisible(false);
                chart->setMargins(QMargins(10, 10, 10, 40));

                double maxVal = 0.0;
                QStringList dates;
                bool xSet = false;

                // Color palette for zones
                QStringList colors = {"#5390D9", "#7400B8", "#56CFE1", "#48BFE3", "#5E60CE", "#6930C3"};
                int colorIdx = 0;

                for (auto it = root.begin(); it != root.end(); ++it) {
                    QString zone = it.key();
                    QJsonArray data = it.value().toArray();
                    
                    auto *series = new QLineSeries();
                    series->setName(zone);
                    
                    QColor color(colors.at(colorIdx % colors.size()));
                    series->setColor(color);
                    colorIdx++;

                    for (int i = 0; i < data.size(); ++i) {
                        QJsonObject pt = data[i].toObject();
                        double val = pt["val"].toDouble();
                        series->append(i, val);
                        if (val > maxVal) maxVal = val;
                        
                        if (!xSet) {
                            dates << QDate::fromString(pt["date"].toString(), "yyyy-MM-dd").toString("dd/MM");
                        }
                    }
                    xSet = true;

                    // Use QAreaSeries with transparency for better visibility of overlapping lines
                    auto *upperSeries = series;
                    auto *areaSeries = new QAreaSeries(upperSeries);
                    areaSeries->setName(zone);
                    
                    QColor fillColor = color;
                    fillColor.setAlpha(60); // Semi-transparent to see other lines
                    areaSeries->setBrush(QBrush(fillColor));
                    
                    QPen pen(color);
                    pen.setWidth(2);
                    areaSeries->setPen(pen);
                    
                    chart->addSeries(areaSeries);
                }

                chart->legend()->setVisible(true);
                chart->legend()->setAlignment(Qt::AlignBottom);

                // Setup Axes
                auto *axisX = new QCategoryAxis();
                // To avoid crowding 90 days, we only show one label per week
                for (int i = 0; i < dates.size(); ++i) {
                    if (i % 7 == 0 || i == dates.size() - 1) {
                        axisX->append(dates[i], i);
                    } else {
                        axisX->append("", i);
                    }
                }
                axisX->setLabelsAngle(-90);
                
                auto *axisY = new QValueAxis();
                axisY->setLabelFormat("%.1f");
                // Dynamic scaling for visibility
                double targetMax = 1.0;
                if (maxVal > 1.0) targetMax = (int(maxVal/2)+1)*2 + 2;
                if (maxVal > 10.0) targetMax = (int(maxVal/5)+1)*5 + 5;
                
                axisY->setRange(0, targetMax);
                axisY->setTitleText("Operations (Est.)");

                chart->addAxis(axisX, Qt::AlignBottom);
                chart->addAxis(axisY, Qt::AlignLeft);

                for (auto *s : chart->series()) {
                    s->attachAxis(axisX);
                    s->attachAxis(axisY);
                }

                replaceChartInContainer(container, chart);
            } else {
                auto *lbl = new QLabel("Format de données AI invalide.", container);
                lbl->setAlignment(Qt::AlignCenter);
                replaceChartInContainer(container, nullptr);
                container->layout()->addWidget(lbl);
            }
        } else {
            QString errorOutput = process->readAllStandardError();
            if (errorOutput.isEmpty()) errorOutput = "Le processus s'est arrêté de manière inattendue.";
            auto *lbl = new QLabel("Échec de l'exécution du moteur AI :\n" + errorOutput, container);
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setStyleSheet("color: #ef4444; font-size: 11px;");
            replaceChartInContainer(container, nullptr);
            container->layout()->addWidget(lbl);
        }
        process->deleteLater();
    });
}
