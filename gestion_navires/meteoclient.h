#ifndef METEOCLIENT_H
#define METEOCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "meteo_data.h"

class MeteoClient : public QObject {
    Q_OBJECT
public:
    explicit MeteoClient(QObject *parent = nullptr);
    void fetchWeatherData(double lat, double lon);

signals:
    void weatherDataReceived(const MeteoData::MeteoReport &report);
    void errorOccurred(const QString &error);

private:
    QNetworkAccessManager *networkManager;
};

#endif // METEOCLIENT_H
