#include "camionefficiencypage.h"
#include <QScrollArea>
#include <QFrame>
#include <QGridLayout>
#include <QProgressBar>
#include <QComboBox>

CamionEfficiencyPage::CamionEfficiencyPage(QWidget *parent) : QWidget(parent)
{
    setupUI();
    applyStyles();
    loadData();
}

void CamionEfficiencyPage::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("Analyse Écologique & Maintenance");
    title->setStyleSheet("font-family: 'Segoe UI', 'Helvetica Neue', Helvetica, Arial, sans-serif; font-size: 28px; font-weight: 900; color: #1B1B2F; letter-spacing: 1px;");

    refreshBtn = new QPushButton("Rafraîchir");
    refreshBtn->setFixedHeight(36);
    connect(refreshBtn, &QPushButton::clicked, this, &CamionEfficiencyPage::loadData);

    filterCombo = new QComboBox();
    filterCombo->addItems({"Tous les camions", "⚠️ Besoins Maintenance", "✅ État Optimal"});
    filterCombo->setFixedHeight(36);
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CamionEfficiencyPage::loadData);

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(new QLabel("Filtrer :"));
    headerLayout->addWidget(filterCombo);
    headerLayout->addWidget(refreshBtn);
    mainLayout->addLayout(headerLayout);

    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent;");

    cardsContainer = new QWidget();
    cardsContainer->setObjectName("cardsContainer");
    auto *cardsLayout = new QGridLayout(cardsContainer);
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(20);
    cardsLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    scrollArea->setWidget(cardsContainer);
    mainLayout->addWidget(scrollArea);
}

void CamionEfficiencyPage::loadData()
{
    // Clear existing cards
    QLayoutItem *child;
    while ((child = cardsContainer->layout()->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    QSqlQuery q("SELECT CamionId, Immatriculation, Kilometrage, Consommation_totale, Statut, TypeCamion FROM Camions ORDER BY CamionId");
    
    int row = 0, col = 0;
    const int maxCols = 3;
    int filterIdx = filterCombo->currentIndex(); // 0: Tous, 1: Maintenance, 2: Optimal

    while (q.next()) {
        QString immat = q.value(1).toString();
        double kilo = q.value(2).toDouble();
        double conso = q.value(3).toDouble();
        QString statut = q.value(4).toString();
        QString type = q.value(5).toString();
        
        double co2Total = conso * 2.31;
        double co2PerKm = (kilo > 0) ? (co2Total / kilo) : 0.0;
        
        // Maintenance logic: mileage check OR high CO2 emissions check OR breakdowns
        bool highCO2 = (co2PerKm > 1.0); // Threshold for suspicious emissions
        bool needsMaintenance = (fmod(kilo, 10000.0) > 9500.0) || statut.contains("panne", Qt::CaseInsensitive) || highCO2;

        // Apply UI Filter
        if (filterIdx == 1 && !needsMaintenance) continue;
        if (filterIdx == 2 && needsMaintenance) continue;

        auto *card = new QFrame();
        card->setObjectName("truckCard");
        card->setFixedSize(300, 250);
        card->setStyleSheet("QFrame#truckCard { background-color: white; border: 1px solid #E5E7EB; border-radius: 12px; } "
                            "QFrame#truckCard:hover { background-color: #F8FAFF; border: 2px solid #5390D9; }");
        
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setSpacing(10);

        auto *title = new QLabel(immat);
        title->setStyleSheet("font-size: 18px; font-weight: bold; color: #1B1B2F;");
        cardLayout->addWidget(title);

        auto *subTitle = new QLabel(type);
        subTitle->setStyleSheet("font-size: 12px; color: #64748B; margin-top: -5px;");
        cardLayout->addWidget(subTitle);

        auto *infoLayout = new QGridLayout();
        infoLayout->setSpacing(5);

        auto addInfo = [&](int r, const QString &lbl, const QString &val) {
            auto *l1 = new QLabel(lbl);
            l1->setStyleSheet("font-size: 11px; color: #64748B; font-weight: 600;");
            auto *l2 = new QLabel(val);
            l2->setStyleSheet("font-size: 13px; color: #1B1B2F; font-weight: bold;");
            infoLayout->addWidget(l1, r, 0);
            infoLayout->addWidget(l2, r, 1);
        };

        addInfo(0, "Distance:", QString::number(kilo, 'f', 0) + " km");
        addInfo(1, "Émissions:", QString::number(co2Total, 'f', 1) + " kg CO2");
        cardLayout->addLayout(infoLayout);

        // Eco Impact Indicator
        QString impact, impactColor;
        if (kilo <= 0) { impact = "Inconnu"; impactColor = "#94A3B8"; }
        else if (co2PerKm < 0.3) { impact = "Eco-Friendly"; impactColor = "#10B981"; }
        else if (co2PerKm < 0.7) { impact = "Eco-Moyen"; impactColor = "#3B82F6"; }
        else { impact = "Haut Impact"; impactColor = "#EF4444"; }

        auto *impactLbl = new QLabel(impact);
        impactLbl->setAlignment(Qt::AlignCenter);
        impactLbl->setStyleSheet(QString("background: %1; color: white; border-radius: 5px; padding: 4px; font-weight: bold; font-size: 11px;").arg(impactColor));
        cardLayout->addWidget(impactLbl);

        // Maintenance Status
        auto *maintFrame = new QFrame();
        maintFrame->setObjectName("maintFrame");
        auto *maintLayout = new QHBoxLayout(maintFrame);
        maintLayout->setContentsMargins(8, 5, 8, 5);

        auto *maintIcon = new QLabel(needsMaintenance ? "⚠️" : "✅");
        QString maintMsg = "État Optimal";
        if (highCO2) maintMsg = "Émissions Élevées";
        else if (needsMaintenance) maintMsg = "Maintenance Requise";
        
        auto *maintText = new QLabel(maintMsg);
        maintText->setStyleSheet(needsMaintenance ? "color: #EF4444; font-weight: bold;" : "color: #10B981; font-weight: bold;");
        
        maintLayout->addWidget(maintIcon);
        maintLayout->addWidget(maintText);
        maintLayout->addStretch();
        
        cardLayout->addWidget(maintFrame);

        static_cast<QGridLayout*>(cardsContainer->layout())->addWidget(card, row, col);
        
        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }
}

void CamionEfficiencyPage::applyStyles()
{
    setStyleSheet(
        "QWidget#cardsContainer { background: transparent; }"
        "QFrame#maintFrame {"
        "  background-color: #F3F4F6;"
        "  border-radius: 8px;"
        "}"
        "QLabel { background: transparent; color: #1B1B2F; }"
        "QPushButton {"
        "  background: #5390D9; color: white; border: none; border-radius: 8px;"
        "  padding: 8px 16px; font-weight: 600;"
        "}"
        "QPushButton:hover { background: #48BFE3; }"
        "QComboBox { background: white; border: 1px solid #5390D9; border-radius: 5px; padding: 5px; color: #1B1B2F; }"
    );
}
