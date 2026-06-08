#include "meteoclient.h"
#include <QUrlQuery>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include "NavireConstants.h"
#include "meteotranslator.h"
#include <QDebug>

MeteoClient::MeteoClient(QObject *parent) : QObject(parent) {
    networkManager = new QNetworkAccessManager(this);
}

void MeteoClient::fetchWeatherData(double lat, double lon) {
    // 1. Fetch Marine Data (Waves)
    QUrl marineUrl(NavireConstants::METEO_MARINE_URL);
    QUrlQuery marineQuery;
    marineQuery.addQueryItem("latitude", QString::number(lat));
    marineQuery.addQueryItem("longitude", QString::number(lon));
    marineQuery.addQueryItem("current", "wave_height");
    marineQuery.addQueryItem("daily", "wave_height_max");
    marineQuery.addQueryItem("timezone", "auto");
    marineUrl.setQuery(marineQuery);

    // 2. Fetch Forecast Data (Visibility, Pressure, Rain)
    QUrl forecastUrl(NavireConstants::METEO_FORECAST_URL);
    QUrlQuery forecastQuery;
    forecastQuery.addQueryItem("latitude", QString::number(lat));
    forecastQuery.addQueryItem("longitude", QString::number(lon));
    forecastQuery.addQueryItem("current", "wind_speed_10m,visibility,surface_pressure,precipitation");
    forecastQuery.addQueryItem("daily", "wind_speed_10m_max,precipitation_probability_max");
    forecastQuery.addQueryItem("wind_speed_unit", "kn");
    forecastQuery.addQueryItem("timezone", "auto");
    forecastUrl.setQuery(forecastQuery);

    auto handleReply = [this, lat, lon](QNetworkReply* reply, bool isMarine) {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[METEO TEST] API Error:" << reply->errorString();
            emit errorOccurred(reply->errorString());
            reply->deleteLater();
            return;
        }
        
        QByteArray data = reply->readAll();
        qDebug() << "[METEO TEST] Received" << (isMarine ? "Marine" : "Forecast") << "data for" << lat << "," << lon;
        qDebug() << "[METEO TEST] Raw Response snippet:" << data.left(200) << "...";

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull()) {
            qDebug() << "[METEO TEST] Error: Failed to parse JSON";
            return;
        }

        static MeteoData::MeteoReport pendingReport;
        static bool marineDone = false;
        static bool forecastDone = false;

        if (isMarine) {
            QJsonObject obj = doc.object();
            auto current = obj["current"].toObject();
            pendingReport.current.waveHeight = current["wave_height"].toDouble();
            
            qDebug() << "[METEO TEST] Parsed Marine -> Wave:" << pendingReport.current.waveHeight << "m";

            auto daily = obj["daily"].toObject();
            auto times = daily["time"].toArray();
            auto maxWaves = daily["wave_height_max"].toArray();
            
            // Sync forecast length
            if (pendingReport.forecast.size() < 7) {
                pendingReport.forecast.clear();
                for(int i=0; i<7; ++i) pendingReport.forecast.append(MeteoData::ForecastDay());
            }

            for (int i = 0; i < qMin(7, (int)times.size()); ++i) {
                pendingReport.forecast[i].date = QDate::fromString(times[i].toString(), Qt::ISODate);
                pendingReport.forecast[i].maxWaveHeight = maxWaves[i].toDouble();
            }
            marineDone = true;
        } else {
            QJsonObject obj = doc.object();
            auto current = obj["current"].toObject();
            pendingReport.current.windSpeed = current["wind_speed_10m"].toDouble();
            pendingReport.current.visibility = current["visibility"].toDouble() / 1000.0;
            pendingReport.current.pressure = current["surface_pressure"].toDouble();
            
            qDebug() << "[METEO TEST] Parsed Forecast -> Wind:" << pendingReport.current.windSpeed << "kn | Vis:" << pendingReport.current.visibility << "km";

            auto daily = obj["daily"].toObject();
            auto maxWinds = daily["wind_speed_10m_max"].toArray();
            auto rainProbs = daily["precipitation_probability_max"].toArray();

            if (pendingReport.forecast.size() < 7) {
                pendingReport.forecast.clear();
                for(int i=0; i<7; ++i) pendingReport.forecast.append(MeteoData::ForecastDay());
            }

            for (int i = 0; i < qMin(7, (int)maxWinds.size()); ++i) {
                pendingReport.forecast[i].maxWindSpeed = maxWinds[i].toDouble();
                pendingReport.forecast[i].rainProbability = rainProbs[i].toDouble();
            }
            forecastDone = true;
        }

        if (marineDone && forecastDone) {
            marineDone = false;
            forecastDone = false;
            pendingReport.current.timestamp = QDateTime::currentDateTime();
            qDebug() << "[METEO TEST] Pipeline Complete. Emitting report for" << NavireConstants::PORT_BASE_NAME;
            emit weatherDataReceived(pendingReport);
        }
        reply->deleteLater();
    };

    QNetworkReply *marineReply = networkManager->get(QNetworkRequest(marineUrl));
    connect(marineReply, &QNetworkReply::finished, [handleReply, marineReply]() { handleReply(marineReply, true); });

    QNetworkReply *forecastReply = networkManager->get(QNetworkRequest(forecastUrl));
    connect(forecastReply, &QNetworkReply::finished, [handleReply, forecastReply]() { handleReply(forecastReply, false); });
}
