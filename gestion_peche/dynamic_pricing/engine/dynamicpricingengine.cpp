#include "dynamicpricingengine.h"

#include <QDateTime>
#include <QStringList>
#include <QtMath>

namespace {
double clampPrice(double value)
{
    return value < 0.0 ? 0.0 : value;
}

double safePercent(double numerator, double denominator)
{
    if (qFuzzyIsNull(denominator)) {
        return 0.0;
    }
    return (numerator / denominator) * 100.0;
}

QString signedCurrency(double value)
{
    const QString prefix = value >= 0.0 ? QStringLiteral("+") : QString();
    return prefix + QString::number(value, 'f', 3) + QStringLiteral(" TND/kg");
}
}

DynamicPricingEngine::DynamicPricingEngine() = default;

DynamicPricingRecommendation DynamicPricingEngine::calculate(const DynamicPricingContext &context,
                                                            bool manualOverrideEnabled,
                                                            double manualOverridePrice) const
{
    const DynamicPricingRule &rule = context.activeRule();
    const double basePrice = context.basePrice() > 0.0
                                 ? context.basePrice()
                                 : (context.currentAveragePrice() > 0.0
                                        ? context.currentAveragePrice()
                                        : context.lastMarketPrice());

    const double freshnessSignal =
        qBound(-1.0, (context.freshnessScore() - 50.0) / 50.0, 1.0);
    const double demandSignal =
        qBound(-1.0, (context.demandIndex() - 50.0) / 50.0, 1.0);
    const double stockSignal = context.stockCoverageDays() <= rule.lowStockCoverageDays()
                                   ? -1.0
                                   : (context.stockCoverageDays() >= rule.highStockCoverageDays()
                                          ? 1.0
                                          : ((context.stockCoverageDays() - rule.lowStockCoverageDays())
                                             / qMax(1.0,
                                                    rule.highStockCoverageDays()
                                                        - rule.lowStockCoverageDays()))
                                                * 2.0
                                                - 1.0);
    const double weatherSignal =
        qBound(-1.0, (context.marineRiskScore() - 50.0) / 50.0, 1.0);
    const double marketSignal =
        qBound(-1.0, context.marketTrendPct() / 15.0, 1.0);
    const double raritySignal =
        qBound(-1.0, (context.rarityScore() - 50.0) / 50.0, 1.0);

    const double freshnessFactor = basePrice * (rule.freshnessWeightPct() / 100.0) * freshnessSignal;
    const double demandFactor = basePrice * (rule.demandWeightPct() / 100.0) * demandSignal;
    const double stockPressureFactor =
        basePrice * (rule.stockPressureWeightPct() / 100.0) * stockSignal;
    const double weatherRiskFactor =
        basePrice * (rule.weatherRiskWeightPct() / 100.0) * weatherSignal;
    const double marketTrendFactor =
        basePrice * (rule.marketTrendWeightPct() / 100.0) * marketSignal;
    const double rarityFactor = basePrice * (rule.rarityWeightPct() / 100.0) * raritySignal;

    const double algorithmicPrice = clampPrice(basePrice
                                               + freshnessFactor
                                               + demandFactor
                                               - stockPressureFactor
                                               + weatherRiskFactor
                                               + marketTrendFactor
                                               + rarityFactor);

    const double marginFloorPrice =
        qMax(context.baseCost() * (1.0 + rule.marginFloorPct() / 100.0), 0.0);
    const double guardrailFloor =
        qMax(qMax(basePrice * (1.0 + rule.minGuardrailPct() / 100.0), context.minPrice()),
             marginFloorPrice);

    double guardrailCeiling = basePrice * (1.0 + rule.maxGuardrailPct() / 100.0);
    if (context.maxPrice() > 0.0) {
        guardrailCeiling = qMin(guardrailCeiling, context.maxPrice());
    }
    if (guardrailCeiling < guardrailFloor) {
        guardrailCeiling = guardrailFloor;
    }

    const DynamicPricingManualOverride &dbOverride = context.activeOverride();
    const bool hasOperatorOverride = manualOverrideEnabled || dbOverride.active();
    const double operatorPrice = manualOverrideEnabled ? manualOverridePrice : dbOverride.overridePrice();

    DynamicPricingRecommendation recommendation;
    recommendation.setGeneratedAt(QDateTime::currentDateTime());
    recommendation.setBasePrice(basePrice);
    recommendation.setFreshnessFactor(freshnessFactor);
    recommendation.setDemandFactor(demandFactor);
    recommendation.setStockPressureFactor(stockPressureFactor);
    recommendation.setWeatherRiskFactor(weatherRiskFactor);
    recommendation.setMarketTrendFactor(marketTrendFactor);
    recommendation.setRarityFactor(rarityFactor);
    recommendation.setAlgorithmicPrice(qBound(guardrailFloor, algorithmicPrice, guardrailCeiling));
    recommendation.setFloorPrice(guardrailFloor);
    recommendation.setCeilingPrice(guardrailCeiling);
    recommendation.setMarginFloorPrice(marginFloorPrice);
    recommendation.setManualOverrideEnabled(hasOperatorOverride);
    recommendation.setManualOverridePrice(hasOperatorOverride ? operatorPrice : 0.0);

    const double boundedOperatorPrice =
        hasOperatorOverride ? qBound(guardrailFloor, clampPrice(operatorPrice), guardrailCeiling) : 0.0;
    const double effectivePrice =
        hasOperatorOverride ? boundedOperatorPrice : recommendation.algorithmicPrice();
    recommendation.setSuggestedPrice(effectivePrice);
    recommendation.setOverrideApplied(hasOperatorOverride);
    recommendation.setDecisionSource(hasOperatorOverride
                                         ? QStringLiteral("MANUAL_OVERRIDE")
                                         : QStringLiteral("ENGINE"));

    const double changePct = safePercent(effectivePrice - basePrice, qMax(basePrice, 0.001));
    QStringList reasons;
    if (stockSignal < -0.2) {
        reasons << QStringLiteral("low stock");
    } else if (stockSignal > 0.2) {
        reasons << QStringLiteral("high stock");
    }
    if (weatherSignal > 0.2) {
        reasons << QStringLiteral("rough sea conditions");
    }
    if (demandSignal > 0.2) {
        reasons << QStringLiteral("strong demand");
    }
    if (marketSignal > 0.2) {
        reasons << QStringLiteral("rising external market prices");
    }
    if (freshnessSignal > 0.2) {
        reasons << QStringLiteral("high freshness");
    }
    if (raritySignal > 0.2) {
        reasons << QStringLiteral("product rarity");
    }
    if (reasons.isEmpty()) {
        reasons << QStringLiteral("balanced market conditions");
    }

    QString explanation;
    if (changePct >= 0.0) {
        explanation = QStringLiteral("Price increased by %1% due to %2.")
                          .arg(QString::number(changePct, 'f', 1),
                               reasons.join(QStringLiteral(" and ")));
    } else {
        explanation = QStringLiteral("Price decreased by %1% due to %2.")
                          .arg(QString::number(qAbs(changePct), 'f', 1),
                               reasons.join(QStringLiteral(" and ")));
    }

    explanation += QStringLiteral(" Base %1, freshness %2, demand %3, stock pressure %4, weather %5, market %6, rarity %7.")
                       .arg(QString::number(basePrice, 'f', 3) + QStringLiteral(" TND/kg"),
                            signedCurrency(freshnessFactor),
                            signedCurrency(demandFactor),
                            signedCurrency(-stockPressureFactor),
                            signedCurrency(weatherRiskFactor),
                            signedCurrency(marketTrendFactor),
                            signedCurrency(rarityFactor));

    if (hasOperatorOverride) {
        explanation += QStringLiteral(" Manual override has priority and sets the effective price to %1.")
                           .arg(QString::number(effectivePrice, 'f', 3) + QStringLiteral(" TND/kg"));
    }

    recommendation.setExplanation(explanation);
    recommendation.setConfidenceScore(
        qBound(35.0,
               84.0
                   - qAbs(context.marketTrendPct()) * 0.6
                   - qAbs(context.weatherImpactScore() - 50.0) * 0.18
                   - qAbs(context.marineRiskScore() - 50.0) * 0.18
                   - qAbs(context.stockCoverageDays() - 3.0) * 2.0,
               97.0));
    return recommendation;
}
