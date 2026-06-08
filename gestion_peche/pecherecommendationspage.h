#ifndef PECHERECOMMENDATIONSPAGE_H
#define PECHERECOMMENDATIONSPAGE_H

#include <QDateTime>
#include <QList>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QLayout;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;
class QShowEvent;
class QVBoxLayout;
QT_END_NAMESPACE

class QGridLayout;

struct PecheRecommendationItem
{
    QString tone;
    QString title;
    QString detail;
    QString meta;
};

struct PecheRecommendationReport
{
    QString summary;
    QString marketFocus;
    QString freshnessFocus;
    QString actionFocus;
    int confidenceScore = 0;
    QDateTime updatedAt;
    QList<PecheRecommendationItem> pricing;
    QList<PecheRecommendationItem> alerts;
    QList<PecheRecommendationItem> priorities;
    QList<PecheRecommendationItem> sales;
    QList<PecheRecommendationItem> freshness;
    QList<PecheRecommendationItem> anomalies;
    QList<PecheRecommendationItem> actions;
    bool fromFallback = false;
};

class PecheRecommendationsPage : public QWidget
{
    Q_OBJECT

public:
    explicit PecheRecommendationsPage(QWidget *parent = nullptr);
    ~PecheRecommendationsPage() override = default;

signals:
    void backRequested();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onRefreshClicked();
    void onReplyFinished(QNetworkReply *reply);

private:
    void buildUi();
    void requestRecommendations(bool forceRefresh);
    QByteArray buildGeminiPayload() const;
    QString extractGeminiText(const QByteArray &responseBytes, QString *errorMessage) const;
    bool parseReport(const QString &jsonText, PecheRecommendationReport *report, QString *errorMessage) const;
    PecheRecommendationReport buildFallbackReport(const QString &reason = QString()) const;
    void applyReport(const PecheRecommendationReport &report, const QString &statusText, bool isErrorState);
    void setLoadingState();
    void rebuildSection(QVBoxLayout *layout, const QList<PecheRecommendationItem> &items, const QString &emptyMessage);
    QWidget *createSectionCard(const QString &eyebrow, const QString &title, QVBoxLayout **itemsLayout);
    QWidget *createInsightCard(const QString &value, const QString &label, const QString &detail);
    QWidget *createRecommendationCard(const PecheRecommendationItem &item);
    void clearLayout(QLayout *layout);
    QString summarizeBytes(const QByteArray &bytes, int maxLen = 700) const;
    static QString toneColor(const QString &tone);
    static QString toneBadgeStyle(const QString &tone);
    static QString normalizedTone(const QString &tone);

    QNetworkAccessManager *m_networkManager = nullptr;
    QPushButton *m_backButton = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QLabel *m_statusBadge = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QLabel *m_metaLabel = nullptr;
    QLabel *m_errorLabel = nullptr;
    QGridLayout *m_insightLayout = nullptr;
    QVBoxLayout *m_pricingLayout = nullptr;
    QVBoxLayout *m_alertsLayout = nullptr;
    QVBoxLayout *m_prioritiesLayout = nullptr;
    QVBoxLayout *m_salesLayout = nullptr;
    QVBoxLayout *m_freshnessLayout = nullptr;
    QVBoxLayout *m_anomaliesLayout = nullptr;
    QVBoxLayout *m_actionsLayout = nullptr;
    bool m_hasLoadedOnce = false;
    bool m_isLoading = false;
};

#endif // PECHERECOMMENDATIONSPAGE_H
