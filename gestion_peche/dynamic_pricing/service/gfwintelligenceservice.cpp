#include "gfwintelligenceservice.h"

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>
#include <QtMath>

namespace {

// Local development bootstrap for GFW.
// Replace the placeholder below with your token once for this project.
// If QSettings does not already contain a token, the service will persist
// this value automatically under dynamic_pricing/gfw_token at runtime.
static const char * const kLocalDevGfwToken = "";

double clampScore(double value)
{
    return qBound(0.0, value, 100.0);
}

QString scoreSummary(double hours, double score)
{
    if (hours <= 0.01) {
        return QStringLiteral("Aucune activite de peche detectee pres du port selectionne.");
    }
    if (score >= 75.0) {
        return QStringLiteral("Activite de peche elevee pres du port. La pression externe sur le prix est forte.");
    }
    if (score >= 45.0) {
        return QStringLiteral("Activite de peche regionale moderee. La pression externe reste utile mais equilibree.");
    }
    return QStringLiteral("Activite de peche regionale faible. Pression concurrentielle externe limitee.");
}

} // namespace

GfwIntelligenceService::GfwIntelligenceService(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_timeoutTimer(new QTimer(this))
    , m_dispatchTimer(new QTimer(this))
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(kTimeoutMs);
    m_dispatchTimer->setSingleShot(true);
    m_dispatchTimer->setInterval(kDispatchDelayMs);

    connect(m_timeoutTimer, &QTimer::timeout,
            this, &GfwIntelligenceService::onRequestTimeout);
    connect(m_dispatchTimer, &QTimer::timeout,
            this, &GfwIntelligenceService::dispatchScheduledRequest);
}

GfwIntelligenceService::~GfwIntelligenceService()
{
    cancelPendingRequest();
}

void GfwIntelligenceService::fetchPortActivity(double latitude,
                                               double longitude,
                                               const QString &portName,
                                               const QString &zoneHint)
{
    if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
        emit intelligenceError(QStringLiteral("Coordonnees du port invalides pour Global Fishing Watch."));
        return;
    }

    m_lastToken = loadToken();
    if (m_lastToken.isEmpty()) {
        emit intelligenceUnavailable(
            QStringLiteral("Jeton Global Fishing Watch manquant. Definissez dynamic_pricing/gfw_token dans QSettings ou GFW_TOKEN dans l'environnement local."));
        return;
    }

    queueRequest(latitude, longitude, portName, zoneHint);
    emit intelligenceLoading();
}

void GfwIntelligenceService::queueRequest(double latitude,
                                          double longitude,
                                          const QString &portName,
                                          const QString &zoneHint)
{
    m_pendingLatitude = latitude;
    m_pendingLongitude = longitude;
    m_pendingPortName = portName;
    m_pendingZoneHint = zoneHint;
    m_hasQueuedRequest = true;

    if (m_pendingReply) {
        m_retryAfterReply = true;
        return;
    }

    m_dispatchTimer->start();
}

void GfwIntelligenceService::dispatchScheduledRequest()
{
    if (m_pendingReply || !m_hasQueuedRequest) {
        return;
    }
    issueRequest();
}

void GfwIntelligenceService::issueRequest()
{
    if (!m_hasQueuedRequest) {
        return;
    }
    m_hasQueuedRequest = false;

    // GFW indique un delai de traitement d'environ 96 heures pour
    // public-global-fishing-effort:latest. On recule donc la fenetre.
    const QDate endDate = QDate::currentDate().addDays(-4);
    const QDate startDate = endDate.addDays(-6);

    QUrl url(QStringLiteral("https://gateway.api.globalfishingwatch.org/v3/4wings/report"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("datasets[0]"),
                       QStringLiteral("public-global-fishing-effort:latest"));
    query.addQueryItem(QStringLiteral("spatial-resolution"), QStringLiteral("LOW"));
    query.addQueryItem(QStringLiteral("temporal-resolution"), QStringLiteral("ENTIRE"));
    query.addQueryItem(QStringLiteral("spatial-aggregation"), QStringLiteral("false"));
    query.addQueryItem(QStringLiteral("date-range"),
                       startDate.toString(Qt::ISODate) + QStringLiteral(",")
                           + endDate.toString(Qt::ISODate));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("JSON"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("DataDock Tarification Dynamique"));
    request.setRawHeader("Authorization",
                         QByteArray("Bearer ") + m_lastToken.toUtf8());

    m_pendingReply = m_networkManager->post(request, buildRequestBody(m_pendingLatitude, m_pendingLongitude));
    connect(m_pendingReply, &QNetworkReply::finished,
            this, &GfwIntelligenceService::onReplyFinished);
    m_timeoutTimer->start();
}

void GfwIntelligenceService::cancelPendingRequest()
{
    m_dispatchTimer->stop();
    m_hasQueuedRequest = false;
    m_retryAfterReply = false;

    if (m_pendingReply) {
        disconnect(m_pendingReply, &QNetworkReply::finished,
                   this, &GfwIntelligenceService::onReplyFinished);
        m_pendingReply->abort();
        m_pendingReply->deleteLater();
    }

    clearPendingReply();
}

bool GfwIntelligenceService::isBusy() const
{
    return !m_pendingReply.isNull() || m_dispatchTimer->isActive();
}

bool GfwIntelligenceService::hasConfiguredToken() const
{
    return !loadToken().isEmpty();
}

GfwIntelligenceScores GfwIntelligenceService::computeScores(const GfwPortIntelligence &intel)
{
    GfwIntelligenceScores scores;
    if (!intel.valid) {
        scores.summary = QStringLiteral("Global Fishing Watch indisponible. Utilisation du contexte local uniquement.");
        scores.sourceTag = QStringLiteral("LOCAL_SEUL");
        return scores;
    }

    const double activityScore = clampScore(100.0 * (1.0 - qExp(-intel.totalFishingHours / 8.0)));
    scores.activityScore = activityScore;
    scores.summary = scoreSummary(intel.totalFishingHours, activityScore);
    scores.sourceTag = QStringLiteral("GFW_4WINGS");
    return scores;
}

void GfwIntelligenceService::onReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply || reply != m_pendingReply) {
        if (reply) {
            reply->deleteLater();
        }
        return;
    }

    m_timeoutTimer->stop();
    m_pendingReply.clear();

    const QByteArray payload = reply->readAll();
    const int httpStatus =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        QString serverDetail;
        QJsonParseError parseError;
        const QJsonDocument errorDocument = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error == QJsonParseError::NoError && errorDocument.isObject()) {
            const QJsonObject root = errorDocument.object();
            if (root.value(QStringLiteral("message")).isString()) {
                serverDetail = root.value(QStringLiteral("message")).toString().trimmed();
            }
            const QJsonValue messagesValue = root.value(QStringLiteral("messages"));
            if (serverDetail.isEmpty() && messagesValue.isArray()) {
                const QJsonArray messages = messagesValue.toArray();
                if (!messages.isEmpty() && messages.first().isObject()) {
                    const QJsonObject messageObject = messages.first().toObject();
                    const QString title = messageObject.value(QStringLiteral("title")).toString().trimmed();
                    const QString detail = messageObject.value(QStringLiteral("detail")).toString().trimmed();
                    serverDetail = QStringLiteral("%1 %2").arg(title, detail).trimmed();
                }
            }
        }

        const QString message =
            (httpStatus == 429)
                ? QStringLiteral("Global Fishing Watch limite les rapports consecutifs. La derniere demande va etre relancee automatiquement.")
                : (serverDetail.isEmpty()
                       ? QStringLiteral("Erreur reseau Global Fishing Watch : %1").arg(reply->errorString())
                       : QStringLiteral("Erreur Global Fishing Watch : %1").arg(serverDetail));
        emit intelligenceError(message);
        if (httpStatus == 429) {
            m_retryAfterReply = true;
        }
        reply->deleteLater();
        if (m_retryAfterReply && !m_pendingReply && !m_dispatchTimer->isActive() && !m_hasQueuedRequest) {
            m_hasQueuedRequest = true;
            m_dispatchTimer->start(kRetryAfterReplyMs);
        } else if (m_retryAfterReply && !m_pendingReply && !m_dispatchTimer->isActive()) {
            m_dispatchTimer->start(kRetryAfterReplyMs);
        }
        return;
    }

    if (httpStatus < 200 || httpStatus >= 300) {
        QString serverDetail;
        QJsonParseError parseError;
        const QJsonDocument errorDocument = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error == QJsonParseError::NoError && errorDocument.isObject()) {
            const QJsonObject root = errorDocument.object();
            if (root.value(QStringLiteral("message")).isString()) {
                serverDetail = root.value(QStringLiteral("message")).toString().trimmed();
            }
            const QJsonValue messagesValue = root.value(QStringLiteral("messages"));
            if (serverDetail.isEmpty() && messagesValue.isArray()) {
                const QJsonArray messages = messagesValue.toArray();
                if (!messages.isEmpty() && messages.first().isObject()) {
                    const QJsonObject messageObject = messages.first().toObject();
                    const QString title = messageObject.value(QStringLiteral("title")).toString().trimmed();
                    const QString detail = messageObject.value(QStringLiteral("detail")).toString().trimmed();
                    serverDetail = QStringLiteral("%1 %2").arg(title, detail).trimmed();
                }
            }
        }
        emit intelligenceError(
            serverDetail.isEmpty()
                ? QStringLiteral("Global Fishing Watch a retourne HTTP %1.").arg(httpStatus)
                : QStringLiteral("Global Fishing Watch a retourne HTTP %1 : %2")
                      .arg(httpStatus)
                      .arg(serverDetail));
        reply->deleteLater();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        emit intelligenceError(
            QStringLiteral("JSON Global Fishing Watch invalide : %1").arg(parseError.errorString()));
        reply->deleteLater();
        return;
    }

    const QJsonObject root = document.object();
    int sampleCount = 0;
    const double totalHours = extractHours(root, &sampleCount);

    GfwPortIntelligence intel;
    intel.portName = m_pendingPortName;
    intel.zoneHint = m_pendingZoneHint;
    intel.startDate = QDate::currentDate().addDays(-10).toString(Qt::ISODate);
    intel.endDate = QDate::currentDate().addDays(-4).toString(Qt::ISODate);
    intel.totalFishingHours = totalHours;
    intel.sampleCells = sampleCount;
    intel.rawJson = QString::fromUtf8(payload.left(kMaxRawJsonBytes));
    intel.valid = true;
    intel.tokenConfigured = true;

    const GfwIntelligenceScores scores = computeScores(intel);
    emit intelligenceReady(intel, scores);

    reply->deleteLater();
    if (m_retryAfterReply || m_hasQueuedRequest) {
        m_retryAfterReply = false;
        m_dispatchTimer->start(kRetryAfterReplyMs);
    }
}

void GfwIntelligenceService::onRequestTimeout()
{
    if (!m_pendingReply) {
        return;
    }

    disconnect(m_pendingReply, &QNetworkReply::finished,
               this, &GfwIntelligenceService::onReplyFinished);
    m_pendingReply->abort();
    m_pendingReply->deleteLater();
    clearPendingReply();

    emit intelligenceError(QStringLiteral("Delai depasse pour la requete Global Fishing Watch."));
    if (m_hasQueuedRequest) {
        m_dispatchTimer->start(kRetryAfterReplyMs);
    }
}

QString GfwIntelligenceService::loadToken() const
{
    QSettings settings;
    QString token = settings.value(QStringLiteral("dynamic_pricing/gfw_token")).toString().trimmed();
    if (token.isEmpty()) {
        token = settings.value(QStringLiteral("DynamicPricing/gfwToken")).toString().trimmed();
    }
    if (token.isEmpty()) {
        const QString appIniPath =
            QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("dynamic_pricing.local.ini"));
        QSettings appIni(appIniPath, QSettings::IniFormat);
        token = appIni.value(QStringLiteral("dynamic_pricing/gfw_token")).toString().trimmed();
    }
    if (token.isEmpty()) {
        const QString currentIniPath =
            QDir::current().filePath(QStringLiteral("dynamic_pricing.local.ini"));
        QSettings currentIni(currentIniPath, QSettings::IniFormat);
        token = currentIni.value(QStringLiteral("dynamic_pricing/gfw_token")).toString().trimmed();
    }
    if (token.isEmpty()) {
        token = qEnvironmentVariable("GFW_TOKEN").trimmed();
    }
    if (token.isEmpty()) {
        const QString bootstrapToken = QString::fromUtf8(kLocalDevGfwToken).trimmed();
        if (!bootstrapToken.isEmpty()
            && bootstrapToken != QStringLiteral("PASTE_YOUR_GFW_TOKEN_HERE")) {
            settings.setValue(QStringLiteral("dynamic_pricing/gfw_token"), bootstrapToken);
            token = bootstrapToken;
        }
    }
    return token;
}

QByteArray GfwIntelligenceService::buildRequestBody(double latitude, double longitude) const
{
    const double latDelta = 0.18;
    const double lonDelta = qMax(0.18, 0.18 / qMax(0.25, qCos(qDegreesToRadians(latitude))));

    QJsonArray polygon;
    polygon.append(QJsonArray{longitude - lonDelta, latitude - latDelta});
    polygon.append(QJsonArray{longitude + lonDelta, latitude - latDelta});
    polygon.append(QJsonArray{longitude + lonDelta, latitude + latDelta});
    polygon.append(QJsonArray{longitude - lonDelta, latitude + latDelta});
    polygon.append(QJsonArray{longitude - lonDelta, latitude - latDelta});

    QJsonObject geometry;
    geometry.insert(QStringLiteral("type"), QStringLiteral("Polygon"));
    geometry.insert(QStringLiteral("coordinates"), QJsonArray{polygon});

    QJsonObject payload;
    payload.insert(QStringLiteral("geojson"), geometry);

    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

void GfwIntelligenceService::clearPendingReply()
{
    m_timeoutTimer->stop();
    m_pendingReply.clear();
}

double GfwIntelligenceService::extractHours(const QJsonValue &value, int *sampleCount) const
{
    double total = 0.0;

    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        if (object.contains(QStringLiteral("hours")) && object.value(QStringLiteral("hours")).isDouble()) {
            total += object.value(QStringLiteral("hours")).toDouble();
            if (sampleCount) {
                ++(*sampleCount);
            }
        }

        const QJsonObject::const_iterator end = object.constEnd();
        for (QJsonObject::const_iterator it = object.constBegin(); it != end; ++it) {
            if (it.key() == QStringLiteral("hours")) {
                continue;
            }
            total += extractHours(it.value(), sampleCount);
        }
    } else if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue &entry : array) {
            total += extractHours(entry, sampleCount);
        }
    }

    return total;
}
