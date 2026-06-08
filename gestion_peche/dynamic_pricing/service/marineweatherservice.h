#ifndef MARINEWEATHERSERVICE_H
#define MARINEWEATHERSERVICE_H

#include "../models/marinedto.h"

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QObject>
#include <QTimer>
#include <QUrl>

class QNetworkReply;

class MarineWeatherService : public QObject
{
    Q_OBJECT

public:
    explicit MarineWeatherService(QObject *parent = nullptr);
    ~MarineWeatherService() override;

    void fetchMarineData(double latitude, double longitude);
    void cancelPendingRequest();
    bool isBusy() const;

    static MarineWeatherScores computeScores(const MarineCurrentData &data);

signals:
    void marineDataLoading();
    void marineDataReady(const MarineCurrentData &data,
                         const MarineWeatherScores &scores);
    void marineDataError(const QString &errorMessage);

private slots:
    void onReplyFinished();
    void onRequestTimeout();

private:
    static QUrl buildRequestUrl(double latitude, double longitude);
    static bool extractDouble(const QJsonObject &object,
                              const QString &key,
                              double *outValue);
    void clearPendingReply();

    QNetworkAccessManager *m_networkManager = nullptr;
    QPointer<QNetworkReply> m_pendingReply;
    QTimer *m_timeoutTimer = nullptr;

    static constexpr int kTimeoutMs = 10000;
    static constexpr int kMaxRawJsonBytes = 4096;
};

#endif // MARINEWEATHERSERVICE_H
