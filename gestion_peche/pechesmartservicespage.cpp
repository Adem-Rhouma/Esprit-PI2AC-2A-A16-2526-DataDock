#include "pechesmartservicespage.h"
#include "pechefontutils.h"

#include <QFile>
#include <QTextStream>

#include <QEvent>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {

QFrame *makeMetricCard(const QString &value,
                       const QString &label,
                       const QString &detail,
                       const QString &icon,
                       const QString &colorTheme,
                       const QString &idSuffix,
                       QWidget *parent = nullptr)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("metricCard_%1").arg(idSuffix));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(8);

    auto *topRow = new QHBoxLayout();
    topRow->setSpacing(8);
    auto *iconLabel = new QLabel(icon, card);
    iconLabel->setStyleSheet(QStringLiteral("font-size: 24px; background: transparent;"));
    
    auto *valueLabel = new QLabel(value, card);
    valueLabel->setProperty("role", QStringLiteral("metricNumber_%1").arg(colorTheme));
    
    topRow->addWidget(iconLabel);
    topRow->addWidget(valueLabel);
    topRow->addStretch();

    auto *labelText = new QLabel(label, card);
    labelText->setProperty("role", QStringLiteral("pill_%1").arg(colorTheme));
    
    auto *detailText = new QLabel(detail, card);
    detailText->setProperty("role", "cardDesc");
    detailText->setWordWrap(true);

    layout->addLayout(topRow);
    layout->addWidget(labelText);
    layout->addWidget(detailText);
    layout->addStretch();

    // Add slight drop shadow
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 4);
    // card->setGraphicsEffect(shadow);

    return card;
}

QFrame *makeServiceCard(const QString &tag,
                        const QString &title,
                        const QString &description,
                        const QStringList &bullets,
                        const QString &buttonText,
                        const QString &icon,
                        const QString &colorTheme,
                        const QString &idSuffix,
                        QWidget *parent,
                        QFrame **cardOut)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("serviceCard_%1").arg(idSuffix));
    card->setCursor(Qt::PointingHandCursor);
    
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(28, 28, 28, 28);
    layout->setSpacing(16);

    auto *tagLabel = new QLabel(tag, card);
    tagLabel->setProperty("role", QStringLiteral("pill_%1").arg(colorTheme));
    tagLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setProperty("role", "cardTitle");
    titleLabel->setWordWrap(true);

    auto *descLabel = new QLabel(description, card);
    descLabel->setProperty("role", "cardDesc");
    descLabel->setWordWrap(true);

    layout->addWidget(tagLabel);
    layout->addWidget(titleLabel);
    layout->addWidget(descLabel);
    layout->addSpacing(8);

    for (const QString &bullet : bullets) {
        auto *bulletRow = new QHBoxLayout();
        auto *dot = new QLabel("•", card);
        dot->setStyleSheet(QStringLiteral("color: %1; font-weight: bold; font-size: 16px;").arg(
            colorTheme == "cyan" ? "#0284c7" : (colorTheme == "green" ? "#059669" : "#ea580c")));
        auto *bulletLabel = new QLabel(bullet, card);
        bulletLabel->setProperty("role", "bulletPoint");
        bulletLabel->setWordWrap(true);
        bulletRow->addWidget(dot);
        bulletRow->addWidget(bulletLabel, 1);
        layout->addLayout(bulletRow);
    }

    layout->addStretch();

    // Bottom row with action label and watermark
    auto *bottomRow = new QHBoxLayout();
    
    auto *actionLabel = new QLabel(QStringLiteral("→ %1").arg(buttonText), card);
    actionLabel->setProperty("role", QStringLiteral("actionLabel_%1").arg(colorTheme));
    actionLabel->setObjectName("actionLabel");
    actionLabel->setGraphicsEffect(new QGraphicsOpacityEffect(actionLabel));
    static_cast<QGraphicsOpacityEffect*>(actionLabel->graphicsEffect())->setOpacity(0.0);
    
    auto *watermark = new QLabel(icon, card);
    watermark->setProperty("role", "watermarkEmoji");
    watermark->setAlignment(Qt::AlignBottom | Qt::AlignRight);

    bottomRow->addWidget(actionLabel, 0, Qt::AlignBottom | Qt::AlignLeft);
    bottomRow->addStretch();
    bottomRow->addWidget(watermark, 0, Qt::AlignBottom | Qt::AlignRight);
    layout->addLayout(bottomRow);

    // Initial Drop Shadow
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(20);
    QColor shadowColor;
    if (colorTheme == "cyan") shadowColor = QColor("#0284c7");
    else if (colorTheme == "green") shadowColor = QColor("#059669");
    else shadowColor = QColor("#ea580c");
    
    shadowColor.setAlpha(60);
    shadow->setColor(shadowColor);
    shadow->setOffset(0, 0);
    // card->setGraphicsEffect(shadow);

    card->setProperty("shadowColor", shadowColor);
    card->setProperty("colorTheme", colorTheme);

    if (cardOut) {
        *cardOut = card;
    }
    return card;
}

} // namespace

PecheSmartServicesPage::PecheSmartServicesPage(QWidget *parent)
    : QWidget(parent)
{
    PecheFontUtils::applyModuleFont(this);
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("PecheSmartServicesPage");
    
    QFile qssFile("gestion_peche/assets/qss/peche_smartservices.qss");
    if (!qssFile.exists()) {
        qssFile.setFileName("gestion_peche/assets/qss/peche_smartservices.qss");
    }
    if (qssFile.open(QFile::ReadOnly | QFile::Text)) {
        setStyleSheet(QTextStream(&qssFile).readAll());
    }
    
    buildUi();
}

void PecheSmartServicesPage::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    root->addWidget(scroll);

    auto *container = new QWidget(scroll);
    container->setAttribute(Qt::WA_StyledBackground, true);
    container->setMinimumWidth(900);
    scroll->setWidget(container);

    auto *content = new QVBoxLayout(container);
    content->setContentsMargins(40, 40, 40, 60);
    content->setSpacing(32);

    // 1. Hero Card
    auto *hero = new QFrame(container);
    hero->setObjectName("heroCard");
    auto *heroLayout = new QHBoxLayout(hero);
    heroLayout->setContentsMargins(40, 40, 40, 40);
    heroLayout->setSpacing(32);

    auto *heroLeft = new QVBoxLayout();
    heroLeft->setSpacing(16);
    
    auto *heroPill = new QLabel("SMART OPERATIONS COCKPIT", hero);
    heroPill->setProperty("role", "pill_cyan");
    heroLeft->addWidget(heroPill, 0, Qt::AlignLeft);

    auto *heroTitle = new QLabel("Services Intelligents pour la Filière Pêche", hero);
    heroTitle->setProperty("role", "heroTitle");
    heroTitle->setWordWrap(true);
    
    // Add text drop shadow to hero title to stand out from background
    auto *textShadow = new QGraphicsDropShadowEffect(heroTitle);
    textShadow->setBlurRadius(15);
    textShadow->setColor(QColor(0, 0, 0, 180));
    textShadow->setOffset(0, 4);
    // heroTitle->setGraphicsEffect(textShadow);
    
    auto *heroSubtitle = new QLabel("Le hub est recentré sur les leviers utiles au métier: tarification, vision IA et recommandations opérationnelles enrichies.", hero);
    heroSubtitle->setProperty("role", "heroSubtitle");
    heroSubtitle->setWordWrap(true);

    heroLeft->addWidget(heroTitle);
    heroLeft->addWidget(heroSubtitle);
    
    auto *heroStats = new QHBoxLayout();
    heroStats->setSpacing(12);
    
    auto *tag1 = new QLabel("3 PARCOURS ACTIFS", hero);
    tag1->setProperty("role", "pill_cyan");
    auto *tag2 = new QLabel("GEMINI BRANCHÉ", hero);
    tag2->setProperty("role", "pill_green");
    auto *tag3 = new QLabel("LAYOUT RECOMPOSÉ", hero);
    tag3->setProperty("role", "pill_coral");
    
    heroStats->addWidget(tag1);
    heroStats->addWidget(tag2);
    heroStats->addWidget(tag3);
    heroStats->addStretch();
    
    heroLeft->addSpacing(16);
    heroLeft->addLayout(heroStats);
    heroLeft->addStretch();

    auto *heroRight = new QVBoxLayout();
    heroRight->setSpacing(0);
    
    auto *heroReadiness = new QFrame(hero);
    heroReadiness->setObjectName("heroReadinessBox");
    auto *readinessLayout = new QVBoxLayout(heroReadiness);
    readinessLayout->setContentsMargins(20, 20, 20, 20);
    auto *valLabel = new QLabel("94%", heroReadiness);
    valLabel->setProperty("role", "readinessVal");
    valLabel->setAlignment(Qt::AlignCenter);
    auto *txtLabel = new QLabel("READINESS", heroReadiness);
    txtLabel->setProperty("role", "pill_cyan");
    txtLabel->setAlignment(Qt::AlignCenter);
    readinessLayout->addWidget(valLabel);
    readinessLayout->addWidget(txtLabel);
    
    auto *rShadow = new QGraphicsDropShadowEffect(heroReadiness);
    rShadow->setBlurRadius(25);
    rShadow->setColor(QColor(2, 132, 199, 40));
    rShadow->setOffset(0, 4);
    // heroReadiness->setGraphicsEffect(rShadow);

    auto *heroEmoji = new QLabel("⚓", hero);
    heroEmoji->setProperty("role", "heroEmoji");
    heroEmoji->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    heroRight->addWidget(heroReadiness, 0, Qt::AlignTop | Qt::AlignRight);
    heroRight->addStretch();
    heroRight->addWidget(heroEmoji, 0, Qt::AlignBottom | Qt::AlignRight);

    heroLayout->addLayout(heroLeft, 7);
    heroLayout->addLayout(heroRight, 3);
    content->addWidget(hero);

    // 1.5 Horizontal Separator
    auto *separator = new QFrame(container);
    separator->setFixedHeight(1);
    separator->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 transparent, stop:0.5 rgba(255, 255, 255, 0.4), stop:1 transparent);");
    content->addWidget(separator);

    // 2. Metrics Row
    auto *metricsRow = new QHBoxLayout();
    metricsRow->setSpacing(24);
    metricsRow->addWidget(makeMetricCard("12", "ACTIONS IA", "Actions recommandées regroupées par lot, espèce et potentiel de marge.", "⚡", "cyan", "actions", container));
    metricsRow->addWidget(makeMetricCard("3", "SERVICES COEUR", "Tarification, FishVision et recommandations globales en un clic.", "🎯", "green", "services", container));
    metricsRow->addWidget(makeMetricCard("0", "ZONES MORTES", "Une composition plus dense et plus premium sans widget inutile.", "🔕", "coral", "deadzones", container));
    content->addLayout(metricsRow);

    // 3. Services Grid
    auto *servicesLayout = new QHBoxLayout();
    servicesLayout->setSpacing(24);

    QFrame *tarificationCard = nullptr;
    QFrame *fishVisionCard = nullptr;
    QFrame *recoCard = nullptr;

    servicesLayout->addWidget(
        makeServiceCard("PRICING INTELLIGENCE",
                        "Tarification Dynamique",
                        "Calcule un prix recommandé par espèce et soutient la décision commerciale.",
                        {"Simulation locale et historique", "Lecture des signaux marché", "Workflow intégré"},
                        "Ouvrir", "💹", "cyan", "tarification", container, &tarificationCard), 1);

    servicesLayout->addWidget(
        makeServiceCard("VISUAL AI",
                        "FishVision",
                        "Analyse visuelle des lots pour estimer espèce, fraîcheur et positionnement.",
                        {"Lecture image + JSON Gemini", "Historique d'analyses", "Assistants de vente et qualité"},
                        "Lancer", "🐟", "green", "fishvision", container, &fishVisionCard), 1);

    servicesLayout->addWidget(
        makeServiceCard("OPERATIONAL AI",
                        "Recommandations IA",
                        "Page dédiée aux priorités du jour, alertes et arbitrages de vente pour la pêche.",
                        {"Appel Gemini pour brief structuré", "Sections par lot et espèce", "Vue premium orientée action"},
                        "Explorer", "🧠", "coral", "recommandations", container, &recoCard), 1);

    tarificationCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    fishVisionCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    recoCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    tarificationCard->installEventFilter(this);
    fishVisionCard->installEventFilter(this);
    recoCard->installEventFilter(this);

    content->addLayout(servicesLayout);
    content->addStretch();
}

bool PecheSmartServicesPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
        auto *card = qobject_cast<QFrame*>(watched);
        if (card && card->objectName().startsWith("serviceCard_")) {
            
            bool isEnter = (event->type() == QEvent::Enter);
            
            auto *shadow = qobject_cast<QGraphicsDropShadowEffect*>(card->graphicsEffect());
            if (shadow) {
                QColor baseColor = card->property("shadowColor").value<QColor>();
                
                auto *blurAnim = new QPropertyAnimation(shadow, "blurRadius", card);
                blurAnim->setDuration(250);
                blurAnim->setStartValue(shadow->blurRadius());
                blurAnim->setEndValue(isEnter ? 35.0 : 20.0);
                
                auto *colorAnim = new QPropertyAnimation(shadow, "color", card);
                colorAnim->setDuration(250);
                colorAnim->setStartValue(shadow->color());
                baseColor.setAlpha(isEnter ? 120 : 60);
                colorAnim->setEndValue(baseColor);
                
                blurAnim->start(QAbstractAnimation::DeleteWhenStopped);
                colorAnim->start(QAbstractAnimation::DeleteWhenStopped);
            }

            auto *actionLabel = card->findChild<QLabel*>("actionLabel");
            if (actionLabel) {
                auto *opEffect = qobject_cast<QGraphicsOpacityEffect*>(actionLabel->graphicsEffect());
                if (opEffect) {
                    auto *opAnim = new QPropertyAnimation(opEffect, "opacity", card);
                    opAnim->setDuration(200);
                    opAnim->setStartValue(opEffect->opacity());
                    opAnim->setEndValue(isEnter ? 1.0 : 0.0);
                    opAnim->start(QAbstractAnimation::DeleteWhenStopped);
                }
            }

            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        auto *card = qobject_cast<QFrame*>(watched);
        if (card && card->objectName().startsWith("serviceCard_")) {
            QString theme = card->property("colorTheme").toString();
            if (theme == "cyan") emit openDynamicPricingRequested();
            else if (theme == "green") emit fishVisionRequested();
            else if (theme == "coral") emit openRecommendationsRequested();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}
