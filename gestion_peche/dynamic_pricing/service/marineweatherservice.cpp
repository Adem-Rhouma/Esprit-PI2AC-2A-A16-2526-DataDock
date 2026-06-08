#include "marineweatherservice.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QtMath>

namespace {

double clampScore(double value)
{
    return qBound(0.0, value, 100.0);
}

} // namespace

MarineWeatherService::MarineWeatherService(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_timeoutTimer(new QTimer(this))
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(kTimeoutMs);

    connect(m_timeoutTimer, &QTimer::timeout,
            this, &MarineWeatherService::onRequestTimeout);
}

MarineWeatherService::~MarineWeatherService()
{
    cancelPendingRequest();
}

void MarineWeatherService::fetchMarineData(double latitude, double longitude)
{
    if (latitude < -90.0 || latitude > 90.0
        || longitude < -180.0 || longitude > 180.0) {
        emit marineDataError(
            QStringLiteral("Coordonnees du port invalides. Verifiez DP_PORT_CONFIG."));
        return;
    }

    cancelPendingRequest();

    const QUrl url = buildRequestUrl(latitude, longitude);
    if (!url.isValid()) {
        emit marineDataError(QStringLiteral("URL Open-Meteo invalide."));
        return;
    }

    emit marineDataLoading();

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("DataDock Tarification Dynamique"));

    m_pendingReply = m_networkManager->get(request);
    connect(m_pendingReply, &QNetworkReply::finished,
            this, &MarineWeatherService::onReplyFinished);
    m_timeoutTimer->start();
}

void MarineWeatherService::cancelPendingRequest()
{
    if (m_pendingReply) {
        disconnect(m_pendingReply, &QNetworkReply::finished,
                   this, &MarineWeatherService::onReplyFinished);
        m_pendingReply->abort();
        m_pendingReply->deleteLater();
    }
    clearPendingReply();
}

bool MarineWeatherService::isBusy() const
{
    return !m_pendingReply.isNull();
}

MarineWeatherScores MarineWeatherService::computeScores(const MarineCurrentData &data)
{
    MarineWeatherScores scores;
    if (!data.valid) {
        return scores;
    }

    const double waveScore = clampScore(100.0 - ((data.waveHeight / 3.0) * 100.0));
    const double currentScore =
        clampScore(100.0 - ((data.oceanCurrentVelocity / 20.0) * 100.0));

    double sstScore = 100.0;
    if (data.seaSurfaceTemperature < 10.0) {
        sstScore = clampScore(100.0 - ((10.0 - data.seaSurfaceTemperature) * 12.0));
    } else if (data.seaSurfaceTemperature > 28.0) {
        sstScore = clampScore(100.0 - ((data.seaSurfaceTemperature - 28.0) * 12.0));
    }

    scores.weatherScore = clampScore((waveScore * 0.50)
                                   + (currentScore * 0.30)
                                   + (sstScore * 0.20));

    const double waveRisk = clampScore((data.waveHeight / 4.0) * 100.0);
    const double currentRisk = clampScore((data.oceanCurrentVelocity / 25.0) * 100.0);
    scores.riskScore = clampScore((waveRisk * 0.60) + (currentRisk * 0.40));

    if (scores.riskScore >= 70.0) {
        scores.summary = QStringLiteral("Conditions marines difficiles");
    } else if (scores.weatherScore >= 70.0) {
        scores.summary = QStringLiteral("Conditions marines favorables");
    } else if (scores.weatherScore >= 45.0) {
        scores.summary = QStringLiteral("Conditions marines moderees");
    } else {
        scores.summary = QStringLiteral("Conditions marines instables");
    }

    return scores;
}

void MarineWeatherService::onReplyFinished()
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
        emit marineDataError(
            QStringLiteral("Echec reseau Open-Meteo: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    if (httpStatus < 200 || httpStatus >= 300) {
        emit marineDataError(
            QStringLiteral("Open-Meteo a retourne HTTP %1.").arg(httpStatus));
        reply->deleteLater();
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        emit marineDataError(
            QStringLiteral("JSON marine invalide: %1").arg(parseError.errorString()));
        reply->deleteLater();
        return;
    }

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("error")).toBool(false)) {
        emit marineDataError(
            root.value(QStringLiteral("reason")).toString(
                QStringLiteral("Erreur Open-Meteo inconnue.")));
        reply->deleteLater();
        return;
    }

    const QJsonValue currentValue = root.value(QStringLiteral("current"));
    if (!currentValue.isObject()) {
        emit marineDataError(
            QStringLiteral("La reponse Open-Meteo ne contient pas l'objet 'current'."));
        reply->deleteLater();
        return;
    }

    const QJsonObject current = currentValue.toObject();
    MarineCurrentData data;
    bool ok = true;
    ok = extractDouble(current, QStringLiteral("wave_height"), &data.waveHeight) && ok;
    ok = extractDouble(current, QStringLiteral("wave_direction"), &data.waveDirection) && ok;
    ok = extractDouble(current, QStringLiteral("wave_period"), &data.wavePeriod) && ok;
    ok = extractDouble(current, QStringLiteral("ocean_current_velocity"), &data.oceanCurrentVelocity) && ok;
    ok = extractDouble(current, QStringLiteral("ocean_current_direction"), &data.oceanCurrentDirection) && ok;
    ok = extractDouble(current, QStringLiteral("sea_surface_temperature"), &data.seaSurfaceTemperature) && ok;

    if (!ok) {
        emit marineDataError(
            QStringLiteral("La reponse Open-Meteo ne contient pas tous les champs marins attendus."));
        reply->deleteLater();
        return;
    }

    data.requestLatitude = root.value(QStringLiteral("latitude")).toDouble();
    data.requestLongitude = root.value(QStringLiteral("longitude")).toDouble();
    data.observationTime = current.value(QStringLiteral("time")).toString();
    data.rawJson = QString::fromUtf8(payload.left(kMaxRawJsonBytes));
    data.valid = true;

    const MarineWeatherScores scores = computeScores(data);
    emit marineDataReady(data, scores);

    reply->deleteLater();
}

void MarineWeatherService::onRequestTimeout()
{
    if (!m_pendingReply) {
        return;
    }

    disconnect(m_pendingReply, &QNetworkReply::finished,
               this, &MarineWeatherService::onReplyFinished);
    m_pendingReply->abort();
    m_pendingReply->deleteLater();
    clearPendingReply();

    emit marineDataError(
        QStringLiteral("Delai depasse lors du chargement Open-Meteo."));
}

QUrl MarineWeatherService::buildRequestUrl(double latitude, double longitude)
{
    QUrl url(QStringLiteral("https://marine-api.open-meteo.com/v1/marine"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("latitude"), QString::number(latitude, 'f', 6));
    query.addQueryItem(QStringLiteral("longitude"), QString::number(longitude, 'f', 6));
    query.addQueryItem(
        QStringLiteral("current"),
        QStringLiteral("wave_height,wave_direction,wave_period,ocean_current_velocity,ocean_current_direction,sea_surface_temperature"));
    url.setQuery(query);
    return url;
}

bool MarineWeatherService::extractDouble(const QJsonObject &object,
                                         const QString &key,
                                         double *outValue)
{
    if (!outValue || !object.contains(key) || object.value(key).isNull()) {
        return false;
    }

    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        return false;
    }

    *outValue = value.toDouble();
    return true;
}

void MarineWeatherService::clearPendingReply()
{
    m_timeoutTimer->stop();
    m_pendingReply.clear();
}
