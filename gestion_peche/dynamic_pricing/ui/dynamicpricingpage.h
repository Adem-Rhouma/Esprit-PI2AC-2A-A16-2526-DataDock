#ifndef DYNAMICPRICINGPAGE_H
#define DYNAMICPRICINGPAGE_H

#include "../models/dynamicpricingmodels.h"
#include "../models/marinedto.h"
#include "../repository/dynamicpricingrepository.h"
#include "../service/gfwintelligenceservice.h"
#include "../service/marineweatherservice.h"
#include "pricehistorytablemodel.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QDialog;
class QFrame;
class QLabel;
class QProgressBar;
class QPushButton;
class QModelIndex;
class QScrollArea;
class QShowEvent;
class QTableView;
QT_END_NAMESPACE

class DpEngine
{
public:
    DpPricingResult compute(const DpPricingInput &input) const;
};

class DynamicPricingPage : public QWidget
{
    Q_OBJECT

public:
    explicit DynamicPricingPage(QWidget *parent = nullptr);
    ~DynamicPricingPage() override = default;

signals:
    void backRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onSpeciesChanged(int index);
    void onPortChanged(int index);
    void onRecalculateClicked();
    void onSaveClicked();
    void onHistoryCellClicked(const QModelIndex &index);

    void onMarineDataLoading();
    void onMarineDataReady(const MarineCurrentData &data,
                           const MarineWeatherScores &scores);
    void onMarineDataError(const QString &message);

    void onGfwLoading();
    void onGfwReady(const GfwPortIntelligence &intel,
                    const GfwIntelligenceScores &scores);
    void onGfwUnavailable(const QString &message);
    void onGfwError(const QString &message);

private:
    void buildUi();
    void applyStyleSheet();
    void connectSignals();

    QFrame *buildTopBar();
    QScrollArea *buildScrollArea();
    QWidget *buildContent();
    QFrame *buildSpeciesCard();
    QFrame *buildPortCard();
    QFrame *buildBasePriceCard();
    QFrame *buildQuantityCard();
    QFrame *buildFreshnessCard();
    QFrame *buildMarineCard();
    QFrame *buildIntelligenceCard();
    QFrame *buildEnvironmentCard();
    QFrame *buildRecommendationCard();
    QFrame *buildExplanationCard();
    QFrame *buildHistoryCard();

    QFrame *makeCard(const QString &variant = QStringLiteral("default")) const;
    QLabel *makeSectionLabel(const QString &text, QWidget *parent) const;
    QLabel *makeCardTitle(const QString &text, QWidget *parent) const;
    QLabel *makeValueLabel(const QString &text, QWidget *parent, bool compact = false) const;
    QLabel *makeHelperLabel(const QString &text, QWidget *parent) const;
    QProgressBar *makeProgressBar(QWidget *parent, bool accent = false) const;

    void loadSpecies();
    void loadPorts();
    void loadCatchContext();
    void loadHistory();

    void updatePortWidgets();
    void updateContextWidgets();
    void updateMarineCard();
    void updateIntelligenceCard();
    void updateResultWidgets(const DpPricingResult &result);
    void updateEnvironmentCard(const MarineCurrentData &data, const MarineWeatherScores &scores);
    void updateHistoryTable(const QVector<DpHistoryEntry> &entries);
    void updateStatusBadge(QLabel *label, const QString &text, const QString &role) const;
    void showHistoryEntryDialog(int row);

    void rebuildInput();
    void refreshRecommendation();
    void requestExternalData();
    void runEngine();
    void fetchMarineData();
    void applyMarineScores(const MarineCurrentData &data, const MarineWeatherScores &scores);
    void resetEnvironmentCard();
    void setEnvironmentLoadingState();
    void setEnvironmentErrorState(const QString &message);

    void resetMarineState();
    void resetGfwState();
    void setBusy(bool busy);
    void showError(const QString &message);
    void clearError();

    QString currentSpeciesName() const;
    DpPort currentPort() const;

    static QString fmtPrice(double value);
    static QString fmtScore(double value);
    static QString fmtHours(double value);
    static QString fmtBearing(double degrees);

    DpRepository m_repo;
    DpEngine m_engine;

    QVector<DpSpecies> m_species;
    QVector<DpPort> m_ports;
    DpCatchContext m_catchCtx;
    DpPricingInput m_input;
    DpPricingResult m_result;
    MarineCurrentData m_marineData;
    MarineWeatherScores m_marineScores;
    GfwPortIntelligence m_gfwIntel;
    GfwIntelligenceScores m_gfwScores;

    PriceHistoryTableModel *m_historyModel = nullptr;
    MarineWeatherService *m_marineService = nullptr;
    GfwIntelligenceService *m_gfwService = nullptr;

    QPushButton *m_backButton = nullptr;
    QComboBox *m_speciesCombo = nullptr;
    QLabel *m_speciesDesc = nullptr;
    QComboBox *m_portCombo = nullptr;
    QLabel *m_portCoords = nullptr;

    QLabel *m_basePriceValue = nullptr;
    QLabel *m_avgPriceValue = nullptr;
    QLabel *m_basePriceHelper = nullptr;

    QLabel *m_quantityValue = nullptr;
    QLabel *m_catchCountValue = nullptr;
    QLabel *m_availabilityValue = nullptr;
    QLabel *m_availScore = nullptr;
    QLabel *m_zoneValue = nullptr;
    QProgressBar *m_availabilityBar = nullptr;
    QProgressBar *m_availBar = nullptr;

    QLabel *m_freshnessValue = nullptr;
    QLabel *m_catchDateValue = nullptr;
    QLabel *m_freshnessHelper = nullptr;
    QProgressBar *m_freshnessBar = nullptr;

    QLabel *m_marineBadge = nullptr;
    QLabel *m_marineObservedValue = nullptr;
    QLabel *m_waveHeightValue = nullptr;
    QLabel *m_waveDirectionValue = nullptr;
    QLabel *m_wavePeriodValue = nullptr;
    QLabel *m_currentVelocityValue = nullptr;
    QLabel *m_currentDirectionValue = nullptr;
    QLabel *m_sstValue = nullptr;
    QLabel *m_weatherScoreValue = nullptr;
    QLabel *m_riskScoreValue = nullptr;
    QProgressBar *m_weatherBar = nullptr;
    QProgressBar *m_riskBar = nullptr;
    QLabel *m_marineSummary = nullptr;
    QLabel *m_marineError = nullptr;

    QLabel *m_envStatusBadge = nullptr;
    QLabel *m_envObsTime = nullptr;
    QLabel *m_envWaveHeight = nullptr;
    QLabel *m_envWaveDirection = nullptr;
    QLabel *m_envWavePeriod = nullptr;
    QLabel *m_envCurrentVelocity = nullptr;
    QLabel *m_envCurrentDirection = nullptr;
    QLabel *m_envSST = nullptr;
    QLabel *m_envWeatherLabel = nullptr;
    QLabel *m_envMarineLabel = nullptr;
    QProgressBar *m_envWeatherBar = nullptr;
    QProgressBar *m_envRiskBar = nullptr;
    QLabel *m_envSummaryLabel = nullptr;
    QLabel *m_envErrorLabel = nullptr;

    QLabel *m_gfwBadge = nullptr;
    QLabel *m_gfwTokenState = nullptr;
    QLabel *m_gfwHoursValue = nullptr;
    QLabel *m_gfwScoreValue = nullptr;
    QLabel *m_gfwWindowValue = nullptr;
    QLabel *m_gfwSourceValue = nullptr;
    QLabel *m_gfwSummary = nullptr;
    QLabel *m_gfwError = nullptr;
    QProgressBar *m_gfwBar = nullptr;

    QLabel *m_recPriceLabel = nullptr;
    QLabel *m_recDeltaLabel = nullptr;
    QLabel *m_recBandLabel = nullptr;
    QLabel *m_recCompositeLabel = nullptr;
    QLabel *m_recSourceLabel = nullptr;
    QLabel *m_recStatusLabel = nullptr;
    QPushButton *m_recalcButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QLabel *m_errorBanner = nullptr;

    QLabel *m_formulaLabel = nullptr;
    QLabel *m_explanationLabel = nullptr;
    QLabel *m_fallbackLabel = nullptr;

    QTableView *m_historyTable = nullptr;
    QLabel *m_historyStatus = nullptr;
};

#endif // DYNAMICPRICINGPAGE_H
