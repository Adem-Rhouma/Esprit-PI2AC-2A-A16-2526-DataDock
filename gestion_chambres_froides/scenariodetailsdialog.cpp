#include "scenariodetailsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QJsonArray>
#include <QJsonObject>
#include <QSlider>
#include <QSpinBox>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QApplication>
#include <QFont>
#include <QColor>
#include <QMargins>
#include <QPainter>

// Chart headers
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>

ScenarioDetailsDialog::ScenarioDetailsDialog(const QJsonObject &solution, int scenarioIndex, QWidget *parent) :
    QDialog(parent),
    m_freshnessView(nullptr)
{
    setupUI();
    populateData(solution, scenarioIndex);
    
    setWindowTitle("Rapport d'Expertise Prédictive 🚀");
    resize(1100, 950);
}

ScenarioDetailsDialog::~ScenarioDetailsDialog()
{
}

void ScenarioDetailsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- Header ---
    QFrame *header = new QFrame();
    header->setFixedHeight(100);
    header->setStyleSheet("background: #0f172a; border-bottom: 3px solid #3b82f6;");
    QHBoxLayout *hl = new QHBoxLayout(header);
    hl->setContentsMargins(40, 0, 40, 0);
    
    QLabel *title = new QLabel("ANALYSE STRATÉGIQUE IA");
    title->setStyleSheet("color: white; font-size: 24px; font-weight: 900; letter-spacing: 1px;");
    hl->addWidget(title);
    hl->addStretch();
    
    QLabel *badge = new QLabel("CERTIFIÉ ANALYTIQUE");
    badge->setStyleSheet("background: #3b82f6; color: white; padding: 6px 14px; border-radius: 15px; font-weight: 800; font-size: 10px;");
    hl->addWidget(badge);
    mainLayout->addWidget(header);

    // --- Main Content Scroll Area ---
    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("background: #f8fafc;");
    
    QWidget *content = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(30);

    // --- Metrics Row ---
    QHBoxLayout *metricsRow = new QHBoxLayout();
    metricsRow->setObjectName("metricsRow");
    layout->addLayout(metricsRow);

    // --- Visual Simulator Section ---
    QFrame *simuBox = new QFrame();
    simuBox->setStyleSheet("background: white; border-radius: 20px; border: 1px solid #e2e8f0;");
    QVBoxLayout *sl = new QVBoxLayout(simuBox);
    sl->setContentsMargins(30, 30, 30, 30);
    
    QLabel *slTitle = new QLabel("🕒 SIMULATION DE VIE UTILE RÉSIDUELLE");
    slTitle->setStyleSheet("font-weight: 800; color: #0f172a; font-size: 15px;");
    sl->addWidget(slTitle);

    QHBoxLayout *simuLogic = new QHBoxLayout();
    
    // State Icon (Large indicator)
    m_stateIcon = new QLabel("🐟");
    m_stateIcon->setAlignment(Qt::AlignCenter);
    m_stateIcon->setFixedSize(120, 120);
    m_stateIcon->setStyleSheet("font-size: 60px; background: #f8fafc; border-radius: 60px; border: 4px solid #3b82f6;");
    simuLogic->addWidget(m_stateIcon);

    // Simulation controls with explicit day labels
    QVBoxLayout *simuControls = new QVBoxLayout();
    m_predictionValue = new QLabel("100.0%");
    m_predictionValue->setStyleSheet("font-size: 42px; font-weight: 900; color: #10b981;");
    
    m_predictionVerdict = new QLabel("Qualité Initiale Optimale");
    m_predictionVerdict->setStyleSheet("font-size: 16px; font-weight: 700; color: #64748b;");
    
    m_daySlider = new QSlider(Qt::Horizontal);
    m_daySlider->setRange(0, 7);
    m_daySlider->setTickPosition(QSlider::TicksBelow);
    m_daySlider->setTickInterval(1);
    m_daySlider->setStyleSheet("QSlider::groove:horizontal { height: 10px; background: #e2e8f0; border-radius: 5px; } "
                               "QSlider::handle:horizontal { background: #3b82f6; width: 34px; height: 34px; margin: -12px 0; border-radius: 17px; border: 4px solid white; }");
    connect(m_daySlider, &QSlider::valueChanged, this, &ScenarioDetailsDialog::updatePrediction);

    QHBoxLayout *ticks = new QHBoxLayout();
    ticks->setContentsMargins(17, 0, 17, 0); // Align labels with slider handle center
    ticks->setSpacing(0);
    
    // Horizon Control
    QHBoxLayout *horizonBox = new QHBoxLayout();
    QLabel *hLabel = new QLabel("🔭 Horizon de Prédiction :");
    hLabel->setStyleSheet("font-weight: bold; color: #475569;");
    QSpinBox *hSpin = new QSpinBox();
    hSpin->setRange(1, 30);
    hSpin->setValue(7);
    hSpin->setSuffix(" jours");
    hSpin->setMinimumWidth(120);
    hSpin->setFixedHeight(35);
    hSpin->setStyleSheet("QSpinBox { padding: 5px; border-radius: 8px; border: 2px solid #3b82f6; font-size: 14px; font-weight: bold; color: #1e293b; background: white; } "
                         "QSpinBox::up-button, QSpinBox::down-button { width: 25px; border-radius: 4px; }");
    
    horizonBox->addWidget(hLabel);
    horizonBox->addWidget(hSpin);
    horizonBox->addStretch();
    
    simuControls->addLayout(horizonBox);
    simuControls->addWidget(m_predictionValue);
    simuControls->addWidget(m_predictionVerdict);
    simuControls->addSpacing(10);
    simuControls->addWidget(m_daySlider);
    simuControls->addLayout(ticks);
    
    connect(hSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this, ticks](int val){
        m_daySlider->setRange(0, val);
        // Clear and Re-label ticks
        QLayoutItem *child;
        while ((child = ticks->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        for (int i = 0; i <= val; ++i) {
            if (val > 14 && i % 2 != 0 && i != val) continue; // Sparse labels if horizon is long
            QLabel *lbl = new QLabel(QString::number(i) + "j");
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setStyleSheet("color: #94a3b8; font-weight: bold; font-size: 10px;");
            ticks->addWidget(lbl);
            if (i < val) ticks->addStretch();
        }
        // Update Chart X Axis
        if (m_freshnessChart) {
            auto axes = m_freshnessChart->axes(Qt::Horizontal);
            if (!axes.isEmpty()) {
                QValueAxis *ax = qobject_cast<QValueAxis*>(axes.first());
                if (ax) {
                    ax->setRange(0, val);
                    ax->setTickCount(val + 1);
                }
            }
        }
    });
    hSpin->setValue(7); // This will trigger the lambda connection below
    
    simuLogic->addSpacing(30);
    simuLogic->addLayout(simuControls);
    sl->addLayout(simuLogic);
    layout->addWidget(simuBox);

    // --- Charts ---
    QHBoxLayout *chartsRow = new QHBoxLayout();
    m_chartsLayout = new QVBoxLayout();
    m_allocLayout = new QVBoxLayout();
    chartsRow->addLayout(m_chartsLayout, 2);
    chartsRow->addLayout(m_allocLayout, 1);
    layout->addLayout(chartsRow);

    // --- Advice Section ---
    QLabel *adviceTitle = new QLabel("🧠 RECOMMANDATIONS STRATÉGIQUES DE L'IA");
    adviceTitle->setStyleSheet("font-weight: 900; color: #0f172a; font-size: 16px; margin-top: 10px;");
    layout->addWidget(adviceTitle);

    m_adviceLayout = new QVBoxLayout();
    m_adviceLayout->setSpacing(15);
    layout->addLayout(m_adviceLayout);

    scroll->setWidget(content);
    mainLayout->addWidget(scroll);

    // --- Footer ---
    QFrame *footer = new QFrame();
    footer->setFixedHeight(80);
    footer->setStyleSheet("background: white; border-top: 2px solid #e2e8f0;");
    QHBoxLayout *fl = new QHBoxLayout(footer);
    QPushButton *btn = new QPushButton("Appliquer ce Scénario");
    btn->setStyleSheet("background: #0f172a; color: white; font-weight: 800; padding: 15px 40px; border-radius: 12px; font-size: 14px;");
    connect(btn, &QPushButton::clicked, this, &QDialog::accept);
    fl->addStretch();
    fl->addWidget(btn);
    fl->setContentsMargins(40, 0, 40, 0);
    mainLayout->addWidget(footer);
}

QWidget* ScenarioDetailsDialog::createAdviceCard(const QString &title, const QString &content, const QString &icon, const QString &color)
{
    QFrame *card = new QFrame();
    card->setStyleSheet(QString("background: white; border-radius: 15px; border: 1px solid #e2e8f0; border-left: 6px solid %1;").arg(color));
    QVBoxLayout *l = new QVBoxLayout(card);
    l->setContentsMargins(20, 20, 20, 20);
    
    QLabel *tl = new QLabel(icon + " " + title);
    tl->setStyleSheet(QString("font-weight: 900; color: %1; font-size: 14px; text-transform: uppercase;").arg(color));
    l->addWidget(tl);
    
    QLabel *cl = new QLabel(content);
    cl->setWordWrap(true);
    cl->setStyleSheet("color: #334155; font-size: 13px; line-height: 1.6;");
    l->addWidget(cl);
    
    return card;
}

QWidget* ScenarioDetailsDialog::createMetricCard(const QString &title, const QString &value, const QString &prefix, const QString &color)
{
    QFrame *card = new QFrame();
    card->setFixedHeight(100);
    card->setStyleSheet(QString("background: white; border-radius: 16px; border: 1px solid #e2e8f0; border-left: 5px solid %1;").arg(color));
    
    QVBoxLayout *l = new QVBoxLayout(card);
    l->setContentsMargins(15, 10, 15, 10);
    
    QLabel *tl = new QLabel(title);
    tl->setStyleSheet("color: #64748b; font-size: 11px; font-weight: 800; text-transform: uppercase;");
    l->addWidget(tl);
    
    QLabel *vl = new QLabel(prefix + value);
    vl->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: 900;").arg("#1e293b"));
    l->addWidget(vl);
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(15);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 15));
    card->setGraphicsEffect(shadow);
    
    return card;
}

void ScenarioDetailsDialog::populateData(const QJsonObject &solution, int index)
{
    Q_UNUSED(index);
    
    // Recalculate total weight to fix 0kg bug
    double totalKg = 0;
    QJsonArray allocs = solution["allocations"].toArray();
    for (const auto &v : allocs) totalKg += v.toObject()["quantity"].toDouble();
    
    double score = solution["score"].toDouble();
    QJsonObject preds = solution["predictions"].toObject();
    double risk = preds["spoilage_risk"].toDouble() * 100;
    m_estimatedShelfLife = preds["estimated_shelf_life_days"].toDouble();
    m_freshnessCurve = preds["freshness_curve"].toArray();

    QHBoxLayout *row = findChild<QHBoxLayout*>("metricsRow");
    row->addWidget(createMetricCard("Indice de Qualité Biologique", QString::number(score, 'f', 0) + "/100", "⭐ ", "#8b5cf6"));
    row->addWidget(createMetricCard("Masse Entreposée", QString::number(totalKg, 'f', 0) + " kg", "📦 ", "#3b82f6"));
    row->addWidget(createMetricCard("Sécurité Sanitaire", QString::number(100 - risk, 'f', 1) + "%", "🛡️ ", risk > 10 ? "#ef4444" : "#10b981"));
    row->addWidget(createMetricCard("Vie Utile Résiduelle", QString::number(m_estimatedShelfLife, 'f', 1) + " j", "🗓️ ", "#f59e0b"));

    // Parse Advice from explanation
    QString explanation = solution["explanation"].toString();
    QStringList sections = explanation.split("\n\n");
    for (const QString &section : sections) {
        if (section.startsWith("📍")) m_adviceLayout->addWidget(createAdviceCard("Résumé Logistique", section.mid(section.indexOf(":")+1).trimmed(), "📍", "#3b82f6"));
        else if (section.startsWith("💡")) m_adviceLayout->addWidget(createAdviceCard("Justification Stratégique", section.mid(section.indexOf(":")+1).trimmed(), "🧠", "#8b5cf6"));
        else if (section.startsWith("⚠️")) m_adviceLayout->addWidget(createAdviceCard("Points de Vigilance", section.mid(section.indexOf(":")+1).trimmed(), "🚨", "#ef4444"));
        else if (section.startsWith("🚀")) m_adviceLayout->addWidget(createAdviceCard("Conseils d'Expert", section.mid(section.indexOf(":")+1).trimmed(), "⚡", "#f59e0b"));
    }

    createFreshnessChart(m_freshnessCurve, m_chartsLayout);
    createAllocationChart(solution["allocations"].toArray(), m_allocLayout);
    updatePrediction(0);
}

void ScenarioDetailsDialog::createFreshnessChart(const QJsonArray &curve, QVBoxLayout *layout)
{
    QLineSeries *series = new QLineSeries();
    series->setName("Qualité Biologique");
    series->setPen(QPen(QColor("#3b82f6"), 3));
    
    // Populate with existing curve data
    int lastDay = 0;
    for (const auto &v : curve) {
        QJsonObject point = v.toObject();
        int d = point["day"].toInt();
        series->append(d, point["freshness"].toDouble());
        lastDay = std::max(lastDay, d);
    }

    // Extend curve if it's too short (up to 30 days) using exponential decay
    // k = -log(0.5) / shelf_life
    double k = -std::log(0.5) / (m_estimatedShelfLife > 0 ? m_estimatedShelfLife : 15.0);
    for (int d = lastDay + 1; d <= 30; ++d) {
        double f = 100.0 * std::exp(-k * d);
        series->append(d, f);
    }

    m_freshnessChart = new QChart();
    m_freshnessChart->addSeries(series);
    m_freshnessChart->setTitle("ÉVOLUTION DE LA QUALITÉ DU STOCK");
    m_freshnessChart->setTitleFont(QFont("Segoe UI", 12, QFont::Bold));
    m_freshnessChart->setAnimationOptions(QChart::SeriesAnimations);
    m_freshnessChart->setMargins(QMargins(15, 15, 15, 15));
    m_freshnessChart->legend()->hide();

    QValueAxis *axisX = new QValueAxis();
    axisX->setTitleText("Projection Temporelle (Jours)");
    axisX->setRange(0, 7);
    axisX->setTickCount(8);
    axisX->setLabelFormat("%d j");
    m_freshnessChart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Indice de Fraîcheur (%)");
    axisY->setRange(0, 100);
    axisY->setLabelFormat("%d%");
    m_freshnessChart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // Marker Line (Vertical)
    m_markerLine = new QLineSeries();
    m_markerLine->setPen(QPen(QColor("#ef4444"), 2, Qt::DashLine));
    m_freshnessChart->addSeries(m_markerLine);
    m_markerLine->attachAxis(axisX);
    m_markerLine->attachAxis(axisY);

    m_freshnessView = new QChartView(m_freshnessChart);
    m_freshnessView->setRenderHint(QPainter::Antialiasing);
    m_freshnessView->setFixedHeight(350);
    m_freshnessView->setStyleSheet("background: white; border-radius: 15px; border: 1px solid #e2e8f0;");
    
    layout->addWidget(m_freshnessView);
}

void ScenarioDetailsDialog::createAllocationChart(const QJsonArray &allocs, QVBoxLayout *layout)
{
    QPieSeries *series = new QPieSeries();
    for (const auto &v : allocs) {
        QJsonObject a = v.toObject();
        QString label = a["cold_room"].toString() + " (" + QString::number(a["quantity"].toDouble(), 'f', 0) + "kg)";
        series->append(label, a["quantity"].toDouble());
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("RÉPARTITION PAR ZONE");
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setMargins(QMargins(10, 10, 10, 10));

    QChartView *view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    view->setFixedHeight(350);
    view->setStyleSheet("background: white; border-radius: 15px; border: 1px solid #e2e8f0;");

    layout->addWidget(view);
}

void ScenarioDetailsDialog::updatePrediction(int day)
{
    double val = -1.0;
    for (const auto &v : m_freshnessCurve) {
        QJsonObject p = v.toObject();
        if (p["day"].toInt() == day) {
            val = p["freshness"].toDouble();
            break;
        }
    }

    // Fallback: Calculate decay if day is out of initial JSON range
    if (val < 0) {
        double k = -std::log(0.5) / (m_estimatedShelfLife > 0 ? m_estimatedShelfLife : 15.0);
        val = 100.0 * std::exp(-k * day);
    }

    m_predictionValue->setText(QString::number(val, 'f', 1) + "%");
    
    // Move Chart Marker
    if (m_markerLine) {
        m_markerLine->clear();
        m_markerLine->append(day, 0);
        m_markerLine->append(day, 100);
    }
    
    // State Changes
    if (val > 85) {
        m_stateIcon->setText("🐟");
        m_stateIcon->setStyleSheet("font-size: 60px; background: #f0fdf4; border-radius: 60px; border: 4px solid #10b981;");
        m_predictionValue->setStyleSheet("font-size: 42px; font-weight: 900; color: #10b981;");
        m_predictionVerdict->setText("État : Fraîcheur Maximale (Qualité Premium)");
    } else if (val > 65) {
        m_stateIcon->setText("❄️");
        m_stateIcon->setStyleSheet("font-size: 60px; background: #eff6ff; border-radius: 60px; border: 4px solid #3b82f6;");
        m_predictionValue->setStyleSheet("font-size: 42px; font-weight: 900; color: #3b82f6;");
        m_predictionVerdict->setText("État : Standard Commercial (Prêt à la vente)");
    } else if (val > 45) {
        m_stateIcon->setText("⚠️");
        m_stateIcon->setStyleSheet("font-size: 60px; background: #fffbeb; border-radius: 60px; border: 4px solid #f59e0b;");
        m_predictionValue->setStyleSheet("font-size: 42px; font-weight: 900; color: #f59e0b;");
        m_predictionVerdict->setText("État : Dégradation Modérée (Process requis)");
    } else {
        m_stateIcon->setText("🛑");
        m_stateIcon->setStyleSheet("font-size: 60px; background: #fef2f2; border-radius: 60px; border: 4px solid #ef4444;");
        m_predictionValue->setStyleSheet("font-size: 42px; font-weight: 900; color: #ef4444;");
        m_predictionVerdict->setText("État : Risque Sanitaire (Vente interdite)");
    }

    // Small animation
    QPropertyAnimation *anim = new QPropertyAnimation(m_stateIcon, "geometry");
    QRect g = m_stateIcon->geometry();
    anim->setDuration(200);
    anim->setStartValue(QRect(g.x(), g.y()-5, g.width(), g.height()));
    anim->setEndValue(g);
    anim->setEasingCurve(QEasingCurve::OutBack);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}
