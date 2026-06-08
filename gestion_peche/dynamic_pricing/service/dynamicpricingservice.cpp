#include "dynamicpricingservice.h"

DynamicPricingService::DynamicPricingService() = default;

bool DynamicPricingService::loadSpeciesCatalog(QVector<DynamicPricingSpecies> *speciesList,
                                               QString *errorMessage) const
{
    return m_repository.loadSpeciesCatalog(speciesList, errorMessage);
}

bool DynamicPricingService::buildDashboard(int productId,
                                           bool manualOverrideEnabled,
                                           double manualOverridePrice,
                                           DynamicPricingDashboardData *dashboardData,
                                           QString *errorMessage) const
{
    if (!dashboardData) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Le tableau de bord dynamique est invalide.");
        }
        return false;
    }

    DynamicPricingContext context;
    if (!m_repository.loadContext(productId, &context, errorMessage)) {
        return false;
    }

    if (manualOverrideEnabled) {
        DynamicPricingManualOverride transientOverride;
        transientOverride.setProductId(productId);
        transientOverride.setOverridePrice(manualOverridePrice);
        transientOverride.setStatus(QStringLiteral("ACTIVE"));
        transientOverride.setActive(true);
        transientOverride.setOperatorName(QStringLiteral("UI_SESSION"));
        transientOverride.setReasonText(QStringLiteral("Temporary operator override from DynamicPricingPage"));
        context.setActiveOverride(transientOverride);
    }

    const DynamicPricingRecommendation recommendation =
        m_engine.calculate(context, manualOverrideEnabled, manualOverridePrice);

    QVector<DynamicPricingHistoryEntry> history;
    if (!m_repository.loadHistory(productId, recommendation, &history, errorMessage)) {
        return false;
    }

    dashboardData->setContext(context);
    dashboardData->setRecommendation(recommendation);
    dashboardData->setHistory(history);
    return true;
}

bool DynamicPricingService::saveRecommendation(const DynamicPricingDashboardData &dashboardData,
                                               QString *errorMessage) const
{
    return m_repository.saveRecommendation(dashboardData.context(),
                                           dashboardData.recommendation(),
                                           errorMessage);
}

bool DynamicPricingService::saveManualOverride(int productId,
                                               double overridePrice,
                                               const QString &operatorName,
                                               const QString &reasonText,
                                               const QDateTime &startsAt,
                                               const QDateTime &endsAt,
                                               QString *errorMessage) const
{
    DynamicPricingManualOverride manualOverride;
    manualOverride.setProductId(productId);
    manualOverride.setOverridePrice(overridePrice);
    manualOverride.setOperatorName(operatorName);
    manualOverride.setReasonText(reasonText);
    manualOverride.setStartsAt(startsAt);
    manualOverride.setEndsAt(endsAt);
    manualOverride.setStatus(QStringLiteral("ACTIVE"));
    manualOverride.setActive(true);
    return m_repository.saveManualOverride(manualOverride, errorMessage);
}

bool DynamicPricingService::cancelManualOverride(int overrideId, QString *errorMessage) const
{
    return m_repository.cancelManualOverride(overrideId, errorMessage);
}
