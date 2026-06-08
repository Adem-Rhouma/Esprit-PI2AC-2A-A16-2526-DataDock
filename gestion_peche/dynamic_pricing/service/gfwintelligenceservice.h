#ifndef GFWINTELLIGENCESERVICE_H
#define GFWINTELLIGENCESERVICE_H

#include "../models/marinedto.h"

#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QObject>
#include <QTimer>

class QNetworkReply;

class GfwIntelligenceService : public QObject
{
    Q_OBJECT

public:
    explicit GfwIntelligenceService(QObject *parent = nullptr);
    ~GfwIntelligenceService() override;

    void fetchPortActivity(double latitude,
                           double longitude,
                           const QString &portName,
                           const QString &zoneHint = QString());
    void cancelPendingRequest();
    bool isBusy() const;
    bool hasConfiguredToken() const;

    static GfwIntelligenceScores computeScores(const GfwPortIntelligence &intel);

signals:
    void intelligenceLoading();
    void intelligenceReady(const GfwPortIntelligence &intel,
                           const GfwIntelligenceScores &scores);
    void intelligenceUnavailable(const QString &message);
    void intelligenceError(const QString &message);

private slots:
    void dispatchScheduledRequest();
    void onReplyFinished();
    void onRequestTimeout();

private:
    void queueRequest(double latitude,
                      double longitude,
                      const QString &portName,
                      const QString &zoneHint);
    void issueRequest();
    QString loadToken() const;
    QByteArray buildRequestBody(double latitude, double longitude) const;
    void clearPendingReply();
    double extractHours(const QJsonValue &value, int *sampleCount) const;

    QNetworkAccessManager *m_networkManager = nullptr;
    QPointer<QNetworkReply> m_pendingReply;
    QTimer *m_timeoutTimer = nullptr;
    QTimer *m_dispatchTimer = nullptr;

    QString m_lastToken;
    QString m_pendingPortName;
    QString m_pendingZoneHint;
    double m_pendingLatitude = 0.0;
    double m_pendingLongitude = 0.0;
    bool m_hasQueuedRequest = false;
    bool m_retryAfterReply = false;

    static constexpr int kTimeoutMs = 12000;
    static constexpr int kDispatchDelayMs = 700;
    static constexpr int kRetryAfterReplyMs = 1500;
    static constexpr int kMaxRawJsonBytes = 4096;
};

#endif // GFWINTELLIGENCESERVICE_H
