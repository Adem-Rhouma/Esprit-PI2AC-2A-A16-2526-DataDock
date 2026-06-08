#include "meteostripwidget.h"
#include "NavireConstants.h"
#include <QIcon>
#include <QMouseEvent>

MeteoStripWidget::MeteoStripWidget(QWidget *parent) : QFrame(parent) {
    setupUi();
    applyStyle();
}

void MeteoStripWidget::setupUi() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 8, 15, 8);
    mainLayout->setSpacing(20);

    // Risk Status Group
    QHBoxLayout *statusGroup = new QHBoxLayout();
    m_riskOrb = new QLabel();
    m_riskOrb->setFixedSize(12, 12);
    m_riskOrb->setStyleSheet("background: #10B981; border-radius: 6px;");
    
    m_statusLabel = new QLabel("SAFE TO OPERATE");
    m_statusLabel->setStyleSheet("font-weight: 800; font-size: 11px; color: #0f172a;");
    
    statusGroup->addWidget(m_riskOrb);
    statusGroup->addWidget(m_statusLabel);
    
    // Details
    m_detailsLabel = new QLabel("Port of Tunis: Calm | Wind 8kts | Waves 0.4m");
    m_detailsLabel->setStyleSheet("color: #64748b; font-size: 11px; font-weight: 500;");

    mainLayout->addLayout(statusGroup);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_detailsLabel);
    mainLayout->addStretch();
    
    // Forecast Capsule Area
    m_forecastLayout = new QHBoxLayout();
    m_forecastLayout->setSpacing(10);
    mainLayout->addLayout(m_forecastLayout);
}

void MeteoStripWidget::applyStyle() {
    setStyleSheet(R"(
        MeteoStripWidget {
            background: rgba(255, 255, 255, 0.9);
            border: 1px solid #e2e8f0;
            border-radius: 12px;
        }
    )");
}

void MeteoStripWidget::updateData(const MeteoData::MeteoReport &report) {
    // Update Risk UI (French Wording)
    QString color = "#10B981";
    QString text = "OPÉRATION AUTORISÉE";
    
    if (report.advisory.globalRiskScore >= 70) {
        color = "#EF4444";
        text = "RISQUE ÉLEVÉ";
    } else if (report.advisory.globalRiskScore >= 40) {
        color = "#F59E0B";
        text = "OPÉRATIONS RESTREINTES";
    }
    
    m_riskOrb->setStyleSheet(QString("background: %1; border-radius: 6px;").arg(color));
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(QString("font-weight: 800; font-size: 13px; color: %1;").arg(color == "#10B981" ? "#0f172a" : color));
    
    m_detailsLabel->setText(QString("💨 Vent: %1 kts  |  🌊 Houle: %2 m")
                             .arg(QString::number(report.current.windSpeed, 'f', 1))
                             .arg(QString::number(report.current.waveHeight, 'f', 1)));
    m_detailsLabel->setStyleSheet("font-family: 'Segoe UI Emoji', 'Noto Color Emoji', 'Apple Color Emoji', 'Segoe UI Symbol', sans-serif; "
                                  "color: #1e293b; font-size: 14px; font-weight: 700;");

    // Update Forecast Strip (Mini Capsule)
    QLayoutItem *item;
    while ((item = m_forecastLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }
    
    for (int i = 0; i < qMin(5, (int)report.forecast.size()); ++i) {
        const auto &day = report.forecast[i];
        QString dayRisk = day.riskScore >= 70 ? "🔴" : (day.riskScore >= 40 ? "🟡" : "🟢");
        
        QLabel *cap = new QLabel(QString("%1 %2 %3m").arg(dayRisk).arg(day.date.toString("ddd")).arg(QString::number(day.maxWaveHeight, 'f', 1)));
        cap->setStyleSheet("font-family: 'Segoe UI Emoji', 'Noto Color Emoji', 'Apple Color Emoji', 'Segoe UI Symbol', sans-serif; "
                           "background: #f8fafc; padding: 4px 10px; border: 1px solid #e2e8f0; border-radius: 6px; font-size: 11px; font-weight: 800; color: #475569;");
        m_forecastLayout->addWidget(cap);
    }
}

void MeteoStripWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QFrame::mousePressEvent(event);
}
