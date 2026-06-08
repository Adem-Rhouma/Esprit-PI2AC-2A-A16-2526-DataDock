#include "pecherecommendationspage.h"
#include "pechefontutils.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QVBoxLayout>

namespace {

QString getApiKey() {
    QString key = QString::fromLocal8Bit(qgetenv("GEMINI_API_KEY"));
    return key;
}
const char kEndpoint[] =
    "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent";
const char kPrompt[] =
    "Tu es un systeme d'aide a la decision pour la gestion de la peche. "
    "Retourne UNIQUEMENT un JSON valide sans markdown avec les champs "
    "summary, market_focus, freshness_focus, action_focus, confidence_score, "
    "pricing, alerts, priorities, sales, freshness, anomalies, actions. "
    "Chaque tableau contient des objets {tone,title,detail,meta}. "
    "Contexte: La Goulette, 22/03/2026, Sardine 420kg, Dorade 180kg, Thon 95kg, Merlan 130kg, "
    "marche sardine tendu, dorade premium stable, thon fort en restauration, merlan lent. "
    "Donne des recommandations concretes par lot, espece et etat du marche.";

QString sv(const QJsonObject &o, const QString &k) { return o.value(k).toVariant().toString().trimmed(); }

QList<PecheRecommendationItem> parseItems(const QJsonArray &array)
{
    QList<PecheRecommendationItem> out;
    for (const QJsonValue &v : array) {
        const QJsonObject o = v.toObject();
        const QString title = sv(o, QStringLiteral("title"));
        if (title.isEmpty()) continue;
        out.append({sv(o, QStringLiteral("tone")), title, sv(o, QStringLiteral("detail")), sv(o, QStringLiteral("meta"))});
    }
    return out;
}

const QString kStandardCardStyle = QStringLiteral(
    "#%1 {"
    "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #8BCBFC, stop:0.33 #7CD7F7, stop:0.66 #90E6FC, stop:1 #A8EEFF);"
    "border: 1px solid rgba(255, 255, 255, 0.60); border-radius: 22px;"
    "}");

const QString kHeroCardStyle = QStringLiteral(
    "#%1 {"
    "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #8BCBFC, stop:0.33 #7CD7F7, stop:0.66 #90E6FC, stop:1 #A8EEFF);"
    "border: 1px solid rgba(255, 255, 255, 0.80); border-radius: 26px;"
    "}");

const QString kErrorCardStyle = QStringLiteral(
    "#%1 {"
    "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #fee2e2, stop:1 #fecaca);"
    "border: 1px solid rgba(239,68,68,0.40); border-radius: 18px;"
    "}");

QFrame *card(const QString &style, const QString &name = QString(), QWidget *parent = nullptr)
{
    auto *frame = new QFrame(parent);
    if (!name.isEmpty()) frame->setObjectName(name);
    frame->setFrameShape(QFrame::NoFrame);
    frame->setAttribute(Qt::WA_StyledBackground, true);
    if (!style.isEmpty() && !name.isEmpty()) frame->setStyleSheet(style.arg(name));
    return frame;
}

QLabel *label(const QString &text, const QString &style, QWidget *parent = nullptr)
{
    auto *w = new QLabel(text, parent);
    w->setStyleSheet(style);
    return w;
}

void applyDropShadow(QWidget *widget, int blurRadius = 16, int offsetY = 2, const QColor &color = QColor(0, 0, 0, 160))
{
    if (!widget) return;
    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(blurRadius);
    shadow->setOffset(0, offsetY);
    shadow->setColor(color);
    // widget->setGraphicsEffect(shadow);
}

} // namespace

PecheRecommendationsPage::PecheRecommendationsPage(QWidget *parent)
    : QWidget(parent), m_networkManager(new QNetworkAccessManager(this))
{
    PecheFontUtils::applyModuleFont(this);
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "QScrollArea { background:transparent; border:none; }"
        "QScrollArea > QWidget > QWidget { background:transparent; }"
        "QPushButton { min-height: 42px; border: none; border-radius: 14px; padding: 0 18px; font-size: 13px; font-weight: 800; }"
        "QPushButton#primaryAction { color: #ffffff; background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #0284c7, stop:1 #0ea5e9); }"
        "QPushButton#ghostAction { color: #1e3a8a; background: rgba(255,255,255,0.60); border: 1px solid rgba(255,255,255,0.80); }"));
    buildUi();
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &PecheRecommendationsPage::onReplyFinished);
}

void PecheRecommendationsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!event->spontaneous() && !m_hasLoadedOnce && !m_isLoading) requestRecommendations(false);
}

void PecheRecommendationsPage::onRefreshClicked() { requestRecommendations(true); }

void PecheRecommendationsPage::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    root->addWidget(scroll);

    auto *container = new QWidget(scroll);
    container->setStyleSheet(QStringLiteral("background:transparent;"));
    scroll->setWidget(container);
    auto *content = new QVBoxLayout(container);
    content->setContentsMargins(18, 18, 18, 24);
    content->setSpacing(18);

    auto *hero = card(kHeroCardStyle, QStringLiteral("heroCard"), container);
    auto *heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(28, 26, 28, 26);
    heroLayout->setSpacing(14);
    auto *top = new QHBoxLayout();
    m_backButton = new QPushButton(QStringLiteral("Retour"), hero);
    m_backButton->setObjectName(QStringLiteral("ghostAction"));
    connect(m_backButton, &QPushButton::clicked, this, &PecheRecommendationsPage::backRequested);
    m_refreshButton = new QPushButton(QStringLiteral("Actualiser avec Gemini"), hero);
    m_refreshButton->setObjectName(QStringLiteral("primaryAction"));
    connect(m_refreshButton, &QPushButton::clicked, this, &PecheRecommendationsPage::onRefreshClicked);
    m_statusBadge = label(QStringLiteral("Pret"),
                          QStringLiteral("font-size:12px;font-weight:800;padding:7px 13px;border-radius:16px;color:#1d4ed8;background:rgba(59,130,246,0.15);border:1px solid rgba(147,197,253,0.24);"),
                          hero);
    top->addWidget(m_backButton);
    top->addStretch();
    top->addWidget(m_statusBadge);
    top->addWidget(m_refreshButton);
    heroLayout->addLayout(top);
    auto *title = label(QStringLiteral("🔮 Recommandations IA"),
                        QStringLiteral("font-size:38px;font-weight:900;color:#1e3a8a;background:transparent;"), hero);
    applyDropShadow(title, 10, 1, QColor(255, 255, 255, 130));
    auto *sub = label(QStringLiteral("Pilotage premium: pricing, alertes, et actions IA dynamiques par lot."), QStringLiteral("font-size:15px;color:#1e40af;font-weight:600;background:transparent;"), hero);
    sub->setWordWrap(true);
    m_summaryLabel = label(QStringLiteral("En attente d une synthese Gemini."), QStringLiteral("font-size:16px;font-weight:600;color:#1e3a8a;background:transparent;"), hero);
    m_summaryLabel->setWordWrap(true);
    m_metaLabel = label(QStringLiteral("Derniere mise a jour non disponible."), QStringLiteral("font-size:13px;color:#0369a1;background:transparent;"), hero);
    heroLayout->addWidget(title);
    heroLayout->addWidget(sub);
    heroLayout->addWidget(m_summaryLabel);
    heroLayout->addWidget(m_metaLabel);
    content->addWidget(hero);

    auto *errorCard = card(kErrorCardStyle, QStringLiteral("errorCard"), container);
    auto *errorLayout = new QVBoxLayout(errorCard);
    errorLayout->setContentsMargins(18, 14, 18, 14);
    m_errorLabel = label(QString(), QStringLiteral("font-size:13px;color:#7f1d1d;background:transparent;"), errorCard);
    m_errorLabel->setWordWrap(true);
    errorLayout->addWidget(m_errorLabel);
    errorCard->hide();
    content->addWidget(errorCard);

    auto *insightHost = new QWidget(container);
    insightHost->setStyleSheet(QStringLiteral("background:transparent;"));
    m_insightLayout = new QGridLayout(insightHost);
    m_insightLayout->setContentsMargins(0, 0, 0, 0);
    m_insightLayout->setHorizontalSpacing(16);
    content->addWidget(insightHost);

    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(16);
    
    // Reduce density from 6 grids down to 3 high-value, emoji-rich areas
    grid->addWidget(createSectionCard(QStringLiteral("💰 PRICING"), QStringLiteral("Strategie Tarifaire"), &m_pricingLayout), 0, 0);
    grid->addWidget(createSectionCard(QStringLiteral("⚡ EXECUTION"), QStringLiteral("Priorites & Actions"), &m_actionsLayout), 0, 1);
    grid->addWidget(createSectionCard(QStringLiteral("🚨 SURVEILLANCE"), QStringLiteral("Alertes & Qualite"), &m_alertsLayout), 0, 2);
    
    for (int i=0; i<3; ++i) grid->setColumnStretch(i, 1); // Equal 3-columns
    
    content->addLayout(grid);

    applyReport(buildFallbackReport(), QStringLiteral("Mode demo pret"), false);
}

void PecheRecommendationsPage::requestRecommendations(bool forceRefresh)
{
    if (m_isLoading) return;
    if (m_hasLoadedOnce && !forceRefresh) return;
    setLoadingState();
    QNetworkRequest req(QUrl(QStringLiteral("%1?key=%2").arg(QString::fromLatin1(kEndpoint), getApiKey())));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *reply = m_networkManager->post(req, buildGeminiPayload());
    reply->setProperty("pecheRecommendations", true);
}

QByteArray PecheRecommendationsPage::buildGeminiPayload() const
{
    QJsonObject part; part.insert(QStringLiteral("text"), QString::fromLatin1(kPrompt));
    QJsonObject content; content.insert(QStringLiteral("role"), QStringLiteral("user")); content.insert(QStringLiteral("parts"), QJsonArray{part});
    QJsonObject cfg; cfg.insert(QStringLiteral("temperature"), 0.4); cfg.insert(QStringLiteral("responseMimeType"), QStringLiteral("application/json"));
    QJsonObject payload; payload.insert(QStringLiteral("contents"), QJsonArray{content}); payload.insert(QStringLiteral("generationConfig"), cfg);
    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

QString PecheRecommendationsPage::extractGeminiText(const QByteArray &bytes, QString *errorMessage) const
{
    QJsonParseError err; const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) { if (errorMessage) *errorMessage = QStringLiteral("Reponse Gemini invalide."); return {}; }
    const QJsonObject root = doc.object(); if (root.contains(QStringLiteral("error"))) { if (errorMessage) *errorMessage = QStringLiteral("Gemini a retourne une erreur."); return {}; }
    const QJsonArray candidates = root.value(QStringLiteral("candidates")).toArray();
    for (const QJsonValue &c : candidates) for (const QJsonValue &p : c.toObject().value(QStringLiteral("content")).toObject().value(QStringLiteral("parts")).toArray()) {
        const QString text = p.toObject().value(QStringLiteral("text")).toString().trimmed(); if (!text.isEmpty()) return text;
    }
    if (errorMessage) *errorMessage = QStringLiteral("Aucun contenu exploitable.");
    return {};
}

bool PecheRecommendationsPage::parseReport(const QString &jsonText, PecheRecommendationReport *report, QString *errorMessage) const
{
    QJsonParseError err; const QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) { if (errorMessage) *errorMessage = QStringLiteral("JSON IA invalide."); return false; }
    const QJsonObject o = doc.object();
    report->summary = sv(o, QStringLiteral("summary"));
    report->marketFocus = sv(o, QStringLiteral("market_focus"));
    report->freshnessFocus = sv(o, QStringLiteral("freshness_focus"));
    report->actionFocus = sv(o, QStringLiteral("action_focus"));
    report->confidenceScore = qBound(0, o.value(QStringLiteral("confidence_score")).toInt(), 100);
    report->updatedAt = QDateTime::currentDateTime();
    report->pricing = parseItems(o.value(QStringLiteral("pricing")).toArray());
    report->alerts = parseItems(o.value(QStringLiteral("alerts")).toArray());
    report->priorities = parseItems(o.value(QStringLiteral("priorities")).toArray());
    report->sales = parseItems(o.value(QStringLiteral("sales")).toArray());
    report->freshness = parseItems(o.value(QStringLiteral("freshness")).toArray());
    report->anomalies = parseItems(o.value(QStringLiteral("anomalies")).toArray());
    report->actions = parseItems(o.value(QStringLiteral("actions")).toArray());
    if (report->summary.isEmpty()) { if (errorMessage) *errorMessage = QStringLiteral("Resume IA absent."); return false; }
    return true;
}

PecheRecommendationReport PecheRecommendationsPage::buildFallbackReport(const QString &reason) const
{
    PecheRecommendationReport r;
    r.summary = QStringLiteral("La journee demande de tenir le premium sur Dorade et Thon, d accelerer Merlan et de monetiser la tension Sardine.");
    if (!reason.trimmed().isEmpty()) r.summary += QStringLiteral(" Mode secours: %1").arg(reason.trimmed());
    r.marketFocus = QStringLiteral("Sardine tendue, Dorade stable, Thon porteur.");
    r.freshnessFocus = QStringLiteral("Un lot a verifier avant arbitrage final.");
    r.actionFocus = QStringLiteral("Fixer les prix avant midi puis pousser Thon vers restauration.");
    r.confidenceScore = 82;
    r.updatedAt = QDateTime::currentDateTime();
    r.fromFallback = true;
    r.pricing = {{QStringLiteral("pricing"), QStringLiteral("Sardine a revaloriser"), QStringLiteral("Hausse cible de 0.30 a 0.45 TND/kg sur les meilleurs bacs."), QStringLiteral("Impact marge eleve")},
                 {QStringLiteral("opportunity"), QStringLiteral("Dorade premium maintenue"), QStringLiteral("Defendre un prix premium sans remise de volume."), QStringLiteral("Positionnement stable")},
                 {QStringLiteral("warning"), QStringLiteral("Merlan a adoucir"), QStringLiteral("Adapter legerement le prix pour proteger la rotation."), QStringLiteral("Debit a surveiller")}};
    r.alerts = {{QStringLiteral("critical"), QStringLiteral("Lot a verifier"), QStringLiteral("Un lot risque un surclassement si la fraicheur n est pas controlee."), QStringLiteral("Controle qualite")},
                {QStringLiteral("warning"), QStringLiteral("Sous-pricing Thon"), QStringLiteral("La restauration peut absorber un prix plus ambitieux."), QStringLiteral("Fenetre courte")}};
    r.priorities = {{QStringLiteral("critical"), QStringLiteral("Verifier le lot sensible"), QStringLiteral("Valider la qualite avant toute vente."), QStringLiteral("Avant 11h30")},
                    {QStringLiteral("warning"), QStringLiteral("Finaliser la grille tarifaire"), QStringLiteral("Eviter les arbitrages tardifs."), QStringLiteral("Avant midi")},
                    {QStringLiteral("good"), QStringLiteral("Pousser Thon vers restauration"), QStringLiteral("Prioriser les acheteurs a forte valeur."), QStringLiteral("Canal prioritaire")}};
    r.sales = {{QStringLiteral("opportunity"), QStringLiteral("Pack Merlan + Sardine"), QStringLiteral("Composer une offre de volume pour accelerer le Merlan."), QStringLiteral("Vente locale")},
               {QStringLiteral("pricing"), QStringLiteral("Dorade a narration premium"), QStringLiteral("Mettre en avant regularite et presentation."), QStringLiteral("Marge defendable")}};
    r.freshness = {{QStringLiteral("good"), QStringLiteral("Deux lots premium confirmes"), QStringLiteral("Peuvent soutenir un discours premium immediat."), QStringLiteral("Valorisation rapide")},
                   {QStringLiteral("critical"), QStringLiteral("Tracer les ecarts de qualite"), QStringLiteral("Isoler les lots heterogenes pour eviter les promesses incoherentes."), QStringLiteral("Risque image")}};
    r.anomalies = {{QStringLiteral("warning"), QStringLiteral("Ecart prix / qualite"), QStringLiteral("Des lots differents portent un niveau de prix trop proche."), QStringLiteral("Recalibrage utile")},
                   {QStringLiteral("neutral"), QStringLiteral("Charge vendeurs non uniforme"), QStringLiteral("Les actions critiques sont concentrees sur peu de references."), QStringLiteral("Pilotage possible")}};
    r.actions = {{QStringLiteral("pricing"), QStringLiteral("Lot Sardine 420kg"), QStringLiteral("Segmenter les meilleurs bacs pour vendre plus haut puis ouvrir une offre volume."), QStringLiteral("Espece: Sardine")},
                 {QStringLiteral("opportunity"), QStringLiteral("Lot Thon 95kg"), QStringLiteral("Proposer d abord aux acheteurs restauration."), QStringLiteral("Marche: porteur")},
                 {QStringLiteral("warning"), QStringLiteral("Lot Merlan 130kg"), QStringLiteral("Activer une vente d acceleration si la rotation reste molle."), QStringLiteral("Marche: lent")},
                 {QStringLiteral("opportunity"), QStringLiteral("Lot Dorade 180kg"), QStringLiteral("Reserver les plus belles pieces aux canaux premium."), QStringLiteral("Espece: Dorade")}};
    return r;
}

void PecheRecommendationsPage::applyReport(const PecheRecommendationReport &r, const QString &statusText, bool isErrorState)
{
    m_hasLoadedOnce = true; m_isLoading = false;
    m_backButton->setEnabled(true); m_refreshButton->setEnabled(true); m_refreshButton->setText(QStringLiteral("Actualiser avec Gemini"));
    m_statusBadge->setText(statusText);
    m_statusBadge->setStyleSheet(r.fromFallback
        ? QStringLiteral("font-size:12px;font-weight:800;padding:7px 13px;border-radius:16px;color:#b45309;background:rgba(245,158,11,0.14);border:1px solid rgba(253,230,138,0.22);")
        : QStringLiteral("font-size:12px;font-weight:800;padding:7px 13px;border-radius:16px;color:#047857;background:rgba(34,197,94,0.14);border:1px solid rgba(134,239,172,0.22);"));
    m_summaryLabel->setText(r.summary);
    m_metaLabel->setText(QStringLiteral("Confiance %1/100 | Marche: %2 | Fraicheur: %3 | Mise a jour: %4")
                             .arg(r.confidenceScore).arg(r.marketFocus).arg(r.freshnessFocus)
                             .arg(r.updatedAt.toString(QStringLiteral("dd/MM/yyyy HH:mm"))));
    if (QWidget *errorCard = m_errorLabel->parentWidget()) errorCard->setVisible(isErrorState);
    m_errorLabel->setText(isErrorState ? QStringLiteral("Gemini indisponible. Scenario de secours affiche.") : QString());
    clearLayout(m_insightLayout);
    m_insightLayout->addWidget(createInsightCard(QStringLiteral("🎯 ") + QString::number(r.confidenceScore) + "%", QStringLiteral("CONFIANCE IA"), QStringLiteral("Fiabilite actuelle du modele.")), 0, 0);
    m_insightLayout->addWidget(createInsightCard(QStringLiteral("⚠️ ") + QString::number(r.alerts.size()), QStringLiteral("ALERTES"), QStringLiteral("Anomalies et urgences prioritaires.")), 0, 1);
    m_insightLayout->addWidget(createInsightCard(QStringLiteral("⚡ ") + QString::number(r.actions.size()), QStringLiteral("ACTIONS IMMEDIATES"), r.actionFocus), 0, 2);
    m_insightLayout->addWidget(createInsightCard(QStringLiteral("📈 ") + QString::number(r.sales.size()), QStringLiteral("OPPORTUNITES"), QStringLiteral("Repositionnements commerciaux proposes.")), 0, 3);
    
    // Combine arrays for the simplified layout
    QList<PecheRecommendationItem> combinedActions = r.priorities + r.actions;
    QList<PecheRecommendationItem> combinedAlerts = r.alerts + r.freshness + r.anomalies;

    rebuildSection(m_pricingLayout, r.pricing, QStringLiteral("Aucune recommandation tarifaire active."));
    rebuildSection(m_actionsLayout, combinedActions, QStringLiteral("Aucune action requise."));
    rebuildSection(m_alertsLayout, combinedAlerts, QStringLiteral("Aucune anomalie ou alerte detectee."));
}

void PecheRecommendationsPage::setLoadingState()
{
    m_isLoading = true;
    m_backButton->setEnabled(false);
    m_refreshButton->setEnabled(false);
    m_refreshButton->setText(QStringLiteral("Analyse en cours..."));
    m_statusBadge->setText(QStringLiteral("Gemini en cours"));
    m_summaryLabel->setText(QStringLiteral("Generation du brief decisionnel en cours..."));
    if (QWidget *errorCard = m_errorLabel->parentWidget()) errorCard->hide();
}

void PecheRecommendationsPage::rebuildSection(QVBoxLayout *layout, const QList<PecheRecommendationItem> &items, const QString &emptyMessage)
{
    if (!layout) return;
    clearLayout(layout);
    if (items.isEmpty()) {
        auto *empty = label(emptyMessage, QStringLiteral("font-size:13px;color:#2563eb;background:transparent;"));
        empty->setWordWrap(true);
        layout->addWidget(empty);
        layout->addStretch();
        return;
    }
    for (const PecheRecommendationItem &item : items) layout->addWidget(createRecommendationCard(item));
    layout->addStretch();
}

QWidget *PecheRecommendationsPage::createSectionCard(const QString &eyebrow, const QString &title, QVBoxLayout **itemsLayout)
{
    auto *w = card(kStandardCardStyle, QStringLiteral("sectionCard"), this);
    auto *layout = new QVBoxLayout(w);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);
    auto *e = label(eyebrow, QStringLiteral("font-size:11px;font-weight:800;letter-spacing:0.08em;color:#0284c7;background:transparent;"), w);
    auto *t = label(title, QStringLiteral("font-size:19px;font-weight:900;color:#1e3a8a;background:transparent;"), w);
    t->setWordWrap(true);
    layout->addWidget(e);
    layout->addWidget(t);
    auto *host = new QWidget(w);
    host->setStyleSheet(QStringLiteral("background:transparent;"));
    auto *items = new QVBoxLayout(host);
    items->setContentsMargins(0, 0, 0, 0);
    items->setSpacing(10);
    layout->addWidget(host);
    if (itemsLayout) *itemsLayout = items;
    return w;
}

QWidget *PecheRecommendationsPage::createInsightCard(const QString &value, const QString &labelText, const QString &detail)
{
    auto *w = card(kStandardCardStyle, QStringLiteral("insightCard"), this);
    auto *layout = new QVBoxLayout(w);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(6);
    auto *v = label(value, QStringLiteral("font-size:30px;font-weight:900;color:#1e3a8a;background:transparent;"), w);
    auto *l = label(labelText, QStringLiteral("font-size:12px;font-weight:800;letter-spacing:0.08em;color:#0369a1;background:transparent;"), w);
    auto *d = label(detail, QStringLiteral("font-size:13px;color:#2563eb;background:transparent;"), w);
    d->setWordWrap(true);
    layout->addWidget(v); layout->addWidget(l); layout->addWidget(d); layout->addStretch();
    return w;
}

QWidget *PecheRecommendationsPage::createRecommendationCard(const PecheRecommendationItem &item)
{
    auto *w = card(kStandardCardStyle, QStringLiteral("itemCard"), this);
    auto *layout = new QVBoxLayout(w);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);
    auto *top = new QHBoxLayout();
    auto *dot = new QLabel(w); dot->setFixedSize(10, 10); dot->setStyleSheet(QStringLiteral("background:%1;border-radius:5px;").arg(toneColor(item.tone)));
    auto *t = label(item.title, QStringLiteral("font-size:15px;font-weight:800;color:#1e3a8a;background:transparent;"), w); t->setWordWrap(true);
    auto *badge = label(normalizedTone(item.tone).toUpper(), toneBadgeStyle(item.tone), w); badge->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    top->addWidget(dot, 0, Qt::AlignTop); top->addWidget(t, 1); top->addWidget(badge, 0, Qt::AlignTop);
    auto *d = label(item.detail, QStringLiteral("font-size:13px;color:#2563eb;background:transparent;"), w); d->setWordWrap(true);
    layout->addLayout(top); layout->addWidget(d);
    if (!item.meta.isEmpty()) { auto *m = label(item.meta, QStringLiteral("font-size:12px;font-weight:700;color:#0369a1;background:transparent;"), w); m->setWordWrap(true); layout->addWidget(m); }
    return w;
}

void PecheRecommendationsPage::clearLayout(QLayout *layout)
{
    if (!layout) return;
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QLayout *child = item->layout()) { clearLayout(child); delete child; }
        if (QWidget *widget = item->widget()) widget->deleteLater();
        delete item;
    }
}

QString PecheRecommendationsPage::summarizeBytes(const QByteArray &bytes, int maxLen) const
{
    QString text = QString::fromUtf8(bytes).trimmed();
    if (text.isEmpty()) text = QString::fromLatin1(bytes).trimmed();
    if (text.size() > maxLen) text = text.left(maxLen) + QStringLiteral("...");
    return text;
}

QString PecheRecommendationsPage::toneColor(const QString &tone)
{
    const QString n = normalizedTone(tone);
    if (n == QStringLiteral("critical")) return QStringLiteral("#f87171");
    if (n == QStringLiteral("warning")) return QStringLiteral("#fbbf24");
    if (n == QStringLiteral("pricing")) return QStringLiteral("#60a5fa");
    if (n == QStringLiteral("opportunity") || n == QStringLiteral("good")) return QStringLiteral("#34d399");
    return QStringLiteral("#2563eb");
}

QString PecheRecommendationsPage::toneBadgeStyle(const QString &tone)
{
    const QString n = normalizedTone(tone);
    if (n == QStringLiteral("critical")) return QStringLiteral("font-size:10px;font-weight:800;padding:4px 10px;border-radius:13px;color:#b91c1c;background:rgba(220,38,38,0.16);border:1px solid rgba(248,113,113,0.26);");
    if (n == QStringLiteral("warning")) return QStringLiteral("font-size:10px;font-weight:800;padding:4px 10px;border-radius:13px;color:#b45309;background:rgba(245,158,11,0.16);border:1px solid rgba(251,191,36,0.26);");
    if (n == QStringLiteral("pricing")) return QStringLiteral("font-size:10px;font-weight:800;padding:4px 10px;border-radius:13px;color:#1d4ed8;background:rgba(37,99,235,0.16);border:1px solid rgba(96,165,250,0.26);");
    if (n == QStringLiteral("opportunity") || n == QStringLiteral("good")) return QStringLiteral("font-size:10px;font-weight:800;padding:4px 10px;border-radius:13px;color:#047857;background:rgba(22,163,74,0.16);border:1px solid rgba(74,222,128,0.24);");
    return QStringLiteral("font-size:10px;font-weight:800;padding:4px 10px;border-radius:13px;color:#2563eb;background:rgba(100,116,139,0.16);border:1px solid rgba(148,163,184,0.20);");
}

QString PecheRecommendationsPage::normalizedTone(const QString &tone)
{
    const QString n = tone.trimmed().toLower();
    if (n == QStringLiteral("critique")) return QStringLiteral("critical");
    if (n == QStringLiteral("attention")) return QStringLiteral("warning");
    if (n == QStringLiteral("ok")) return QStringLiteral("good");
    return n;
}

void PecheRecommendationsPage::onReplyFinished(QNetworkReply *reply)
{
    if (!reply->property("pecheRecommendations").toBool()) { reply->deleteLater(); return; }
    const QByteArray bytes = reply->readAll();
    QString errorMessage;
    if (reply->error() != QNetworkReply::NoError) {
        errorMessage = QStringLiteral("Echec Gemini: %1").arg(reply->errorString());
        const QString body = summarizeBytes(bytes); if (!body.isEmpty()) errorMessage += QStringLiteral("\n%1").arg(body);
        applyReport(buildFallbackReport(errorMessage), QStringLiteral("Fallback actif"), true);
        reply->deleteLater(); return;
    }
    const QString text = extractGeminiText(bytes, &errorMessage);
    PecheRecommendationReport report;
    if (text.isEmpty() || !parseReport(text, &report, &errorMessage)) {
        applyReport(buildFallbackReport(errorMessage), QStringLiteral("Fallback actif"), true);
        reply->deleteLater(); return;
    }
    report.fromFallback = false;
    applyReport(report, QStringLiteral("Gemini a jour"), false);
    reply->deleteLater();
}
