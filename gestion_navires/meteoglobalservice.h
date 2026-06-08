#ifndef METEOGLOBALSERVICE_H
#define METEOGLOBALSERVICE_H

#include <QObject>
#include <QDateTime>
#include "meteo_data.h"
#include "meteoclient.h"

class MeteoGlobalService : public QObject {
    Q_OBJECT
public:
    static MeteoGlobalService& instance();
    
    // Core API
    void refresh();
    int calculateRiskScore(const MeteoData::MeteoReport &report);
    void saveSnapshot(const MeteoData::MeteoReport &report);
    
    // Analytics
    struct HistoricalWeather {
        double wind;
        double waves;
        int risk;
    };
    HistoricalWeather getHistoricalWeather(const QDate &date);

signals:
    void dataUpdated(const MeteoData::MeteoReport &report);

private slots:
    void onWeatherDataReceived(const MeteoData::MeteoReport &report);

private:
    explicit MeteoGlobalService(QObject *parent = nullptr);
    MeteoClient *m_client;
    MeteoData::MeteoReport m_lastReport;
};

#endif // METEOGLOBALSERVICE_H
