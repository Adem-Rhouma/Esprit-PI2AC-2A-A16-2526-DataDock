#include "chambresfroidesstatspage.h"
#include "ui_chambresfroidesstatspage.h"
#include "chambresfroides.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QVBoxLayout>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QPushButton>
#include <QApplication>
#include <QToolTip>
#include "chambresfroidesalertinbox.h"
#include <QDebug>

ChambresFroidesStatsPage::ChambresFroidesStatsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChambresFroidesStatsPage)
{
    ui->setupUi(this);
    applyStyles();
    setupCharts();
    refreshStats();
    
    connect(ui->btnRefresh, &QPushButton::clicked, this, &ChambresFroidesStatsPage::refreshStats);
    
    setupIntelligenceDashboard();
    
    // Global explanatory tooltips for page headers
    ui->lblTitle->setToolTip("Vue d'ensemble analytique de votre infrastructure de stockage à froid. Les données sont agrégées en temps réel.");
    ui->lblSubtitle->setToolTip("Suivi de précision des performances thermiques, capacitaires et conformité opérationnelle.");
    ui->btnRefresh->setToolTip("Synchroniser toutes les métriques analytiques avec les données d'inventaire actuelles.");
}

ChambresFroidesStatsPage::~ChambresFroidesStatsPage()
{
    delete ui;
}

void ChambresFroidesStatsPage::setupIntelligenceDashboard()
{
    // 1. Add Alert Inbox Button to Header
    m_btnAlertInbox = new QPushButton("Notifications (0)", this);
    m_btnAlertInbox->setObjectName("btnAlertInbox");
    m_btnAlertInbox->setCursor(Qt::PointingHandCursor);
    m_btnAlertInbox->setFixedWidth(150);
    ui->headerLayout->insertWidget(2, m_btnAlertInbox);
    
    connect(m_btnAlertInbox, &QPushButton::clicked, [this]() {
        ChambresFroidesAlertInbox inbox(this);
        inbox.exec();
        refreshStats(); // Update counters after inbox closure
    });

    // 2. Alert Blinking Timer
    m_alertBlinkTimer = new QTimer(this);
    connect(m_alertBlinkTimer, &QTimer::timeout, [this]() {
        static bool state = false;
        state = !state;
        if (state) {
            m_btnAlertInbox->setStyleSheet("background-color: #ef4444; color: white; border: none; font-weight: bold; border-radius: 8px;");
        } else {
            m_btnAlertInbox->setStyleSheet("background-color: #fee2e2; color: #b91c1c; border: 1px solid #ef4444; font-weight: bold; border-radius: 8px;");
        }
    });

    // 3. Add Health Card to KPI Layout
    QFrame *cardHealth = new QFrame(this);
    cardHealth->setObjectName("cardHealth");
    cardHealth->setProperty("card", "content");
    QVBoxLayout *vlHealth = new QVBoxLayout(cardHealth);
    
    QLabel *lblTitle = new QLabel("Score de Santé Portuaire", cardHealth);
    lblTitle->setProperty("role", "cardTitle");
    lblTitle->setToolTip("Indice de performance globale du pôle froid. Un score de 100% est l'objectif optimal.");
    
    QLabel *lblValue = new QLabel("100%", cardHealth);
    lblValue->setObjectName("lblHealthScore");
    lblValue->setProperty("role", "kpiValue");
    lblValue->setAlignment(Qt::AlignCenter);
    lblValue->setToolTip("Note calculée en temps réel selon les températures, la saturation et l'état des machines.");
    
    vlHealth->addWidget(lblTitle);
    vlHealth->addWidget(lblValue);
    ui->kpiLayout->addWidget(cardHealth);

    // 4. Add Intelligence Hub (New section below charts)
    QFrame *intelHub = new QFrame(this);
    intelHub->setObjectName("intelHub");
    intelHub->setProperty("card", "content");
    intelHub->setMinimumHeight(200);
    
    auto *hubLayout = new QHBoxLayout(intelHub);
    hubLayout->setContentsMargins(24, 24, 24, 24);
    hubLayout->setSpacing(40);

    // Thermal Outliers Column
    auto *thermalCol = new QVBoxLayout();
    QLabel *thermalTitle = new QLabel("EXTRÊMES THERMIQUES", intelHub);
    thermalTitle->setProperty("role", "cardTitle");
    thermalCol->addWidget(thermalTitle);
    
    QHBoxLayout *thermalCards = new QHBoxLayout();
    
    // Hottest Card
    QFrame *cardHot = new QFrame(intelHub);
    cardHot->setObjectName("badgeHottest");
    cardHot->setProperty("role", "intelBadge");
    auto *vlHot = new QVBoxLayout(cardHot);
    QLabel *lblHotTitle = new QLabel("POINT CHAUD", cardHot);
    lblHotTitle->setToolTip("Identifie le compartiment ayant la température la plus élevée (risque potentiel).");
    QLabel *lblHotVal = new QLabel("--", cardHot);
    lblHotVal->setObjectName("lblIntelHottest");
    lblHotVal->setStyleSheet("font-size: 14px; font-weight: 800; color: #7f1d1d;");
    vlHot->addWidget(lblHotTitle);
    vlHot->addWidget(lblHotVal);
    thermalCards->addWidget(cardHot);

    // Coldest Card
    QFrame *cardCold = new QFrame(intelHub);
    cardCold->setObjectName("badgeColdest");
    cardCold->setProperty("role", "intelBadge");
    auto *vlCold = new QVBoxLayout(cardCold);
    QLabel *lblColdTitle = new QLabel("POINT FROID", cardCold);
    lblColdTitle->setToolTip("Identifie le compartiment le plus performant en termes de puissance frigorifique.");
    lblColdTitle->setStyleSheet("font-size: 9px; font-weight: 700; color: #166534;");
    QLabel *lblColdVal = new QLabel("--", cardCold);
    lblColdVal->setObjectName("lblIntelColdest");
    lblColdVal->setStyleSheet("font-size: 14px; font-weight: 800; color: #064e3b;");
    vlCold->addWidget(lblColdTitle);
    vlCold->addWidget(lblColdVal);
    thermalCards->addWidget(cardCold);
    
    thermalCol->addLayout(thermalCards);
    hubLayout->addLayout(thermalCol, 1);

    // Optimization segments Column
    auto *optCol = new QVBoxLayout();
    QLabel *optTitle = new QLabel("SEGMENTS D'OPTIMISATION", intelHub);
    optTitle->setProperty("role", "cardTitle");
    optCol->addWidget(optTitle);
    
    QHBoxLayout *optCards = new QHBoxLayout();
    
    // Critical Card
    QFrame *cardCrit = new QFrame(intelHub);
    cardCrit->setObjectName("badgeCritical");
    cardCrit->setProperty("role", "intelBadge");
    auto *vlCrit = new QVBoxLayout(cardCrit);
    QLabel *lblCritTitle = new QLabel("ZONES CRITIQUES", cardCrit);
    lblCritTitle->setStyleSheet("font-size: 9px; font-weight: 700; color: #9a3412;");
    QLabel *lblCritVal = new QLabel("--", cardCrit);
    lblCritVal->setObjectName("lblIntelCriticalLoad");
    lblCritVal->setStyleSheet("font-size: 14px; font-weight: 800; color: #7c2d12;");
    vlCrit->addWidget(lblCritTitle);
    vlCrit->addWidget(lblCritVal);
    optCards->addWidget(cardCrit);

    // Underused Card
    QFrame *cardUnder = new QFrame(intelHub);
    cardUnder->setObjectName("badgeUnderused");
    cardUnder->setProperty("role", "intelBadge");
    auto *vlUnder = new QVBoxLayout(cardUnder);
    QLabel *lblUnderTitle = new QLabel("SOUS-UTILISATION", cardUnder);
    lblUnderTitle->setStyleSheet("font-size: 9px; font-weight: 700; color: #374151;");
    QLabel *lblUnderVal = new QLabel("--", cardUnder);
    lblUnderVal->setObjectName("lblIntelUnderused");
    lblUnderVal->setStyleSheet("font-size: 14px; font-weight: 800; color: #111827;");
    vlUnder->addWidget(lblUnderTitle);
    vlUnder->addWidget(lblUnderVal);
    optCards->addWidget(cardUnder);

    optCol->addLayout(optCards);
    hubLayout->addLayout(optCol, 1);

    ui->verticalLayout->addWidget(intelHub);
}

void ChambresFroidesStatsPage::updateIntelligenceUI()
{
    auto intel = ChambresFroides::getAdvancedIntelligence();
    auto alerts = ChambresFroides::getActiveAlerts();

    // Health Score
    QLabel *lblHealth = findChild<QLabel*>("lblHealthScore");
    if (lblHealth) {
        double score = intel["health_score"].toDouble();
        lblHealth->setText(intel["health_score"] + "%");
        if (score > 80) lblHealth->setStyleSheet("color: #10b981; font-weight: 800;");
        else if (score > 50) lblHealth->setStyleSheet("color: #f59e0b; font-weight: 800;");
        else lblHealth->setStyleSheet("color: #ef4444; font-weight: 800;");
        
        lblHealth->parentWidget()->setToolTip("Note globale d'intégrité du port froid. Elle chute si des alertes critiques non gérées s'accumulent.");
        lblHealth->setToolTip("Calcul mathématique pondéré basé sur : Températures + Occupation + Alertes de maintenance.");
    }

    // Thermal
    QLabel *lblHottest = findChild<QLabel*>("lblIntelHottest");
    if (lblHottest) {
        lblHottest->setText(intel["hottest"]);
        lblHottest->setToolTip("Identifie le maillon faible thermique du port.");
    }
    QLabel *lblColdest = findChild<QLabel*>("lblIntelColdest");
    if (lblColdest) {
        lblColdest->setText(intel["coldest"]);
        lblColdest->setToolTip("Indique la chambre la plus performante.");
    }

    // Segments
    QLabel *lblCrit = findChild<QLabel*>("lblIntelCriticalLoad");
    if (lblCrit) {
        lblCrit->setText(intel["critical_load_count"] + " Zones");
        lblCrit->setToolTip("Nombre de chambres saturées (>85%).");
    }
    QLabel *lblUnder = findChild<QLabel*>("lblIntelUnderused");
    if (lblUnder) {
        lblUnder->setText(intel["underused_count"] + " Zones");
        lblUnder->setToolTip("Chambres sous-utilisées (<30%).");
    }

    // Set tooltips for remaining section headers
    if (ui->lblChartStatusTitle) ui->lblChartStatusTitle->setToolTip("Vision globale de la santé de vos équipements (Actifs, En panne, etc.).");
    if (ui->lblChartSpeciesTitle) ui->lblChartSpeciesTitle->setToolTip("Aperçu graphique du taux d'occupation total du pôle froid.");
    if (ui->lblTitle) ui->lblTitle->setToolTip("Centre de commandement des statistiques portuaires.");
    if (ui->lblSubtitle) ui->lblSubtitle->setToolTip("Surveillance continue des indicateurs clés de performance (KPI).");
    if (ui->lblChartTempTitle) ui->lblChartTempTitle->setToolTip("Cliquez pour voir l'historique détaillé des sondes thermiques.");

    // Alert Inbox
    if (m_btnAlertInbox) {
        int count = alerts.size();
        m_btnAlertInbox->setText(QString("Alertes (%1)").arg(count));
        
        bool hasNewAlerts = false;
        for (const auto &a : alerts) {
            if (a["status"] == "New") {
                hasNewAlerts = true;
                break;
            }
        }

        if (hasNewAlerts) {
            if (!m_alertBlinkTimer->isActive()) m_alertBlinkTimer->start(500);
        } else {
            m_alertBlinkTimer->stop();
            if (count > 0) {
                m_btnAlertInbox->setStyleSheet("background-color: #fee2e2; color: #b91c1c; border: 1px solid #ef4444; font-weight: bold; border-radius: 8px;");
            } else {
                m_btnAlertInbox->setStyleSheet("background-color: #f1f5f9; color: #64748b; border: 1px solid #e2e8f0; font-weight: bold; border-radius: 8px;");
            }
        }
    }
}

void ChambresFroidesStatsPage::refreshStats()
{
    QString err;
    auto stats = ChambresFroides::getStatistics(&err);
    
    // 1. Update Temperature KPI with color logic
    double avgTemp = stats["avg_temp"];
    ui->lblKpiTempValue->setText(QString::number(avgTemp, 'f', 1) + " °C");
    
    // Dynamic color based on temperature
    if (avgTemp > -10) {
        ui->lblKpiTempValue->setStyleSheet("color: #e63946;"); // Critical (too warm)
    } else if (avgTemp > -18) {
        ui->lblKpiTempValue->setStyleSheet("color: #f4a261;"); // Warning
    } else {
        ui->lblKpiTempValue->setStyleSheet("color: #2a9d8f;"); // Ideal
    }
    
    // Add detail to subtitle or tooltip
    ui->lblKpiTempTitle->setToolTip(QString("Min: %1 °C | Max: %2 °C").arg(stats["min_temp"], 0, 'f', 1).arg(stats["max_temp"], 0, 'f', 1));
    ui->lblKpiTempValue->setToolTip("Calculé sur l'ensemble des chambres opérationnelles.");
    ui->cardKpiTemp->setToolTip("Cette valeur indique la température moyenne du pôle froid.\nUne déviation peut signaler une défaillance globale du système de refroidissement.");

    // 2. Update Stock KPI with Occupancy Rate
    // 2. Update Stock KPI with Occupancy Rate (Standardized to KG)
    double stockKg = stats["total_stock"]; 
    ui->lblKpiStockValue->setText(QString::number(stockKg, 'f', 0) + " kg");
    ui->lblKpiStockTitle->setText(QString("Stock Total (%1%)").arg((int)stats["occ_rate"]));
    ui->cardKpiStock->setToolTip(QString("Quantité totale de poisson stockée (%1 kg).\nTaux d'occupation global: %2%").arg(stockKg, 0, 'f', 0).arg(stats["occ_rate"], 0, 'f', 1));
    ui->lblKpiStockValue->setToolTip("Métrique indiquant la pression logistique actuelle sur vos capacités froides.");
    ui->lblKpiStockTitle->setToolTip("Taux d'utilisation effectif du volume portuaire disponible.");
    
    // 3. Update Status KPI with alerts
    ui->lblKpiActiveValue->setText(QString("%1 / %2").arg((int)stats["active_count"]).arg((int)stats["total_count"]));
    if (stats["alert_count"] > 0) {
        ui->lblKpiActiveValue->setStyleSheet("color: #ef4444; font-weight: 800;");
        ui->lblKpiActiveTitle->setText(QString("Actives (%1 Alertes!)").arg((int)stats["alert_count"]));
        ui->cardKpiActive->setToolTip("Alerte: Certaines chambres nécessitent une intervention (Maintenance ou Panne detectée).");
    } else {
        ui->lblKpiActiveValue->setStyleSheet("color: #1e293b;");
        ui->lblKpiActiveTitle->setText("Chambres Opérationnelles");
        ui->cardKpiActive->setToolTip("Indique la proportion de chambres prêtes à l'emploi et sans alerte critique.");
    }
    ui->lblKpiActiveValue->setToolTip("Nombre de chambres en état de marche par rapport au parc total.");
    ui->lblKpiActiveTitle->setToolTip("Disponibilité matérielle de votre infrastructure.");

    // Update Temperature Chart (Bar Chart)
    if (tempSet) {
        tempSet->remove(0, tempSet->count());
        QSqlQueryModel *tempModel = ChambresFroides::getTemperatureStats();
        if (tempModel) {
            QStringList categories;
            double minY = 0, maxY = -30;
            for (int i = 0; i < tempModel->rowCount(); ++i) {
                QString id = tempModel->record(i).value("TAG_NUMBER").toString();
                double val = tempModel->record(i).value("TEMP").toDouble();
                *tempSet << val;
                categories << id;
                if (val < minY) minY = val;
                if (val > maxY) maxY = val;
            }
            
            // Refresh X-Axis categories (Room IDs)
            QChart *tChart = ui->chartTempContainer->findChild<QChartView*>()->chart();
            QBarCategoryAxis *axisX = qobject_cast<QBarCategoryAxis*>(tChart->axes(Qt::Horizontal).first());
            if (axisX) {
                axisX->clear();
                axisX->append(categories);
            }

            // Adjust Y-axis for Temperature
            QValueAxis *axisY = qobject_cast<QValueAxis*>(tChart->axes(Qt::Vertical).first());
            if (axisY) {
                axisY->setRange(qMin(minY - 2, -30.0), qMax(maxY + 5, 15.0));
            }

            // Dynamic Coloring logic for bars
            // Note: QBarSet doesn't support per-bar colors easily without multiple sets.
            // We'll keep the standard color or use the tooltip to highlight.
            
            delete tempModel;
        }
    }


    // Update Status Chart
    if (statusSeries) {
        statusSeries->clear();
        QSqlQueryModel *statusModel = ChambresFroides::getStatusDistribution();
        if (statusModel) {
            for (int i = 0; i < statusModel->rowCount(); ++i) {
                QString status = statusModel->record(i).value(0).toString();
                int count = statusModel->record(i).value(1).toInt();
                statusSeries->append(status, count);
            }
            delete statusModel;
            
            for (auto *slice : statusSeries->slices()) {
                if (slice->label() == "Active") {
                    slice->setColor(QColor("#2a9d8f"));
                    slice->setExploded(true);
                }
                else if (slice->label() == "Broken") slice->setColor(QColor("#e63946"));
                else if (slice->label() == "Maintenance") slice->setColor(QColor("#f4a261"));
                else slice->setColor(QColor("#457b9d"));
                slice->setLabelVisible(true);
            }
        }
    }

    // Update Occupancy Rate Donut
    if (speciesSeries) {
        speciesSeries->clear();
        double rate = stats["occ_rate"];
        auto *occSlice = speciesSeries->append("Occupé", rate);
        auto *freeSlice = speciesSeries->append("Libre", 100.0 - rate);
        occSlice->setColor(QColor("#6366f1"));
        freeSlice->setColor(QColor("#f1f5f9"));
        ui->lblChartSpeciesTitle->setText(QString("Taux d'Occupation: %1%").arg((int)rate));
        ui->lblChartSpeciesTitle->setVisible(true);
        ui->chartSpeciesContainer->setVisible(true);
    }

    updateIntelligenceUI();
}

void ChambresFroidesStatsPage::applyStyles()
{
    setAttribute(Qt::WA_StyledBackground, true);
    
    // Assign properties for styling
    if (ui->lblTitle) ui->lblTitle->setProperty("role", "pageTitle");
    if (ui->lblSubtitle) ui->lblSubtitle->setProperty("role", "pageSubtitle");
    
    // Cards
    QList<QFrame*> cards = {
        ui->cardKpiTemp, ui->cardKpiStock, ui->cardKpiActive,
        ui->cardChartTemp, ui->cardChartStatus
    };
    for(auto *card : cards) {
        if(card) {
            card->setProperty("card", "content");
            card->setAttribute(Qt::WA_StyledBackground, true);
        }
    }

    // Value Labels
    QList<QLabel*> values = { ui->lblKpiTempValue, ui->lblKpiStockValue, ui->lblKpiActiveValue };
    for(auto *lbl : values) if(lbl) lbl->setProperty("role", "kpiValue");

    // Titles
    QList<QLabel*> titles = { 
        ui->lblKpiTempTitle, ui->lblKpiStockTitle, ui->lblKpiActiveTitle,
        ui->lblChartTempTitle, ui->lblChartStatusTitle, ui->lblChartSpeciesTitle
    };
    for(auto *lbl : titles) if(lbl) lbl->setProperty("role", "cardTitle");

    const QString style = QStringLiteral(R"(
        QWidget {
            font-family: 'Inter', 'Segoe UI', sans-serif;
            background-color: #f1f5f9;
        }
        /* Page Header */
        QLabel[role="pageTitle"] {
            font-size: 28px;
            font-weight: 800;
            color: #1e293b;
            letter-spacing: -0.5px;
        }
        QLabel[role="pageSubtitle"] {
            font-size: 15px;
            color: #64748b;
            margin-bottom: 20px;
        }

        /* Cards with Premium Styling */
        QFrame[card="content"] {
            background: #ffffff;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
        }
        QFrame[card="content"]:hover {
            border: 1px solid #cbd5e1;
            background: #ffffff;
        }
        
        /* KPI Highlights */
        #cardKpiTemp { border-left: 4px solid #3b82f6; }
        #cardKpiStock { border-left: 4px solid #10b981; }
        #cardKpiActive { border-left: 4px solid #f59e0b; }
        #cardHealth { border-left: 4px solid #6366f1; }

        /* Titles and Labels */
        QLabel[role="cardTitle"] {
            color: #64748b;
            font-size: 11px;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        QLabel[role="kpiValue"] {
            color: #0f172a;
            font-size: 32px;
            font-weight: 800;
            letter-spacing: -1px;
        }

        /* Dashboard Intel Section */
        #intelHub { padding: 10px; border: 1px solid #e2e8f0; border-top: 4px solid #6366f1; border-radius: 12px; background-color: #ffffff; }
        
        QFrame[role="intelBadge"] {
            background-color: #f8fafc;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
            min-width: 140px;
            margin: 4px;
            padding: 8px;
        }
        
        QFrame[role="intelBadge"]:hover {
            border: 1px solid #cbd5e1;
        }
        
        #badgeHottest { background-color: #fef2f2; border: 1px solid #fecaca; border-left: 4px solid #ef4444; }
        #badgeColdest { background-color: #f0fdf4; border: 1px solid #bbf7d0; border-left: 4px solid #22c55e; }
        #badgeCritical { background-color: #fff7ed; border: 1px solid #fed7aa; border-left: 4px solid #f97316; }
        #badgeUnderused { background-color: #f8fafc; border: 1px solid #e2e8f0; border-left: 4px solid #64748b; }

        /* Buttons */
        QPushButton#btnRefresh {
            background-color: #3b82f6;
            color: white;
            border-radius: 6px;
            font-weight: 700;
            padding: 8px 16px;
        }
        QPushButton#btnRefresh:hover { 
            background-color: #2563eb; 
        }
        
        #btnAlertInbox {
            margin-right: 10px;
            font-size: 12px;
        }
    )");
    
    setStyleSheet(style);
}

void ChambresFroidesStatsPage::setupCharts()
{
    // 1. Temperature Chart (Bar Chart)
    tempSet = new QBarSet("Température (°C)");
    tempSet->setColor(QColor("#3b82f6"));
    
    tempSeries = new QBarSeries();
    tempSeries->append(tempSet);
    
    auto *tempChart = new QChart();
    tempChart->addSeries(tempSeries);
    tempChart->setTitle("Températures par Chambre");
    tempChart->setAnimationOptions(QChart::SeriesAnimations);
    
    QBarCategoryAxis *axisX_temp = new QBarCategoryAxis();
    tempChart->addAxis(axisX_temp, Qt::AlignBottom);
    tempSeries->attachAxis(axisX_temp);

    QValueAxis *axisY_temp = new QValueAxis();
    axisY_temp->setTitleText("Temp (°C)");
    axisY_temp->setLabelFormat("%.1f");
    tempChart->addAxis(axisY_temp, Qt::AlignLeft);
    tempSeries->attachAxis(axisY_temp);
    
    auto *tempView = new QChartView(tempChart);
    tempView->setRenderHint(QPainter::Antialiasing);
    tempView->setToolTip("Température actuelle pour chaque chambre froide.");
    ui->lblChartTempTitle->setText("Températures par Chambre");
    ui->lblChartTempTitle->setToolTip("Visualisation directe des conditions thermiques par emplacement.");
    
    // Interactive Tooltip for Bar Chart
    connect(tempSeries, &QBarSeries::hovered, [](bool status, int index, QBarSet *set) {
        if (status) {
            QToolTip::showText(QCursor::pos(), QString("Chambre: %1\nTempérature: %2 °C").arg(set->label()).arg(QString::number(set->at(index), 'f', 1)));
        }
    });

    auto *tempLayout = new QVBoxLayout(ui->chartTempContainer);
    tempLayout->addWidget(tempView);
    tempLayout->setContentsMargins(0, 0, 0, 0);

    // 3. Status Chart (Pie Chart)
    statusSeries = new QPieSeries();
    statusSeries->setHoleSize(0.35); // Donut style
    
    auto *statusChart = new QChart();
    statusChart->addSeries(statusSeries);
    statusChart->setTitle("Distribution de l'État");
    statusChart->setAnimationOptions(QChart::SeriesAnimations);
    statusChart->legend()->setAlignment(Qt::AlignRight);
    
    auto *statusView = new QChartView(statusChart);
    statusView->setRenderHint(QPainter::Antialiasing);
    statusView->setToolTip("Analyse de la disponibilité technique de vos chambres.");
    ui->lblChartStatusTitle->setToolTip("Répartition statistique de l'état opérationnel du parc matériel.");

    // Interactive Tooltip for Pie Chart
    connect(statusSeries, &QPieSeries::hovered, [](QPieSlice *slice, bool state) {
        if (state) {
            QToolTip::showText(QCursor::pos(), QString("État: %1\nVolume: %2 chambres (%3%)")
                .arg(slice->label()).arg(slice->value()).arg(QString::number(slice->percentage() * 100, 'f', 1)));
        }
    });

    auto *statusLayout = new QVBoxLayout(ui->chartStatusContainer);
    statusLayout->addWidget(statusView);
    statusLayout->setContentsMargins(0, 0, 0, 0);

    // 4. Detailed Occupancy Rate (Re-purposing the species container)
    speciesSeries = new QPieSeries();
    speciesSeries->setHoleSize(0.6); // Thin donut
    
    auto *rateChart = new QChart();
    rateChart->addSeries(speciesSeries);
    rateChart->setTitle("Taux d'Occupation Global");
    rateChart->legend()->hide();
    
    auto *rateView = new QChartView(rateChart);
    rateView->setRenderHint(QPainter::Antialiasing);
    rateView->setToolTip("Représente la part occupée de vos ressources froides totales.");
    ui->lblChartSpeciesTitle->setToolTip("Visualisation de l'équilibre entre stock et capacité libre.");

    auto *rateLayout = new QVBoxLayout(ui->chartSpeciesContainer);
    rateLayout->addWidget(rateView);
    rateLayout->setContentsMargins(0, 0, 0, 0);
    
    // Update rate chart initial state
    QString err;
    auto stats = ChambresFroides::getStatistics(&err);
    double rate = stats["occ_rate"];
    speciesSeries->append("Occupé", rate)->setColor(QColor("#6366f1"));
    speciesSeries->append("Libre", 100.0 - rate)->setColor(QColor("#f1f5f9"));
}

