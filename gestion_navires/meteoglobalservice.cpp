#include "meteoglobalservice.h"
#include "NavireConstants.h"
#include "connection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QDebug>

MeteoGlobalService& MeteoGlobalService::instance() {
    static MeteoGlobalService instance;
    return instance;
}

MeteoGlobalService::MeteoGlobalService(QObject *parent) : QObject(parent) {
    m_client = new MeteoClient(this);
    connect(m_client, &MeteoClient::weatherDataReceived, this, &MeteoGlobalService::onWeatherDataReceived);
}

void MeteoGlobalService::refresh() {
    m_client->fetchWeatherData(NavireConstants::TUNIS_LAT, NavireConstants::TUNIS_LON);
}

int MeteoGlobalService::calculateRiskScore(const MeteoData::MeteoReport &report) {
    // Weighted Sum Model (Normalized to 0-100)
    double windScore = qMin(report.current.windSpeed / 30.0, 1.0) * 40.0; // Max at 30 knots
    double waveScore = qMin(report.current.waveHeight / 4.0, 1.0) * 30.0; // Max at 4 meters
    double visScore = (1.0 - qMin(report.current.visibility / 10.0, 1.0)) * 20.0; // 0 risk at 10km+
    double rainScore = (report.forecast.isEmpty() ? 0 : report.forecast[0].rainProbability / 100.0) * 10.0;
    
    return static_cast<int>(windScore + waveScore + visScore + rainScore);
}

void MeteoGlobalService::saveSnapshot(const MeteoData::MeteoReport &report) {
    QSqlQuery q(Connection::createInstance().db);
    q.prepare("MERGE INTO WEATHER_HISTORY t "
              "USING (SELECT TRUNC(SYSDATE) as d FROM DUAL) s "
              "ON (t.RECORD_DATE = s.d) "
              "WHEN MATCHED THEN UPDATE SET JSON_DATA = :json, WIND_SPEED = :wind, WAVE_HEIGHT = :wave, "
              "VISIBILITY = :vis, PORT_OPERATIONAL_RISK_SCORE = :risk "
              "WHEN NOT MATCHED THEN INSERT (RECORD_DATE, JSON_DATA, WIND_SPEED, WAVE_HEIGHT, VISIBILITY, PORT_OPERATIONAL_RISK_SCORE) "
              "VALUES (TRUNC(SYSDATE), :json, :wind, :wave, :vis, :risk)");
    
    QJsonObject obj;
    obj["wind"] = report.current.windSpeed;
    obj["wave"] = report.current.waveHeight;
    obj["vis"] = report.current.visibility;
    obj["risk"] = calculateRiskScore(report);
    
    q.bindValue(":json", QJsonDocument(obj).toJson());
    q.bindValue(":wind", report.current.windSpeed);
    q.bindValue(":wave", report.current.waveHeight);
    q.bindValue(":vis", report.current.visibility);
    q.bindValue(":risk", calculateRiskScore(report));
    
    if (!q.exec()) {
        qDebug() << "Failed to save weather snapshot:" << q.lastError().text();
    }
}

MeteoGlobalService::HistoricalWeather MeteoGlobalService::getHistoricalWeather(const QDate &date) {
    QSqlQuery q(Connection::createInstance().db);
    q.prepare("SELECT WIND_SPEED, WAVE_HEIGHT, PORT_OPERATIONAL_RISK_SCORE FROM WEATHER_HISTORY "
              "WHERE RECORD_DATE = :d");
    q.bindValue(":d", date);
    
    HistoricalWeather hw = {0.0, 0.0, 0};
    if (q.exec() && q.next()) {
        hw.wind = q.value(0).toDouble();
        hw.waves = q.value(1).toDouble();
        hw.risk = q.value(2).toInt();
    }
    return hw;
}

void MeteoGlobalService::onWeatherDataReceived(const MeteoData::MeteoReport &report) {
    MeteoData::MeteoReport finalReport = report;
    finalReport.advisory.globalRiskScore = calculateRiskScore(finalReport);
    
    // Also calculate risk for forecast days
    for (int i = 0; i < finalReport.forecast.size(); ++i) {
        auto &day = finalReport.forecast[i];
        double windScore = qMin(day.maxWindSpeed / 30.0, 1.0) * 50.0;
        double waveScore = qMin(day.maxWaveHeight / 4.0, 1.0) * 40.0;
        double rainScore = (day.rainProbability / 100.0) * 10.0;
        day.riskScore = static_cast<int>(windScore + waveScore + rainScore);
    }
    
    if (finalReport.advisory.globalRiskScore < 40) finalReport.advisory.riskLevel = NavireConstants::RiskLevel::LOW;
    else if (finalReport.advisory.globalRiskScore < 70) finalReport.advisory.riskLevel = NavireConstants::RiskLevel::MODERATE;
    else finalReport.advisory.riskLevel = NavireConstants::RiskLevel::HIGH;
    
    m_lastReport = finalReport;
    
    // Auto-save snapshot at the 12:00 UTC window
    if (QDateTime::currentDateTime().time().hour() == 12) {
        saveSnapshot(finalReport);
    }
    
    emit dataUpdated(finalReport);
}
