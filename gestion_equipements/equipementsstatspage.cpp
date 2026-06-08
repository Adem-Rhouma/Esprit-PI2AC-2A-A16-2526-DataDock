#include "equipementsstatspage.h"
#include "ui_equipementsstatspage.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QPainter>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QValueAxis>

// Helper functions
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

void clearLayoutWidgets(QWidget *container)
{
    if (!container || !container->layout()) {
        return;
    }

    while (QLayoutItem *item = container->layout()->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

QString normalizeEtat(const QString &etat)
{
    return etat.trimmed().toLower();
}

bool isPanneEtat(const QString &etat)
{
    const QString v = normalizeEtat(etat);
    return v.contains("panne") || v.contains("hs") || v.contains("ko");
}

bool isMaintenanceEtat(const QString &etat)
{
    return normalizeEtat(etat).contains("maintenance");
}
}

EquipementsStatsPage::EquipementsStatsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EquipementsStatsPage)
{
    ui->setupUi(this);

    applyStyling();
    setupIcons();
    loadStatistics();

    if (ui->btnRefresh) {
        connect(ui->btnRefresh, &QToolButton::clicked, this, &EquipementsStatsPage::loadStatistics);
    }
}

EquipementsStatsPage::~EquipementsStatsPage()
{
    delete ui;
}

void EquipementsStatsPage::refreshData()
{
    loadStatistics();
}

void EquipementsStatsPage::applyStyling()
{
    setAttribute(Qt::WA_StyledBackground, true);

    const QList<QFrame *> kpiCards = {
        ui->cardKpiTotalMateriels,
        ui->cardKpiEnPanne,
        ui->cardKpiMaintenance,
        ui->cardKpiCout
    };
    for (QFrame *card : kpiCards) {
        if (card) {
            card->setProperty("card", "kpi");
            card->setAttribute(Qt::WA_StyledBackground, true);
        }
    }

    const QList<QFrame *> chartCards = {
        ui->cardChartLine,
        ui->cardChartDonut,
        ui->cardChartBar,
        ui->cardBottomBar,
        ui->cardRecentTable
    };
    for (QFrame *card : chartCards) {
        if (card) {
            card->setProperty("card", "chart");
            card->setAttribute(Qt::WA_StyledBackground, true);
        }
    }

    if (ui->lblTitle) {
        ui->lblTitle->setProperty("role", "pageTitle");
    }
    if (ui->lblSubtitle) {
        ui->lblSubtitle->setProperty("role", "pageSubtitle");
    }

    const QList<QLabel *> kpiValues = {
        ui->lblKpiTotalMaterielsValue,
        ui->lblKpiEnPanneValue,
        ui->lblKpiMaintenanceValue,
        ui->lblKpiCoutValue
    };
    for (QLabel *label : kpiValues) {
        if (label) {
            label->setProperty("role", "kpiValue");
        }
    }

    const QList<QLabel *> kpiBadges = {
        ui->lblKpiTotalMaterielsBadge,
        ui->lblKpiEnPanneBadge,
        ui->lblKpiMaintenanceBadge,
        ui->lblKpiCoutBadge
    };
    for (QLabel *label : kpiBadges) {
        if (label) {
            label->setProperty("role", "kpiBadge");
        }
    }

    const QString style = QStringLiteral(R"(
QLabel {
    color: #1c232b;
}
QLabel[role="pageTitle"] {
    font-size: 26px;
    font-weight: 700;
}
QLabel[role="pageSubtitle"] {
    color: #5f6b7a;
    font-size: 14px;
}
QFrame[card="kpi"],
QFrame[card="chart"] {
    background: #ffffff;
    border: 1px solid #e3e8ee;
    border-radius: 14px;
}
QLabel[role="kpiValue"] {
    font-size: 20px;
    font-weight: 700;
}
QLabel[role="kpiBadge"] {
    background: #e2f6ec;
    color: #1f7a4d;
    border-radius: 10px;
    padding: 2px 8px;
    font-weight: 600;
    max-width: 80px;
}
QToolButton {
    background: #f1f4f8;
    border: 1px solid #e1e6ed;
    border-radius: 8px;
    padding: 4px;
}
QToolButton:hover {
    background: #e6ecf3;
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

void EquipementsStatsPage::setupIcons()
{
    const QString base = QStringLiteral(":/peche/icons/"); // Reusing Peche icons

    setLabelIcon(ui->lblKpiTotalMaterielsIcon, base + "ic_kpi_total_peche.png", 48);
    setLabelIcon(ui->lblKpiEnPanneIcon, base + "ic_kpi_total_achats.png", 48);
    setLabelIcon(ui->lblKpiMaintenanceIcon, base + "ic_kpi_total_ventes.png", 48);
    setLabelIcon(ui->lblKpiCoutIcon, base + "ic_kpi_total_tonnes.png", 48);

    setLabelIcon(ui->lblChartLineIcon, base + "ic_chart_line.png", 24);
    setLabelIcon(ui->lblChartDonutIcon, base + "ic_chart_donut.png", 24);
    setLabelIcon(ui->lblChartBarIcon, base + "ic_chart_bar.png", 24);
    setLabelIcon(ui->lblBottomBarIcon, base + "ic_chart_bar.png", 24);

    setLabelIcon(ui->lblRecentIcon, base + "ic_table_recent.png", 24);

    setToolButtonIcon(ui->btnFilter, base + "ic_filter.png", 20);
    setToolButtonIcon(ui->btnCalendar, base + "ic_calendar.png", 20);
    setToolButtonIcon(ui->btnRefresh, base + "ic_refresh.png", 20);
    setToolButtonIcon(ui->btnExport, base + "ic_export.png", 20);
}

void EquipementsStatsPage::loadStatistics()
{
    const QList<materiel> materiels = materiel::afficherTous();

    const int total = materiels.size();
    int enPanne = 0;
    int maintenance = 0;
    int reparables = 0;

    for (const materiel &value : materiels) {
        if (isPanneEtat(value.etat())) {
            ++enPanne;
        } else if (isMaintenanceEtat(value.etat())) {
            ++maintenance;
        }

        if (value.reparable()) {
            ++reparables;
        }
    }

    const int nonReparables = total - reparables;
    const int coutEstime = enPanne * 1200 + maintenance * 700;
    const int tauxCritique = total > 0 ? ((enPanne + maintenance) * 100) / total : 0;

    if (ui->lblKpiTotalMaterielsValue) ui->lblKpiTotalMaterielsValue->setText(QString::number(total));
    if (ui->lblKpiEnPanneValue) ui->lblKpiEnPanneValue->setText(QString::number(enPanne));
    if (ui->lblKpiMaintenanceValue) ui->lblKpiMaintenanceValue->setText(QString::number(maintenance));
    if (ui->lblKpiCoutValue) ui->lblKpiCoutValue->setText(QString::number(coutEstime) + " TND");

    if (ui->lblKpiTotalMaterielsBadge) ui->lblKpiTotalMaterielsBadge->setText(QString::number(reparables) + " rep.");
    if (ui->lblKpiEnPanneBadge) ui->lblKpiEnPanneBadge->setText(QString::number(tauxCritique) + "% crit.");
    if (ui->lblKpiMaintenanceBadge) ui->lblKpiMaintenanceBadge->setText(QString::number(nonReparables) + " non rep.");
    if (ui->lblKpiCoutBadge) ui->lblKpiCoutBadge->setText("estime");

    if (ui->lblChartBarTitle) ui->lblChartBarTitle->setText("Répartition par Type");
    if (ui->lblBottomBarTitle) ui->lblBottomBarTitle->setText("Top Employés (Nb matériels)");
    if (ui->lblRecentTitle) ui->lblRecentTitle->setText("Derniers Matériels");

    setupCharts(materiels);
    setupTable(materiels);
}

void EquipementsStatsPage::setupCharts(const QList<materiel> &materiels)
{
    clearLayoutWidgets(ui->chartLineContainer);
    clearLayoutWidgets(ui->chartDonutContainer);
    clearLayoutWidgets(ui->chartBarContainer);
    clearLayoutWidgets(ui->chartBottomBarContainer);

    QList<materiel> sorted = materiels;
    std::sort(sorted.begin(), sorted.end(), [](const materiel &a, const materiel &b) {
        return a.idMateriel() < b.idMateriel();
    });

    QMap<QString, int> typeCounts;
    QMap<int, int> employeCounts;
    int panneCount = 0;
    int maintenanceCount = 0;

    for (const materiel &value : sorted) {
        const QString typeKey = value.type().trimmed().isEmpty() ? "Sans type" : value.type().trimmed();
        typeCounts[typeKey] += 1;
        employeCounts[value.idEmploye()] += 1;

        if (isPanneEtat(value.etat())) {
            ++panneCount;
        } else if (isMaintenanceEtat(value.etat())) {
            ++maintenanceCount;
        }
    }

    const int bonEtatCount = std::max(0, static_cast<int>(sorted.size()) - panneCount - maintenanceCount);

    // Line Chart: Cumul des pannes selon l'ordre des ID
    auto *lineSeries = new QLineSeries();
    auto *lineAxisX = new QCategoryAxis();
    auto *lineAxisY = new QValueAxis();

    int cumulPannes = 0;
    if (sorted.isEmpty()) {
        lineSeries->append(0, 0);
        lineAxisX->append("Aucun", 0);
    } else {
        const int points = std::min(8, static_cast<int>(sorted.size()));
        for (int i = 0; i < points; ++i) {
            const int idx = (sorted.size() - 1) * i / std::max(1, points - 1);
            cumulPannes = 0;
            for (int j = 0; j <= idx; ++j) {
                if (isPanneEtat(sorted[j].etat())) {
                    ++cumulPannes;
                }
            }
            lineSeries->append(i, cumulPannes);
            lineAxisX->append(QString::number(sorted[idx].idMateriel()), i);
        }
    }

    auto *lineChart = new QChart();
    lineChart->addSeries(lineSeries);
    lineChart->legend()->setVisible(false);
    lineChart->setBackgroundVisible(false);
    lineChart->setMargins(QMargins(0, 0, 0, 0));
    lineAxisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    lineAxisY->setRange(0, std::max(1, panneCount + 1));

    lineChart->addAxis(lineAxisX, Qt::AlignBottom);
    lineChart->addAxis(lineAxisY, Qt::AlignLeft);
    lineSeries->attachAxis(lineAxisX);
    lineSeries->attachAxis(lineAxisY);

    ensureChartLayout(ui->chartLineContainer);
    if (ui->chartLineContainer && ui->chartLineContainer->layout()) {
        auto *view = new QChartView(lineChart, ui->chartLineContainer);
        view->setRenderHint(QPainter::Antialiasing);
        ui->chartLineContainer->layout()->addWidget(view);
    }

    // Donut Chart: Distribution des états
    auto *donutSeries = new QPieSeries();
    donutSeries->append("Bon État", bonEtatCount);
    donutSeries->append("En Panne", panneCount);
    donutSeries->append("Maintenance", maintenanceCount);
    donutSeries->setHoleSize(0.55);

    auto *donutChart = new QChart();
    donutChart->addSeries(donutSeries);
    donutChart->legend()->setAlignment(Qt::AlignBottom);
    donutChart->setBackgroundVisible(false);
    donutChart->setMargins(QMargins(0, 0, 0, 0));

    ensureChartLayout(ui->chartDonutContainer);
    if (ui->chartDonutContainer && ui->chartDonutContainer->layout()) {
        auto *view = new QChartView(donutChart, ui->chartDonutContainer);
        view->setRenderHint(QPainter::Antialiasing);
        ui->chartDonutContainer->layout()->addWidget(view);
    }

    // Bar Chart: Répartition par type
    QVector<QPair<QString, int>> sortedTypes;
    sortedTypes.reserve(typeCounts.size());
    for (auto it = typeCounts.cbegin(); it != typeCounts.cend(); ++it) {
        sortedTypes.push_back({it.key(), it.value()});
    }
    std::sort(sortedTypes.begin(), sortedTypes.end(), [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
        if (a.second == b.second) {
            return a.first < b.first;
        }
        return a.second > b.second;
    });

    auto *barSet = new QBarSet("Quantité");
    QStringList barLabels;
    const int maxTypes = std::min(6, static_cast<int>(sortedTypes.size()));
    for (int i = 0; i < maxTypes; ++i) {
        *barSet << sortedTypes[i].second;
        barLabels << sortedTypes[i].first;
    }
    if (barLabels.isEmpty()) {
        *barSet << 0;
        barLabels << "Aucun";
    }

    auto *barSeries = new QBarSeries();
    barSeries->append(barSet);

    auto *barChart = new QChart();
    barChart->addSeries(barSeries);
    barChart->legend()->setVisible(false);
    barChart->setBackgroundVisible(false);
    barChart->setMargins(QMargins(0, 0, 0, 0));

    auto *barAxisX = new QBarCategoryAxis();
    barAxisX->append(barLabels);
    auto *barAxisY = new QValueAxis();
    int maxBar = 0;
    for (int i = 0; i < barSet->count(); ++i) {
        maxBar = std::max(maxBar, static_cast<int>(barSet->at(i)));
    }
    barAxisY->setRange(0, std::max(1, maxBar + 1));

    barChart->addAxis(barAxisX, Qt::AlignBottom);
    barChart->addAxis(barAxisY, Qt::AlignLeft);
    barSeries->attachAxis(barAxisX);
    barSeries->attachAxis(barAxisY);

    ensureChartLayout(ui->chartBarContainer);
    if (ui->chartBarContainer && ui->chartBarContainer->layout()) {
        auto *view = new QChartView(barChart, ui->chartBarContainer);
        view->setRenderHint(QPainter::Antialiasing);
        ui->chartBarContainer->layout()->addWidget(view);
    }

    // Bottom Bar Chart: Top employés par nombre de matériels
    QVector<QPair<int, int>> sortedEmployes;
    sortedEmployes.reserve(employeCounts.size());
    for (auto it = employeCounts.cbegin(); it != employeCounts.cend(); ++it) {
        sortedEmployes.push_back({it.key(), it.value()});
    }
    std::sort(sortedEmployes.begin(), sortedEmployes.end(), [](const QPair<int, int> &a, const QPair<int, int> &b) {
        if (a.second == b.second) {
            return a.first < b.first;
        }
        return a.second > b.second;
    });

    auto *intervSet = new QBarSet("Interventions");
    QStringList employeLabels;
    const int maxEmployes = std::min(7, static_cast<int>(sortedEmployes.size()));
    for (int i = 0; i < maxEmployes; ++i) {
        *intervSet << sortedEmployes[i].second;
        employeLabels << ("Emp " + QString::number(sortedEmployes[i].first));
    }
    if (employeLabels.isEmpty()) {
        *intervSet << 0;
        employeLabels << "Aucun";
    }

    auto *bottomSeries = new QBarSeries();
    bottomSeries->append(intervSet);

    auto *bottomChart = new QChart();
    bottomChart->addSeries(bottomSeries);
    bottomChart->legend()->setVisible(false);
    bottomChart->setBackgroundVisible(false);
    bottomChart->setMargins(QMargins(0, 0, 0, 0));

    auto *bottomAxisX = new QBarCategoryAxis();
    bottomAxisX->append(employeLabels);
    auto *bottomAxisY = new QValueAxis();
    int maxBottom = 0;
    for (int i = 0; i < intervSet->count(); ++i) {
        maxBottom = std::max(maxBottom, static_cast<int>(intervSet->at(i)));
    }
    bottomAxisY->setRange(0, std::max(1, maxBottom + 1));

    bottomChart->addAxis(bottomAxisX, Qt::AlignBottom);
    bottomChart->addAxis(bottomAxisY, Qt::AlignLeft);
    bottomSeries->attachAxis(bottomAxisX);
    bottomSeries->attachAxis(bottomAxisY);

    ensureChartLayout(ui->chartBottomBarContainer);
    if (ui->chartBottomBarContainer && ui->chartBottomBarContainer->layout()) {
        auto *view = new QChartView(bottomChart, ui->chartBottomBarContainer);
        view->setRenderHint(QPainter::Antialiasing);
        ui->chartBottomBarContainer->layout()->addWidget(view);
    }
}

void EquipementsStatsPage::setupTable(const QList<materiel> &materiels)
{
    if (!ui->tableRecent) {
        return;
    }

    ui->tableRecent->setColumnCount(5);
    ui->tableRecent->setRowCount(0);
    ui->tableRecent->setHorizontalHeaderLabels({
        "ID Matériel", "Type", "Etat", "Réparable", "ID Employé"
    });
    ui->tableRecent->verticalHeader()->setVisible(false);
    ui->tableRecent->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableRecent->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableRecent->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableRecent->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QList<materiel> sorted = materiels;
    std::sort(sorted.begin(), sorted.end(), [](const materiel &a, const materiel &b) {
        return a.idMateriel() > b.idMateriel();
    });

    const int rowCount = std::min(8, static_cast<int>(sorted.size()));
    ui->tableRecent->setRowCount(rowCount);

    for (int row = 0; row < rowCount; ++row) {
        const materiel &value = sorted[row];
        const QStringList values = {
            QString::number(value.idMateriel()),
            value.type(),
            value.etat(),
            value.reparable() ? "Oui" : "Non",
            QString::number(value.idEmploye())
        };

        for (int col = 0; col < values.size(); ++col) {
            auto *item = new QTableWidgetItem(values[col]);
            item->setTextAlignment(Qt::AlignCenter);
            ui->tableRecent->setItem(row, col, item);
        }
    }
}


