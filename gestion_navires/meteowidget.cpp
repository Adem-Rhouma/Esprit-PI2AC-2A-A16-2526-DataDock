#include "meteowidget.h"
#include "ui_meteowidget.h"
#include "meteoglobalservice.h"
#include "NavireConstants.h"
#include <QHeaderView>
#include <QTableWidgetItem>

MeteoWidget::MeteoWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MeteoWidget)
{
    ui->setupUi(this);
    setupUiManual();
    applyStyle();

    connect(&MeteoGlobalService::instance(), &MeteoGlobalService::dataUpdated,
            this, &MeteoWidget::updateData);
    
    // Initial sync
    MeteoGlobalService::instance().refresh();
}

MeteoWidget::~MeteoWidget() {
    delete ui;
}

void MeteoWidget::setupUiManual() {
    // We reuse parts of the existing UI or inject into layouts
    // Clear legacy hero card content if necessary, but here we'll just redirect labels
    
    m_riskGaugeLabel = ui->lblTemp; // Reusing large label for gauge
    m_riskDescription = ui->lblRiskValue;
    m_portConditionLabel = ui->lblSubtitle;

    // Inject Sea Matrix into the calendar section for better planning
    // Remove legacy calendar grid
    QLayoutItem *item;
    while ((item = ui->calendarGrid->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    m_seaMatrix = new QTableWidget(7, 4);
    m_seaMatrix->setHorizontalHeaderLabels({
        "DATE", "CONDITIONS MÉTÉO", "SCORE", "NIVEAU"
    });
    
    // Set column proportions for cleaner distribution (4 columns)
    m_seaMatrix->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_seaMatrix->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_seaMatrix->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    
    m_seaMatrix->verticalHeader()->setVisible(false);
    m_seaMatrix->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_seaMatrix->setAlternatingRowColors(true);
    m_seaMatrix->setMinimumHeight(450);
    m_seaMatrix->verticalHeader()->setDefaultSectionSize(55);
    
    m_seaMatrix->setStyleSheet(R"(
        QTableWidget {
            background-color: #ffffff;
            color: #1e293b;
            font-family: 'Segoe UI Emoji', 'Noto Color Emoji', 'Apple Color Emoji', 'Segoe UI Symbol', sans-serif;
            font-size: 13px;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
            gridline-color: #f1f5f9;
            alternate-background-color: #f8fafc;
        }
        QHeaderView::section {
            background-color: #f1f5f9;
            color: #475569;
            font-family: 'Segoe UI Emoji', 'Noto Color Emoji', 'Apple Color Emoji', 'Segoe UI Symbol', sans-serif;
            font-size: 12px;
            font-weight: 800;
            border: none;
            padding: 10px;
            text-transform: uppercase;
        }
    )");

    // Last update label
    m_lastUpdateLabel = new QLabel("Dernière mise à jour: --:--");
    m_lastUpdateLabel->setStyleSheet("color: #64748b; font-size: 11px; font-weight: 600;");
    m_lastUpdateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    ui->calendarGrid->addWidget(m_lastUpdateLabel, 0, 0);
    ui->calendarGrid->addWidget(m_seaMatrix, 1, 0);
    
    ui->lblCalendarTitle->setText("MATRICE D'ANALYSE OPÉRATIONNELLE (7 JOURS)");
    ui->lblCalendarTitle->setStyleSheet("color: #475569; font-weight: 800; text-transform: uppercase;");

    // Cleanup: Hide legacy forecast elements as they are now consolidated in the matrix
    if (ui->forecastContainer) ui->forecastContainer->hide();
    if (ui->lblForecastTitle) ui->lblForecastTitle->hide();
}

void MeteoWidget::applyStyle() {
    // Ensure high contrast: white background with dark slate text
    m_seaMatrix->setStyleSheet(R"(
        QTableWidget {
            background-color: #ffffff;
            color: #1e293b;
            font-size: 13px;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
            gridline-color: #f1f5f9;
            alternate-background-color: #f8fafc;
        }
        QHeaderView::section {
            background-color: #f1f5f9;
            color: #475569;
            font-size: 13px;
            font-weight: bold;
            border: none;
            padding: 10px;
        }
    )");
}

void MeteoWidget::updateData(const MeteoData::MeteoReport &report) {
    emit weatherUpdated(report);

    // 1. Language & Risk Hierarchy
    // LEFT HUB: Decision Authority
    QString riskLevel = "OPÉRATION AUTORISÉE";
    QString mainRecommendation = "Navigation autorisée";
    QString statusColor = "#10B981";
    QString portCondition = "Stables";
    
    if (report.advisory.globalRiskScore >= 70) {
        riskLevel = "RISQUE ÉLEVÉ";
        mainRecommendation = "Sortie déconseillée";
        statusColor = "#EF4444";
        portCondition = "Critiques";
    } else if (report.advisory.globalRiskScore >= 40) {
        riskLevel = "OPÉRATIONS RESTREINTES";
        mainRecommendation = "Prudence recommandée";
        statusColor = "#F59E0B";
        portCondition = "Dégradées";
    }

    // Left Decision Panel
    m_riskDescription->setText(riskLevel);
    m_riskDescription->setStyleSheet(QString("font-size: 26px; font-weight: 900; color: %1; letter-spacing: 1px;").arg(statusColor));

    m_riskGaugeLabel->setText(QString("<span style='font-size: 13px; color: rgba(255,255,255,0.8);'>SCORE DE RISQUE</span><br/>"
                                     "<b style='font-size: 38px;'>%1</b><span style='font-size: 16px;'>/100</span><br/>"
                                     "<span style='font-size: 14px; font-weight: 500;'>Recommandation:</span><br/>"
                                     "<b style='font-size: 18px;'>%2</b>")
                             .arg(report.advisory.globalRiskScore)
                             .arg(mainRecommendation));
    
    // RIGHT HUB: Environmental Intelligence (Context)
    QString contextHtml = QString(
        "<div style='line-height: 1.4;'>"
        "  <span style='font-size: 11px; font-weight: 800; color: #64748b; text-transform: uppercase;'>Intelligence Environnementale</span><br/>"
        "  <span style='font-size: 13px; font-weight: 700; color: #334155;'>Conditions portuaires : %1</span><br/>"
        "  <span style='font-size: 12px; color: #475569; font-style: italic;'>%2</span>"
        "</div>"
    ).arg(portCondition).arg(report.advisory.text);

    ui->lblAdvisoryText->setText(contextHtml);
    ui->lblAdvisoryText->setTextFormat(Qt::RichText);
    ui->lblAdvisoryText->setWordWrap(true);

    // Key Metrics (High Visibility Refinement)
    ui->lblWind->setText(QString("💨 Vent: %1 kts  |  🌊 Houle: %2 m")
                        .arg(QString::number(report.current.windSpeed, 'f', 1))
                        .arg(QString::number(report.current.waveHeight, 'f', 1)));
    ui->lblWind->setStyleSheet("font-family: 'Segoe UI Emoji', 'Noto Color Emoji', 'Apple Color Emoji', 'Segoe UI Symbol', sans-serif; "
                               "font-weight: 700; color: #1e293b; font-size: 18px;");
    
    m_lastUpdateLabel->setText(QString("Dernière mise à jour: %1").arg(QTime::currentTime().toString("HH:mm")));

    // 2. Update Unified Operational Matrix (4 Columns - Strategic View)
    m_seaMatrix->setRowCount(qMin(7, (int)report.forecast.size()));
    for (int i = 0; i < m_seaMatrix->rowCount(); ++i) {
        const auto &day = report.forecast[i];
        
        // Logical assessment
        QString color = "#10B981"; // Green
        QString statusText = "CALME";
        QString recommendation = "Navigation autorisée";
        QString explanation = "Conditions optimales";

        if (day.riskScore >= 70) {
            color = "#EF4444"; // Red
            statusText = "CRITIQUE";
            recommendation = "Sortie déconseillée";
        } else if (day.riskScore >= 40) {
            color = "#F59E0B"; // Amber
            statusText = "RISQUÉ";
            recommendation = "Prudence recommandée";
        }

        // Rule-based justification
        if (day.maxWindSpeed > 25) explanation = QString("Vent élevé (>25 kts)");
        else if (day.maxWaveHeight > 2.0) explanation = QString("Houle importante (>2.0m)");
        else if (day.riskScore < 20) explanation = "Conditions idéales";
        else explanation = "Conditions stables";

        QString fullAdvisory = QString("%1\n%2").arg(recommendation).arg(explanation);

        auto createItem = [this, fullAdvisory](const QString &text, const QString &colorHex = "", bool bold = false) {
            auto *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            item->setToolTip(fullAdvisory); // Explanation layer on hover
            if (!colorHex.isEmpty()) item->setForeground(QBrush(QColor(colorHex)));
            
            QFont font = item->font();
            font.setPointSizeF(12.5);
            font.setFamilies({ "Segoe UI Emoji", "Noto Color Emoji", "Apple Color Emoji", "Segoe UI Symbol", "Sans Serif" });
            if (bold) font.setWeight(QFont::Bold);
            item->setFont(font);
            return item;
        };

        // Col 0: DATE
        m_seaMatrix->setItem(i, 0, createItem(day.date.toString("ddd dd MMM"), "", true));
        
        // Col 1: CONDITIONS (Icon-driven Observation)
        m_seaMatrix->setItem(i, 1, createItem(QString("🌊 %1 m | 💨 %2 kts")
                                            .arg(QString::number(day.maxWaveHeight, 'f', 1))
                                            .arg(QString::number(day.maxWindSpeed, 'f', 0))));

        // Col 2: SCORE (Secondary Metric)
        m_seaMatrix->setItem(i, 2, createItem(QString::number(day.riskScore), color, true));

        // Col 3: NIVEAU (Light Status Badge)
        m_seaMatrix->setItem(i, 3, createItem(statusText, color, true));
        
        // Final row styling: Light tint based on risk (unified 4-column background)
        for (int c = 0; c < 4; ++c) {
            if (day.riskScore >= 40) {
                 m_seaMatrix->item(i, c)->setBackground(QBrush(QColor(day.riskScore >= 70 ? "#fff1f2" : "#fffbeb")));
            }
        }
    }
}

void MeteoWidget::showError(const QString &error) {
    qDebug() << "[Planificateur] Error:" << error;
}
