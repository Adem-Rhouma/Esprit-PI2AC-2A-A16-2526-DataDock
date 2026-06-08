#include "fishvisionpage.h"
#include "../pechefontutils.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLayout>
#include <QLinearGradient>
#include <QMessageBox>
#include <QMetaType>
#include <QMimeDatabase>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QMenu>
#include <QPainter>
#include <QPrinter>
#include <QTextDocument>
#include <QTextEdit>
#include <QLineEdit>
#include <QScrollBar>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSslError>
#include <QSslSocket>
#include <QStyle>

namespace {

QString getApiKey() {
    QString key = QString::fromLocal8Bit(qgetenv("GEMINI_API_KEY"));
    return key;
}
const char kEndpoint[] =
    "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent";
const char kPrompt[] =
    "Tu es un expert en poissonnerie, qualite marine et valorisation des produits de la peche.\n"
    "Analyse cette image de poisson et retourne UNIQUEMENT un objet JSON valide, sans markdown.\n"
    "Tous les scores numeriques doivent etre sur 100, jamais sur 5, jamais sur 10.\n"
    "{\"espece\":\"nom commun francais de l'espece\",\"nom_scientifique\":\"nom scientifique latin\","
    "\"poids_estime\":\"1.8\",\"tags\":[\"tag1\",\"tag2\",\"tag3\"],\"grade\":\"A ou B ou C ou D\","
    "\"score_fraicheur\":0,\"score_oeil\":0,\"score_branchies\":0,\"score_peau\":0,"
    "\"verdict\":\"Qualite excellente ou Qualite acceptable ou Qualite mediocre\","
    "\"rapport_ia\":\"paragraphe professionnel detaille de 3-4 phrases\","
    "\"indicateurs\":[{\"label\":\"description indicateur\",\"status\":\"ok ou warning ou bad\"}],"
    "\"valeur_kg\":0.00,\"fourchette_min\":0.00,\"fourchette_max\":0.00,"
    "\"canaux_vente\":[\"canal1\",\"canal2\",\"canal3\"],"
    "\"recommandations\":[{\"icon\":\"emoji\",\"titre\":\"titre action\",\"detail\":\"description courte\"}],"
    "\"positionnement\":[{\"label\":\"vs. prix moyen port\",\"valeur\":\"+X%\",\"status\":\"avantage ou ecart ou neutre\"}]}";

QString strv(const QJsonObject &o, const QString &k) { return o.value(k).toVariant().toString().trimmed(); }
double dblv(const QJsonObject &o, const QString &k) { return o.value(k).toVariant().toDouble(); }

int normalizedScore(const QJsonValue &value)
{
    if (value.isDouble()) {
        const double raw = value.toDouble();
        if (raw >= 0.0 && raw <= 1.0) {
            return qBound(0, qRound(raw * 100.0), 100);
        }
        return qBound(0, qRound(raw), 100);
    }

    const QString text = value.toVariant().toString().trimmed();
    if (text.isEmpty()) {
        return 0;
    }

    const QRegularExpression fractionPattern(QStringLiteral("(-?\\d+(?:[\\.,]\\d+)?)\\s*/\\s*(\\d+(?:[\\.,]\\d+)?)"));
    const QRegularExpressionMatch fractionMatch = fractionPattern.match(text);
    if (fractionMatch.hasMatch()) {
        QString numeratorText = fractionMatch.captured(1);
        QString denominatorText = fractionMatch.captured(2);
        numeratorText.replace(',', '.');
        denominatorText.replace(',', '.');
        const double numerator = numeratorText.toDouble();
        const double denominator = denominatorText.toDouble();
        if (denominator > 0.0) {
            return qBound(0, qRound((numerator / denominator) * 100.0), 100);
        }
    }

    const QRegularExpressionMatch match =
        QRegularExpression(QStringLiteral("(-?\\d+(?:[\\.,]\\d+)?)")).match(text);
    if (!match.hasMatch()) {
        return 0;
    }

    QString numberText = match.captured(1);
    numberText.replace(',', '.');
    const double raw = numberText.toDouble();
    if (raw >= 0.0 && raw <= 1.0) {
        return qBound(0, qRound(raw * 100.0), 100);
    }
    return qBound(0, qRound(raw), 100);
}

QStringList strList(const QJsonArray &a)
{
    QStringList out;
    for (const QJsonValue &v : a) {
        const QString s = v.toVariant().toString().trimmed();
        if (!s.isEmpty()) {
            out << s;
        }
    }
    return out;
}

QString summarizeBytes(const QByteArray &bytes, int maxLen = 900)
{
    QString text = QString::fromUtf8(bytes).trimmed();
    if (text.isEmpty()) {
        text = QString::fromLatin1(bytes).trimmed();
    }
    if (text.size() > maxLen) {
        text = text.left(maxLen) + "...";
    }
    return text;
}

QString ringCaption(int score)
{
    if (score >= 80) return QStringLiteral("Premium");
    if (score >= 60) return QStringLiteral("Vente");
    return QStringLiteral("Controle");
}

QString captionForGrade(const QString &grade)
{
    if (grade == "A") return QStringLiteral("Premium");
    if (grade == "B") return QStringLiteral("Standard");
    if (grade == "C") return QStringLiteral("Controle");
    return QStringLiteral("Critique");
}

QString verdictFromScore(int score)
{
    if (score < 40) return QStringLiteral("Qualite mediocre");
    if (score < 70) return QStringLiteral("Qualite acceptable");
    return QStringLiteral("Qualite excellente");
}

QString gradeFromScore(int score)
{
    if (score >= 80) return QStringLiteral("A");
    if (score >= 60) return QStringLiteral("B");
    if (score >= 40) return QStringLiteral("C");
    return QStringLiteral("D");
}

QString normalizedIndicatorStatus(QString status)
{
    status = status.trimmed().toLower();
    if (status == "ok" || status == "success" || status == "good") return QStringLiteral("ok");
    if (status == "warning" || status == "warn") return QStringLiteral("warning");
    if (status == "bad" || status == "danger" || status == "critical") return QStringLiteral("bad");
    return QStringLiteral("warning");
}

QString jsonOnly(QString text)
{
    text = text.trimmed();
    const int first = text.indexOf('{');
    const int last = text.lastIndexOf('}');
    return (first >= 0 && last > first) ? text.mid(first, last - first + 1).trimmed() : text;
}

QString statusDotColor(const QString &status)
{
    if (status == "ok" || status == "avantage") return QStringLiteral("#34d399");
    if (status == "warning" || status == "ecart") return QStringLiteral("#f59e0b");
    if (status == "bad") return QStringLiteral("#ef4444");
    return QStringLiteral("#38bdf8");
}

QString badgeClassForStatus(QString status)
{
    status = status.trimmed().toLower();
    if (status == "ok" || status == "success" || status == "good" || status == "avantage") return QStringLiteral("ok");
    if (status == "warning" || status == "warn" || status == "ecart") return QStringLiteral("warning");
    if (status == "bad" || status == "danger" || status == "critical") return QStringLiteral("critical");
    if (status == "neutre") return QStringLiteral("blue");
    if (status == "ai") return QStringLiteral("violet");
    return QStringLiteral("blue");
}

QString badgeText(const QString &status)
{
    if (status == "avantage") return QStringLiteral("Avantage");
    if (status == "ecart") return QStringLiteral("Ecart");
    if (status == "neutre") return QStringLiteral("Neutre");
    if (status == "warning") return QStringLiteral("Attention");
    if (status == "bad") return QStringLiteral("Critique");
    return QStringLiteral("OK");
}

QString tagClass(const QString &tag)
{
    const QString text = tag.toLower();
    if (text.contains("premium") || text.contains("grade a") || text.contains("excellent") || text.contains("frais") ||
        text.contains("ok") || text.contains("export")) {
        return QStringLiteral("green");
    }
    if (text.contains("marche") || text.contains("vente") || text.contains("ia") || text.contains("gemini") ||
        text.contains("analyse")) {
        return QStringLiteral("blue");
    }
    return QStringLiteral("violet");
}

QString channelIcon(const QString &channel)
{
    const QString text = channel.toLower();
    if (text.contains("restauration") || text.contains("restaurant")) return QStringLiteral("RS");
    if (text.contains("export")) return QStringLiteral("EX");
    if (text.contains("poissonnerie")) return QStringLiteral("PO");
    if (text.contains("marche")) return QStringLiteral("MA");
    if (text.contains("gros") || text.contains("grossiste")) return QStringLiteral("GR");
    if (text.contains("transformation")) return QStringLiteral("TR");
    return QStringLiteral("CV");
}

QString channelSubtitle(const QString &channel)
{
    const QString text = channel.toLower();
    if (text.contains("restauration") || text.contains("restaurant")) return QStringLiteral("Consommation rapide");
    if (text.contains("export")) return QStringLiteral("Flux premium");
    if (text.contains("poissonnerie")) return QStringLiteral("Detail specialise");
    if (text.contains("marche")) return QStringLiteral("Volume journalier");
    if (text.contains("gros") || text.contains("grossiste")) return QStringLiteral("Rotation forte");
    if (text.contains("transformation")) return QStringLiteral("Valorisation secondaire");
    return QStringLiteral("Canal recommande");
}

QString channelTone(const QString &channel)
{
    const QString text = channel.toLower();
    if (text.contains("export") || text.contains("restauration")) return QStringLiteral("green");
    if (text.contains("poissonnerie") || text.contains("marche")) return QStringLiteral("blue");
    return QStringLiteral("violet");
}

QString recommendationTone(const QString &titre, const QString &detail)
{
    const QString text = (titre + ' ' + detail).toLower();
    if (text.contains("stock") || text.contains("froid") || text.contains("glace") || text.contains("conservation")) {
        return QStringLiteral("cold");
    }
    if (text.contains("lot") || text.contains("trace") || text.contains("tra") || text.contains("identif")) {
        return QStringLiteral("trace");
    }
    if (text.contains("prix") || text.contains("marge") || text.contains("vente") || text.contains("tarif")) {
        return QStringLiteral("pricing");
    }
    return QStringLiteral("analytics");
}

QString recommendationFallbackIcon(const QString &titre, const QString &detail)
{
    const QString tone = recommendationTone(titre, detail);
    if (tone == "cold") return QStringLiteral("CL");
    if (tone == "trace") return QStringLiteral("LT");
    if (tone == "pricing") return QStringLiteral("PX");
    return QStringLiteral("AI");
}

QString recentIcon(const QString &title)
{
    const QString text = title.toLower();
    if (text.contains("gemini")) return QStringLiteral("GV");
    if (text.contains("json")) return QStringLiteral("JS");
    if (text.contains("live")) return QStringLiteral("LV");
    return QStringLiteral("FV");
}

bool isTableAccessible(const QString &tableName, QString *errorMessage = nullptr)
{
    QSqlQuery query;
    if (!query.exec(QStringLiteral("SELECT 1 FROM %1 WHERE 1 = 0").arg(tableName))) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }
    return true;
}

QString jsonFromStringList(const QStringList &values)
{
    QJsonArray array;
    for (const QString &value : values) {
        array.append(value);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QString jsonFromIndicators(const QList<FishVisionIndicator> &indicators)
{
    QJsonArray array;
    for (const FishVisionIndicator &indicator : indicators) {
        QJsonObject obj;
        obj.insert(QStringLiteral("label"), indicator.label);
        obj.insert(QStringLiteral("status"), indicator.status);
        array.append(obj);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QString jsonFromRecommendations(const QList<FishVisionRecommendation> &recommendations)
{
    QJsonArray array;
    for (const FishVisionRecommendation &recommendation : recommendations) {
        QJsonObject obj;
        obj.insert(QStringLiteral("icon"), recommendation.icon);
        obj.insert(QStringLiteral("titre"), recommendation.titre);
        obj.insert(QStringLiteral("detail"), recommendation.detail);
        array.append(obj);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QString jsonFromPositionnements(const QList<FishVisionPositionnement> &positionnements)
{
    QJsonArray array;
    for (const FishVisionPositionnement &positionnement : positionnements) {
        QJsonObject obj;
        obj.insert(QStringLiteral("label"), positionnement.label);
        obj.insert(QStringLiteral("valeur"), positionnement.valeur);
        obj.insert(QStringLiteral("status"), positionnement.status);
        array.append(obj);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QVariant nullIntVariant()
{
    return QVariant(QMetaType(QMetaType::Int));
}

QVariant nullDoubleVariant()
{
    return QVariant(QMetaType(QMetaType::Double));
}

QVariant nullStringVariant()
{
    return QVariant(QMetaType(QMetaType::QString));
}

QStringList parseStringListJson(const QString &json)
{
    if (json.trimmed().isEmpty()) {
        return {};
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        return {};
    }
    return strList(doc.array());
}

QList<FishVisionIndicator> parseIndicatorsJson(const QString &json)
{
    QList<FishVisionIndicator> out;
    if (json.trimmed().isEmpty()) {
        return out;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        return out;
    }

    for (const QJsonValue &value : doc.array()) {
        const QJsonObject obj = value.toObject();
        const QString label = strv(obj, QStringLiteral("label"));
        if (label.isEmpty()) {
            continue;
        }
        out.append({label, normalizedIndicatorStatus(strv(obj, QStringLiteral("status")))});
    }
    return out;
}

QList<FishVisionRecommendation> parseRecommendationsJson(const QString &json)
{
    QList<FishVisionRecommendation> out;
    if (json.trimmed().isEmpty()) {
        return out;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        return out;
    }

    for (const QJsonValue &value : doc.array()) {
        const QJsonObject obj = value.toObject();
        const QString titre = strv(obj, QStringLiteral("titre"));
        if (titre.isEmpty()) {
            continue;
        }
        out.append({strv(obj, QStringLiteral("icon")), titre, strv(obj, QStringLiteral("detail"))});
    }
    return out;
}

QList<FishVisionPositionnement> parsePositionnementsJson(const QString &json)
{
    QList<FishVisionPositionnement> out;
    if (json.trimmed().isEmpty()) {
        return out;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        return out;
    }

    for (const QJsonValue &value : doc.array()) {
        const QJsonObject obj = value.toObject();
        const QString label = strv(obj, QStringLiteral("label"));
        if (label.isEmpty()) {
            continue;
        }
        out.append({label, strv(obj, QStringLiteral("valeur")), strv(obj, QStringLiteral("status")).toLower()});
    }
    return out;
}

} // namespace

ScoreRingWidget::ScoreRingWidget(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
}

void ScoreRingWidget::setScore(int score)
{
    m_score = qBound(0, score, 100);
    update();
}

void ScoreRingWidget::setCaption(const QString &caption)
{
    m_caption = caption;
    update();
}

int ScoreRingWidget::score() const
{
    return m_score;
}

void ScoreRingWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(8, 8, -8, -8);
    const qreal penWidth = 7.0;

    p.setPen(QPen(QColor(15, 23, 42, 20), penWidth, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(bounds, 90 * 16, -360 * 16);

    QLinearGradient strokeGradient(bounds.topLeft(), bounds.bottomRight());
    strokeGradient.setColorAt(0.0, QColor("#10b981"));
    strokeGradient.setColorAt(1.0, QColor("#34d399"));
    p.setPen(QPen(QBrush(strokeGradient), penWidth, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(bounds, 90 * 16, int(-360.0 * m_score / 100.0 * 16.0));

    QFont scoreFont = font();
    scoreFont.setPointSize(20);
    scoreFont.setWeight(QFont::Bold);
    p.setFont(scoreFont);
    p.setPen(QColor("#1e3a8a"));
    p.drawText(rect().adjusted(0, -8, 0, 0), Qt::AlignHCenter | Qt::AlignVCenter, QString::number(m_score));

    QFont suffixFont = font();
    suffixFont.setPointSize(9);
    suffixFont.setWeight(QFont::Bold);
    p.setFont(suffixFont);
    p.setPen(QColor("#2563eb"));
    p.drawText(rect().adjusted(0, 12, 0, 0), Qt::AlignHCenter | Qt::AlignVCenter, QStringLiteral("/100"));

    p.setPen(QColor("#047857"));
    p.drawText(rect().adjusted(0, 42, 0, 0), Qt::AlignHCenter | Qt::AlignVCenter,
               m_caption.isEmpty() ? ringCaption(m_score) : m_caption);
}

GradientValueWidget::GradientValueWidget(QWidget *parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    setFixedHeight(68);
}

void GradientValueWidget::setValue(int value)
{
    m_value = qBound(0, value, 100);
    updateGeometry();
    update();
}

int GradientValueWidget::value() const
{
    return m_value;
}

QSize GradientValueWidget::minimumSizeHint() const
{
    QFont valueFont = font();
    valueFont.setPointSize(48);
    valueFont.setWeight(QFont::Bold);
    QFontMetrics valueMetrics(valueFont);

    QFont suffixFont = font();
    suffixFont.setPointSize(14);
    suffixFont.setWeight(QFont::Bold);
    QFontMetrics suffixMetrics(suffixFont);

    return QSize(valueMetrics.horizontalAdvance(QString::number(m_value)) + suffixMetrics.horizontalAdvance("/100") + 10, 68);
}

void GradientValueWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QString valueText = QString::number(m_value);
    const QString suffixText = QStringLiteral("/100");

    QFont valueFont = font();
    valueFont.setPointSize(48);
    valueFont.setWeight(QFont::Bold);
    QFontMetrics valueMetrics(valueFont);

    QFont suffixFont = font();
    suffixFont.setPointSize(14);
    suffixFont.setWeight(QFont::Bold);
    QFontMetrics suffixMetrics(suffixFont);

    const int spacing = 6;
    const int baseline = (height() + valueMetrics.ascent() - valueMetrics.descent()) / 2;
    const int totalWidth = valueMetrics.horizontalAdvance(valueText) + spacing + suffixMetrics.horizontalAdvance(suffixText);
    const int startX = (width() - totalWidth) / 2;

    QPainterPath path;
    path.addText(startX, baseline, valueFont, valueText);

    QLinearGradient gradient(0, 0, width(), height());
    gradient.setColorAt(0.0, QColor("#10b981"));
    gradient.setColorAt(1.0, QColor("#34d399"));
    p.fillPath(path, gradient);

    p.setFont(suffixFont);
    p.setPen(QColor("#2563eb"));
    p.drawText(startX + valueMetrics.horizontalAdvance(valueText) + spacing,
               (height() + suffixMetrics.ascent() - suffixMetrics.descent()) / 2 + 8,
               suffixText);
}
FishVisionPage::FishVisionPage(QWidget *parent)
    : QWidget(parent), m_scoreTimer(new QTimer(this)),
      m_networkManager(new QNetworkAccessManager(this))
{
    PecheFontUtils::applyModuleFont(this);

    setupUi();
    setupStyleSheet();

    connect(btnRetour, &QPushButton::clicked, this, &FishVisionPage::onRetourClicked);
    connect(btnNouvelleAnalyse, &QPushButton::clicked, this, &FishVisionPage::onNouvelleAnalyseClicked);
    connect(btnNouvelleAnalyseBas, &QPushButton::clicked, this, &FishVisionPage::onNouvelleAnalyseClicked);
    connect(btnImporterImage, &QPushButton::clicked, this, &FishVisionPage::onImporterImageClicked);
    connect(btnLancerDemo, &QPushButton::clicked, this, &FishVisionPage::onLancerDemoClicked);
    connect(btnOngletFraicheur, &QPushButton::clicked, this, &FishVisionPage::onTabFraicheurClicked);
    connect(btnOngletRapport, &QPushButton::clicked, this, &FishVisionPage::onTabRapportClicked);
    connect(btnOngletMarche, &QPushButton::clicked, this, &FishVisionPage::onTabMarcheClicked);
    connect(btnOngletRecommandations, &QPushButton::clicked, this, &FishVisionPage::onTabRecommandationsClicked);
    connect(btnOngletAssistant, &QPushButton::clicked, this, &FishVisionPage::onTabAssistantClicked);
    connect(btnSauvegarder, &QPushButton::clicked, this, &FishVisionPage::onSauvegarderClicked);
    connect(btnRattacher, &QPushButton::clicked, this, &FishVisionPage::onRattacherClicked);
    connect(btnExporterPdf, &QPushButton::clicked, this, &FishVisionPage::onExporterPdfClicked);
    connect(btnHistorique, &QPushButton::clicked, this, &FishVisionPage::onHistoriqueClicked);
    connect(btnSendChat, &QPushButton::clicked, this, &FishVisionPage::onEnvoyerChatClicked);
    connect(m_scoreTimer, &QTimer::timeout, this, &FishVisionPage::animerScore);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &FishVisionPage::onGeminiReplyFinished);

    afficherEtatVide();
}

FishVisionPage::~FishVisionPage()
{
    if (m_scoreTimer) {
        m_scoreTimer->stop();
    }
}

void FishVisionPage::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. Header
    QWidget *headerBar = new QWidget(this);
    headerBar->setObjectName("headerBar");
    headerBar->setMinimumHeight(64);
    headerBar->setMaximumHeight(64);
    
    QHBoxLayout *headerLayout = new QHBoxLayout(headerBar);
    headerLayout->setContentsMargins(20, 10, 20, 10);
    headerLayout->setSpacing(12);

    btnRetour = new QPushButton("<- Retour", headerBar);
    btnRetour->setObjectName("btnRetour");
    
    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(0);
    lblTitrePage = new QLabel("FishVision AI", headerBar);
    lblTitrePage->setObjectName("lblTitrePage");
    lblSoustitrePage = new QLabel("Analyse visuelle intelligente - Identification - Qualite - Valorisation", headerBar);
    lblSoustitrePage->setObjectName("lblSoustitrePage");
    titleLayout->addWidget(lblTitrePage);
    titleLayout->addWidget(lblSoustitrePage);

    m_headerBadge = new QLabel("Gemini Vision", headerBar);
    m_headerBadge->setObjectName("badgeGeminiTop");

    btnHistorique = new QPushButton(QString::fromUtf8("🕗 Historique"), headerBar);
    btnHistorique->setObjectName("btnSecondary");

    btnNouvelleAnalyse = new QPushButton("+ Nouvelle analyse", headerBar);
    btnNouvelleAnalyse->setObjectName("btnNouvelleAnalyse");

    headerLayout->addWidget(btnRetour);
    headerLayout->addLayout(titleLayout);
    headerLayout->addWidget(m_headerBadge);
    headerLayout->addStretch();
    headerLayout->addWidget(btnHistorique);
    headerLayout->addWidget(btnNouvelleAnalyse);

    mainLayout->addWidget(headerBar);

    // 2. Stacked Content
    stackedContent = new QStackedWidget(this);
    stackedContent->setObjectName("stackedContent");
    mainLayout->addWidget(stackedContent);

    // 2.1 Empty Page
    pageVide = new QWidget();
    pageVide->setObjectName("pageVide");
    QVBoxLayout *emptyLayout = new QVBoxLayout(pageVide);
    emptyLayout->setContentsMargins(20, 14, 20, 20);
    emptyLayout->setSpacing(12);

    frameUpload = new QFrame(pageVide);
    frameUpload->setObjectName("carteUpload");
    QVBoxLayout *uploadLayout = new QVBoxLayout(frameUpload);
    uploadLayout->setContentsMargins(20, 38, 20, 38);
    uploadLayout->setSpacing(10);
    
    lblIconePoisson = new QLabel(QString::fromUtf8("\xf0\x9f\x90\x9f"), frameUpload);
    lblIconePoisson->setObjectName("lblIconePoisson");
    lblIconePoisson->setAlignment(Qt::AlignCenter);
    
    labelUploadTitre = new QLabel("Deposer une photo de poisson pour analyser", frameUpload);
    labelUploadTitre->setObjectName("labelUploadTitre");
    labelUploadTitre->setAlignment(Qt::AlignCenter);
    
    labelUploadSousTitre = new QLabel("JPEG, PNG - Glisser-deposer ou cliquer", frameUpload);
    labelUploadSousTitre->setObjectName("labelUploadSousTitre");
    labelUploadSousTitre->setAlignment(Qt::AlignCenter);
    
    btnImporterImage = new QPushButton("Importer une image", frameUpload);
    btnImporterImage->setObjectName("btnImporterImage");
    
    btnLancerDemo = new QPushButton("-> Lancer la demo", frameUpload);
    btnLancerDemo->setObjectName("btnLancerDemo");

    uploadLayout->addWidget(lblIconePoisson, 0, Qt::AlignHCenter);
    uploadLayout->addWidget(labelUploadTitre, 0, Qt::AlignHCenter);
    uploadLayout->addWidget(labelUploadSousTitre, 0, Qt::AlignHCenter);
    uploadLayout->addWidget(btnImporterImage, 0, Qt::AlignHCenter);
    uploadLayout->addWidget(btnLancerDemo, 0, Qt::AlignHCenter);

    lblSectionTitle = new QLabel("Analyses recentes", pageVide);
    lblSectionTitle->setObjectName("lblSectionTitle");
    
    layoutRecents = new QHBoxLayout();
    layoutRecents->setSpacing(14);
    
    emptyLayout->addWidget(frameUpload);
    emptyLayout->addWidget(lblSectionTitle);
    emptyLayout->addLayout(layoutRecents);
    emptyLayout->addStretch();

    stackedContent->addWidget(pageVide);

    // 2.2 Result Page
    pageResultat = new QWidget();
    pageResultat->setObjectName("pageResultat");
    QVBoxLayout *resultPageLayout = new QVBoxLayout(pageResultat);
    resultPageLayout->setContentsMargins(0, 0, 0, 0);

    scrollResultat = new QScrollArea(pageResultat);
    scrollResultat->setObjectName("scrollResultat");
    scrollResultat->setWidgetResizable(true);
    scrollResultat->setFrameShape(QFrame::NoFrame);

    scrollAreaWidgetContents = new QWidget();
    scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
    QVBoxLayout *scrollContentLayout = new QVBoxLayout(scrollAreaWidgetContents);
    scrollContentLayout->setContentsMargins(20, 12, 20, 18);
    scrollContentLayout->setSpacing(16);

    carteResultatHeader = new QFrame(scrollAreaWidgetContents);
    carteResultatHeader->setObjectName("carteResultatHeader");
    carteResultatHeader->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    QHBoxLayout *resultHeaderLayout = new QHBoxLayout(carteResultatHeader);
    resultHeaderLayout->setContentsMargins(16, 16, 16, 16);
    resultHeaderLayout->setSpacing(18);

    lblImagePoisson = new QLabel(QString::fromUtf8("\xf0\x9f\x90\xa0"), carteResultatHeader);
    lblImagePoisson->setObjectName("lblImagePoisson");
    lblImagePoisson->setMinimumSize(90, 90);
    lblImagePoisson->setMaximumSize(90, 90);
    lblImagePoisson->setAlignment(Qt::AlignCenter);

    QVBoxLayout *resultInfoLayout = new QVBoxLayout();
    resultInfoLayout->setSpacing(8);

    QHBoxLayout *speciesTitleLayout = new QHBoxLayout();
    speciesTitleLayout->setSpacing(8);
    lblNomEspece = new QLabel("Dorade Royale", carteResultatHeader);
    lblNomEspece->setObjectName("lblNomEspece");
    badgeAI = new QLabel("IA identifiee", carteResultatHeader);
    badgeAI->setObjectName("badgeAI");
    lblGrade = new QLabel("Grade A", carteResultatHeader);
    lblGrade->setObjectName("gradeBadge");
    speciesTitleLayout->addWidget(lblNomEspece);
    speciesTitleLayout->addWidget(badgeAI);
    speciesTitleLayout->addWidget(lblGrade);
    speciesTitleLayout->addStretch();

    lblNomScientifique = new QLabel("Sparus aurata", carteResultatHeader);
    lblNomScientifique->setObjectName("lblNomScientifique");
    
    layoutChipsHeader = new QHBoxLayout();
    layoutChipsHeader->setSpacing(8);
    
    resultInfoLayout->addLayout(speciesTitleLayout);
    resultInfoLayout->addWidget(lblNomScientifique);
    resultInfoLayout->addLayout(layoutChipsHeader);

    m_scoreRing = new ScoreRingWidget(carteResultatHeader);
    m_scoreRing->setObjectName("widgetScoreRingCanvas");
    m_scoreRing->setFixedSize(96, 96);

    resultHeaderLayout->addWidget(lblImagePoisson);
    resultHeaderLayout->addLayout(resultInfoLayout);
    resultHeaderLayout->addStretch();
    resultHeaderLayout->addWidget(m_scoreRing);

    scrollContentLayout->addWidget(carteResultatHeader);

    // Tabs Bar
    barreOnglets = new QWidget(scrollAreaWidgetContents);
    barreOnglets->setObjectName("tabsContainer");
    QHBoxLayout *tabsBarLayout = new QHBoxLayout(barreOnglets);
    tabsBarLayout->setContentsMargins(0, 0, 0, 0);
    tabsBarLayout->setSpacing(0);
    
    btnOngletFraicheur = new QPushButton("Fraicheur", barreOnglets);
    btnOngletFraicheur->setObjectName("btnOngletFraicheur");
    btnOngletRapport = new QPushButton("Rapport IA", barreOnglets);
    btnOngletRapport->setObjectName("btnOngletRapport");
    btnOngletMarche = new QPushButton("Valeur Marche", barreOnglets);
    btnOngletMarche->setObjectName("btnOngletMarche");
    btnOngletRecommandations = new QPushButton("Recommandations", barreOnglets);
    btnOngletRecommandations->setObjectName("btnOngletRecommandations");
    btnOngletAssistant = new QPushButton("Assistant IA", barreOnglets);
    btnOngletAssistant->setObjectName("btnOngletAssistant");
    
    tabsBarLayout->addWidget(btnOngletFraicheur);
    tabsBarLayout->addWidget(btnOngletRapport);
    tabsBarLayout->addWidget(btnOngletMarche);
    tabsBarLayout->addWidget(btnOngletRecommandations);
    tabsBarLayout->addWidget(btnOngletAssistant);
    tabsBarLayout->addStretch();

    scrollContentLayout->addWidget(barreOnglets);

    // Stack Onglets
    stackOnglets = new QStackedWidget(scrollAreaWidgetContents);
    stackOnglets->setObjectName("stackOnglets");

    // Page Fraicheur
    pageFraicheur = new QWidget();
    pageFraicheur->setObjectName("pageFraicheur");
    
    QVBoxLayout *freshnessPageLayout = new QVBoxLayout(pageFraicheur);
    freshnessPageLayout->setContentsMargins(0, 0, 0, 0);
    freshnessPageLayout->setSpacing(16);
    
    QHBoxLayout *freshnessRow = new QHBoxLayout();
    freshnessRow->setContentsMargins(0, 0, 0, 0);
    freshnessRow->setSpacing(16);

    carteAnalyseIndicateurs = new QFrame(pageFraicheur);
    carteAnalyseIndicateurs->setObjectName("carteAnalyse");
    carteAnalyseIndicateurs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    QVBoxLayout *indicatorsLayout = new QVBoxLayout(carteAnalyseIndicateurs);
    indicatorsLayout->setContentsMargins(16, 16, 16, 16);
    indicatorsLayout->setSpacing(10);
    
    labelIndicateurs = new QLabel("Indicateurs visuels", carteAnalyseIndicateurs);
    labelOeil = new QLabel("Clarte de l'oeil", carteAnalyseIndicateurs);
    barreOeil = new QProgressBar(carteAnalyseIndicateurs);
    barreOeil->setObjectName("barreOeil");
    lblValOeil = new QLabel("0", carteAnalyseIndicateurs);
    
    labelBranchies = new QLabel("Couleur des branchies", carteAnalyseIndicateurs);
    barreBranchies = new QProgressBar(carteAnalyseIndicateurs);
    barreBranchies->setObjectName("barreBranchies");
    lblValBranchies = new QLabel("0", carteAnalyseIndicateurs);
    
    labelPeau = new QLabel("Brillance de la peau", carteAnalyseIndicateurs);
    barrePeau = new QProgressBar(carteAnalyseIndicateurs);
    barrePeau->setObjectName("barrePeau");
    lblValPeau = new QLabel("0", carteAnalyseIndicateurs);
    
    lblNote = new QLabel("Lecture automatisee des marqueurs visuels.", carteAnalyseIndicateurs);
    lblNote->setObjectName("lblNote");
    lblNote->setWordWrap(true);

    auto addIndicatorRow = [indicatorsLayout](QLabel* lbl, QProgressBar* bar, QLabel* val) {
        indicatorsLayout->addWidget(lbl);
        QHBoxLayout* hl = new QHBoxLayout();
        hl->addWidget(bar);
        hl->addWidget(val);
        indicatorsLayout->addLayout(hl);
    };

    indicatorsLayout->addWidget(labelIndicateurs);
    addIndicatorRow(labelOeil, barreOeil, lblValOeil);
    addIndicatorRow(labelBranchies, barreBranchies, lblValBranchies);
    addIndicatorRow(labelPeau, barrePeau, lblValPeau);
    indicatorsLayout->addWidget(lblNote);

    carteVerdict = new QFrame(pageFraicheur);
    carteVerdict->setObjectName("carteAnalyse");
    carteVerdict->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    QVBoxLayout *verdictLayout = new QVBoxLayout(carteVerdict);
    verdictLayout->setContentsMargins(16, 16, 16, 16);
    verdictLayout->setSpacing(10);

    labelVerdict = new QLabel("Verdict fraicheur", carteVerdict);
    verdictHeader = new QHBoxLayout();
    verdictHeader->setSpacing(14);
    
    m_verdictScore = new GradientValueWidget(carteVerdict);
    m_verdictScore->setObjectName("verdictScoreWidget");
    
    QVBoxLayout *verdictTextLayout = new QVBoxLayout();
    verdictTextLayout->setSpacing(2);
    lblVerdictTitre = new QLabel("Qualite excellente", carteVerdict);
    lblVerdictTitre->setObjectName("lblVerdictTitre");
    lblVerdictGrade = new QLabel("Grade A", carteVerdict);
    lblVerdictGrade->setObjectName("lblVerdictGrade");
    verdictTextLayout->addWidget(lblVerdictTitre);
    verdictTextLayout->addWidget(lblVerdictGrade);
    
    verdictHeader->addWidget(m_verdictScore);
    verdictHeader->addLayout(verdictTextLayout);

    QFrame *lineVerdictTop = new QFrame(carteVerdict);
    lineVerdictTop->setObjectName("lineVerdictTop");
    layoutTimeline = new QVBoxLayout();
    layoutTimeline->setSpacing(10);
    QFrame *lineVerdictBottom = new QFrame(carteVerdict);
    lineVerdictBottom->setObjectName("lineVerdictBottom");

    lblAgeCapture = new QLabel("Age de capture estime :", carteVerdict);
    lblAgeCapture->setObjectName("lblAgeCapture");
    lblFenetreVente = new QLabel("0-24h", carteVerdict);
    lblFenetreVente->setObjectName("lblFenetreVente");

    verdictLayout->addWidget(labelVerdict);
    verdictLayout->addLayout(verdictHeader);
    verdictLayout->addWidget(lineVerdictTop);
    verdictLayout->addLayout(layoutTimeline);
    verdictLayout->addWidget(lineVerdictBottom);
    verdictLayout->addWidget(lblAgeCapture);
    verdictLayout->addWidget(lblFenetreVente);

    freshnessRow->addWidget(carteAnalyseIndicateurs);
    freshnessRow->addWidget(carteVerdict);
    
    freshnessPageLayout->addLayout(freshnessRow);
    freshnessPageLayout->addStretch();
    
    stackOnglets->addWidget(pageFraicheur);

    // Page Rapport
    pageRapport = new QWidget();
    pageRapport->setObjectName("pageRapport");
    QVBoxLayout *reportLayout = new QVBoxLayout(pageRapport);
    reportLayout->setContentsMargins(0, 0, 0, 0);
    reportLayout->setSpacing(16);

    carteRapportAI = new QFrame(pageRapport);
    carteRapportAI->setObjectName("carteAnalyse");
    carteRapportAI->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    QVBoxLayout *reportCardLayout = new QVBoxLayout(carteRapportAI);
    reportCardLayout->setContentsMargins(16, 16, 16, 16);
    reportCardLayout->setSpacing(10);

    blocRapportAI = new QFrame(carteRapportAI);
    blocRapportAI->setObjectName("blocRapportAI");
    QVBoxLayout *aiBlockLayout = new QVBoxLayout(blocRapportAI);
    aiBlockLayout->setContentsMargins(16, 16, 16, 16);
    aiBlockLayout->setSpacing(6);
    labelRapportTitre = new QLabel("Rapport d'analyse IA", blocRapportAI);
    labelRapportTitre->setObjectName("labelRapportTitre");
    lblRapportTexte = new QLabel("", blocRapportAI);
    lblRapportTexte->setObjectName("lblRapportTexte");
    lblRapportTexte->setWordWrap(true);
    aiBlockLayout->addWidget(labelRapportTitre);
    aiBlockLayout->addWidget(lblRapportTexte);

    QFrame *lineRapport = new QFrame(carteRapportAI);
    lineRapport->setObjectName("lineRapport");
    layoutRapportChips = new QHBoxLayout();
    layoutRapportChips->setSpacing(8);

    reportCardLayout->addWidget(blocRapportAI);
    reportCardLayout->addWidget(lineRapport);
    reportCardLayout->addLayout(layoutRapportChips);

    carteFlags = new QFrame(pageRapport);
    carteFlags->setObjectName("carteAnalyse");
    carteFlags->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    QVBoxLayout *flagsLayout = new QVBoxLayout(carteFlags);
    flagsLayout->setContentsMargins(16, 16, 16, 16);
    flagsLayout->setSpacing(10);
    labelFlags = new QLabel("Indicateurs de detection", carteFlags);
    labelFlags->setObjectName("labelFlags");
    layoutDetectionBadges = new QHBoxLayout();
    layoutDetectionBadges->setSpacing(8);
    flagsLayout->addWidget(labelFlags);
    flagsLayout->addLayout(layoutDetectionBadges);

    reportLayout->addWidget(carteRapportAI);
    reportLayout->addWidget(carteFlags);
    reportLayout->addStretch();
    stackOnglets->addWidget(pageRapport);

    // Page Marche
    pageMarche = new QWidget();
    pageMarche->setObjectName("pageMarche");
    QVBoxLayout *marketLayout = new QVBoxLayout(pageMarche);
    marketLayout->setContentsMargins(0, 0, 0, 0);
    marketLayout->setSpacing(16);

    QHBoxLayout *marketCardsRow = new QHBoxLayout();
    marketCardsRow->setSpacing(16);

    carteValeurEstimee = new QFrame(pageMarche);
    carteValeurEstimee->setObjectName("carteAnalyse");
    carteValeurEstimee->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    QVBoxLayout *valueCardLayout = new QVBoxLayout(carteValeurEstimee);
    valueCardLayout->setContentsMargins(16, 16, 16, 16);
    valueCardLayout->setSpacing(8);
    labelValeur = new QLabel("Valeur estimee", carteValeurEstimee);
    lblPrixKg = new QLabel("14.20 TND", carteValeurEstimee);
    lblPrixKg->setObjectName("lblPrixKg");
    labelValeurSousTitre = new QLabel("par kilogramme", carteValeurEstimee);
    labelValeurSousTitre->setObjectName("labelValeurSousTitre");
    QFrame *lineValeur = new QFrame(carteValeurEstimee);
    lineValeur->setObjectName("lineValeur");
    labelFourchette = new QLabel("Fourchette marche: 12.00 – 16.50 TND/kg", carteValeurEstimee);
    labelFourchette->setObjectName("labelFourchette");
    valueCardLayout->addWidget(labelValeur);
    valueCardLayout->addWidget(lblPrixKg);
    valueCardLayout->addWidget(labelValeurSousTitre);
    valueCardLayout->addWidget(lineValeur);
    valueCardLayout->addWidget(labelFourchette);

    cartePositionnement = new QFrame(pageMarche);
    cartePositionnement->setObjectName("carteAnalyse");
    cartePositionnement->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    QVBoxLayout *positionCardLayout = new QVBoxLayout(cartePositionnement);
    positionCardLayout->setContentsMargins(16, 16, 16, 16);
    positionCardLayout->setSpacing(10);
    labelPositionnement = new QLabel("Positionnement marche", cartePositionnement);
    layoutPositionnement = new QVBoxLayout();
    layoutPositionnement->setSpacing(10);
    QFrame *linePositionnement = new QFrame(cartePositionnement);
    linePositionnement->setObjectName("linePositionnement");
    labelPositionnementNote = new QLabel("Prix dynamique", cartePositionnement);
    positionCardLayout->addWidget(labelPositionnement);
    positionCardLayout->addLayout(layoutPositionnement);
    positionCardLayout->addWidget(linePositionnement);
    positionCardLayout->addWidget(labelPositionnementNote);

    marketCardsRow->addWidget(carteValeurEstimee);
    marketCardsRow->addWidget(cartePositionnement);

    carteAcheteurs = new QFrame(pageMarche);
    carteAcheteurs->setObjectName("carteAnalyse");
    carteAcheteurs->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    QVBoxLayout *buyersLayout = new QVBoxLayout(carteAcheteurs);
    buyersLayout->setContentsMargins(16, 16, 16, 16);
    buyersLayout->setSpacing(10);
    labelCanaux = new QLabel("Canaux de vente suggeres", carteAcheteurs);
    labelCanaux->setObjectName("labelCanaux");
    layoutCanauxVente = new QHBoxLayout();
    layoutCanauxVente->setSpacing(0);
    buyersLayout->addWidget(labelCanaux);
    buyersLayout->addLayout(layoutCanauxVente);

    marketLayout->addLayout(marketCardsRow);
    marketLayout->addWidget(carteAcheteurs);
    marketLayout->addStretch();
    stackOnglets->addWidget(pageMarche);

    // Page Recommandations
    pageRecommandations = new QWidget();
    pageRecommandations->setObjectName("pageRecommandations");
    QVBoxLayout *recommendationsPageLayout = new QVBoxLayout(pageRecommandations);
    recommendationsPageLayout->setContentsMargins(0, 0, 0, 0);
    recommendationsPageLayout->setSpacing(16);

    carteRecommandations = new QFrame(pageRecommandations);
    carteRecommandations->setObjectName("carteAnalyse");
    carteRecommandations->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    QVBoxLayout *recommendationCardLayout = new QVBoxLayout(carteRecommandations);
    recommendationCardLayout->setContentsMargins(16, 16, 16, 16);
    recommendationCardLayout->setSpacing(8);
    labelRecommandations = new QLabel("Recommandations intelligentes", carteRecommandations);
    layoutRecommandations = new QVBoxLayout();
    layoutRecommandations->setSpacing(0);
    recommendationCardLayout->addWidget(labelRecommandations);
    recommendationCardLayout->addLayout(layoutRecommandations);

    QHBoxLayout *actionsRow = new QHBoxLayout();
    actionsRow->setSpacing(10);
    btnSauvegarder = new QPushButton("Sauvegarder en base", pageRecommandations);
    btnSauvegarder->setObjectName("btnSauvegarder");
    btnRattacher = new QPushButton("Rattacher a une peche", pageRecommandations);
    btnRattacher->setObjectName("btnRattacher");
    btnExporterPdf = new QPushButton("Exporter rapport PDF", pageRecommandations);
    btnExporterPdf->setObjectName("btnExporterPdf");
    btnNouvelleAnalyseBas = new QPushButton("Nouvelle analyse", pageRecommandations);
    btnNouvelleAnalyseBas->setObjectName("btnNouvelleAnalyseBas");
    actionsRow->addWidget(btnSauvegarder);
    actionsRow->addWidget(btnRattacher);
    actionsRow->addWidget(btnExporterPdf);
    actionsRow->addWidget(btnNouvelleAnalyseBas);
    actionsRow->addStretch();

    recommendationsPageLayout->addWidget(carteRecommandations);
    recommendationsPageLayout->addLayout(actionsRow);
    recommendationsPageLayout->addStretch();
    stackOnglets->addWidget(pageRecommandations);

    // Page Assistant IA
    pageAssistant = new QWidget();
    pageAssistant->setObjectName("pageAssistant");
    QVBoxLayout *assistantLayout = new QVBoxLayout(pageAssistant);
    assistantLayout->setContentsMargins(0, 0, 0, 0);
    assistantLayout->setSpacing(16);

    QFrame *carteAssistant = new QFrame(pageAssistant);
    carteAssistant->setObjectName("carteAnalyse");
    carteAssistant->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    QVBoxLayout *chatCardLayout = new QVBoxLayout(carteAssistant);
    chatCardLayout->setContentsMargins(16, 16, 16, 16);
    chatCardLayout->setSpacing(10);
    
    QLabel *lblChatTitre = new QLabel("Discutez avec Gemini AI", carteAssistant);
    lblChatTitre->setObjectName("labelRapportTitre");
    
    chatHistory = new QTextEdit(carteAssistant);
    chatHistory->setObjectName("chatHistory");
    chatHistory->setReadOnly(true);
    chatHistory->setMinimumHeight(180);
    chatHistory->setMaximumHeight(260);
    chatHistory->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    chatHistory->setStyleSheet(QStringLiteral("QTextEdit { background: rgba(255, 255, 255, 0.5); border: 1px solid rgba(255, 255, 255, 0.8); border-radius: 8px; color: #1e3a8a; font-size: 13px; padding: 10px; }"));
    
    QHBoxLayout *chatInputRow = new QHBoxLayout();
    chatInputRow->setSpacing(8);
    chatInput = new QLineEdit(carteAssistant);
    chatInput->setObjectName("chatInput");
    chatInput->setPlaceholderText("Posez une question sur le poisson analyse (marinade, peche, saison)...");
    chatInput->setMinimumHeight(44);
    chatInput->setStyleSheet(QStringLiteral("QLineEdit { background: rgba(255, 255, 255, 0.6); border: 1px solid rgba(255, 255, 255, 0.8); border-radius: 18px; color: #1e3a8a; padding: 8px 15px; } QLineEdit:focus { border: 1px solid #38bdf8; }"));
    connect(chatInput, &QLineEdit::returnPressed, this, &FishVisionPage::onEnvoyerChatClicked);
    
    btnSendChat = new QPushButton("Envoyer", carteAssistant);
    btnSendChat->setObjectName("btnPrimary");
    btnSendChat->setCursor(Qt::PointingHandCursor);
    
    chatInputRow->addWidget(chatInput, 1);
    chatInputRow->addWidget(btnSendChat, 0);
    
    chatCardLayout->addWidget(lblChatTitre);
    chatCardLayout->addWidget(chatHistory, 1);
    chatCardLayout->addLayout(chatInputRow);
    
    assistantLayout->addWidget(carteAssistant, 0, Qt::AlignTop);
    assistantLayout->addStretch();
    stackOnglets->addWidget(pageAssistant);

    scrollContentLayout->addWidget(stackOnglets);
    scrollResultat->setWidget(scrollAreaWidgetContents);
    resultPageLayout->addWidget(scrollResultat);
    stackedContent->addWidget(pageResultat);
    
    labelRapportTitre->setVisible(false);

    for (QProgressBar *bar : {barreOeil, barreBranchies, barrePeau}) {
        if (!bar) continue;
        bar->setTextVisible(false);
        bar->setRange(0, 100);
        bar->setProperty("warning", false);
    }
    
    // Extra elements replaced
    btnNouvelleAnalyse->setObjectName("btnPrimary");
    btnImporterImage->setObjectName("btnSecondary");
    btnRattacher->setObjectName("btnSecondary");
    btnExporterPdf->setObjectName("btnSecondary");
    btnNouvelleAnalyseBas->setObjectName("btnSecondary");
    btnLancerDemo->setObjectName("btnTextLink");
}

void FishVisionPage::setupStyleSheet()
{
    QFile file(":/peche/fishvision/fishvisionpage.qss");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(file.readAll()));
    }
}

void FishVisionPage::afficherEtatVide()
{
    m_resetInProgress = true;
    const QList<QNetworkReply *> replies = findChildren<QNetworkReply *>();
    for (QNetworkReply *reply : replies) {
        reply->setProperty("ignoreResult", true);
        if (reply->isRunning()) {
            reply->abort();
        }
    }

    m_scoreTimer->stop();
    m_result = FishVisionResult();
    m_scoreAnimCurrent = 0;
    m_loadedPixmap = QPixmap();
    m_currentImagePath.clear();
    m_isLoading = false;
    m_scoreRing->setScore(0);
    m_scoreRing->setCaption(QString());
    m_verdictScore->setValue(0);
    setFishImage(QPixmap());

    stackedContent->setCurrentIndex(0);
    stackOnglets->setCurrentIndex(0);
    mettreAJourOngletActif(0);
    setLoadingUiEnabled(true);

    labelUploadTitre->setText(QStringLiteral("Deposer une photo de poisson pour analyser"));
    labelUploadSousTitre->setText(QStringLiteral("JPEG, PNG - analyse reelle via Gemini"));
    lblNomEspece->setText(QStringLiteral("Analyse en attente"));
    lblNomScientifique->setText(QStringLiteral("Importez une image pour lancer Gemini"));
    lblRapportTexte->clear();
    lblPrixKg->setText(QStringLiteral("-- TND"));
    labelFourchette->setText(QStringLiteral("Fourchette marche: --"));
    applyGradeStyle(QStringLiteral("idle"), QStringLiteral("Grade --"));
    updateVerdictWidgets(0, QStringLiteral("Aucune analyse lancee"), QStringLiteral("Les donnees reelles apparaitront ici"));
    lblAgeCapture->setText(QStringLiteral("Poids estime : -- kg"));
    lblFenetreVente->setText(QStringLiteral("En attente du verdict Gemini"));
    
    chatHistory->clear();
    appendChatBotMessage("Bonjour ! Je suis l'assistant IA de DataDock. Une fois l'analyse terminee, vous pourrez me poser des questions sur l'espece identifiee (saisonnalite, conseils de peche, marche, preparations...).", false);
    chatInput->clear();
    chatInput->setEnabled(false);
    btnSendChat->setEnabled(false);
    m_currentAnalysisId = 0;
    m_currentLinkedPecheId = 0;

    barreOeil->setValue(0);
    barreBranchies->setValue(0);
    barrePeau->setValue(0);
    updateBarTone(barreOeil, 0);
    updateBarTone(barreBranchies, 0);
    updateBarTone(barrePeau, 0);
    lblValOeil->setText(QStringLiteral("0"));
    lblValBranchies->setText(QStringLiteral("0"));
    lblValPeau->setText(QStringLiteral("0"));

    clearLayoutWidgets(layoutChipsHeader);
    layoutChipsHeader->addWidget(creerBadge(QStringLiteral("IA prete"), QStringLiteral("blue")));
    layoutChipsHeader->addWidget(creerBadge(QStringLiteral("Vision active"), QStringLiteral("violet")));
    layoutChipsHeader->addStretch();

    clearLayoutWidgets(layoutTimeline);
    layoutTimeline->addWidget(creerItemTimeline(QStringLiteral("#38bdf8"), QStringLiteral("Chargez une image pour lancer l'analyse.")));
    layoutTimeline->addWidget(creerItemTimeline(QStringLiteral("#f59e0b"), QStringLiteral("Le verdict detaille apparaitra apres la reponse de Gemini.")));

    clearLayoutWidgets(layoutRapportChips);
    layoutRapportChips->addWidget(creerBadge(QStringLiteral("Aucune donnee"), QStringLiteral("blue")));
    layoutRapportChips->addStretch();

    clearLayoutWidgets(layoutDetectionBadges);
    layoutDetectionBadges->addWidget(creerIndicatorCard({QStringLiteral("Indicateurs en attente d'analyse"), QStringLiteral("warning")}));
    layoutDetectionBadges->addStretch();

    clearLayoutWidgets(layoutPositionnement);
    layoutPositionnement->addWidget(creerPositionnementItem({QStringLiteral("Positionnement"), QStringLiteral("n/a"), QStringLiteral("neutre")}));

    clearLayoutWidgets(layoutCanauxVente);
    auto *emptyCanal = new QWidget(this);
    auto *emptyGrid = new QGridLayout(emptyCanal);
    emptyGrid->setContentsMargins(0, 0, 0, 0);
    emptyGrid->setHorizontalSpacing(12);
    emptyGrid->setVerticalSpacing(12);
    for (int i = 0; i < 4; ++i) {
        emptyGrid->addWidget(creerCanalBadge(i == 0 ? QStringLiteral("Aucun canal") : QStringLiteral("Canal a confirmer")), i / 2, i % 2);
    }
    layoutCanauxVente->addWidget(emptyCanal);

    clearLayoutWidgets(layoutRecommandations);
    layoutRecommandations->addWidget(creerItemRecommandation(QString(), QStringLiteral("Analyse en attente"),
                                                                 QStringLiteral("Les recommandations dependront de la reponse du modele.")));

    clearLayoutWidgets(layoutRecents);
    layoutRecents->addWidget(creerCarteRecente(QStringLiteral("Gemini actif"), 100, QStringLiteral("API connectee"), QStringLiteral("#34d399")));
    layoutRecents->addWidget(creerCarteRecente(QStringLiteral("JSON structure"), 100, QStringLiteral("Parsing direct"), QStringLiteral("#38bdf8")));
    layoutRecents->addWidget(creerCarteRecente(QStringLiteral("Mode live"), 100, QStringLiteral("Aucune mock data"), QStringLiteral("#a78bfa")));

    m_resetInProgress = false;
}

void FishVisionPage::afficherChargement(const QString &sourceLabel)
{
    m_isLoading = true;
    m_scoreTimer->stop();
    m_scoreAnimCurrent = 0;
    m_scoreRing->setScore(0);
    m_scoreRing->setCaption(QStringLiteral("Analyse"));
    m_verdictScore->setValue(0);
    setLoadingUiEnabled(false);

    stackedContent->setCurrentIndex(1);
    stackOnglets->setCurrentIndex(0);
    mettreAJourOngletActif(0);

    lblNomEspece->setText(QStringLiteral("Analyse Gemini en cours"));
    lblNomScientifique->setText(sourceLabel);
    applyGradeStyle(QStringLiteral("loading"), QStringLiteral("Traitement"));
    lblRapportTexte->setText(QStringLiteral("L'image est encodee en base64 puis envoyee a Gemini Vision pour produire un rapport JSON."));
    lblPrixKg->setText(QStringLiteral("Analyse..."));
    labelFourchette->setText(QStringLiteral("Fourchette marche: calcul en cours"));
    updateVerdictWidgets(0, QStringLiteral("Analyse de fraicheur en cours"), QStringLiteral("Attente de la reponse API"));
    lblAgeCapture->setText(QStringLiteral("Poids estime : calcul en cours"));
    lblFenetreVente->setText(QStringLiteral("Gemini inspecte l'image chargee"));

    barreOeil->setValue(0);
    barreBranchies->setValue(0);
    barrePeau->setValue(0);
    updateBarTone(barreOeil, 0);
    updateBarTone(barreBranchies, 0);
    updateBarTone(barrePeau, 0);
    lblValOeil->setText(QStringLiteral("--"));
    lblValBranchies->setText(QStringLiteral("--"));
    lblValPeau->setText(QStringLiteral("--"));

    clearLayoutWidgets(layoutChipsHeader);
    layoutChipsHeader->addWidget(creerBadge(QStringLiteral("Upload termine"), QStringLiteral("green")));
    layoutChipsHeader->addWidget(creerBadge(QStringLiteral("Gemini analyse"), QStringLiteral("violet")));
    layoutChipsHeader->addStretch();

    clearLayoutWidgets(layoutTimeline);
    layoutTimeline->addWidget(creerItemTimeline(QStringLiteral("#38bdf8"), QStringLiteral("Requete HTTP POST en cours vers Gemini 2.5 Flash.")));
    layoutTimeline->addWidget(creerItemTimeline(QStringLiteral("#34d399"), QStringLiteral("Le score final sera calcule des que le JSON sera valide.")));

    clearLayoutWidgets(layoutRapportChips);
    layoutRapportChips->addWidget(creerBadge(QStringLiteral("JSON attendu"), QStringLiteral("blue")));
    layoutRapportChips->addStretch();

    clearLayoutWidgets(layoutDetectionBadges);
    layoutDetectionBadges->addWidget(creerIndicatorCard({QStringLiteral("Analyse visuelle en cours"), QStringLiteral("warning")}));
    layoutDetectionBadges->addStretch();

    clearLayoutWidgets(layoutPositionnement);
    layoutPositionnement->addWidget(creerPositionnementItem({QStringLiteral("Positionnement"), QStringLiteral("calcul en cours"), QStringLiteral("neutre")}));

    clearLayoutWidgets(layoutCanauxVente);
    auto *loadingCanal = new QWidget(this);
    auto *loadingGrid = new QGridLayout(loadingCanal);
    loadingGrid->setContentsMargins(0, 0, 0, 0);
    loadingGrid->setHorizontalSpacing(12);
    loadingGrid->setVerticalSpacing(12);
    for (int i = 0; i < 4; ++i) {
        loadingGrid->addWidget(creerCanalBadge(i == 0 ? QStringLiteral("Evaluation en cours") : QStringLiteral("Canal a confirmer")), i / 2, i % 2);
    }
    layoutCanauxVente->addWidget(loadingCanal);

    clearLayoutWidgets(layoutRecommandations);
    layoutRecommandations->addWidget(creerItemRecommandation(QString(), QStringLiteral("Traitement IA"),
                                                                 QStringLiteral("Patientez pendant l'analyse de l'image.")));
}

void FishVisionPage::afficherErreur(const QString &message)
{
    m_isLoading = false;
    setLoadingUiEnabled(true);

    applyGradeStyle(QStringLiteral("critical"), QStringLiteral("Erreur API"));
    updateVerdictWidgets(0, QStringLiteral("Analyse indisponible"), message);
    lblRapportTexte->setText(message);

    clearLayoutWidgets(layoutDetectionBadges);
    layoutDetectionBadges->addWidget(creerIndicatorCard({QStringLiteral("Echec Gemini"), QStringLiteral("bad")}));
    layoutDetectionBadges->addStretch();

    QMessageBox::warning(this, QStringLiteral("FishVision AI"), message);
}

void FishVisionPage::afficherResultat(const FishVisionResult &result)
{
    m_result = result;
    m_isLoading = false;
    setLoadingUiEnabled(true);
    chatInput->setEnabled(true);
    btnSendChat->setEnabled(true);
    btnSendChat->setText("Envoyer");

    lblNomEspece->setText(result.espece);
    lblNomScientifique->setText(result.nomScientifique);
    lblRapportTexte->setText(result.rapportIA);
    lblPrixKg->setText(QStringLiteral("%1 TND").arg(result.valeurKg, 0, 'f', 2));
    labelFourchette->setText(
        QStringLiteral("Fourchette marche: %1 - %2 TND/kg").arg(result.fourchetteMin, 0, 'f', 2).arg(result.fourchetteMax, 0, 'f', 2));

    applyGradeStyle(result.grade, QStringLiteral("Grade %1").arg(result.grade));

    barreOeil->setValue(result.scoreOeil);
    barreBranchies->setValue(result.scoreBranchies);
    barrePeau->setValue(result.scorePeau);
    updateBarTone(barreOeil, result.scoreOeil);
    updateBarTone(barreBranchies, result.scoreBranchies);
    updateBarTone(barrePeau, result.scorePeau);
    lblValOeil->setText(QString::number(result.scoreOeil));
    lblValBranchies->setText(QString::number(result.scoreBranchies));
    lblValPeau->setText(QString::number(result.scorePeau));

    updateVerdictWidgets(result.scoreFraicheur, result.verdict,
                         QStringLiteral("Grade %1 - score fraicheur %2/100").arg(result.grade).arg(result.scoreFraicheur));
    lblAgeCapture->setText(QStringLiteral("Poids estime : %1 kg").arg(result.poidsEstime));
    lblFenetreVente->setText(QStringLiteral("Espece analysee : %1").arg(result.espece));

    m_scoreRing->setCaption(captionForGrade(result.grade));
    remplirLayoutsAnalyse(result);

    stackedContent->setCurrentIndex(1);
    stackOnglets->setCurrentIndex(0);
    mettreAJourOngletActif(0);
    m_scoreAnimCurrent = 0;
    m_scoreRing->setScore(0);
    m_scoreTimer->start(12);
    chatInput->setFocus();
}

void FishVisionPage::remplirLayoutsAnalyse(const FishVisionResult &result)
{
    clearLayoutWidgets(layoutChipsHeader);
    for (const QString &tag : result.tags) {
        layoutChipsHeader->addWidget(creerBadge(tag, tagClass(tag)));
    }
    layoutChipsHeader->addStretch();

    clearLayoutWidgets(layoutTimeline);
    layoutTimeline->addWidget(
        creerItemTimeline(statusDotColor(result.scoreFraicheur >= 70 ? QStringLiteral("ok") : result.scoreFraicheur >= 40 ? QStringLiteral("warning") : QStringLiteral("bad")),
                          QStringLiteral("%1 (%2/100)").arg(result.verdict).arg(result.scoreFraicheur)));
    layoutTimeline->addWidget(creerItemTimeline(QStringLiteral("#34d399"), QStringLiteral("Poids estime par Gemini : %1 kg").arg(result.poidsEstime)));
    layoutTimeline->addWidget(creerItemTimeline(QStringLiteral("#f59e0b"), QStringLiteral("Nom scientifique : %1").arg(result.nomScientifique)));

    clearLayoutWidgets(layoutRapportChips);
    for (const FishVisionIndicator &indicator : result.indicateurs) {
        layoutRapportChips->addWidget(creerBadge(indicator.label, badgeClassForStatus(indicator.status)));
    }
    layoutRapportChips->addStretch();

    clearLayoutWidgets(layoutDetectionBadges);
    for (const FishVisionIndicator &indicator : result.indicateurs) {
        layoutDetectionBadges->addWidget(creerIndicatorCard(indicator));
    }
    layoutDetectionBadges->addStretch();

    clearLayoutWidgets(layoutPositionnement);
    for (const FishVisionPositionnement &positionnement : result.positionnement) {
        layoutPositionnement->addWidget(creerPositionnementItem(positionnement));
    }

    clearLayoutWidgets(layoutCanauxVente);
    auto *gridHost = new QWidget(this);
    auto *grid = new QGridLayout(gridHost);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(12);
    const int count = qMax(4, result.canauxVente.size());
    for (int i = 0; i < count; ++i) {
        const QString canal = i < result.canauxVente.size()
                                  ? result.canauxVente.at(i)
                                  : (i == 0 ? QStringLiteral("Canal non precise") : QStringLiteral("Canal a confirmer"));
        grid->addWidget(creerCanalBadge(canal), i / 2, i % 2);
    }
    layoutCanauxVente->addWidget(gridHost);

    clearLayoutWidgets(layoutRecommandations);
    for (const FishVisionRecommendation &recommendation : result.recommandations) {
        layoutRecommandations->addWidget(
            creerItemRecommandation(recommendation.icon, recommendation.titre, recommendation.detail));
    }
}

void FishVisionPage::mettreAJourOngletActif(int tabIndex)
{
    m_activeTab = tabIndex;
    const QList<QPushButton *> buttons = {
        btnOngletFraicheur, btnOngletRapport, btnOngletMarche, btnOngletRecommandations, btnOngletAssistant};
    for (int i = 0; i < buttons.size(); ++i) {
        buttons[i]->setProperty("tabActif", i == tabIndex);
        polishWidget(buttons[i]);
    }
}

void FishVisionPage::setFishImage(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        lblImagePoisson->setPixmap(QPixmap());
        lblImagePoisson->setText(QStringLiteral("FV"));
        return;
    }

    lblImagePoisson->setText(QString());
    const QPixmap scaled = pixmap.scaled(lblImagePoisson->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPixmap rounded(lblImagePoisson->size());
    rounded.fill(Qt::transparent);
    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(rounded.rect(), 12, 12);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, scaled);
    lblImagePoisson->setPixmap(rounded);
}

void FishVisionPage::clearLayoutWidgets(QLayout *layout)
{
    while (layout && layout->count() > 0) {
        QLayoutItem *item = layout->takeAt(0);
        if (!item) {
            continue;
        }

        if (QLayout *childLayout = item->layout()) {
            clearLayoutWidgets(childLayout);
            delete childLayout;
            delete item;
            continue;
        }

        if (QWidget *childWidget = item->widget()) {
            delete childWidget;
            delete item;
            continue;
        }

        delete item;
    }
}

QWidget *FishVisionPage::creerCarteRecente(const QString &espece, int score, const QString &etat, const QString &couleurAccent)
{
    auto *card = new QFrame(this);
    card->setObjectName("carteRecente");
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    auto *top = new QHBoxLayout();
    top->setSpacing(8);

    auto *icon = new QLabel(recentIcon(espece), card);
    icon->setAlignment(Qt::AlignCenter);
    icon->setFixedSize(32, 32);
    icon->setStyleSheet(QStringLiteral("border-radius:10px;background:rgba(255,255,255,0.05);color:%1;font-size:11px;font-weight: 700;")
                            .arg(couleurAccent));

    auto *scoreLabel = new QLabel(QStringLiteral("%1/100").arg(score), card);
    scoreLabel->setStyleSheet(QStringLiteral("font-size:12px;font-weight: 700;color:%1;").arg(couleurAccent));

    auto *title = new QLabel(espece, card);
    title->setWordWrap(true);
    title->setStyleSheet(QStringLiteral("font-size:13px;font-weight: 700;color:#1e3a8a;"));

    auto *state = new QLabel(etat, card);
    state->setWordWrap(true);
    state->setStyleSheet(QStringLiteral("font-size:11px;color:#7c8ca0;"));

    auto *accent = new QFrame(card);
    accent->setFixedHeight(2);
    accent->setStyleSheet(QStringLiteral("background:%1;border:none;border-radius:2px;").arg(couleurAccent));

    top->addWidget(icon);
    top->addStretch();
    top->addWidget(scoreLabel);

    layout->addLayout(top);
    layout->addWidget(title);
    layout->addWidget(state);
    layout->addWidget(accent);
    return card;
}

QWidget *FishVisionPage::creerBadge(const QString &texte, const QString &type)
{
    auto *label = new QLabel(texte, this);
    label->setProperty("badgeType", type);
    label->setObjectName("infoBadge");
    polishWidget(label);
    return label;
}

QWidget *FishVisionPage::creerCanalBadge(const QString &texte)
{
    const QString tone = channelTone(texte);

    auto *card = new QFrame(this);
    card->setObjectName("channelCard");
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);

    auto *icon = new QLabel(channelIcon(texte), card);
    icon->setAlignment(Qt::AlignCenter);
    icon->setFixedSize(32, 32);
    icon->setProperty("tone", tone);
    icon->setObjectName("channelIconBox");
    polishWidget(icon);

    auto *textWrap = new QVBoxLayout();
    textWrap->setContentsMargins(0, 0, 0, 0);
    textWrap->setSpacing(2);

    auto *title = new QLabel(texte.isEmpty() ? QStringLiteral("Canal recommande") : texte, card);
    title->setWordWrap(true);
    title->setStyleSheet(QStringLiteral("font-size:12px;font-weight: 700;color:#1e40af;"));

    auto *subtitle = new QLabel(channelSubtitle(texte), card);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet(QStringLiteral("font-size:10px;color:#7c8ca0;"));

    textWrap->addWidget(title);
    textWrap->addWidget(subtitle);
    layout->addWidget(icon, 0, Qt::AlignTop);
    layout->addLayout(textWrap, 1);
    return card;
}

QWidget *FishVisionPage::creerIndicatorCard(const FishVisionIndicator &indicator)
{
    auto *card = new QFrame(this);
    card->setObjectName("indicatorCard");
    card->setProperty("indicatorType", badgeClassForStatus(indicator.status));
    card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    polishWidget(card);

    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(10, 9, 10, 9);
    layout->setSpacing(8);

    auto *dot = new QLabel(card);
    dot->setFixedSize(8, 8);
    dot->setStyleSheet(QStringLiteral("border-radius:4px;background:%1;").arg(statusDotColor(indicator.status)));

    auto *label = new QLabel(indicator.label, card);
    label->setWordWrap(true);
    label->setStyleSheet(QStringLiteral("font-size:12px;color:#2563eb;"));

    layout->addWidget(dot, 0, Qt::AlignTop);
    layout->addWidget(label, 1);
    return card;
}

QWidget *FishVisionPage::creerItemRecommandation(const QString &icone, const QString &titre, const QString &detail)
{
    const QString tone = recommendationTone(titre, detail);
    const QString iconText = icone.trimmed().isEmpty() ? recommendationFallbackIcon(titre, detail) : icone.trimmed().left(2).toUpper();

    auto *container = new QWidget(this);
    container->setObjectName("itemRecommandation");

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 10, 0, 10);
    layout->setSpacing(10);

    auto *iconLabel = new QLabel(iconText, container);
    iconLabel->setObjectName("recommendationIcon");
    iconLabel->setProperty("tone", tone);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFixedSize(36, 36);
    polishWidget(iconLabel);

    auto *textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);

    auto *titleLabel = new QLabel(titre, container);
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet(QStringLiteral("font-size:13px;font-weight: 700;color:#1e3a8a;"));

    auto *detailLabel = new QLabel(detail, container);
    detailLabel->setWordWrap(true);
    detailLabel->setStyleSheet(QStringLiteral("font-size:11px;color:#7c8ca0;line-height:1.45;"));

    textLayout->addWidget(titleLabel);
    textLayout->addWidget(detailLabel);
    layout->addWidget(iconLabel, 0, Qt::AlignTop);
    layout->addLayout(textLayout, 1);
    return container;
}

QWidget *FishVisionPage::creerItemTimeline(const QString &couleur, const QString &texte)
{
    auto *container = new QWidget(this);
    container->setObjectName("timelineRow");

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 9, 0, 9);
    layout->setSpacing(10);

    auto *dot = new QLabel(container);
    dot->setFixedSize(8, 8);
    dot->setStyleSheet(QStringLiteral("background:%1;border-radius:4px;").arg(couleur));

    auto *text = new QLabel(texte, container);
    text->setWordWrap(true);
    text->setStyleSheet(QStringLiteral("font-size:12px;color:#8ea0b5;"));

    layout->addWidget(dot, 0, Qt::AlignTop);
    layout->addWidget(text, 1);
    return container;
}

QWidget *FishVisionPage::creerPositionnementItem(const FishVisionPositionnement &positionnement)
{
    auto *container = new QWidget(this);
    container->setObjectName("marketRow");

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 9, 0, 9);
    layout->setSpacing(8);

    auto *left = new QLabel(positionnement.label, container);
    left->setWordWrap(true);
    left->setStyleSheet(QStringLiteral("font-size:12px;color:#2563eb;"));

    auto *value = new QLabel(positionnement.valeur, container);
    value->setStyleSheet(QStringLiteral("font-size:12px;font-weight: 700;color:#1e3a8a;"));

    auto *badge = qobject_cast<QLabel *>(creerBadge(badgeText(positionnement.status), badgeClassForStatus(positionnement.status)));

    layout->addWidget(left, 1);
    layout->addWidget(value, 0, Qt::AlignVCenter);
    layout->addWidget(badge, 0, Qt::AlignVCenter);
    return container;
}

QString FishVisionPage::demanderImageEtAnalyser()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Selectionner une image"), QString(),
                                                      QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (!path.isEmpty()) {
        analyserImage(path);
    }
    return path;
}

void FishVisionPage::analyserImage(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("FishVision AI"), QStringLiteral("Impossible de lire l'image selectionnee."));
        return;
    }

    const QByteArray bytes = file.readAll();
    if (bytes.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("FishVision AI"), QStringLiteral("Le fichier image est vide."));
        return;
    }

    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::warning(this, QStringLiteral("FishVision AI"), QStringLiteral("Impossible de decoder l'image selectionnee."));
        return;
    }

    m_loadedPixmap = QPixmap::fromImage(image);
    m_currentImagePath = filePath;
    setFishImage(m_loadedPixmap);
    afficherChargement(QFileInfo(filePath).fileName());

    QNetworkRequest req(QUrl(QStringLiteral("%1?key=%2").arg(QString::fromLatin1(kEndpoint), getApiKey())));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_networkManager->post(req, construireGeminiPayload(bytes, detecterMimeType(filePath)));
    reply->setProperty("imagePath", filePath);
    reply->setProperty("modelName", QStringLiteral("gemini-2.5-flash"));
    QObject::connect(reply, &QNetworkReply::sslErrors, this, [this](const QList<QSslError> &errors) {
        QStringList lines;
        for (const QSslError &error : errors) {
            lines << error.errorString();
        }
        afficherErreur(QStringLiteral("SSL/TLS errors while calling Gemini:\n%1\nSSL supported: %2\nBuild SSL: %3\nRuntime SSL: %4")
                           .arg(lines.join('\n'),
                                QSslSocket::supportsSsl() ? QStringLiteral("yes") : QStringLiteral("no"),
                                QSslSocket::sslLibraryBuildVersionString(),
                                QSslSocket::sslLibraryVersionString()));
    });
}

QString FishVisionPage::detecterMimeType(const QString &filePath) const
{
    QMimeDatabase db;
    const QString mime = db.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name();
    return mime.startsWith(QStringLiteral("image/")) ? mime : QStringLiteral("image/jpeg");
}

QByteArray FishVisionPage::construireGeminiPayload(const QByteArray &imageBytes, const QString &mimeType) const
{
    const QJsonObject inlineData{{"mime_type", mimeType}, {"data", QString::fromLatin1(imageBytes.toBase64())}};
    const QJsonObject textPart{{"text", QString::fromLatin1(kPrompt)}};
    const QJsonObject imagePart{{"inline_data", inlineData}};
    const QJsonArray parts{textPart, imagePart};
    const QJsonObject content{{"role", "user"}, {"parts", parts}};
    const QJsonObject config{{"temperature", 0.2}, {"responseMimeType", "application/json"}};
    const QJsonObject payload{{"contents", QJsonArray{content}}, {"generationConfig", config}};
    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

QString FishVisionPage::extraireTexteGemini(const QByteArray &responseBytes, QString *errorMessage) const
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(responseBytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) *errorMessage = QStringLiteral("La reponse Gemini n'est pas un JSON valide.");
        return QString();
    }

    const QJsonObject root = doc.object();
    if (root.contains("error")) {
        if (errorMessage) {
            *errorMessage = strv(root.value("error").toObject(), QStringLiteral("message"));
            if (errorMessage->isEmpty()) {
                *errorMessage = QStringLiteral("Gemini a retourne une erreur.");
            }
        }
        return QString();
    }

    const QJsonArray candidates = root.value("candidates").toArray();
    if (candidates.isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Aucune reponse exploitable n'a ete retournee par Gemini.");
        return QString();
    }

    const QJsonArray parts = candidates.at(0).toObject().value("content").toObject().value("parts").toArray();
    for (const QJsonValue &part : parts) {
        const QString text = part.toObject().value("text").toString().trimmed();
        if (!text.isEmpty()) {
            return jsonOnly(text);
        }
    }

    if (errorMessage) *errorMessage = QStringLiteral("La reponse Gemini ne contient aucun texte JSON.");
    return QString();
}

bool FishVisionPage::parserResultatGemini(const QString &jsonText, FishVisionResult *result, QString *errorMessage) const
{
    if (!result) {
        return false;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) *errorMessage = QStringLiteral("Le JSON de resultat Gemini est invalide.");
        return false;
    }

    const QJsonObject o = doc.object();
    result->espece = strv(o, QStringLiteral("espece"));
    result->nomScientifique = strv(o, QStringLiteral("nom_scientifique"));
    result->poidsEstime = strv(o, QStringLiteral("poids_estime"));
    result->tags = strList(o.value("tags").toArray());
    result->grade = strv(o, QStringLiteral("grade")).toUpper();
    result->scoreFraicheur = normalizedScore(o.value("score_fraicheur"));
    result->scoreOeil = normalizedScore(o.value("score_oeil"));
    result->scoreBranchies = normalizedScore(o.value("score_branchies"));
    result->scorePeau = normalizedScore(o.value("score_peau"));
    result->verdict = strv(o, QStringLiteral("verdict"));
    result->rapportIA = strv(o, QStringLiteral("rapport_ia"));
    result->valeurKg = dblv(o, QStringLiteral("valeur_kg"));
    result->fourchetteMin = dblv(o, QStringLiteral("fourchette_min"));
    result->fourchetteMax = dblv(o, QStringLiteral("fourchette_max"));
    result->canauxVente = strList(o.value("canaux_vente").toArray());

    result->indicateurs.clear();
    for (const QJsonValue &v : o.value("indicateurs").toArray()) {
        const QJsonObject item = v.toObject();
        const FishVisionIndicator indicator{strv(item, QStringLiteral("label")),
                                            normalizedIndicatorStatus(strv(item, QStringLiteral("status")))};
        if (!indicator.label.isEmpty()) {
            result->indicateurs << indicator;
        }
    }

    result->recommandations.clear();
    for (const QJsonValue &v : o.value("recommandations").toArray()) {
        const QJsonObject item = v.toObject();
        FishVisionRecommendation recommendation{strv(item, QStringLiteral("icon")), strv(item, QStringLiteral("titre")),
                                                strv(item, QStringLiteral("detail"))};
        if (!recommendation.titre.isEmpty()) {
            result->recommandations << recommendation;
        }
    }

    result->positionnement.clear();
    for (const QJsonValue &v : o.value("positionnement").toArray()) {
        const QJsonObject item = v.toObject();
        const FishVisionPositionnement positionnement{strv(item, QStringLiteral("label")), strv(item, QStringLiteral("valeur")),
                                                      strv(item, QStringLiteral("status")).toLower()};
        if (!positionnement.label.isEmpty()) {
            result->positionnement << positionnement;
        }
    }

    if (result->espece.isEmpty() || result->nomScientifique.isEmpty() || result->poidsEstime.isEmpty() ||
        result->grade.isEmpty() || result->rapportIA.isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Le JSON Gemini ne contient pas tous les champs obligatoires.");
        return false;
    }

    if (result->tags.isEmpty()) result->tags << QStringLiteral("Analyse IA");
    if (result->indicateurs.isEmpty()) result->indicateurs << FishVisionIndicator{QStringLiteral("Aucun indicateur detaille retourne"), QStringLiteral("warning")};
    if (result->canauxVente.isEmpty()) result->canauxVente << QStringLiteral("Canal non precise");
    if (result->recommandations.isEmpty()) {
        result->recommandations << FishVisionRecommendation{QString(), QStringLiteral("Aucune recommandation detaillee"),
                                                            QStringLiteral("Gemini n'a pas renseigne cette section.")};
    }
    if (result->positionnement.isEmpty()) {
        result->positionnement << FishVisionPositionnement{QStringLiteral("vs. marche"), QStringLiteral("n/a"), QStringLiteral("neutre")};
    }

    result->grade = gradeFromScore(result->scoreFraicheur);
    result->verdict = verdictFromScore(result->scoreFraicheur);
    result->loaded = true;
    return true;
}

void FishVisionPage::setLoadingUiEnabled(bool enabled)
{
    btnImporterImage->setEnabled(enabled);
    btnLancerDemo->setEnabled(enabled);
    btnNouvelleAnalyse->setEnabled(enabled);
    btnNouvelleAnalyseBas->setEnabled(enabled);
}

void FishVisionPage::updateBarTone(QProgressBar *bar, int score)
{
    if (!bar) {
        return;
    }
    bar->setProperty("warning", score > 0 && score < 65);
    polishWidget(bar);
}

void FishVisionPage::applyGradeStyle(const QString &grade, const QString &text)
{
    QString tone = QStringLiteral("neutral");
    if (grade == "A") tone = QStringLiteral("gradeA");
    else if (grade == "B" || grade == "loading") tone = QStringLiteral("warning");
    else if (grade == "C" || grade == "critical") tone = QStringLiteral("critical");
    else if (grade == "D") tone = QStringLiteral("critical");
    else if (grade == "idle") tone = QStringLiteral("neutral");

    lblGrade->setProperty("badgeType", tone);
    lblGrade->setObjectName("gradeBadge");
    lblGrade->setText(text.isEmpty() ? QStringLiteral("Grade %1").arg(grade) : text);
    polishWidget(lblGrade);
}

void FishVisionPage::updateVerdictWidgets(int score, const QString &verdict, const QString &subtext)
{
    m_verdictScore->setValue(score);
    lblVerdictTitre->setText(verdict);
    lblVerdictTitre->setStyleSheet(
        QStringLiteral("font-size:14px;font-weight: 700;color:%1;").arg(score >= 70 ? "#34d399" : score >= 40 ? "#f59e0b" : "#ef4444"));
    lblVerdictGrade->setText(subtext);
}

void FishVisionPage::polishWidget(QWidget *widget) const
{
    if (!widget) {
        return;
    }
    style()->unpolish(widget);
    style()->polish(widget);
    widget->update();
}

bool FishVisionPage::isFishVisionStorageReady(QString *errorMessage) const
{
    const QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("La connexion a la base de donnees n'est pas disponible.");
        }
        return false;
    }

    QString tableError;
    if (!isTableAccessible(QStringLiteral("FISHVISION_ANALYSES"), &tableError)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("La table FISHVISION_ANALYSES est inaccessible. %1").arg(tableError);
        }
        return false;
    }

    return true;
}

bool FishVisionPage::saveCurrentAnalysisToDatabase(int linkedPecheId, QString *savedId, QString *errorMessage)
{
    if (!m_result.loaded) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Aucune analyse FishVision n'est disponible a sauvegarder.");
        }
        return false;
    }

    if (!isFishVisionStorageReady(errorMessage)) {
        return false;
    }

    if (linkedPecheId > 0) {
        QString pecheError;
        if (!isTableAccessible(QStringLiteral("PECHES"), &pecheError)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Le rattachement est impossible: table PECHES inaccessible. %1").arg(pecheError);
            }
            return false;
        }
    }

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        if (errorMessage) {
            *errorMessage = db.lastError().text();
        }
        return false;
    }

    int analysisId = m_currentAnalysisId;
    if (analysisId <= 0) {
        QSqlQuery nextIdQuery(db);
        nextIdQuery.prepare(QStringLiteral("SELECT NVL(MAX(ANALYSIS_ID), 0) + 1 FROM FISHVISION_ANALYSES"));
        if (!nextIdQuery.exec() || !nextIdQuery.next()) {
            db.rollback();
            if (errorMessage) {
                *errorMessage = nextIdQuery.lastError().text();
            }
            return false;
        }
        analysisId = nextIdQuery.value(0).toInt();
    }

    QSqlQuery query(db);
    if (m_currentAnalysisId > 0) {
        query.prepare(QStringLiteral(
            "UPDATE FISHVISION_ANALYSES SET "
            "    IDPECHE = :idPeche, IMAGE_PATH = :imagePath, ESPECE = :espece, NOM_SCIENTIFIQUE = :nomScientifique, "
            "    POIDS_ESTIME = :poidsEstime, TAGS_JSON = :tagsJson, GRADE = :grade, "
            "    SCORE_FRAICHEUR = :scoreFraicheur, SCORE_OEIL = :scoreOeil, SCORE_BRANCHIES = :scoreBranchies, SCORE_PEAU = :scorePeau, "
            "    VERDICT = :verdict, RAPPORT_IA = :rapportIa, INDICATEURS_JSON = :indicateursJson, "
            "    VALEUR_KG = :valeurKg, FOURCHETTE_MIN = :fourchetteMin, FOURCHETTE_MAX = :fourchetteMax, "
            "    CANAUX_VENTE_JSON = :canauxJson, RECOMMANDATIONS_JSON = :recommandationsJson, POSITIONNEMENT_JSON = :positionnementJson, "
            "    UPDATED_AT = SYSTIMESTAMP "
            "WHERE ANALYSIS_ID = :analysisId"));
    } else {
        query.prepare(QStringLiteral(
            "INSERT INTO FISHVISION_ANALYSES ( "
            "    ANALYSIS_ID, IDPECHE, IMAGE_PATH, ESPECE, NOM_SCIENTIFIQUE, POIDS_ESTIME, TAGS_JSON, GRADE, "
            "    SCORE_FRAICHEUR, SCORE_OEIL, SCORE_BRANCHIES, SCORE_PEAU, VERDICT, RAPPORT_IA, INDICATEURS_JSON, "
            "    VALEUR_KG, FOURCHETTE_MIN, FOURCHETTE_MAX, CANAUX_VENTE_JSON, RECOMMANDATIONS_JSON, POSITIONNEMENT_JSON "
            ") VALUES ( "
            "    :analysisId, :idPeche, :imagePath, :espece, :nomScientifique, :poidsEstime, :tagsJson, :grade, "
            "    :scoreFraicheur, :scoreOeil, :scoreBranchies, :scorePeau, :verdict, :rapportIa, :indicateursJson, "
            "    :valeurKg, :fourchetteMin, :fourchetteMax, :canauxJson, :recommandationsJson, :positionnementJson "
            ")"));
    }

    query.bindValue(QStringLiteral(":analysisId"), analysisId);
    query.bindValue(QStringLiteral(":idPeche"), linkedPecheId > 0 ? QVariant(linkedPecheId) : nullIntVariant());
    query.bindValue(QStringLiteral(":imagePath"),
                    m_currentImagePath.isEmpty() ? nullStringVariant() : QVariant(m_currentImagePath.left(500)));
    query.bindValue(QStringLiteral(":espece"), m_result.espece.left(120));
    query.bindValue(QStringLiteral(":nomScientifique"), m_result.nomScientifique.left(180));
    query.bindValue(QStringLiteral(":poidsEstime"), m_result.poidsEstime.left(40));
    query.bindValue(QStringLiteral(":tagsJson"), jsonFromStringList(m_result.tags));
    query.bindValue(QStringLiteral(":grade"), m_result.grade.left(10));
    query.bindValue(QStringLiteral(":scoreFraicheur"), m_result.scoreFraicheur);
    query.bindValue(QStringLiteral(":scoreOeil"), m_result.scoreOeil);
    query.bindValue(QStringLiteral(":scoreBranchies"), m_result.scoreBranchies);
    query.bindValue(QStringLiteral(":scorePeau"), m_result.scorePeau);
    query.bindValue(QStringLiteral(":verdict"), m_result.verdict.left(250));
    query.bindValue(QStringLiteral(":rapportIa"), m_result.rapportIA);
    query.bindValue(QStringLiteral(":indicateursJson"), jsonFromIndicators(m_result.indicateurs));
    query.bindValue(QStringLiteral(":valeurKg"), qIsFinite(m_result.valeurKg) ? QVariant(m_result.valeurKg) : nullDoubleVariant());
    query.bindValue(QStringLiteral(":fourchetteMin"), qIsFinite(m_result.fourchetteMin) ? QVariant(m_result.fourchetteMin) : nullDoubleVariant());
    query.bindValue(QStringLiteral(":fourchetteMax"), qIsFinite(m_result.fourchetteMax) ? QVariant(m_result.fourchetteMax) : nullDoubleVariant());
    query.bindValue(QStringLiteral(":canauxJson"), jsonFromStringList(m_result.canauxVente));
    query.bindValue(QStringLiteral(":recommandationsJson"), jsonFromRecommendations(m_result.recommandations));
    query.bindValue(QStringLiteral(":positionnementJson"), jsonFromPositionnements(m_result.positionnement));

    if (!query.exec()) {
        db.rollback();
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    if (!db.commit()) {
        if (errorMessage) {
            *errorMessage = db.lastError().text();
        }
        return false;
    }

    m_currentAnalysisId = analysisId;
    m_currentLinkedPecheId = linkedPecheId;
    if (savedId) {
        *savedId = QStringLiteral("FV-%1").arg(analysisId);
    }

    QString loadError;
    if (!loadSavedAnalysesFromDatabase(&loadError) && errorMessage && errorMessage->isEmpty()) {
        *errorMessage = loadError;
    }
    return true;
}

bool FishVisionPage::loadSavedAnalysesFromDatabase(QString *errorMessage)
{
    m_savedAnalyses.clear();
    if (!isFishVisionStorageReady(errorMessage)) {
        return false;
    }

    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT ANALYSIS_ID, TO_CHAR(CREATED_AT, 'DD/MM/YYYY HH24:MI'), NVL(IDPECHE, 0), "
        "       ESPECE, GRADE, SCORE_FRAICHEUR, VERDICT "
        "FROM FISHVISION_ANALYSES "
        "ORDER BY CREATED_AT DESC NULLS LAST, ANALYSIS_ID DESC"));

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    while (query.next()) {
        SavedAnalysis save;
        save.id = QStringLiteral("FV-%1").arg(query.value(0).toInt());
        save.date = query.value(1).toString();
        save.linkedPecheId = query.value(2).toInt();
        save.result.espece = query.value(3).toString();
        save.result.grade = query.value(4).toString();
        save.result.scoreFraicheur = query.value(5).toInt();
        save.result.verdict = query.value(6).toString();
        m_savedAnalyses.append(save);
    }

    return true;
}

bool FishVisionPage::loadSavedAnalysisDetails(int analysisId, SavedAnalysis *analysis, QString *errorMessage) const
{
    if (!analysis) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Analyse FishVision invalide.");
        }
        return false;
    }

    const QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("La connexion a la base de donnees n'est pas disponible.");
        }
        return false;
    }

    QString tableError;
    if (!isTableAccessible(QStringLiteral("FISHVISION_ANALYSES"), &tableError)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("La table FISHVISION_ANALYSES est inaccessible. %1").arg(tableError);
        }
        return false;
    }

    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT ANALYSIS_ID, TO_CHAR(CREATED_AT, 'DD/MM/YYYY HH24:MI'), NVL(IDPECHE, 0), IMAGE_PATH, "
        "       ESPECE, NOM_SCIENTIFIQUE, POIDS_ESTIME, TAGS_JSON, GRADE, "
        "       SCORE_FRAICHEUR, SCORE_OEIL, SCORE_BRANCHIES, SCORE_PEAU, VERDICT, RAPPORT_IA, "
        "       INDICATEURS_JSON, VALEUR_KG, FOURCHETTE_MIN, FOURCHETTE_MAX, "
        "       CANAUX_VENTE_JSON, RECOMMANDATIONS_JSON, POSITIONNEMENT_JSON "
        "FROM FISHVISION_ANALYSES "
        "WHERE ANALYSIS_ID = :analysisId"));
    query.bindValue(QStringLiteral(":analysisId"), analysisId);

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    if (!query.next()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Analyse FishVision introuvable en base.");
        }
        return false;
    }

    analysis->id = QStringLiteral("FV-%1").arg(query.value(0).toInt());
    analysis->date = query.value(1).toString();
    analysis->linkedPecheId = query.value(2).toInt();
    analysis->imagePath = query.value(3).toString();
    analysis->result.espece = query.value(4).toString();
    analysis->result.nomScientifique = query.value(5).toString();
    analysis->result.poidsEstime = query.value(6).toString();
    analysis->result.tags = parseStringListJson(query.value(7).toString());
    analysis->result.grade = query.value(8).toString();
    analysis->result.scoreFraicheur = query.value(9).toInt();
    analysis->result.scoreOeil = query.value(10).toInt();
    analysis->result.scoreBranchies = query.value(11).toInt();
    analysis->result.scorePeau = query.value(12).toInt();
    analysis->result.verdict = query.value(13).toString();
    analysis->result.rapportIA = query.value(14).toString();
    analysis->result.indicateurs = parseIndicatorsJson(query.value(15).toString());
    analysis->result.valeurKg = query.value(16).toDouble();
    analysis->result.fourchetteMin = query.value(17).toDouble();
    analysis->result.fourchetteMax = query.value(18).toDouble();
    analysis->result.canauxVente = parseStringListJson(query.value(19).toString());
    analysis->result.recommandations = parseRecommendationsJson(query.value(20).toString());
    analysis->result.positionnement = parsePositionnementsJson(query.value(21).toString());
    analysis->result.loaded = true;
    return true;
}

QList<QPair<int, QString>> FishVisionPage::fetchAvailablePeches(QString *errorMessage) const
{
    QList<QPair<int, QString>> items;

    const QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("La connexion a la base de donnees n'est pas disponible.");
        }
        return items;
    }

    QString tableError;
    if (!isTableAccessible(QStringLiteral("PECHES"), &tableError)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("La table PECHES est inaccessible. %1").arg(tableError);
        }
        return items;
    }

    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT P.IDPECHE, P.DATEPECHE, NVL(N.NOMNAVIRE, P.ZONEPECHE), P.DESCRIPTION "
        "FROM PECHES P "
        "LEFT JOIN NAVIRES N ON N.IDNAVIRE = P.IDNAVIRE "
        "WHERE NVL(P.IS_ARCHIVED, 0) = 0 "
        "ORDER BY P.DATEPECHE DESC, P.IDPECHE DESC"));

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return items;
    }

    while (query.next()) {
        const int idPeche = query.value(0).toInt();
        const QDate datePeche = query.value(1).toDate();
        QString origine = query.value(2).toString().trimmed();
        if (origine.isEmpty()) {
            origine = QStringLiteral("Lot %1").arg(idPeche);
        }

        const QString description = query.value(3).toString();
        const QString statut = description.section('|', 0, 0).trimmed().isEmpty()
                                   ? QStringLiteral("EN_COURS")
                                   : description.section('|', 0, 0).trimmed();

        const QString label = QStringLiteral("#%1 | %2 | %3 | %4")
                                  .arg(idPeche)
                                  .arg(datePeche.isValid() ? datePeche.toString(QStringLiteral("dd/MM/yyyy")) : QStringLiteral("--/--/----"))
                                  .arg(origine)
                                  .arg(statut);
        items.append(qMakePair(idPeche, label));
    }

    return items;
}

void FishVisionPage::onImporterImageClicked() { demanderImageEtAnalyser(); }
void FishVisionPage::onLancerDemoClicked() { demanderImageEtAnalyser(); }
void FishVisionPage::onRetourClicked() { emit backRequested(); }
void FishVisionPage::onTabFraicheurClicked() { stackOnglets->setCurrentIndex(0); mettreAJourOngletActif(0); }
void FishVisionPage::onTabRapportClicked() { stackOnglets->setCurrentIndex(1); mettreAJourOngletActif(1); }
void FishVisionPage::onTabMarcheClicked() { stackOnglets->setCurrentIndex(2); mettreAJourOngletActif(2); }
void FishVisionPage::onTabRecommandationsClicked() { stackOnglets->setCurrentIndex(3); mettreAJourOngletActif(3); }
void FishVisionPage::onTabAssistantClicked() { stackOnglets->setCurrentIndex(4); mettreAJourOngletActif(4); }

void FishVisionPage::onSauvegarderClicked()
{
    if (!m_result.loaded) {
        QMessageBox::warning(this, QStringLiteral("FishVision AI"),
                             QStringLiteral("Lancez d'abord une analyse avant de sauvegarder."));
        return;
    }

    btnSauvegarder->setEnabled(false);
    btnSauvegarder->setText(QStringLiteral("Sauvegarde..."));

    QString savedId;
    QString errorMessage;
    const bool ok = saveCurrentAnalysisToDatabase(m_currentLinkedPecheId, &savedId, &errorMessage);

    btnSauvegarder->setEnabled(true);
    btnSauvegarder->setText(QStringLiteral("Sauvegarder en base"));

    if (!ok) {
        QMessageBox::warning(this, QStringLiteral("Sauvegarde FishVision"), errorMessage);
        return;
    }

    QMessageBox::information(
        this,
        QStringLiteral("Sauvegarde Reussie"),
        QStringLiteral("Analyse FishVision sauvegardee avec succes.\nID : %1%2")
            .arg(savedId)
            .arg(m_currentLinkedPecheId > 0 ? QStringLiteral("\nLot rattache : %1").arg(m_currentLinkedPecheId) : QString()));
}

void FishVisionPage::onRattacherClicked()
{
    if (!m_result.loaded) {
        QMessageBox::warning(this, QStringLiteral("Rattachement"),
                             QStringLiteral("Aucune analyse FishVision n'est disponible a rattacher."));
        return;
    }

    QString errorMessage;
    const QList<QPair<int, QString>> lots = fetchAvailablePeches(&errorMessage);
    if (!errorMessage.isEmpty() && lots.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Rattachement"), errorMessage);
        return;
    }

    if (lots.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Rattachement"),
                                 QStringLiteral("Aucun lot de peche actif n'est disponible pour le rattachement."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Rattacher l'analyse FishVision"));
    dialog.resize(640, 220);
    PecheFontUtils::applyModuleFont(&dialog);
    dialog.setStyleSheet(styleSheet());

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);

    auto *title = new QLabel(
        QStringLiteral("Selectionnez le lot de peche qui doit recevoir l'analyse de %1.").arg(m_result.espece),
        &dialog);
    title->setWordWrap(true);
    layout->addWidget(title);

    auto *combo = new QComboBox(&dialog);
    combo->setMinimumHeight(40);
    for (const auto &lot : lots) {
        combo->addItem(lot.second, lot.first);
    }
    const int currentIndex = combo->findData(m_currentLinkedPecheId);
    if (currentIndex >= 0) {
        combo->setCurrentIndex(currentIndex);
    }
    layout->addWidget(combo);

    auto *note = new QLabel(
        QStringLiteral("Le rattachement sauvegarde aussi l'analyse si elle n'existe pas encore dans la base."),
        &dialog);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("color:#2563eb;"));
    layout->addWidget(note);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    if (auto *okButton = buttons->button(QDialogButtonBox::Ok)) {
        okButton->setText(QStringLiteral("Rattacher"));
        okButton->setObjectName("btnPrimary");
    }
    if (auto *cancelButton = buttons->button(QDialogButtonBox::Cancel)) {
        cancelButton->setText(QStringLiteral("Annuler"));
        cancelButton->setObjectName("btnSecondary");
    }
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const int selectedPecheId = combo->currentData().toInt();
    QString savedId;
    if (!saveCurrentAnalysisToDatabase(selectedPecheId, &savedId, &errorMessage)) {
        QMessageBox::warning(this, QStringLiteral("Rattachement"), errorMessage);
        return;
    }

    QMessageBox::information(
        this,
        QStringLiteral("Rattachement Reussi"),
        QStringLiteral("L'analyse %1 a ete rattachee au lot de peche #%2.").arg(savedId).arg(selectedPecheId));
}

void FishVisionPage::onExporterPdfClicked()
{
    QString defaultName = QStringLiteral("Rapport_FishVision_%1.pdf").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm"));
    QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("Exporter Rapport PDF"), defaultName, QStringLiteral("Fichiers PDF (*.pdf)"));
    if (fileName.isEmpty()) {
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter));

    QTextDocument doc;
    QString html = QString::fromUtf8("<h1>Rapport d'Analyse Intelligente FishVision</h1>"
                                     "<h3>Details de l'espece</h3>"
                                     "<p><b>Nom commun :</b> %1</p>"
                                     "<p><b>Nom scientifique :</b> %2</p>"
                                     "<p><b>Poids estime :</b> %3</p>"
                                     "<h3>Analyse de Fraicheur</h3>"
                                     "<p><b>Score de fraicheur global :</b> <span style='font-size: 18pt; color: #10b981;'>%4 / 100</span></p>"
                                     "<p><b>Verdict :</b> %5</p>"
                                     "<h3>Rapport Detaillé IA</h3>"
                                     "<p>%6</p>"
                                     "<hr>"
                                     "<p style='color: grey; font-size: 10pt;'><i>Genere automatiquement par DataDock le %7</i></p>")
                       .arg(m_result.espece)
                       .arg(m_result.nomScientifique)
                       .arg(m_result.poidsEstime)
                       .arg(m_result.scoreFraicheur)
                       .arg(m_result.verdict)
                       .arg(m_result.rapportIA)
                       .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss"));

    doc.setHtml(html);
    doc.print(&printer);

    QMessageBox::information(this, QStringLiteral("Export PDF"), QStringLiteral("Le rapport a ete exporte avec succes vers:\n%1").arg(fileName));
}

void FishVisionPage::onHistoriqueClicked()
{
    QString errorMessage;
    if (!loadSavedAnalysesFromDatabase(&errorMessage)) {
        QMessageBox::warning(this, QStringLiteral("Historique FishVision"), errorMessage);
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Historique des Sauvegardes FishVision"));
    dialog.resize(860, 480);
    PecheFontUtils::applyModuleFont(&dialog);
    dialog.setStyleSheet(styleSheet());
    
    QVBoxLayout layout(&dialog);
    layout.setContentsMargins(20, 20, 20, 20);
    layout.setSpacing(16);
    
    QLabel *title = new QLabel(QStringLiteral("Vos Sauvegardes d'Analyse"), &dialog);
    title->setObjectName("lblTitrePage");
    layout.addWidget(title);
    
    QTableWidget table(&dialog);
    table.setColumnCount(7);
    table.setHorizontalHeaderLabels({QStringLiteral("ID"), QStringLiteral("Date"), QStringLiteral("Lot"), QStringLiteral("Espece"), QStringLiteral("Grade / Score"), QStringLiteral("Verdict"), QStringLiteral("Action")});
    table.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table.horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table.horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table.horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    table.horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    table.setEditTriggers(QAbstractItemView::NoEditTriggers);
    table.setSelectionBehavior(QAbstractItemView::SelectRows);
    table.setSelectionMode(QAbstractItemView::SingleSelection);
    table.verticalHeader()->setVisible(false);
    table.setShowGrid(false);
    table.setStyleSheet(QStringLiteral("QTableWidget { background-color: rgba(10, 18, 40, 0.88); border-radius: 8px; border: 1px solid rgba(255,255,255,0.1); padding: 5px; color: #1e3a8a; } "
                                       "QHeaderView::section { background-color: rgba(255,255,255,0.05); color: #bae6fd; font-weight: bold; padding: 6px; border: none; border-bottom: 1px solid rgba(255,255,255,0.1); } "
                                       "QTableWidget::item { padding: 8px; border-bottom: 1px solid rgba(255,255,255,0.05); } "
                                       "QTableWidget::item:selected { background-color: rgba(56, 189, 248, 0.15); }"));

    table.setRowCount(m_savedAnalyses.size());
    for (int i = 0; i < m_savedAnalyses.size(); ++i) {
        const auto &save = m_savedAnalyses[i];
        table.setItem(i, 0, new QTableWidgetItem(save.id));
        table.setItem(i, 1, new QTableWidgetItem(save.date));
        table.setItem(i, 2, new QTableWidgetItem(save.linkedPecheId > 0 ? QString::number(save.linkedPecheId) : QStringLiteral("-")));
        table.setItem(i, 3, new QTableWidgetItem(save.result.espece));
        table.setItem(i, 4, new QTableWidgetItem(
                              QStringLiteral("%1 / %2/100")
                                  .arg(save.result.grade.isEmpty() ? QStringLiteral("--") : save.result.grade)
                                  .arg(save.result.scoreFraicheur)));
        table.setItem(i, 5, new QTableWidgetItem(save.result.verdict));

        auto *btnAfficher = new QPushButton(QStringLiteral("Afficher"), &table);
        btnAfficher->setObjectName("btnPrimary");
        btnAfficher->setProperty("analysisId", save.id.mid(3).toInt());
        connect(btnAfficher, &QPushButton::clicked, &dialog, [this, &dialog, btnAfficher]() {
            const int analysisId = btnAfficher->property("analysisId").toInt();
            SavedAnalysis detailed;
            QString detailError;
            if (!loadSavedAnalysisDetails(analysisId, &detailed, &detailError)) {
                QMessageBox::warning(&dialog, QStringLiteral("Historique FishVision"), detailError);
                return;
            }

            QDialog detailsDialog(&dialog);
            detailsDialog.setWindowTitle(QStringLiteral("Detail de l'analyse %1").arg(detailed.id));
            detailsDialog.resize(980, 720);
            PecheFontUtils::applyModuleFont(&detailsDialog);
            detailsDialog.setStyleSheet(styleSheet());

            auto *rootLayout = new QVBoxLayout(&detailsDialog);
            rootLayout->setContentsMargins(18, 18, 18, 18);
            rootLayout->setSpacing(14);

            auto *scrollArea = new QScrollArea(&detailsDialog);
            scrollArea->setWidgetResizable(true);
            scrollArea->setFrameShape(QFrame::NoFrame);

            auto *content = new QWidget(scrollArea);
            auto *contentLayout = new QVBoxLayout(content);
            contentLayout->setContentsMargins(0, 0, 0, 0);
            contentLayout->setSpacing(14);

            auto *topRow = new QHBoxLayout();
            topRow->setSpacing(14);

            auto *imageCard = new QFrame(content);
            imageCard->setObjectName("carteAnalyse");
            imageCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
            auto *imageLayout = new QVBoxLayout(imageCard);
            imageLayout->setContentsMargins(16, 16, 16, 16);
            imageLayout->setSpacing(10);

            auto *imageTitle = new QLabel(QStringLiteral("Image sauvegardee"), imageCard);
            imageTitle->setObjectName("labelRapportTitre");
            imageLayout->addWidget(imageTitle);

            auto *imageLabel = new QLabel(imageCard);
            imageLabel->setAlignment(Qt::AlignCenter);
            imageLabel->setMinimumSize(280, 220);
            imageLabel->setStyleSheet(QStringLiteral("background: rgba(5, 10, 24, 0.45); border: 1px solid rgba(255,255,255,0.08); border-radius: 12px; color:#2563eb;"));

            QPixmap savedPixmap;
            if (!detailed.imagePath.isEmpty() && QFileInfo::exists(detailed.imagePath)) {
                savedPixmap.load(detailed.imagePath);
            }
            if (!savedPixmap.isNull()) {
                imageLabel->setPixmap(savedPixmap.scaled(320, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                imageLabel->setText(detailed.imagePath.isEmpty()
                                        ? QStringLiteral("Aucun chemin image enregistre.")
                                        : QStringLiteral("Image introuvable sur le disque.\n%1").arg(detailed.imagePath));
            }
            imageLayout->addWidget(imageLabel);

            auto *pathLabel = new QLabel(
                detailed.imagePath.isEmpty() ? QStringLiteral("Chemin image : non enregistre") : QStringLiteral("Chemin image : %1").arg(detailed.imagePath),
                imageCard);
            pathLabel->setWordWrap(true);
            pathLabel->setStyleSheet(QStringLiteral("font-size:11px;color:#2563eb;"));
            imageLayout->addWidget(pathLabel);

            auto *summaryCard = new QFrame(content);
            summaryCard->setObjectName("carteAnalyse");
            auto *summaryLayout = new QVBoxLayout(summaryCard);
            summaryLayout->setContentsMargins(16, 16, 16, 16);
            summaryLayout->setSpacing(8);

            auto *summaryTitle = new QLabel(QStringLiteral("Resume"), summaryCard);
            summaryTitle->setObjectName("labelRapportTitre");
            summaryLayout->addWidget(summaryTitle);

            const QString summaryHtml = QStringLiteral(
                "<b>ID:</b> %1<br>"
                "<b>Date:</b> %2<br>"
                "<b>Lot rattache:</b> %3<br>"
                "<b>Espece:</b> %4<br>"
                "<b>Nom scientifique:</b> %5<br>"
                "<b>Poids estime:</b> %6 kg<br>"
                "<b>Grade:</b> %7<br>"
                "<b>Score fraicheur:</b> %8/100<br>"
                "<b>Verdict:</b> %9<br>"
                "<b>Valeur estimee:</b> %10 TND/kg<br>"
                "<b>Fourchette:</b> %11 - %12 TND/kg")
                    .arg(detailed.id)
                    .arg(detailed.date)
                    .arg(detailed.linkedPecheId > 0 ? QString::number(detailed.linkedPecheId) : QStringLiteral("-"))
                    .arg(detailed.result.espece)
                    .arg(detailed.result.nomScientifique)
                    .arg(detailed.result.poidsEstime)
                    .arg(detailed.result.grade)
                    .arg(detailed.result.scoreFraicheur)
                    .arg(detailed.result.verdict)
                    .arg(QString::number(detailed.result.valeurKg, 'f', 2))
                    .arg(QString::number(detailed.result.fourchetteMin, 'f', 2))
                    .arg(QString::number(detailed.result.fourchetteMax, 'f', 2));

            auto *summaryText = new QLabel(summaryHtml, summaryCard);
            summaryText->setTextFormat(Qt::RichText);
            summaryText->setWordWrap(true);
            summaryText->setStyleSheet(QStringLiteral("color:#1e3a8a; font-size:12px; line-height:1.5;"));
            summaryLayout->addWidget(summaryText);

            topRow->addWidget(imageCard, 1);
            topRow->addWidget(summaryCard, 1);
            contentLayout->addLayout(topRow);

            auto *reportCard = new QFrame(content);
            reportCard->setObjectName("carteAnalyse");
            auto *reportLayout = new QVBoxLayout(reportCard);
            reportLayout->setContentsMargins(16, 16, 16, 16);
            reportLayout->setSpacing(8);
            auto *reportTitle = new QLabel(QStringLiteral("Rapport IA"), reportCard);
            reportTitle->setObjectName("labelRapportTitre");
            auto *reportText = new QLabel(detailed.result.rapportIA, reportCard);
            reportText->setWordWrap(true);
            reportText->setStyleSheet(QStringLiteral("color:#1e40af; font-size:12px;"));
            reportLayout->addWidget(reportTitle);
            reportLayout->addWidget(reportText);
            contentLayout->addWidget(reportCard);

            auto *tagsCard = new QFrame(content);
            tagsCard->setObjectName("carteAnalyse");
            auto *tagsLayout = new QVBoxLayout(tagsCard);
            tagsLayout->setContentsMargins(16, 16, 16, 16);
            tagsLayout->setSpacing(10);
            auto *tagsTitle = new QLabel(QStringLiteral("Tags, indicateurs et canaux"), tagsCard);
            tagsTitle->setObjectName("labelRapportTitre");
            tagsLayout->addWidget(tagsTitle);

            auto *tagsText = new QLabel(
                QStringLiteral("<b>Tags:</b> %1<br><b>Canaux:</b> %2")
                    .arg(detailed.result.tags.isEmpty() ? QStringLiteral("-") : detailed.result.tags.join(QStringLiteral(", ")))
                    .arg(detailed.result.canauxVente.isEmpty() ? QStringLiteral("-") : detailed.result.canauxVente.join(QStringLiteral(", "))),
                tagsCard);
            tagsText->setTextFormat(Qt::RichText);
            tagsText->setWordWrap(true);
            tagsText->setStyleSheet(QStringLiteral("color:#1e40af; font-size:12px;"));
            tagsLayout->addWidget(tagsText);

            for (const FishVisionIndicator &indicator : detailed.result.indicateurs) {
                tagsLayout->addWidget(creerIndicatorCard(indicator));
            }
            contentLayout->addWidget(tagsCard);

            auto *recCard = new QFrame(content);
            recCard->setObjectName("carteAnalyse");
            auto *recLayout = new QVBoxLayout(recCard);
            recLayout->setContentsMargins(16, 16, 16, 16);
            recLayout->setSpacing(8);
            auto *recTitle = new QLabel(QStringLiteral("Recommandations"), recCard);
            recTitle->setObjectName("labelRapportTitre");
            recLayout->addWidget(recTitle);
            for (const FishVisionRecommendation &recommendation : detailed.result.recommandations) {
                recLayout->addWidget(creerItemRecommandation(recommendation.icon, recommendation.titre, recommendation.detail));
            }
            contentLayout->addWidget(recCard);

            auto *positionCard = new QFrame(content);
            positionCard->setObjectName("carteAnalyse");
            auto *positionLayout = new QVBoxLayout(positionCard);
            positionLayout->setContentsMargins(16, 16, 16, 16);
            positionLayout->setSpacing(8);
            auto *positionTitle = new QLabel(QStringLiteral("Positionnement marche"), positionCard);
            positionTitle->setObjectName("labelRapportTitre");
            positionLayout->addWidget(positionTitle);
            for (const FishVisionPositionnement &positionnement : detailed.result.positionnement) {
                positionLayout->addWidget(creerPositionnementItem(positionnement));
            }
            contentLayout->addWidget(positionCard);

            contentLayout->addStretch();
            scrollArea->setWidget(content);
            rootLayout->addWidget(scrollArea);

            auto *closeButton = new QPushButton(QStringLiteral("Fermer"), &detailsDialog);
            closeButton->setObjectName("btnSecondary");
            connect(closeButton, &QPushButton::clicked, &detailsDialog, &QDialog::accept);
            rootLayout->addWidget(closeButton, 0, Qt::AlignRight);

            detailsDialog.exec();
        });
        table.setCellWidget(i, 6, btnAfficher);
    }
    
    layout.addWidget(&table);

    if (m_savedAnalyses.isEmpty()) {
        auto *emptyLabel = new QLabel(QStringLiteral("Aucune analyse FishVision n'a encore ete sauvegardee."), &dialog);
        emptyLabel->setStyleSheet(QStringLiteral("color:#2563eb;"));
        layout.addWidget(emptyLabel);
    }
    
    QPushButton *btnClose = new QPushButton("Fermer", &dialog);
    btnClose->setObjectName("btnSecondary");
    connect(btnClose, &QPushButton::clicked, &dialog, &QDialog::accept);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(btnClose);
    layout.addLayout(btnLayout);
    
    dialog.exec();
}

void FishVisionPage::onNouvelleAnalyseClicked()
{
    afficherEtatVide();
}

void FishVisionPage::animerScore()
{
    m_scoreAnimCurrent += 2;
    if (m_scoreAnimCurrent >= m_result.scoreFraicheur) {
        m_scoreAnimCurrent = m_result.scoreFraicheur;
        m_scoreTimer->stop();
    }
    m_scoreRing->setScore(m_scoreAnimCurrent);
}

void FishVisionPage::onGeminiReplyFinished(QNetworkReply *reply)
{
    if (reply->property("ignoreResult").toBool() || m_resetInProgress) {
        reply->deleteLater();
        return;
    }
    
    if (reply->property("isChat").toBool()) {
        processGeminiChatReply(reply);
        return;
    }

    const QByteArray bytes = reply->readAll();
    QString message;
    if (reply->error() != QNetworkReply::NoError) {
        const QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        const QVariant reasonPhrase = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);
        message = QStringLiteral("Echec de l'appel Gemini (%1)").arg(reply->errorString());
        if (statusCode.isValid()) {
            message += QStringLiteral("\nHTTP %1 %2").arg(statusCode.toInt()).arg(reasonPhrase.toString());
        }
        const QString bodyText = summarizeBytes(bytes);
        if (!bodyText.isEmpty()) {
            message += QStringLiteral("\nResponse body:\n%1").arg(bodyText);
        }
        reply->deleteLater();
        afficherErreur(message);
        return;
    }

    const QString jsonText = extraireTexteGemini(bytes, &message);
    if (jsonText.isEmpty()) {
        reply->deleteLater();
        afficherErreur(message);
        return;
    }

    FishVisionResult result;
    if (!parserResultatGemini(jsonText, &result, &message)) {
        reply->deleteLater();
        afficherErreur(message);
        return;
    }

    reply->deleteLater();
    afficherResultat(result);
}

void FishVisionPage::onEnvoyerChatClicked()
{
    const QString text = chatInput->text().trimmed();
    if (text.isEmpty() || !m_result.loaded) {
        return;
    }
    
    chatInput->clear();
    appendChatBotMessage(text, true);
    
    btnSendChat->setEnabled(false);
    btnSendChat->setText("...");
    
    QJsonObject textPart;
    textPart["text"] = QStringLiteral("Dontexte: Le poisson identifie est \"%1\" (Grade %2), %3. Question de l'utilisateur: %4. Reponds brievement comme expert en 1 ou 2 paragraphes maximum, directement.").arg(m_result.espece).arg(m_result.grade).arg(m_result.verdict).arg(text);
    QJsonArray parts;
    parts.append(textPart);
    QJsonObject content;
    content["role"] = "user";
    content["parts"] = parts;
    
    QJsonObject config;
    config["temperature"] = 0.5;
    
    QJsonObject payload;
    payload["contents"] = QJsonArray{content};
    payload["generationConfig"] = config;
    
    const QByteArray requestData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    
    QNetworkRequest req(QUrl(QStringLiteral("%1?key=%2").arg(QString::fromLatin1(kEndpoint), getApiKey())));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_networkManager->post(req, requestData);
    reply->setProperty("isChat", true);
}

void FishVisionPage::processGeminiChatReply(QNetworkReply *reply)
{
    btnSendChat->setEnabled(true);
    btnSendChat->setText("Envoyer");
    
    if (reply->error() != QNetworkReply::NoError) {
        appendChatBotMessage(QStringLiteral("Erreur de connexion : %1").arg(reply->errorString()), false);
        reply->deleteLater();
        return;
    }
    
    const QByteArray bytes = reply->readAll();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        const QJsonObject root = doc.object();
        const QJsonArray candidates = root.value("candidates").toArray();
        if (!candidates.isEmpty()) {
            const QJsonArray parts = candidates.at(0).toObject().value("content").toObject().value("parts").toArray();
            for (const QJsonValue &part : parts) {
                const QString text = part.toObject().value("text").toString().trimmed();
                if (!text.isEmpty()) {
                    appendChatBotMessage(text, false);
                    reply->deleteLater();
                    return;
                }
            }
        }
    }
    
    appendChatBotMessage(QStringLiteral("Je n'ai pas pu generer de reponse."), false);
    reply->deleteLater();
}

void FishVisionPage::appendChatBotMessage(const QString &text, bool fromUser)
{
    const QString color = fromUser ? "#bae6fd" : "#e2e8f0";
    const QString align = fromUser ? "right" : "left";
    const QString icon = fromUser ? "👤 Vous" : "🧠 IA";
    
    QString html = QStringLiteral(
        "<div style='margin-bottom: 12px; text-align: %1;'>"
        "<b><span style='color: %2; font-size: 11px;'>%3</span></b><br>"
        "<span style='color: #1e3a8a; font-size: 13px;'>%4</span>"
        "</div>"
    ).arg(align).arg(color).arg(icon).arg(text.toHtmlEscaped());
    
    chatHistory->append(html);
    
    QScrollBar *sb = chatHistory->verticalScrollBar();
    if (sb) {
        sb->setValue(sb->maximum());
    }
}

