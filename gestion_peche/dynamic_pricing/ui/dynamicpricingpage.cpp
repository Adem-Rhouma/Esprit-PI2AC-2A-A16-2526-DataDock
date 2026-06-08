#include "dynamicpricingpage.h"
#include "../../pechefontutils.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDateTime>
#include <QEvent>
#include <QEnterEvent>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QShowEvent>
#include <QStyle>
#include <QTableView>
#include <QVBoxLayout>
#include <cmath>
#include <QtMath>

namespace {

class HoverCard : public QFrame
{
public:
    explicit HoverCard(QWidget *parent = nullptr)
        : QFrame(parent)
    {
        setObjectName(QStringLiteral("HoverCard"));
        setFrameShape(QFrame::NoFrame);
        setAttribute(Qt::WA_StyledBackground, true);
        setAttribute(Qt::WA_Hover);
        setMouseTracking(true);
    }

    void setCardStyles(const QString &baseStyle, const QString &hoverStyle)
    {
        m_baseStyle = baseStyle;
        m_hoverStyle = hoverStyle;
        setStyleSheet(m_baseStyle);
    }

protected:
    void enterEvent(QEnterEvent *event) override
    {
        setStyleSheet(m_hoverStyle.isEmpty() ? m_baseStyle : m_hoverStyle);
        if (auto *shadow = qobject_cast<QGraphicsDropShadowEffect *>(graphicsEffect())) {
            shadow->setBlurRadius(28);
            shadow->setOffset(0, 6);
        }
        QFrame::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        setStyleSheet(m_baseStyle);
        if (auto *shadow = qobject_cast<QGraphicsDropShadowEffect *>(graphicsEffect())) {
            shadow->setBlurRadius(20);
            shadow->setOffset(0, 4);
        }
        QFrame::leaveEvent(event);
    }

private:
    QString m_baseStyle;
    QString m_hoverStyle;
};

class ScoreBarWidget : public QProgressBar
{
public:
    explicit ScoreBarWidget(QWidget *parent = nullptr)
        : QProgressBar(parent)
    {
        setRange(0, 100);
        setTextVisible(false);
        setFixedHeight(6);
        setMinimumWidth(110);
        setAttribute(Qt::WA_TranslucentBackground);
        setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF trackRect = rect().adjusted(0.0, 0.0, -1.0, -1.0);
        const QColor trackColor = property("trackColor").value<QColor>().isValid()
            ? property("trackColor").value<QColor>()
            : QColor(255, 255, 255, 25);
        const QColor fillColor = property("fillColor").value<QColor>().isValid()
            ? property("fillColor").value<QColor>()
            : QColor(QStringLiteral("#10b981"));
        const qreal radius = 3.0;

        painter.setPen(Qt::NoPen);
        painter.setBrush(trackColor);
        painter.drawRoundedRect(trackRect, radius, radius);

        const double ratio = maximum() <= minimum()
            ? 0.0
            : double(value() - minimum()) / double(maximum() - minimum());
        if (ratio <= 0.0) {
            return;
        }

        QRectF fillRect = trackRect;
        fillRect.setWidth(std::max<qreal>(radius * 2.0, trackRect.width() * qBound(0.0, ratio, 1.0)));
        painter.setBrush(fillColor);
        painter.drawRoundedRect(fillRect, radius, radius);
    }
};

class ArrowComboBox : public QComboBox
{
public:
    explicit ArrowComboBox(QWidget *parent = nullptr)
        : QComboBox(parent)
        , m_arrow(new QLabel(QStringLiteral("⌄"), this))
    {
        m_arrow->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_arrow->setAlignment(Qt::AlignCenter);
        m_arrow->setStyleSheet(QStringLiteral("color:#38bdf8; background:transparent;"));
        m_arrow->setFont(PecheFontUtils::moduleFont(12.0));
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QComboBox::resizeEvent(event);
        const int arrowWidth = 26;
        const int margin = 8;
        m_arrow->setGeometry(width() - arrowWidth - margin,
                             0,
                             arrowWidth,
                             height());
    }

private:
    QLabel *m_arrow = nullptr;
};

class HoverButton : public QPushButton
{
public:
    explicit HoverButton(const QString &text, QWidget *parent = nullptr)
        : QPushButton(text, parent)
    {
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);
        setMouseTracking(true);
    }

    void setButtonStyles(const QString &baseStyle, const QString &hoverStyle)
    {
        m_baseStyle = baseStyle;
        m_hoverStyle = hoverStyle;
        setStyleSheet(m_baseStyle);
    }

protected:
    void enterEvent(QEnterEvent *event) override
    {
        setStyleSheet(m_hoverStyle.isEmpty() ? m_baseStyle : m_hoverStyle);
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        setStyleSheet(m_baseStyle);
        QPushButton::leaveEvent(event);
    }

private:
    QString m_baseStyle;
    QString m_hoverStyle;
};

void applyShadow(QWidget *widget, int blurRadius, int offsetY, const QColor &color)
{
    if (!widget) {
        return;
    }

    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(blurRadius);
    shadow->setOffset(0, offsetY);
    shadow->setColor(color);
    // widget->setGraphicsEffect(shadow);
}

QString standardGlassCardStyle()
{
    return QStringLiteral(
        "#HoverCard { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #061A3A, stop:0.48 #082B5B, stop:1 #06436D);"
        "border: 1px solid rgba(125, 211, 252, 0.40);"
        "border-radius: 12px; }");
}

QString standardGlassCardHoverStyle()
{
    return QStringLiteral(
        "#HoverCard { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #082653, stop:1 #075985);"
        "border: 1px solid rgba(125, 211, 252, 0.78);"
        "border-radius: 12px; }");
}


QString accentBarStyle(const QString &topColor, const QString &bottomColor)
{
    return QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %1, stop:1 %2);"
        "border-radius: 3px;")
        .arg(topColor, bottomColor);
}

bool containsEmoji(const QString &text)
{
    for (const QChar ch : text) {
        const uint code = ch.unicode();
        if ((code >= 0x2190 && code <= 0x27BF) || (code >= 0x1F300 && code <= 0x1FAFF)) {
            return true;
        }
    }
    return false;
}

void applyEmojiFontIfNeeded(QLabel *label, int pointSize)
{
    if (!label || !containsEmoji(label->text())) {
        return;
    }
    QFont font = PecheFontUtils::moduleFont(pointSize);
    label->setFont(font);
}

struct AccentCardContent
{
    QWidget *body = nullptr;
    QVBoxLayout *layout = nullptr;
};

AccentCardContent createAccentCardContent(QFrame *card,
                                          const QString &accentStyle,
                                          const QMargins &margins,
                                          int spacing = 14)
{
    QHBoxLayout *shell = new QHBoxLayout(card);
    shell->setContentsMargins(margins);
    shell->setSpacing(16);

    auto *accent = new QFrame(card);
    accent->setFixedWidth(5);
    accent->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    accent->setStyleSheet(accentStyle);
    shell->addWidget(accent);

    auto *body = new QWidget(card);
    body->setAttribute(Qt::WA_StyledBackground, true);
    body->setAttribute(Qt::WA_TranslucentBackground);
    body->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(spacing);
    shell->addWidget(body, 1);

    return { body, layout };
}

QLabel *makeInfoChip(const QString &text, const QString &role, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setProperty("dprole", role);
    label->setAlignment(Qt::AlignCenter);
    applyEmojiFontIfNeeded(label, 11);
    return label;
}

QLabel *makeTableHeaderLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setProperty("dprole", "tablehead");
    label->setAlignment(Qt::AlignCenter);
    return label;
}

QLabel *makeTableValueLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setProperty("dprole", "databox");
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    return label;
}

void setScoreBarFill(QProgressBar *bar, const QString &color)
{
    if (!bar) {
        return;
    }
    bar->setProperty("fillColor", QColor(color));
    bar->update();
}

void configureComboPopup(QComboBox *combo)
{
    if (!combo) {
        return;
    }

    auto *view = new QListView(combo);
    view->setAttribute(Qt::WA_StyledBackground, true);
    view->setAttribute(Qt::WA_TranslucentBackground, false);
    view->setAutoFillBackground(true);
    view->setFrameShape(QFrame::NoFrame);
    view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    combo->setView(view);

    if (QWidget *popup = view->window()) {
        popup->setAttribute(Qt::WA_StyledBackground, true);
        popup->setAttribute(Qt::WA_TranslucentBackground, false);
        popup->setAutoFillBackground(true);
        popup->setStyleSheet(QStringLiteral(
            "background:#082B5B;"
            "border:1px solid rgba(125, 211, 252, 0.42);"
            "border-radius:8px;"));
    }
}

double dpFreshnessScore(const double freshnessHours)
{
    const double bounded = qBound(0.0, freshnessHours, 120.0);
    return 100.0 * (1.0 - (bounded / 120.0));
}

double dpAvailabilityScore(const double quantityKg)
{
    if (quantityKg <= 0.0) {
        return 85.0;
    }
    return qBound(10.0, 100.0 - (quantityKg / 8.0), 95.0);
}

QString signedAmount(const double value)
{
    const QString prefix = value >= 0.0 ? QStringLiteral("+") : QString();
    return prefix + QString::number(value, 'f', 3) + QStringLiteral(" TND/kg");
}

QString formatObservationTime(const QString &isoTime)
{
    const QDateTime parsed = QDateTime::fromString(isoTime, Qt::ISODate);
    if (parsed.isValid()) {
        return parsed.toLocalTime().toString(QStringLiteral("dd/MM/yyyy hh:mm"));
    }
    return isoTime.isEmpty() ? QStringLiteral("-") : isoTime;
}

} // namespace

DpPricingResult DpEngine::compute(const DpPricingInput &input) const
{
    DpPricingResult result;
    result.apiSource = input.hasLiveMarineData
        ? QStringLiteral("OPEN_METEO_MARINE")
        : QStringLiteral("LOCAL_ENGINE");
    result.hasLiveWeatherData = input.hasLiveMarineData;
    result.freshnessScore = dpFreshnessScore(input.freshnessHours);
    result.availabilityScore = dpAvailabilityScore(input.totalQuantityKg);
    result.weatherScore = input.hasLiveMarineData ? input.liveWeatherScore : 0.0;
    result.riskScore = input.hasLiveMarineData ? input.liveRiskScore : 0.0;

    if (input.basePrice <= 0.0) {
        result.explanation =
            QStringLiteral("Prix de base indisponible. Verifiez PECHESESPECES.PRIXUNITAIRE.");
        return result;
    }

    result.freshnessAdjustment =
        input.basePrice * 0.18 * ((result.freshnessScore / 100.0) - 0.5);
    result.availabilityAdjustment =
        input.basePrice * 0.14 * ((result.availabilityScore / 100.0) - 0.5);
    result.weatherAdjustment = input.hasLiveMarineData
        ? input.basePrice * 0.10 * ((result.weatherScore / 100.0) - 0.5)
        : 0.0;
    result.riskAdjustment = input.hasLiveMarineData
        ? input.basePrice * 0.12 * (result.riskScore / 100.0)
        : 0.0;

    const double rawRecommendation = input.basePrice
        + result.freshnessAdjustment
        + result.availabilityAdjustment
        + result.weatherAdjustment
        - result.riskAdjustment;
    result.recommendedPrice = qBound(input.basePrice * 0.70,
                                     rawRecommendation,
                                     input.basePrice * 1.50);

    QString marineLine;
    QString riskLine;
    if (input.hasLiveMarineData) {
        marineLine = QStringLiteral("Meteo marine   : %1 (score %2)")
            .arg(signedAmount(result.weatherAdjustment),
                 QString::number(result.weatherScore, 'f', 0) + QStringLiteral("%"));
        riskLine = QStringLiteral("Risque marin   : -%1 (score %2)")
            .arg(QString::number(result.riskAdjustment, 'f', 3) + QStringLiteral(" TND/kg"),
                 QString::number(result.riskScore, 'f', 0) + QStringLiteral("%"));
    } else {
        marineLine = QStringLiteral("Meteo marine   : donnees indisponibles, moteur local conserve");
        riskLine = QStringLiteral("Risque marin   : donnees indisponibles, aucun malus applique");
    }

    result.explanation = QStringLiteral(
        "Base %1\n"
        "Fraicheur      : %2\n"
        "Disponibilite  : %3\n"
        "%4\n"
        "%5\n"
        "Prix recommande: %6")
        .arg(QString::number(input.basePrice, 'f', 3) + QStringLiteral(" TND/kg"),
             signedAmount(result.freshnessAdjustment),
             signedAmount(result.availabilityAdjustment),
             marineLine,
             riskLine,
             QString::number(result.recommendedPrice, 'f', 3) + QStringLiteral(" TND/kg"));

    return result;
}

static const char * const kQss = R"QSS(
#DynamicPricingPage,
#DpContent,
#DpTopBar {
    background: transparent;
}
#DpPageTitle {
    font-size: 28px;
    font-weight: 500;
    color: #F8FAFC;
    background: transparent;
}
#DpPageSubtitle {
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 0.5px;
    color: #D9F2FF;
    background: transparent;
}
QLabel[dprole="section"] {
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 1px;
    color: #BAE6FD;
    background: transparent;
}
QLabel[dprole="cardtitle"] {
    font-size: 12px;
    color: #BAE6FD;
    background: transparent;
}
QLabel[dprole="source"] {
    font-size: 11px;
    color: #7DD3FC;
    background: transparent;
}
QLabel[dprole="value"] {
    font-size: 28px;
    font-weight: 700;
    color: #F8FAFC;
    background: transparent;
}
QLabel[dprole="value-sm"] {
    font-size: 16px;
    font-weight: 500;
    color: #F8FAFC;
    background: transparent;
}
QLabel[dprole="metric-value-amber"] {
    font-size: 26px;
    font-weight: 700;
    color: #FDBA74;
    background: transparent;
}
QLabel[dprole="metric-value-teal"] {
    font-size: 26px;
    font-weight: 700;
    color: #86EFAC;
    background: transparent;
}
QLabel[dprole="metric-value-blue"] {
    font-size: 26px;
    font-weight: 700;
    color: #7DD3FC;
    background: transparent;
}
QLabel[dprole="metric-support"] {
    font-size: 12px;
    color: #BAE6FD;
    background: transparent;
}
QLabel[dprole="metric-caption"] {
    font-size: 12px;
    color: #BAE6FD;
    background: transparent;
}
QLabel[dprole="heroprice"] {
    font-size: 42px;
    font-weight: 700;
    color: #F8FAFC;
    background: transparent;
}
QLabel[dprole="heroband"] {
    font-size: 13px;
    color: #BAE6FD;
    background: transparent;
}
QLabel[dprole="herodelta"] {
    font-size: 13px;
    font-weight: 700;
    color: #FCA5A5;
    background: transparent;
}
QLabel[dprole="herosource"] {
    background: rgba(14, 165, 233, 0.18);
    color: #BAE6FD;
    border: 1px solid rgba(125, 211, 252, 0.42);
    border-radius: 6px;
    padding: 3px 10px;
    font-size: 11px;
}
QLabel[dprole="helper"] {
    font-size: 12px;
    color: #BAE6FD;
    background: transparent;
}
QLabel[dprole="expl"] {
    font-size: 12px;
    color: #BAE6FD;
    background: transparent;
}
QLabel[dprole="formula"] {
    background: rgba(2, 14, 35, 0.45);
    border: 1px solid rgba(125, 211, 252, 0.42);
    border-radius: 8px;
    padding: 12px;
    color: #D9F2FF;
    font-size: 12px;
    font-family: %1;
}
QLabel[dprole="formula-title"] {
    color: #F8FAFC;
    font-size: 14px;
    font-weight: 700;
    background: transparent;
}
QLabel[dprole="formula-line"] {
    color: #D9F2FF;
    font-size: 12px;
    background: transparent;
}
QLabel[dprole="formula-line-positive"] {
    color: #86EFAC;
    font-size: 12px;
    font-weight: 700;
    background: transparent;
}
QLabel[dprole="formula-line-negative"] {
    color: #FCA5A5;
    font-size: 12px;
    font-weight: 700;
    background: transparent;
}
QLabel[dprole="formula-final"] {
    color: #F8FAFC;
    font-size: 14px;
    font-weight: 700;
    background: transparent;
}
QLabel[dprole="badge-live"],
QLabel[dprole="badge-loading"],
QLabel[dprole="badge-error"],
QLabel[dprole="badge-indigo"] {
    font-size: 11px;
    border-radius: 6px;
    padding: 3px 10px;
}
QLabel[dprole="badge-live"] {
    background: rgba(16, 185, 129, 0.16);
    color: #BBF7D0;
    border: 1px solid rgba(74, 222, 128, 0.38);
}
QLabel[dprole="badge-loading"] {
    background: rgba(2, 14, 35, 0.45);
    color: #BAE6FD;
    border: 1px solid rgba(125, 211, 252, 0.42);
}
QLabel[dprole="badge-error"] {
    background: rgba(239, 68, 68, 0.16);
    color: #FECACA;
    border: 1px solid rgba(248, 113, 113, 0.38);
}
QLabel[dprole="badge-indigo"] {
    background: rgba(139, 92, 246, 0.16);
    color: #DDD6FE;
    border: 1px solid rgba(167, 139, 250, 0.38);
}
QLabel[dprole="error"] {
    font-size: 12px;
    color: #FECACA;
    background: rgba(239, 68, 68, 0.16);
    border: 1px solid rgba(248, 113, 113, 0.38);
    border-radius: 10px;
    padding: 8px 14px;
}
QLabel[dprole="summary-ok"] {
    color: #86EFAC;
    font-size: 13px;
    font-weight: 700;
    background: transparent;
}
QLabel[dprole="summary-muted"] {
    color: #BAE6FD;
    font-size: 12px;
    font-style: italic;
    background: transparent;
}
QLabel[dprole="tablehead"] {
    color: #BAE6FD;
    font-size: 11px;
    background: transparent;
    border-bottom: 1px solid rgba(125, 211, 252, 0.40);
    padding: 6px 10px;
}
QLabel[dprole="databox"] {
    color: #F8FAFC;
    font-size: 16px;
    font-weight: 700;
    background: rgba(2, 14, 35, 0.45);
    border: 1px solid rgba(125, 211, 252, 0.42);
    border-radius: 8px;
    padding: 8px 10px;
}
QComboBox#DpCombo {
    background: rgba(2, 14, 35, 0.45);
    color: #F8FAFC;
    border: 1px solid rgba(125, 211, 252, 0.42);
    border-radius: 12px;
    padding: 8px 40px 8px 12px;
    font-size: 14px;
    font-weight: 500;
    min-height: 18px;
}
QComboBox#DpCombo:hover, QComboBox#DpCombo:focus {
    border: 1px solid rgba(125, 211, 252, 0.78);
    background: rgba(2, 14, 35, 0.60);
}
QComboBox#DpCombo::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 34px;
    border: none;
    border-left: 1px solid rgba(125, 211, 252, 0.42);
    border-top-right-radius: 12px;
    border-bottom-right-radius: 12px;
    background: transparent;
}
QComboBox#DpCombo:on {
    border: 1px solid rgba(125, 211, 252, 0.78);
}
QComboBox#DpCombo QAbstractItemView {
    background: #082B5B;
    color: #F8FAFC;
    border: 1px solid rgba(125, 211, 252, 0.42);
    border-radius: 8px;
    selection-background-color: rgba(125, 211, 252, 0.25);
    selection-color: #F8FAFC;
    padding: 4px;
    outline: 0;
}
QComboBox#DpCombo QAbstractItemView::item {
    min-height: 28px;
    padding: 6px 10px;
    background: transparent;
    color: #F8FAFC;
}
QComboBox#DpCombo QAbstractItemView::item:selected {
    background: rgba(125, 211, 252, 0.25);
    color: #F8FAFC;
}
QTableView#DpHistoryTable {
    background: rgba(2, 14, 35, 0.45);
    alternate-background-color: rgba(6, 26, 58, 0.45);
    border: 1px solid rgba(125, 211, 252, 0.42);
    border-radius: 12px;
    color: #F8FAFC;
    gridline-color: transparent;
    selection-background-color: rgba(125, 211, 252, 0.25);
    selection-color: #F8FAFC;
    font-size: 13px;
}
QHeaderView::section {
    background: transparent;
    color: #BAE6FD;
    border: none;
    border-bottom: 1px solid rgba(125, 211, 252, 0.40);
    padding: 10px 8px;
    font-size: 11px;
    font-weight: 500;
}
QTableView#DpHistoryTable::item {
    padding: 10px 8px;
    border-bottom: 1px solid rgba(125, 211, 252, 0.20);
    color: #D9F2FF;
}
QTableView#DpHistoryTable::item:selected {
    background: rgba(125, 211, 252, 0.25);
    color: #F8FAFC;
}
QScrollBar:horizontal {
    background: transparent;
    height: 6px;
    border-radius: 3px;
}
QScrollBar::handle:horizontal {
    background: rgba(0, 212, 255, 0.2);
    border-radius: 3px;
}
QScrollBar:vertical {
    background: transparent;
    width: 6px;
    border-radius: 3px;
}
QScrollBar::handle:vertical {
    background: rgba(0, 212, 255, 0.2);
    border-radius: 3px;
}
QScrollArea#DpScroll,
QScrollArea#DpScroll > QWidget > QWidget {
    background: transparent;
    border: none;
}
)QSS";

DynamicPricingPage::DynamicPricingPage(QWidget *parent)
    : QWidget(parent)
    , m_historyModel(new PriceHistoryTableModel(this))
    , m_marineService(new MarineWeatherService(this))
    , m_gfwService(new GfwIntelligenceService(this))
{
    qRegisterMetaType<MarineCurrentData>("MarineCurrentData");
    qRegisterMetaType<MarineWeatherScores>("MarineWeatherScores");
    qRegisterMetaType<GfwPortIntelligence>("GfwPortIntelligence");
    qRegisterMetaType<GfwIntelligenceScores>("GfwIntelligenceScores");

    setObjectName(QStringLiteral("DynamicPricingPage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_TranslucentBackground);
    PecheFontUtils::applyModuleFont(this);
    setStyleSheet(QStringLiteral("background: transparent;"));

    buildUi();
    applyStyleSheet();
    connectSignals();
    resetEnvironmentCard();
    loadSpecies();
    loadPorts();
}

void DynamicPricingPage::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
}

void DynamicPricingPage::showEvent(QShowEvent *event)
{
    if (QWidget *top = window()) {
        if (QLabel *dashTitle = top->findChild<QLabel *>(QStringLiteral("lblTitreDash"))) {
            dashTitle->setText(QStringLiteral("Gestion de la Pêche - Tarification Dynamique"));
        }
    }

    QWidget::showEvent(event);
}

void DynamicPricingPage::buildUi()
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(buildTopBar());
    root->addWidget(buildScrollArea(), 1);
}

void DynamicPricingPage::applyStyleSheet()
{
    setStyleSheet(QString::fromUtf8(kQss).arg(PecheFontUtils::fontCssStack()));

    if (m_historyTable) {
        m_historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        m_historyTable->horizontalHeader()->setSectionResizeMode(
            PriceHistoryTableModel::ColExplanation, QHeaderView::Stretch);
    }
}

void DynamicPricingPage::connectSignals()
{
    connect(m_backButton, &QPushButton::clicked,
            this, &DynamicPricingPage::backRequested);
    connect(m_speciesCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DynamicPricingPage::onSpeciesChanged);
    connect(m_portCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DynamicPricingPage::onPortChanged);
    connect(m_recalcButton, &QPushButton::clicked,
            this, &DynamicPricingPage::onRecalculateClicked);
    connect(m_saveButton, &QPushButton::clicked,
            this, &DynamicPricingPage::onSaveClicked);

    connect(m_marineService, &MarineWeatherService::marineDataLoading,
            this, &DynamicPricingPage::onMarineDataLoading);
    connect(m_marineService, &MarineWeatherService::marineDataReady,
            this, &DynamicPricingPage::onMarineDataReady);
    connect(m_marineService, &MarineWeatherService::marineDataError,
            this, &DynamicPricingPage::onMarineDataError);

    connect(m_gfwService, &GfwIntelligenceService::intelligenceLoading,
            this, &DynamicPricingPage::onGfwLoading);
    connect(m_gfwService, &GfwIntelligenceService::intelligenceReady,
            this, &DynamicPricingPage::onGfwReady);
    connect(m_gfwService, &GfwIntelligenceService::intelligenceUnavailable,
            this, &DynamicPricingPage::onGfwUnavailable);
    connect(m_gfwService, &GfwIntelligenceService::intelligenceError,
            this, &DynamicPricingPage::onGfwError);
}

QFrame *DynamicPricingPage::buildTopBar()
{
    QFrame *bar = new QFrame(this);
    bar->setObjectName(QStringLiteral("DpTopBar"));
    bar->setAttribute(Qt::WA_StyledBackground, true);
    bar->setAttribute(Qt::WA_TranslucentBackground);
    bar->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(28, 18, 28, 18);
    layout->setSpacing(20);

    auto *backButton = new HoverButton(QStringLiteral("← Retour"), bar);
    backButton->setButtonStyles(
        QStringLiteral(
            "QPushButton {"
            "background: rgba(14, 165, 233, 0.18);"
            "color: #BAE6FD;"
            "border: 1px solid rgba(125, 211, 252, 0.42);"
            "border-radius: 8px;"
            "padding: 6px 14px;"
            "font-size: 12px;"
            "}"),
        QStringLiteral(
            "QPushButton {"
            "background: rgba(14, 165, 233, 0.38);"
            "color: #F8FAFC;"
            "border: 1px solid rgba(125, 211, 252, 0.78);"
            "border-radius: 8px;"
            "padding: 6px 14px;"
            "font-size: 12px;"
            "}"));
    m_backButton = backButton;
    m_backButton->setFixedWidth(110);
    layout->addWidget(m_backButton);

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(3);

    QLabel *title = new QLabel(QStringLiteral("💹 Tarification Dynamique"), bar);
    title->setObjectName(QStringLiteral("DpPageTitle"));
    title->setFont(PecheFontUtils::moduleFont(38.0));
    applyShadow(title, 16, 2, QColor(0, 0, 0, 160));
    titleLayout->addWidget(title);

    QLabel *subtitle = new QLabel(
        QStringLiteral("Pilotage tarifaire intelligent dans le module pêche"), bar);
    subtitle->setObjectName(QStringLiteral("DpPageSubtitle"));
    subtitle->setFont(PecheFontUtils::moduleFont(15.0));
    applyShadow(subtitle, 10, 1, QColor(0, 0, 0, 130));
    titleLayout->addWidget(subtitle);

    layout->addLayout(titleLayout, 1);
    return bar;
}

QScrollArea *DynamicPricingPage::buildScrollArea()
{
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("DpScroll"));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; border: none; }"));
    scroll->setWidget(buildContent());
    if (scroll->viewport()) {
        scroll->viewport()->setStyleSheet(QStringLiteral("background: transparent;"));
    }
    return scroll;
}

QWidget *DynamicPricingPage::buildContent()
{
    QWidget *content = new QWidget();
    content->setObjectName(QStringLiteral("DpContent"));
    content->setAttribute(Qt::WA_StyledBackground, true);
    content->setAttribute(Qt::WA_TranslucentBackground);
    content->setStyleSheet(QStringLiteral("background: transparent;"));
    
    QGridLayout *grid = new QGridLayout(content);
    grid->setContentsMargins(28, 24, 28, 32);
    grid->setSpacing(20);

    // Make the columns 4 equal sizes
    for (int i = 0; i < 4; ++i) {
        grid->setColumnStretch(i, 1);
    }

    // Row 0: Selectors & Hero Card // Stack inputs to equal hero height
    QVBoxLayout *selectorsLayout = new QVBoxLayout();
    selectorsLayout->setSpacing(20);
    selectorsLayout->addWidget(buildSpeciesCard());
    selectorsLayout->addWidget(buildPortCard());
    grid->addLayout(selectorsLayout, 0, 0, 1, 1);
    grid->addWidget(buildRecommendationCard(), 0, 1, 1, 3);

    // Row 1: Core Metrics + Explanation
    grid->addWidget(buildBasePriceCard(), 1, 0, 1, 1);
    grid->addWidget(buildQuantityCard(), 1, 1, 1, 1);
    grid->addWidget(buildFreshnessCard(), 1, 2, 1, 1);
    grid->addWidget(buildExplanationCard(), 1, 3, 1, 1);

    // Row 2: Intelligence & Environment (spanning 2 cols each)
    grid->addWidget(buildEnvironmentCard(), 2, 0, 1, 2);
    grid->addWidget(buildIntelligenceCard(), 2, 2, 1, 2);

    // Row 3: History Table
    grid->addWidget(buildHistoryCard(), 3, 0, 1, 4);

    // Row 4: Stretch
    grid->setRowStretch(4, 1);

    return content;
}

QFrame *DynamicPricingPage::buildSpeciesCard()
{
    QFrame *card = makeCard();
    const AccentCardContent accent = createAccentCardContent(
        card,
        accentBarStyle(QStringLiteral("#0d9488"), QStringLiteral("#0891b2")),
        QMargins(20, 18, 20, 18),
        10);
    QVBoxLayout *layout = accent.layout;
    QLabel *section = makeSectionLabel(QStringLiteral("🐟 SÉLECTION DE L'ESPÈCE"), accent.body);
    section->setFont(PecheFontUtils::moduleFont(10.0));
    layout->addWidget(section);
    layout->addWidget(makeCardTitle(QStringLiteral("Espèce / poisson"), accent.body));

    m_speciesCombo = new ArrowComboBox(accent.body);
    m_speciesCombo->setObjectName(QStringLiteral("DpCombo"));
    configureComboPopup(m_speciesCombo);
    m_speciesCombo->addItem(QStringLiteral("Chargement..."));
    layout->addWidget(m_speciesCombo);

    m_speciesDesc = makeHelperLabel(QStringLiteral("Source: ESPECES"), accent.body);
    layout->addWidget(m_speciesDesc);
    layout->addStretch(1);
    return card;
}

QFrame *DynamicPricingPage::buildPortCard()
{
    QFrame *card = makeCard();
    const AccentCardContent accent = createAccentCardContent(
        card,
        accentBarStyle(QStringLiteral("#0d9488"), QStringLiteral("#0891b2")),
        QMargins(20, 18, 20, 18),
        10);
    QVBoxLayout *layout = accent.layout;
    QLabel *section = makeSectionLabel(QStringLiteral("⚓ PORT"), accent.body);
    section->setFont(PecheFontUtils::moduleFont(10.0));
    layout->addWidget(section);
    layout->addWidget(makeCardTitle(QStringLiteral("Port de référence"), accent.body));

    m_portCombo = new ArrowComboBox(accent.body);
    m_portCombo->setObjectName(QStringLiteral("DpCombo"));
    configureComboPopup(m_portCombo);
    m_portCombo->addItem(QStringLiteral("Chargement..."));
    layout->addWidget(m_portCombo);

    m_portCoords = makeHelperLabel(QStringLiteral("Coordonnées: -"), accent.body);
    layout->addWidget(m_portCoords);
    layout->addStretch(1);
    return card;
}

QFrame *DynamicPricingPage::buildBasePriceCard()
{
    QFrame *card = makeCard(QStringLiteral("metric-amber"));
    card->setMinimumHeight(220);
    const AccentCardContent accent = createAccentCardContent(
        card,
        accentBarStyle(QStringLiteral("#f59e0b"), QStringLiteral("#d97706")),
        QMargins(20, 20, 20, 20),
        12);
    QVBoxLayout *layout = accent.layout;
    QLabel *section = makeSectionLabel(QStringLiteral("💰 PRIX LOCAL"), accent.body);
    section->setFont(PecheFontUtils::moduleFont(11.0));
    layout->addWidget(section);

    QLabel *source = makeHelperLabel(QStringLiteral("PECHESESPECES.PRIXUNITAIRE"), accent.body);
    source->setProperty("dprole", "source");
    layout->addWidget(source);

    m_basePriceValue = makeValueLabel(QStringLiteral("-"), accent.body);
    m_basePriceValue->setProperty("dprole", "metric-value-amber");
    layout->addWidget(m_basePriceValue);

    m_avgPriceValue = makeHelperLabel(QStringLiteral("Moyenne: -"), accent.body);
    m_avgPriceValue->setProperty("dprole", "metric-caption");
    layout->addWidget(m_avgPriceValue);

    m_basePriceHelper = makeHelperLabel(QStringLiteral("Contexte local indisponible"), accent.body);
    m_basePriceHelper->setProperty("dprole", "metric-support");
    layout->addWidget(m_basePriceHelper);

    layout->addStretch(1);
    return card;
}

QFrame *DynamicPricingPage::buildQuantityCard()
{
    QFrame *card = makeCard(QStringLiteral("metric-teal"));
    card->setMinimumHeight(220);
    const AccentCardContent accent = createAccentCardContent(
        card,
        accentBarStyle(QStringLiteral("#10b981"), QStringLiteral("#059669")),
        QMargins(20, 20, 20, 20),
        12);
    QVBoxLayout *layout = accent.layout;
    QLabel *section = makeSectionLabel(QStringLiteral("📦 DISPONIBILITÉ"), accent.body);
    section->setFont(PecheFontUtils::moduleFont(11.0));
    layout->addWidget(section);

    QLabel *source = makeHelperLabel(QStringLiteral("PECHESESPECES.QUANTITE"), accent.body);
    source->setProperty("dprole", "source");
    layout->addWidget(source);

    m_quantityValue = makeValueLabel(QStringLiteral("-"), accent.body);
    m_quantityValue->setProperty("dprole", "metric-value-teal");
    layout->addWidget(m_quantityValue);

    m_catchCountValue = makeHelperLabel(QStringLiteral("Captures actives: 0"), accent.body);
    m_catchCountValue->setProperty("dprole", "metric-support");
    layout->addWidget(m_catchCountValue);

    QHBoxLayout *scoreRow = new QHBoxLayout();
    scoreRow->setContentsMargins(0, 4, 0, 0);
    scoreRow->setSpacing(12);

    QLabel *scoreLabel = makeHelperLabel(QStringLiteral("Score"), accent.body);
    scoreLabel->setProperty("dprole", "metric-caption");
    scoreRow->addWidget(scoreLabel);

    scoreRow->addStretch(1);

    m_availScore = makeValueLabel(QStringLiteral("0 %"), accent.body, true);
    m_availScore->setProperty("dprole", "value-sm");
    scoreRow->addWidget(m_availScore);
    layout->addLayout(scoreRow);

    m_availBar = makeProgressBar(accent.body);
    layout->addWidget(m_availBar);

    layout->addStretch(1);
    return card;
}

QFrame *DynamicPricingPage::buildFreshnessCard()
{
    QFrame *card = makeCard(QStringLiteral("metric-blue"));
    card->setMinimumHeight(220);
    const AccentCardContent accent = createAccentCardContent(
        card,
        accentBarStyle(QStringLiteral("#3b82f6"), QStringLiteral("#2563eb")),
        QMargins(20, 20, 20, 20),
        12);
    QVBoxLayout *layout = accent.layout;
    QLabel *section = makeSectionLabel(QStringLiteral("🧊 FRAÎCHEUR"), accent.body);
    section->setFont(PecheFontUtils::moduleFont(11.0));
    layout->addWidget(section);

    QLabel *source = makeHelperLabel(QStringLiteral("PECHES.DATEPECHE"), accent.body);
    source->setProperty("dprole", "source");
    layout->addWidget(source);

    m_freshnessValue = makeValueLabel(QStringLiteral("-"), accent.body);
    m_freshnessValue->setProperty("dprole", "metric-value-blue");
    layout->addWidget(m_freshnessValue);

    m_catchDateValue = makeHelperLabel(QStringLiteral("Dernière capture: -"), accent.body);
    m_catchDateValue->setProperty("dprole", "metric-support");
    layout->addWidget(m_catchDateValue);

    m_freshnessHelper = makeHelperLabel(QStringLiteral("Score linéaire sur 120 heures"), accent.body);
    m_freshnessHelper->setProperty("dprole", "metric-caption");
    layout->addWidget(m_freshnessHelper);

    m_freshnessBar = makeProgressBar(accent.body, true);
    layout->addWidget(m_freshnessBar);

    layout->addStretch(1);
    return card;
}

QFrame *DynamicPricingPage::buildEnvironmentCard()
{
    QFrame *card = makeCard();
    const AccentCardContent accent = createAccentCardContent(
        card,
        accentBarStyle(QStringLiteral("#3b82f6"), QStringLiteral("#1d4ed8")),
        QMargins(20, 20, 20, 20),
        12);
    QGridLayout *layout = new QGridLayout(accent.body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(20);
    layout->setVerticalSpacing(12);

    QLabel *section = makeSectionLabel(QStringLiteral("🌊 ENVIRONNEMENT MARIN"), accent.body);
    section->setFont(PecheFontUtils::moduleFont(11.0));
    layout->addWidget(section, 0, 0, 1, 4);
    m_envStatusBadge = makeInfoChip(QStringLiteral("EN ATTENTE"), QStringLiteral("badge-loading"), accent.body);
    layout->addWidget(m_envStatusBadge, 0, 4, 1, 1, Qt::AlignRight);

    m_envObsTime = makeHelperLabel(QStringLiteral("Observation: -"), accent.body);
    layout->addWidget(m_envObsTime, 1, 0, 1, 5);

    layout->addWidget(makeTableHeaderLabel(QStringLiteral("Vague"), accent.body), 2, 0);
    layout->addWidget(makeTableHeaderLabel(QStringLiteral("Direction"), accent.body), 2, 1);
    layout->addWidget(makeTableHeaderLabel(QStringLiteral("Période"), accent.body), 2, 2);
    layout->addWidget(makeTableHeaderLabel(QStringLiteral("Courant"), accent.body), 2, 3);
    layout->addWidget(makeTableHeaderLabel(QStringLiteral("Température"), accent.body), 2, 4);

    m_envWaveHeight = makeTableValueLabel(QStringLiteral("-"), accent.body);
    m_envWaveDirection = makeTableValueLabel(QStringLiteral("-"), accent.body);
    m_envWavePeriod = makeTableValueLabel(QStringLiteral("-"), accent.body);
    m_envCurrentVelocity = makeTableValueLabel(QStringLiteral("-"), accent.body);
    m_envSST = makeTableValueLabel(QStringLiteral("-"), accent.body);
    layout->addWidget(m_envWaveHeight, 3, 0);
    layout->addWidget(m_envWaveDirection, 3, 1);
    layout->addWidget(m_envWavePeriod, 3, 2);
    layout->addWidget(m_envCurrentVelocity, 3, 3);
    layout->addWidget(m_envSST, 3, 4);

    m_envCurrentDirection = makeHelperLabel(QStringLiteral("Direction courant: -"), accent.body);
    layout->addWidget(m_envCurrentDirection, 4, 0, 1, 5);

    QLabel *weatherTitle = makeCardTitle(QStringLiteral("🌤️ Weather score"), accent.body);
    weatherTitle->setFont(PecheFontUtils::moduleFont(11.0));
    layout->addWidget(weatherTitle, 5, 0);
    m_envWeatherLabel = makeValueLabel(QStringLiteral("0 %"), accent.body, true);
    layout->addWidget(m_envWeatherLabel, 5, 1);
    m_envWeatherBar = makeProgressBar(accent.body);
    layout->addWidget(m_envWeatherBar, 5, 2, 1, 3);

    QLabel *riskTitle = makeCardTitle(QStringLiteral("⚠️ Risk score"), accent.body);
    riskTitle->setFont(PecheFontUtils::moduleFont(11.0));
    layout->addWidget(riskTitle, 6, 0);
    m_envMarineLabel = makeValueLabel(QStringLiteral("0 %"), accent.body, true);
    layout->addWidget(m_envMarineLabel, 6, 1);
    m_envRiskBar = makeProgressBar(accent.body, true);
    layout->addWidget(m_envRiskBar, 6, 2, 1, 3);

    m_envSummaryLabel = makeHelperLabel(QStringLiteral("✅ Sélectionnez un port pour charger Open-Meteo."), accent.body);
    m_envSummaryLabel->setProperty("dprole", "summary-ok");
    m_envSummaryLabel->setFont(PecheFontUtils::moduleFont(11.0));
    layout->addWidget(m_envSummaryLabel, 7, 0, 1, 5);

    m_envErrorLabel = makeHelperLabel(QString(), accent.body);
    m_envErrorLabel->setProperty("dprole", "error");
    m_envErrorLabel->setVisible(false);
    layout->addWidget(m_envErrorLabel, 8, 0, 1, 5);

    return card;
}

QFrame *DynamicPricingPage::buildRecommendationCard()
{
    QFrame *card = makeCard(QStringLiteral("hero"));
    const AccentCardContent accent = createAccentCardContent(
        card,
        accentBarStyle(QStringLiteral("#5b4bcc"), QStringLiteral("#7c3aed")),
        QMargins(22, 22, 22, 22),
        12);
    QVBoxLayout *layout = accent.layout;

    layout->addWidget(makeSectionLabel(QStringLiteral("RÉSULTAT"), accent.body));
    QLabel *heroTitle = makeCardTitle(QStringLiteral("💰 Prix recommandé"), accent.body);
    heroTitle->setFont(PecheFontUtils::moduleFont(12.0));
    heroTitle->setStyleSheet(QStringLiteral("color: #374151; font-size: 14px; background: transparent;"));
    layout->addWidget(heroTitle);

    m_recPriceLabel = new QLabel(QStringLiteral("--"), accent.body);
    m_recPriceLabel->setProperty("dprole", "heroprice");
    layout->addWidget(m_recPriceLabel);

    QHBoxLayout *bandRow = new QHBoxLayout();
    m_recBandLabel = new QLabel(QStringLiteral("📊 Fourchette: --"), accent.body);
    m_recBandLabel->setProperty("dprole", "heroband");
    m_recBandLabel->setFont(PecheFontUtils::moduleFont(11.0));
    bandRow->addWidget(m_recBandLabel);

    m_recDeltaLabel = new QLabel(QStringLiteral("📈 Delta: --"), accent.body);
    m_recDeltaLabel->setProperty("dprole", "herodelta");
    m_recDeltaLabel->setFont(PecheFontUtils::moduleFont(11.0));
    bandRow->addWidget(m_recDeltaLabel);

    bandRow->addStretch(1);

    m_recSourceLabel = new QLabel(QStringLiteral("LOCAL_ENGINE"), accent.body);
    m_recSourceLabel->setProperty("dprole", "herosource");
    bandRow->addWidget(m_recSourceLabel);
    layout->addLayout(bandRow);

    m_errorBanner = new QLabel(accent.body);
    m_errorBanner->setProperty("dprole", "error");
    m_errorBanner->setVisible(false);
    m_errorBanner->setWordWrap(true);
    layout->addWidget(m_errorBanner);

    QHBoxLayout *buttonRow = new QHBoxLayout();
    buttonRow->addStretch(1);

    auto *recalcButton = new HoverButton(QStringLiteral("Recalculer"), accent.body);
    recalcButton->setButtonStyles(
        QStringLiteral(
            "QPushButton {"
            "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0f766e,stop:1 #0e7490);"
            "color: #F8FAFC;"
            "border: 1px solid rgba(45, 212, 191, 0.4);"
            "border-radius: 10px;"
            "padding: 10px 24px;"
            "font-size: 13px;"
            "font-weight: 700;"
            "}"),
        QStringLiteral(
            "QPushButton {"
            "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #115e59,stop:1 #155e75);"
            "color: #F8FAFC;"
            "border: 1px solid rgba(45, 212, 191, 0.8);"
            "border-radius: 10px;"
            "padding: 10px 24px;"
            "font-size: 13px;"
            "font-weight: 700;"
            "}"));
    m_recalcButton = recalcButton;
    buttonRow->addWidget(m_recalcButton);

    auto *saveButton = new HoverButton(QStringLiteral("Enregistrer"), accent.body);
    saveButton->setButtonStyles(
        QStringLiteral(
            "QPushButton {"
            "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #4f46e5,stop:1 #7c3aed);"
            "color: #F8FAFC;"
            "border: 1px solid rgba(167, 139, 250, 0.4);"
            "border-radius: 10px;"
            "padding: 10px 24px;"
            "font-size: 13px;"
            "font-weight: 700;"
            "}"),
        QStringLiteral(
            "QPushButton {"
            "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #3730a3,stop:1 #5b21b6);"
            "color: #F8FAFC;"
            "border: 1px solid rgba(167, 139, 250, 0.8);"
            "border-radius: 10px;"
            "padding: 10px 24px;"
            "font-size: 13px;"
            "font-weight: 700;"
            "}"));
    m_saveButton = saveButton;
    buttonRow->addWidget(m_saveButton);

    layout->addLayout(buttonRow);
    return card;
}

QFrame *DynamicPricingPage::buildIntelligenceCard()
{
    QFrame *card = makeCard();
    const AccentCardContent accent = createAccentCardContent(
        card,
        accentBarStyle(QStringLiteral("#10b981"), QStringLiteral("#059669")),
        QMargins(20, 20, 20, 20),
        12);
    QGridLayout *layout = new QGridLayout(accent.body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(20);
    layout->setVerticalSpacing(12);

    QLabel *section = makeSectionLabel(QStringLiteral("🎣 GLOBAL FISHING WATCH"), accent.body);
    section->setFont(PecheFontUtils::moduleFont(11.0));
    layout->addWidget(section, 0, 0, 1, 3);
    m_gfwBadge = makeInfoChip(QStringLiteral("EN ATTENTE"), QStringLiteral("badge-loading"), accent.body);
    layout->addWidget(m_gfwBadge, 0, 3, 1, 1, Qt::AlignRight);

    m_gfwTokenState = makeHelperLabel(QStringLiteral("Jeton: vérification en attente"), accent.body);
    layout->addWidget(m_gfwTokenState, 1, 0, 1, 4);

    layout->addWidget(makeTableHeaderLabel(QStringLiteral("Activité"), accent.body), 2, 0);
    layout->addWidget(makeTableHeaderLabel(QStringLiteral("Score"), accent.body), 2, 1);
    layout->addWidget(makeTableHeaderLabel(QStringLiteral("Fenêtre"), accent.body), 2, 2);
    layout->addWidget(makeTableHeaderLabel(QStringLiteral("Source"), accent.body), 2, 3);

    m_gfwHoursValue = makeTableValueLabel(QStringLiteral("-"), accent.body);
    m_gfwScoreValue = makeTableValueLabel(QStringLiteral("0 %"), accent.body);
    m_gfwWindowValue = makeTableValueLabel(QStringLiteral("-"), accent.body);
    m_gfwSourceValue = makeTableValueLabel(QStringLiteral("LOCAL_SEUL"), accent.body);
    layout->addWidget(m_gfwHoursValue, 3, 0);
    layout->addWidget(m_gfwScoreValue, 3, 1);
    layout->addWidget(m_gfwWindowValue, 3, 2);
    layout->addWidget(m_gfwSourceValue, 3, 3);

    m_gfwBar = makeProgressBar(accent.body);
    layout->addWidget(m_gfwBar, 4, 0, 1, 4);

    m_gfwSummary = makeHelperLabel(QStringLiteral("Sélectionnez un port pour vérifier l'activité régionale."), accent.body);
    m_gfwSummary->setProperty("dprole", "summary-muted");
    layout->addWidget(m_gfwSummary, 5, 0, 1, 4);

    m_gfwError = makeHelperLabel(QString(), accent.body);
    m_gfwError->setProperty("dprole", "error");
    m_gfwError->setVisible(false);
    layout->addWidget(m_gfwError, 6, 0, 1, 4);

    return card;
}

QFrame *DynamicPricingPage::buildExplanationCard()
{
    QFrame *card = makeCard();
    const AccentCardContent accent = createAccentCardContent(
        card,
        accentBarStyle(QStringLiteral("#5b4bcc"), QStringLiteral("#7c3aed")),
        QMargins(20, 20, 20, 20),
        12);
    QVBoxLayout *layout = accent.layout;
    QLabel *section = makeSectionLabel(QStringLiteral("🧮 EXPLICATION"), accent.body);
    section->setFont(PecheFontUtils::moduleFont(11.0));
    layout->addWidget(section);
    QLabel *formulaTitle = new QLabel(QStringLiteral("Formule appliquée"), accent.body);
    formulaTitle->setProperty("dprole", "formula-title");
    layout->addWidget(formulaTitle);

    m_formulaLabel = new QLabel(
        QStringLiteral(
            "rec = base + freshnessAdj + availabilityAdj + weatherAdj - riskAdj\n"
            "weatherAdj et riskAdj viennent d'Open-Meteo si les donnees marines sont disponibles."),
        accent.body);
    m_formulaLabel->setProperty("dprole", "formula");
    m_formulaLabel->setWordWrap(true);
    layout->addWidget(m_formulaLabel);

    m_explanationLabel = new QLabel(QStringLiteral("Sélectionnez une espèce."), accent.body);
    m_explanationLabel->setProperty("dprole", "expl");
    m_explanationLabel->setWordWrap(true);
    m_explanationLabel->setTextFormat(Qt::RichText);
    layout->addWidget(m_explanationLabel);

    return card;
}

QFrame *DynamicPricingPage::buildHistoryCard()
{
    QFrame *card = makeCard();
    const AccentCardContent accent = createAccentCardContent(
        card,
        accentBarStyle(QStringLiteral("#5b4bcc"), QStringLiteral("#7c3aed")),
        QMargins(20, 20, 20, 20),
        12);
    QVBoxLayout *layout = accent.layout;

    QHBoxLayout *header = new QHBoxLayout();
    QLabel *section = makeSectionLabel(QStringLiteral("📋 HISTORIQUE"), accent.body);
    section->setFont(PecheFontUtils::moduleFont(11.0));
    header->addWidget(section);
    header->addStretch(1);
    m_historyStatus = makeInfoChip(QStringLiteral("0 entrée"), QStringLiteral("badge-indigo"), accent.body);
    header->addWidget(m_historyStatus);
    layout->addLayout(header);

    m_historyTable = new QTableView(accent.body);
    m_historyTable->setObjectName(QStringLiteral("DpHistoryTable"));
    m_historyTable->setModel(m_historyModel);
    m_historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setMinimumHeight(280);
    m_historyTable->verticalHeader()->setVisible(false);
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_historyTable);

    return card;
}

QFrame *DynamicPricingPage::makeCard(const QString &variant) const
{
    auto *card = new HoverCard();
    Q_UNUSED(variant)
    card->setCardStyles(standardGlassCardStyle(), standardGlassCardHoverStyle());
    applyShadow(card, 20, 4, QColor(0, 0, 30, 50));
    return card;
}

QLabel *DynamicPricingPage::makeSectionLabel(const QString &text, QWidget *parent) const
{
    QLabel *label = new QLabel(text, parent);
    label->setProperty("dprole", "section");
    applyEmojiFontIfNeeded(label, 11);
    return label;
}

QLabel *DynamicPricingPage::makeCardTitle(const QString &text, QWidget *parent) const
{
    QLabel *label = new QLabel(text, parent);
    label->setProperty("dprole", "cardtitle");
    applyEmojiFontIfNeeded(label, 13);
    return label;
}

QLabel *DynamicPricingPage::makeValueLabel(const QString &text, QWidget *parent, bool compact) const
{
    QLabel *label = new QLabel(text, parent);
    label->setProperty("dprole", compact ? "value-sm" : "value");
    label->setWordWrap(true);
    return label;
}

QLabel *DynamicPricingPage::makeHelperLabel(const QString &text, QWidget *parent) const
{
    QLabel *label = new QLabel(text, parent);
    label->setProperty("dprole", "helper");
    label->setWordWrap(true);
    applyEmojiFontIfNeeded(label, 12);
    return label;
}

QProgressBar *DynamicPricingPage::makeProgressBar(QWidget *parent, bool warm) const
{
    auto *bar = new ScoreBarWidget(parent);
    bar->setProperty("trackColor", QColor(QStringLiteral("#e5e7eb")));
    bar->setProperty("fillColor", QColor(warm ? QStringLiteral("#f59e0b") : QStringLiteral("#10b981")));
    bar->setValue(0);
    return bar;
}

void DynamicPricingPage::loadSpecies()
{
    QString error;
    if (!m_repo.loadSpecies(&m_species, &error)) {
        showError(QStringLiteral("Chargement ESPECES impossible: %1").arg(error));
        return;
    }

    m_speciesCombo->blockSignals(true);
    m_speciesCombo->clear();
    for (const DpSpecies &species : qAsConst(m_species)) {
        m_speciesCombo->addItem(species.name, species.id);
    }
    m_speciesCombo->blockSignals(false);

    if (!m_species.isEmpty()) {
        m_speciesCombo->setCurrentIndex(0);
        m_speciesDesc->setText(m_species.first().description);
    }

    loadCatchContext();
}

void DynamicPricingPage::loadPorts()
{
    QString error;
    if (!m_repo.loadPorts(&m_ports, &error)) {
        showError(QStringLiteral("Chargement DP_PORT_CONFIG impossible: %1").arg(error));
    }

    m_portCombo->blockSignals(true);
    m_portCombo->clear();
    if (m_ports.isEmpty()) {
        m_portCombo->addItem(QStringLiteral("(aucun port actif)"), 0);
        m_portCoords->setText(QStringLiteral("Coordonnees: -"));
    } else {
        for (const DpPort &port : qAsConst(m_ports)) {
            m_portCombo->addItem(port.name, port.portId);
        }
    }
    m_portCombo->blockSignals(false);

    if (!m_ports.isEmpty()) {
        onPortChanged(0);
    } else {
        resetEnvironmentCard();
        resetGfwState();
    }
}

void DynamicPricingPage::loadCatchContext()
{
    if (m_speciesCombo->currentIndex() < 0) {
        return;
    }

    const int speciesId = m_speciesCombo->currentData().toInt();
    if (speciesId <= 0) {
        return;
    }

    QString error;
    if (!m_repo.loadCatchContext(speciesId, &m_catchCtx, &error)) {
        showError(QStringLiteral("Chargement du contexte impossible: %1").arg(error));
        return;
    }

    clearError();
    updateContextWidgets();
    loadHistory();
    runEngine();
}

void DynamicPricingPage::loadHistory()
{
    QVector<DpHistoryEntry> entries;
    QString error;
    const int speciesId = m_speciesCombo->currentData().toInt();
    if (speciesId > 0) {
        m_repo.loadHistory(speciesId, &entries, &error);
    }
    updateHistoryTable(entries);

    m_historyStatus->setText(entries.isEmpty()
        ? QStringLiteral("0 entrée")
        : QStringLiteral("%1 entrée(s)").arg(entries.size()));
}

void DynamicPricingPage::runEngine()
{
    m_input = DpPricingInput();
    m_input.speciesId = m_catchCtx.speciesId;
    m_input.basePrice = m_catchCtx.lastBasePrice > 0.0
        ? m_catchCtx.lastBasePrice
        : m_catchCtx.avgBasePrice;
    m_input.totalQuantityKg = m_catchCtx.totalQuantity;
    m_input.freshnessHours = m_catchCtx.freshnessHours;

    for (const DpSpecies &species : qAsConst(m_species)) {
        if (species.id == m_input.speciesId) {
            m_input.speciesName = species.name;
            break;
        }
    }

    const int portId = m_portCombo->currentData().toInt();
    for (const DpPort &port : qAsConst(m_ports)) {
        if (port.portId == portId) {
            m_input.portId = port.portId;
            m_input.portName = port.name;
            m_input.portLatitude = port.latitude;
            m_input.portLongitude = port.longitude;
            break;
        }
    }

    if (m_marineData.valid && m_input.portId > 0) {
        m_input.hasLiveMarineData = true;
        m_input.liveWeatherScore = m_marineScores.weatherScore;
        m_input.liveRiskScore = m_marineScores.riskScore;
    }

    m_result = m_engine.compute(m_input);
    updateResultWidgets(m_result);
    m_saveButton->setEnabled(m_result.recommendedPrice > 0.0);
}

void DynamicPricingPage::fetchMarineData()
{
    if (m_input.portId <= 0
        || qFuzzyIsNull(m_input.portLatitude)
        || qFuzzyIsNull(m_input.portLongitude)) {
        m_marineData = MarineCurrentData();
        m_marineScores = MarineWeatherScores();
        m_input.hasLiveMarineData = false;
        runEngine();
        resetEnvironmentCard();
        return;
    }

    m_marineData = MarineCurrentData();
    m_marineScores = MarineWeatherScores();
    m_input.hasLiveMarineData = false;
    runEngine();
    m_marineService->fetchMarineData(m_input.portLatitude, m_input.portLongitude);
}

void DynamicPricingPage::requestExternalData()
{
    if (m_input.portId <= 0
        || qFuzzyIsNull(m_input.portLatitude)
        || qFuzzyIsNull(m_input.portLongitude)) {
        resetGfwState();
        return;
    }

    m_gfwService->fetchPortActivity(m_input.portLatitude,
                                    m_input.portLongitude,
                                    m_input.portName,
                                    m_catchCtx.latestZone);
}

void DynamicPricingPage::applyMarineScores(const MarineCurrentData &data,
                                           const MarineWeatherScores &scores)
{
    m_marineData = data;
    m_marineScores = scores;
    m_input.hasLiveMarineData = data.valid;
    m_input.liveWeatherScore = scores.weatherScore;
    m_input.liveRiskScore = scores.riskScore;
    runEngine();
    updateEnvironmentCard(data, scores);
}

void DynamicPricingPage::updateContextWidgets()
{
    m_basePriceValue->setText(fmtPrice(m_catchCtx.lastBasePrice));
    m_avgPriceValue->setText(QStringLiteral("Moyenne: %1").arg(fmtPrice(m_catchCtx.avgBasePrice)));
    m_basePriceHelper->setText(
        m_catchCtx.activeCatches > 0
            ? QStringLiteral("%1 capture(s) active(s)").arg(m_catchCtx.activeCatches)
            : QStringLiteral("Aucune capture active"));

    m_quantityValue->setText(
        m_catchCtx.totalQuantity > 0.0
            ? QString::number(m_catchCtx.totalQuantity, 'f', 1) + QStringLiteral(" kg")
            : QStringLiteral("-"));
    m_catchCountValue->setText(
        QStringLiteral("Captures actives: %1").arg(m_catchCtx.activeCatches));

    const double availabilityScore = dpAvailabilityScore(m_catchCtx.totalQuantity);
    m_availScore->setText(fmtScore(availabilityScore));
    m_availBar->setValue(qRound(availabilityScore));
    setScoreBarFill(m_availBar, QStringLiteral("#10b981"));

    m_freshnessValue->setText(fmtScore(m_catchCtx.freshnessScore));
    m_freshnessBar->setValue(qRound(m_catchCtx.freshnessScore));
    if (m_catchCtx.freshnessScore > 70.0) {
        setScoreBarFill(m_freshnessBar, QStringLiteral("#10b981"));
    } else if (m_catchCtx.freshnessScore >= 40.0) {
        setScoreBarFill(m_freshnessBar, QStringLiteral("#f59e0b"));
    } else {
        setScoreBarFill(m_freshnessBar, QStringLiteral("#ef4444"));
    }
    m_catchDateValue->setText(m_catchCtx.mostRecentCatch.isValid()
        ? QStringLiteral("Dernière capture: %1 (%2 h)")
            .arg(m_catchCtx.mostRecentCatch.toString(QStringLiteral("dd/MM/yyyy")),
                 QString::number(m_catchCtx.freshnessHours, 'f', 1))
        : QStringLiteral("Dernière capture: -"));
}

void DynamicPricingPage::updateResultWidgets(const DpPricingResult &result)
{
    m_recPriceLabel->setText(fmtPrice(result.recommendedPrice));
    m_recBandLabel->setText(
        QStringLiteral("📊 Fourchette: %1 -> %2")
            .arg(fmtPrice(m_input.basePrice * 0.70), fmtPrice(m_input.basePrice * 1.50)));

    const double delta = result.recommendedPrice - m_input.basePrice;
    m_recDeltaLabel->setText(
        QStringLiteral("%1 Delta: %2")
            .arg(delta >= 0.0 ? QStringLiteral("📈") : QStringLiteral("📉"),
                 QString::number(delta, 'f', 3) + QStringLiteral(" TND/kg")));
    m_recDeltaLabel->setStyleSheet(QStringLiteral(
        "font-size: 13px; font-weight: 700; color: %1; background: transparent;")
        .arg(delta >= 0.0 ? QStringLiteral("#059669") : QStringLiteral("#dc2626")));
    m_recSourceLabel->setText(result.apiSource);

    const QStringList lines = result.explanation.split(QLatin1Char('\n'));
    QStringList richLines;
    for (const QString &line : lines) {
        QString style = QStringLiteral("color:#374151;");
        if (line.startsWith(QStringLiteral("Fraicheur"))
            || line.startsWith(QStringLiteral("Disponibilite"))
            || line.startsWith(QStringLiteral("Meteo marine"))) {
            style = line.contains(QLatin1Char('-'))
                ? QStringLiteral("color:#dc2626;font-weight: 700;")
                : QStringLiteral("color:#059669;font-weight: 700;");
        } else if (line.startsWith(QStringLiteral("Risque marin"))) {
            style = QStringLiteral("color:#dc2626;font-weight: 700;");
        } else if (line.startsWith(QStringLiteral("Prix recommande"))) {
            richLines << QStringLiteral("<span style=\"color:#111827;font-size:14px;font-weight: 700;\">✅ %1</span>")
                .arg(line.toHtmlEscaped());
            continue;
        }
        richLines << QStringLiteral("<span style=\"%1\">%2</span>").arg(style, line.toHtmlEscaped());
    }
    m_explanationLabel->setText(richLines.join(QStringLiteral("<br/>")));
}

void DynamicPricingPage::updateHistoryTable(const QVector<DpHistoryEntry> &entries)
{
    m_historyModel->setEntries(entries);
}

void DynamicPricingPage::updateEnvironmentCard(const MarineCurrentData &data,
                                               const MarineWeatherScores &scores)
{
    if (!data.valid) {
        resetEnvironmentCard();
        return;
    }

    m_envStatusBadge->setText(QStringLiteral("EN DIRECT"));
    m_envStatusBadge->setProperty("dprole", "badge-live");
    m_envStatusBadge->style()->unpolish(m_envStatusBadge);
    m_envStatusBadge->style()->polish(m_envStatusBadge);

    m_envObsTime->setText(
        QStringLiteral("Observation: %1 | Port: %2 (%3, %4)")
            .arg(formatObservationTime(data.observationTime),
                 m_input.portName,
                 QString::number(data.requestLatitude, 'f', 4),
                 QString::number(data.requestLongitude, 'f', 4)));
    m_envWaveHeight->setText(QString::number(data.waveHeight, 'f', 2) + QStringLiteral(" m"));
    m_envWaveDirection->setText(
        QStringLiteral("%1 (%2)")
            .arg(QString::number(data.waveDirection, 'f', 0) + QChar(176),
                 fmtBearing(data.waveDirection)));
    m_envWavePeriod->setText(QString::number(data.wavePeriod, 'f', 1) + QStringLiteral(" s"));
    m_envCurrentVelocity->setText(
        QString::number(data.oceanCurrentVelocity, 'f', 2) + QStringLiteral(" km/h"));
    m_envCurrentDirection->setText(
        QStringLiteral("Direction courant: %1 (%2)")
            .arg(QString::number(data.oceanCurrentDirection, 'f', 0) + QChar(176),
                 fmtBearing(data.oceanCurrentDirection)));
    m_envSST->setText(QString::number(data.seaSurfaceTemperature, 'f', 1) + QStringLiteral(" °C"));

    m_envWeatherLabel->setText(fmtScore(scores.weatherScore));
    m_envMarineLabel->setText(fmtScore(scores.riskScore));
    m_envWeatherBar->setValue(qRound(scores.weatherScore));
    m_envRiskBar->setValue(qRound(scores.riskScore));
    setScoreBarFill(m_envWeatherBar, QStringLiteral("#22c55e"));
    setScoreBarFill(m_envRiskBar, QStringLiteral("#f59e0b"));
    m_envSummaryLabel->setText(QStringLiteral("✅ %1").arg(scores.summary));
    m_envSummaryLabel->setProperty("dprole", "summary-ok");
    m_envErrorLabel->setVisible(false);
}

void DynamicPricingPage::updateIntelligenceCard()
{
    const bool tokenConfigured = m_gfwIntel.tokenConfigured || m_gfwService->hasConfiguredToken();
    m_gfwTokenState->setText(tokenConfigured
        ? QStringLiteral("Jeton: configuré")
        : QStringLiteral("Jeton: manquant (QSettings dynamic_pricing/gfw_token ou GFW_TOKEN)"));

    if (m_gfwIntel.valid) {
        m_gfwBadge->setText(QStringLiteral("ACTIF"));
        m_gfwBadge->setProperty("dprole", "badge-live");
        m_gfwHoursValue->setText(fmtHours(m_gfwIntel.totalFishingHours));
        m_gfwScoreValue->setText(fmtScore(m_gfwScores.activityScore));
        m_gfwWindowValue->setText(QStringLiteral("%1 -> %2").arg(m_gfwIntel.startDate, m_gfwIntel.endDate));
        m_gfwSourceValue->setText(m_gfwScores.sourceTag.isEmpty()
            ? QStringLiteral("GFW")
            : m_gfwScores.sourceTag);
        m_gfwBar->setValue(qRound(m_gfwScores.activityScore));
        setScoreBarFill(m_gfwBar, QStringLiteral("#10b981"));
        m_gfwSummary->setText(
            QStringLiteral("%1 Échantillons: %2 cellule(s).")
                .arg(m_gfwScores.summary)
                .arg(m_gfwIntel.sampleCells));
        m_gfwSummary->setProperty("dprole", "summary-muted");
        m_gfwError->setVisible(false);
    } else if (m_gfwService->isBusy()) {
        m_gfwBadge->setText(QStringLiteral("CHARGEMENT"));
        m_gfwBadge->setProperty("dprole", "badge-loading");
        m_gfwHoursValue->setText(QStringLiteral("-"));
        m_gfwScoreValue->setText(QStringLiteral("0 %"));
        m_gfwWindowValue->setText(QStringLiteral("Analyse en cours"));
        m_gfwSourceValue->setText(QStringLiteral("GFW_4WINGS"));
        m_gfwBar->setValue(0);
        setScoreBarFill(m_gfwBar, QStringLiteral("#10b981"));
        m_gfwSummary->setText(QStringLiteral("Chargement Global Fishing Watch pour le port sélectionné..."));
        m_gfwSummary->setProperty("dprole", "summary-muted");
        m_gfwError->setVisible(false);
    } else {
        m_gfwBadge->setText(tokenConfigured ? QStringLiteral("SECOURS") : QStringLiteral("NON CONFIGURÉ"));
        m_gfwBadge->setProperty("dprole", tokenConfigured ? "badge-loading" : "badge-error");
        m_gfwHoursValue->setText(QStringLiteral("-"));
        m_gfwScoreValue->setText(QStringLiteral("0 %"));
        m_gfwWindowValue->setText(QStringLiteral("-"));
        m_gfwSourceValue->setText(QStringLiteral("LOCAL_SEUL"));
        m_gfwBar->setValue(0);
        setScoreBarFill(m_gfwBar, QStringLiteral("#10b981"));
    }

    m_gfwBadge->style()->unpolish(m_gfwBadge);
    m_gfwBadge->style()->polish(m_gfwBadge);
}

void DynamicPricingPage::resetEnvironmentCard()
{
    m_envStatusBadge->setText(QStringLiteral("EN ATTENTE"));
    m_envStatusBadge->setProperty("dprole", "badge-loading");
    m_envStatusBadge->style()->unpolish(m_envStatusBadge);
    m_envStatusBadge->style()->polish(m_envStatusBadge);

    m_envObsTime->setText(QStringLiteral("Observation: -"));
    m_envWaveHeight->setText(QStringLiteral("-"));
    m_envWaveDirection->setText(QStringLiteral("-"));
    m_envWavePeriod->setText(QStringLiteral("-"));
    m_envCurrentVelocity->setText(QStringLiteral("-"));
    m_envCurrentDirection->setText(QStringLiteral("Direction courant: -"));
    m_envSST->setText(QStringLiteral("-"));
    m_envWeatherLabel->setText(QStringLiteral("0 %"));
    m_envMarineLabel->setText(QStringLiteral("0 %"));
    m_envWeatherBar->setValue(0);
    m_envRiskBar->setValue(0);
    setScoreBarFill(m_envWeatherBar, QStringLiteral("#22c55e"));
    setScoreBarFill(m_envRiskBar, QStringLiteral("#f59e0b"));
    m_envSummaryLabel->setText(QStringLiteral("✅ Le moteur local reste actif sans données marines."));
    m_envSummaryLabel->setProperty("dprole", "summary-ok");
    m_envErrorLabel->setVisible(false);
}

void DynamicPricingPage::setEnvironmentLoadingState()
{
    m_envStatusBadge->setText(QStringLiteral("CHARGEMENT"));
    m_envStatusBadge->setProperty("dprole", "badge-loading");
    m_envStatusBadge->style()->unpolish(m_envStatusBadge);
    m_envStatusBadge->style()->polish(m_envStatusBadge);
    m_envSummaryLabel->setText(
        QStringLiteral("✅ Chargement Open-Meteo pour le port sélectionné..."));
    m_envSummaryLabel->setProperty("dprole", "summary-ok");
    m_envErrorLabel->setVisible(false);
}

void DynamicPricingPage::setEnvironmentErrorState(const QString &message)
{
    m_envStatusBadge->setText(QStringLiteral("ERREUR"));
    m_envStatusBadge->setProperty("dprole", "badge-error");
    m_envStatusBadge->style()->unpolish(m_envStatusBadge);
    m_envStatusBadge->style()->polish(m_envStatusBadge);
    m_envSummaryLabel->setText(
        QStringLiteral("✅ Le moteur local conserve la recommandation sans score marin."));
    m_envSummaryLabel->setProperty("dprole", "summary-ok");
    m_envErrorLabel->setText(message);
    m_envErrorLabel->setVisible(true);
}

void DynamicPricingPage::resetGfwState()
{
    m_gfwIntel = GfwPortIntelligence();
    m_gfwScores = GfwIntelligenceScores();
    m_input.hasExternalIntelligence = false;
    m_input.externalActivityScore = 0.0;
    m_input.externalFishingHours = 0.0;
    m_input.externalSummary.clear();
    m_input.gfwTokenConfigured = m_gfwService->hasConfiguredToken();

    if (m_gfwService->isBusy()) {
        m_gfwService->cancelPendingRequest();
    }

    if (m_gfwSummary) {
        m_gfwSummary->setText(QStringLiteral("Le moteur de prix reste local tant que l'intelligence GFW n'est pas disponible."));
        m_gfwSummary->setProperty("dprole", "summary-muted");
    }
    if (m_gfwError) {
        m_gfwError->clear();
        m_gfwError->setVisible(false);
    }
    if (m_gfwWindowValue) {
        m_gfwWindowValue->setText(QStringLiteral("-"));
    }
    if (m_gfwSourceValue) {
        m_gfwSourceValue->setText(QStringLiteral("LOCAL_SEUL"));
    }
    updateIntelligenceCard();
}

void DynamicPricingPage::setBusy(bool busy)
{
    setCursor(busy ? Qt::WaitCursor : Qt::ArrowCursor);
    m_speciesCombo->setEnabled(!busy);
    m_portCombo->setEnabled(!busy);
    m_backButton->setEnabled(!busy);
    m_recalcButton->setEnabled(!busy);
    m_saveButton->setEnabled(!busy && m_result.recommendedPrice > 0.0);
}

void DynamicPricingPage::showError(const QString &msg)
{
    m_errorBanner->setText(msg);
    m_errorBanner->setVisible(true);
}

void DynamicPricingPage::clearError()
{
    m_errorBanner->clear();
    m_errorBanner->setVisible(false);
}

QString DynamicPricingPage::fmtPrice(double value)
{
    return value > 0.0
        ? QString::number(value, 'f', 3) + QStringLiteral(" TND/kg")
        : QStringLiteral("-");
}

QString DynamicPricingPage::fmtScore(double score)
{
    return QString::number(qRound(score)) + QStringLiteral(" %");
}

QString DynamicPricingPage::fmtHours(double value)
{
    return QString::number(value, 'f', value >= 10.0 ? 1 : 2) + QStringLiteral(" h");
}

QString DynamicPricingPage::fmtBearing(double degrees)
{
    static const char * const bearings[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    const double normalized = std::fmod(degrees + 360.0, 360.0);
    const int index = int(qRound(normalized / 45.0)) % 8;
    return QString::fromLatin1(bearings[index]);
}

void DynamicPricingPage::onSpeciesChanged(int comboIndex)
{
    if (comboIndex < 0 || comboIndex >= m_species.size()) {
        return;
    }

    m_speciesDesc->setText(m_species.at(comboIndex).description);
    loadCatchContext();
}

void DynamicPricingPage::onPortChanged(int comboIndex)
{
    Q_UNUSED(comboIndex)

    const int selectedPortId = m_portCombo->currentData().toInt();
    QString coords = QStringLiteral("Coordonnees: -");
    for (const DpPort &port : qAsConst(m_ports)) {
        if (port.portId == selectedPortId) {
            coords = QStringLiteral("Coordonnees: %1 / %2")
                .arg(QString::number(port.latitude, 'f', 6),
                     QString::number(port.longitude, 'f', 6));
            break;
        }
    }
    m_portCoords->setText(coords);
    runEngine();
    fetchMarineData();
    requestExternalData();
}

void DynamicPricingPage::onRecalculateClicked()
{
    loadCatchContext();
    fetchMarineData();
    requestExternalData();
}

void DynamicPricingPage::onSaveClicked()
{
    if (m_result.recommendedPrice <= 0.0) {
        showError(QStringLiteral("Aucun prix a enregistrer."));
        return;
    }

    setBusy(true);
    QString error;
    const bool ok = m_repo.saveRecommendation(m_input, m_result, &error);
    setBusy(false);

    if (!ok) {
        showError(QStringLiteral("Echec d'enregistrement: %1").arg(error));
        return;
    }

    clearError();
    loadHistory();
    QMessageBox::information(
        this,
        QStringLiteral("Tarification dynamique"),
        QStringLiteral("La recommandation a ete sauvegardee dans DP_PRICE_RECOMMENDATION_LOG."));
}

void DynamicPricingPage::onHistoryCellClicked(const QModelIndex &index)
{
    if (!index.isValid() || !m_historyModel) {
        return;
    }

    const DpHistoryEntry *entry = m_historyModel->entryAt(index.row());
    if (!entry) {
        return;
    }

    QMessageBox::information(
        this,
        QStringLiteral("Detail recommandation"),
        QStringLiteral(
            "Date: %1\n"
            "Port: %2\n"
            "Prix base: %3\n"
            "Prix recommande: %4\n"
            "Source: %5\n"
            "Explication: %6")
            .arg(entry->createdAt.toString(QStringLiteral("dd/MM/yyyy hh:mm")),
                 entry->portName,
                 fmtPrice(entry->basePrice),
                 fmtPrice(entry->recommendedPrice),
                 entry->apiSource,
                 entry->explanation));
}

void DynamicPricingPage::onMarineDataLoading()
{
    setEnvironmentLoadingState();
}

void DynamicPricingPage::onMarineDataReady(const MarineCurrentData &data,
                                           const MarineWeatherScores &scores)
{
    applyMarineScores(data, scores);
}

void DynamicPricingPage::onMarineDataError(const QString &errorMessage)
{
    m_marineData = MarineCurrentData();
    m_marineScores = MarineWeatherScores();
    m_input.hasLiveMarineData = false;
    runEngine();
    setEnvironmentErrorState(errorMessage);
}

void DynamicPricingPage::onGfwLoading()
{
    m_gfwIntel = GfwPortIntelligence();
    m_gfwScores = GfwIntelligenceScores();
    m_input.hasExternalIntelligence = false;
    m_input.externalActivityScore = 0.0;
    m_input.externalFishingHours = 0.0;
    m_input.externalSummary.clear();
    m_input.gfwTokenConfigured = m_gfwService->hasConfiguredToken();
    if (m_gfwError) {
        m_gfwError->clear();
        m_gfwError->setVisible(false);
    }
    updateIntelligenceCard();
}

void DynamicPricingPage::onGfwReady(const GfwPortIntelligence &intel,
                                    const GfwIntelligenceScores &scores)
{
    m_gfwIntel = intel;
    m_gfwScores = scores;
    m_input.hasExternalIntelligence = intel.valid;
    m_input.externalActivityScore = scores.activityScore;
    m_input.externalFishingHours = intel.totalFishingHours;
    m_input.externalSummary = scores.summary;
    m_input.gfwTokenConfigured = intel.tokenConfigured;
    runEngine();
    updateIntelligenceCard();
}

void DynamicPricingPage::onGfwUnavailable(const QString &message)
{
    m_gfwIntel = GfwPortIntelligence();
    m_gfwScores = GfwIntelligenceScores();
    m_input.hasExternalIntelligence = false;
    m_input.externalActivityScore = 0.0;
    m_input.externalFishingHours = 0.0;
    m_input.externalSummary = message;
    m_input.gfwTokenConfigured = false;
    if (m_gfwSummary) {
        m_gfwSummary->setText(message);
    }
    if (m_gfwError) {
        m_gfwError->clear();
        m_gfwError->setVisible(false);
    }
    runEngine();
    updateIntelligenceCard();
}

void DynamicPricingPage::onGfwError(const QString &message)
{
    m_gfwIntel = GfwPortIntelligence();
    m_gfwScores = GfwIntelligenceScores();
    m_input.hasExternalIntelligence = false;
    m_input.externalActivityScore = 0.0;
    m_input.externalFishingHours = 0.0;
    m_input.externalSummary = message;
    m_input.gfwTokenConfigured = m_gfwService->hasConfiguredToken();
    if (m_gfwSummary) {
        m_gfwSummary->setText(QStringLiteral("Le moteur local reste actif sans intelligence externe."));
    }
    if (m_gfwError) {
        m_gfwError->setText(message);
        m_gfwError->setVisible(true);
    }
    runEngine();
    updateIntelligenceCard();
}

