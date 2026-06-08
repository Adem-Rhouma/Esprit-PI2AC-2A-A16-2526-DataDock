#include "dynamicpricingmodels.h"

DynamicPricingSpecies::DynamicPricingSpecies()
    : m_id(0)
{
}

DynamicPricingSpecies::DynamicPricingSpecies(int id, const QString &name, const QString &sku)
    : m_id(id)
    , m_name(name)
    , m_sku(sku)
{
}

int DynamicPricingSpecies::id() const { return m_id; }
const QString &DynamicPricingSpecies::name() const { return m_name; }
const QString &DynamicPricingSpecies::sku() const { return m_sku; }
const QString &DynamicPricingSpecies::status() const { return m_status; }
void DynamicPricingSpecies::setId(int id) { m_id = id; }
void DynamicPricingSpecies::setName(const QString &name) { m_name = name; }
void DynamicPricingSpecies::setSku(const QString &sku) { m_sku = sku; }
void DynamicPricingSpecies::setStatus(const QString &status) { m_status = status; }

DynamicPricingRule::DynamicPricingRule()
    : m_ruleId(0)
    , m_productId(0)
    , m_priority(100)
    , m_freshnessWeightPct(12.0)
    , m_demandWeightPct(18.0)
    , m_stockPressureWeightPct(14.0)
    , m_weatherRiskWeightPct(9.0)
    , m_marketTrendWeightPct(10.0)
    , m_rarityWeightPct(7.0)
    , m_minGuardrailPct(-10.0)
    , m_maxGuardrailPct(18.0)
    , m_marginFloorPct(12.0)
    , m_lowStockCoverageDays(2.0)
    , m_highStockCoverageDays(6.0)
    , m_targetFreshnessHours(24.0)
{
}

int DynamicPricingRule::ruleId() const { return m_ruleId; }
int DynamicPricingRule::productId() const { return m_productId; }
int DynamicPricingRule::priority() const { return m_priority; }
const QString &DynamicPricingRule::ruleName() const { return m_ruleName; }
const QString &DynamicPricingRule::status() const { return m_status; }
double DynamicPricingRule::freshnessWeightPct() const { return m_freshnessWeightPct; }
double DynamicPricingRule::demandWeightPct() const { return m_demandWeightPct; }
double DynamicPricingRule::stockPressureWeightPct() const { return m_stockPressureWeightPct; }
double DynamicPricingRule::weatherRiskWeightPct() const { return m_weatherRiskWeightPct; }
double DynamicPricingRule::marketTrendWeightPct() const { return m_marketTrendWeightPct; }
double DynamicPricingRule::rarityWeightPct() const { return m_rarityWeightPct; }
double DynamicPricingRule::minGuardrailPct() const { return m_minGuardrailPct; }
double DynamicPricingRule::maxGuardrailPct() const { return m_maxGuardrailPct; }
double DynamicPricingRule::marginFloorPct() const { return m_marginFloorPct; }
double DynamicPricingRule::lowStockCoverageDays() const { return m_lowStockCoverageDays; }
double DynamicPricingRule::highStockCoverageDays() const { return m_highStockCoverageDays; }
double DynamicPricingRule::targetFreshnessHours() const { return m_targetFreshnessHours; }
void DynamicPricingRule::setRuleId(int ruleId) { m_ruleId = ruleId; }
void DynamicPricingRule::setProductId(int productId) { m_productId = productId; }
void DynamicPricingRule::setPriority(int priority) { m_priority = priority; }
void DynamicPricingRule::setRuleName(const QString &ruleName) { m_ruleName = ruleName; }
void DynamicPricingRule::setStatus(const QString &status) { m_status = status; }
void DynamicPricingRule::setFreshnessWeightPct(double freshnessWeightPct) { m_freshnessWeightPct = freshnessWeightPct; }
void DynamicPricingRule::setDemandWeightPct(double demandWeightPct) { m_demandWeightPct = demandWeightPct; }
void DynamicPricingRule::setStockPressureWeightPct(double stockPressureWeightPct) { m_stockPressureWeightPct = stockPressureWeightPct; }
void DynamicPricingRule::setWeatherRiskWeightPct(double weatherRiskWeightPct) { m_weatherRiskWeightPct = weatherRiskWeightPct; }
void DynamicPricingRule::setMarketTrendWeightPct(double marketTrendWeightPct) { m_marketTrendWeightPct = marketTrendWeightPct; }
void DynamicPricingRule::setRarityWeightPct(double rarityWeightPct) { m_rarityWeightPct = rarityWeightPct; }
void DynamicPricingRule::setMinGuardrailPct(double minGuardrailPct) { m_minGuardrailPct = minGuardrailPct; }
void DynamicPricingRule::setMaxGuardrailPct(double maxGuardrailPct) { m_maxGuardrailPct = maxGuardrailPct; }
void DynamicPricingRule::setMarginFloorPct(double marginFloorPct) { m_marginFloorPct = marginFloorPct; }
void DynamicPricingRule::setLowStockCoverageDays(double lowStockCoverageDays) { m_lowStockCoverageDays = lowStockCoverageDays; }
void DynamicPricingRule::setHighStockCoverageDays(double highStockCoverageDays) { m_highStockCoverageDays = highStockCoverageDays; }
void DynamicPricingRule::setTargetFreshnessHours(double targetFreshnessHours) { m_targetFreshnessHours = targetFreshnessHours; }

DynamicPricingManualOverride::DynamicPricingManualOverride()
    : m_overrideId(0)
    , m_productId(0)
    , m_active(false)
    , m_overridePrice(0.0)
{
}

int DynamicPricingManualOverride::overrideId() const { return m_overrideId; }
int DynamicPricingManualOverride::productId() const { return m_productId; }
bool DynamicPricingManualOverride::active() const { return m_active; }
double DynamicPricingManualOverride::overridePrice() const { return m_overridePrice; }
const QString &DynamicPricingManualOverride::status() const { return m_status; }
const QString &DynamicPricingManualOverride::operatorName() const { return m_operatorName; }
const QString &DynamicPricingManualOverride::reasonText() const { return m_reasonText; }
const QDateTime &DynamicPricingManualOverride::startsAt() const { return m_startsAt; }
const QDateTime &DynamicPricingManualOverride::endsAt() const { return m_endsAt; }
void DynamicPricingManualOverride::setOverrideId(int overrideId) { m_overrideId = overrideId; }
void DynamicPricingManualOverride::setProductId(int productId) { m_productId = productId; }
void DynamicPricingManualOverride::setActive(bool active) { m_active = active; }
void DynamicPricingManualOverride::setOverridePrice(double overridePrice) { m_overridePrice = overridePrice; }
void DynamicPricingManualOverride::setStatus(const QString &status) { m_status = status; }
void DynamicPricingManualOverride::setOperatorName(const QString &operatorName) { m_operatorName = operatorName; }
void DynamicPricingManualOverride::setReasonText(const QString &reasonText) { m_reasonText = reasonText; }
void DynamicPricingManualOverride::setStartsAt(const QDateTime &startsAt) { m_startsAt = startsAt; }
void DynamicPricingManualOverride::setEndsAt(const QDateTime &endsAt) { m_endsAt = endsAt; }

DynamicPricingHistoryEntry::DynamicPricingHistoryEntry()
    : m_basePrice(0.0)
    , m_observedPrice(0.0)
    , m_suggestedPrice(0.0)
    , m_effectivePrice(0.0)
    , m_quantityKg(0.0)
    , m_manualOverride(false)
{
}

const QDateTime &DynamicPricingHistoryEntry::timestamp() const { return m_timestamp; }
const QString &DynamicPricingHistoryEntry::label() const { return m_label; }
double DynamicPricingHistoryEntry::basePrice() const { return m_basePrice; }
double DynamicPricingHistoryEntry::observedPrice() const { return m_observedPrice; }
double DynamicPricingHistoryEntry::suggestedPrice() const { return m_suggestedPrice; }
double DynamicPricingHistoryEntry::effectivePrice() const { return m_effectivePrice; }
double DynamicPricingHistoryEntry::quantityKg() const { return m_quantityKg; }
bool DynamicPricingHistoryEntry::manualOverride() const { return m_manualOverride; }
const QString &DynamicPricingHistoryEntry::notes() const { return m_notes; }
const QString &DynamicPricingHistoryEntry::explanation() const { return m_explanation; }
void DynamicPricingHistoryEntry::setTimestamp(const QDateTime &timestamp) { m_timestamp = timestamp; }
void DynamicPricingHistoryEntry::setLabel(const QString &label) { m_label = label; }
void DynamicPricingHistoryEntry::setBasePrice(double basePrice) { m_basePrice = basePrice; }
void DynamicPricingHistoryEntry::setObservedPrice(double observedPrice) { m_observedPrice = observedPrice; }
void DynamicPricingHistoryEntry::setSuggestedPrice(double suggestedPrice) { m_suggestedPrice = suggestedPrice; }
void DynamicPricingHistoryEntry::setEffectivePrice(double effectivePrice) { m_effectivePrice = effectivePrice; }
void DynamicPricingHistoryEntry::setQuantityKg(double quantityKg) { m_quantityKg = quantityKg; }
void DynamicPricingHistoryEntry::setManualOverride(bool manualOverride) { m_manualOverride = manualOverride; }
void DynamicPricingHistoryEntry::setNotes(const QString &notes) { m_notes = notes; }
void DynamicPricingHistoryEntry::setExplanation(const QString &explanation) { m_explanation = explanation; }

DynamicPricingContext::DynamicPricingContext()
    : m_productId(0)
    , m_speciesId(0)
    , m_baseCost(0.0)
    , m_basePrice(0.0)
    , m_minPrice(0.0)
    , m_maxPrice(0.0)
    , m_currentAveragePrice(0.0)
    , m_lastMarketPrice(0.0)
    , m_currentStockKg(0.0)
    , m_reservedStockKg(0.0)
    , m_availableStockKg(0.0)
    , m_sevenDayVolumeKg(0.0)
    , m_thirtyDayVolumeKg(0.0)
    , m_salesVelocityKgPerDay(0.0)
    , m_demandIndex(50.0)
    , m_supplyIndex(50.0)
    , m_weatherImpactScore(50.0)
    , m_marineRiskScore(50.0)
    , m_marketTrendPct(0.0)
    , m_volatilityIndex(0.0)
    , m_stockCoverageDays(0.0)
    , m_freshnessHours(0.0)
    , m_freshnessScore(50.0)
    , m_rarityScore(50.0)
{
}

int DynamicPricingContext::productId() const { return m_productId; }
int DynamicPricingContext::speciesId() const { return m_speciesId; }
const QString &DynamicPricingContext::speciesName() const { return m_speciesName; }
const QString &DynamicPricingContext::productCode() const { return m_productCode; }
double DynamicPricingContext::baseCost() const { return m_baseCost; }
double DynamicPricingContext::basePrice() const { return m_basePrice; }
double DynamicPricingContext::minPrice() const { return m_minPrice; }
double DynamicPricingContext::maxPrice() const { return m_maxPrice; }
double DynamicPricingContext::currentAveragePrice() const { return m_currentAveragePrice; }
double DynamicPricingContext::lastMarketPrice() const { return m_lastMarketPrice; }
double DynamicPricingContext::currentStockKg() const { return m_currentStockKg; }
double DynamicPricingContext::reservedStockKg() const { return m_reservedStockKg; }
double DynamicPricingContext::availableStockKg() const { return m_availableStockKg; }
double DynamicPricingContext::sevenDayVolumeKg() const { return m_sevenDayVolumeKg; }
double DynamicPricingContext::thirtyDayVolumeKg() const { return m_thirtyDayVolumeKg; }
double DynamicPricingContext::salesVelocityKgPerDay() const { return m_salesVelocityKgPerDay; }
double DynamicPricingContext::demandIndex() const { return m_demandIndex; }
double DynamicPricingContext::supplyIndex() const { return m_supplyIndex; }
double DynamicPricingContext::weatherImpactScore() const { return m_weatherImpactScore; }
double DynamicPricingContext::marineRiskScore() const { return m_marineRiskScore; }
double DynamicPricingContext::marketTrendPct() const { return m_marketTrendPct; }
double DynamicPricingContext::volatilityIndex() const { return m_volatilityIndex; }
double DynamicPricingContext::stockCoverageDays() const { return m_stockCoverageDays; }
double DynamicPricingContext::freshnessHours() const { return m_freshnessHours; }
double DynamicPricingContext::freshnessScore() const { return m_freshnessScore; }
double DynamicPricingContext::rarityScore() const { return m_rarityScore; }
const QString &DynamicPricingContext::weatherSummary() const { return m_weatherSummary; }
const QString &DynamicPricingContext::marineSummary() const { return m_marineSummary; }
const QVector<QPointF> &DynamicPricingContext::recentTrend() const { return m_recentTrend; }
const DynamicPricingRule &DynamicPricingContext::activeRule() const { return m_activeRule; }
const DynamicPricingManualOverride &DynamicPricingContext::activeOverride() const { return m_activeOverride; }
void DynamicPricingContext::setProductId(int productId) { m_productId = productId; }
void DynamicPricingContext::setSpeciesId(int speciesId) { m_speciesId = speciesId; }
void DynamicPricingContext::setSpeciesName(const QString &speciesName) { m_speciesName = speciesName; }
void DynamicPricingContext::setProductCode(const QString &productCode) { m_productCode = productCode; }
void DynamicPricingContext::setBaseCost(double baseCost) { m_baseCost = baseCost; }
void DynamicPricingContext::setBasePrice(double basePrice) { m_basePrice = basePrice; }
void DynamicPricingContext::setMinPrice(double minPrice) { m_minPrice = minPrice; }
void DynamicPricingContext::setMaxPrice(double maxPrice) { m_maxPrice = maxPrice; }
void DynamicPricingContext::setCurrentAveragePrice(double currentAveragePrice) { m_currentAveragePrice = currentAveragePrice; }
void DynamicPricingContext::setLastMarketPrice(double lastMarketPrice) { m_lastMarketPrice = lastMarketPrice; }
void DynamicPricingContext::setCurrentStockKg(double currentStockKg) { m_currentStockKg = currentStockKg; }
void DynamicPricingContext::setReservedStockKg(double reservedStockKg) { m_reservedStockKg = reservedStockKg; }
void DynamicPricingContext::setAvailableStockKg(double availableStockKg) { m_availableStockKg = availableStockKg; }
void DynamicPricingContext::setSevenDayVolumeKg(double sevenDayVolumeKg) { m_sevenDayVolumeKg = sevenDayVolumeKg; }
void DynamicPricingContext::setThirtyDayVolumeKg(double thirtyDayVolumeKg) { m_thirtyDayVolumeKg = thirtyDayVolumeKg; }
void DynamicPricingContext::setSalesVelocityKgPerDay(double salesVelocityKgPerDay) { m_salesVelocityKgPerDay = salesVelocityKgPerDay; }
void DynamicPricingContext::setDemandIndex(double demandIndex) { m_demandIndex = demandIndex; }
void DynamicPricingContext::setSupplyIndex(double supplyIndex) { m_supplyIndex = supplyIndex; }
void DynamicPricingContext::setWeatherImpactScore(double weatherImpactScore) { m_weatherImpactScore = weatherImpactScore; }
void DynamicPricingContext::setMarineRiskScore(double marineRiskScore) { m_marineRiskScore = marineRiskScore; }
void DynamicPricingContext::setMarketTrendPct(double marketTrendPct) { m_marketTrendPct = marketTrendPct; }
void DynamicPricingContext::setVolatilityIndex(double volatilityIndex) { m_volatilityIndex = volatilityIndex; }
void DynamicPricingContext::setStockCoverageDays(double stockCoverageDays) { m_stockCoverageDays = stockCoverageDays; }
void DynamicPricingContext::setFreshnessHours(double freshnessHours) { m_freshnessHours = freshnessHours; }
void DynamicPricingContext::setFreshnessScore(double freshnessScore) { m_freshnessScore = freshnessScore; }
void DynamicPricingContext::setRarityScore(double rarityScore) { m_rarityScore = rarityScore; }
void DynamicPricingContext::setWeatherSummary(const QString &weatherSummary) { m_weatherSummary = weatherSummary; }
void DynamicPricingContext::setMarineSummary(const QString &marineSummary) { m_marineSummary = marineSummary; }
void DynamicPricingContext::setRecentTrend(const QVector<QPointF> &recentTrend) { m_recentTrend = recentTrend; }
void DynamicPricingContext::setActiveRule(const DynamicPricingRule &activeRule) { m_activeRule = activeRule; }
void DynamicPricingContext::setActiveOverride(const DynamicPricingManualOverride &activeOverride) { m_activeOverride = activeOverride; }

DynamicPricingRecommendation::DynamicPricingRecommendation()
    : m_basePrice(0.0)
    , m_freshnessFactor(0.0)
    , m_demandFactor(0.0)
    , m_stockPressureFactor(0.0)
    , m_weatherRiskFactor(0.0)
    , m_marketTrendFactor(0.0)
    , m_rarityFactor(0.0)
    , m_algorithmicPrice(0.0)
    , m_suggestedPrice(0.0)
    , m_floorPrice(0.0)
    , m_ceilingPrice(0.0)
    , m_marginFloorPrice(0.0)
    , m_confidenceScore(0.0)
    , m_manualOverrideEnabled(false)
    , m_overrideApplied(false)
    , m_manualOverridePrice(0.0)
{
}

const QDateTime &DynamicPricingRecommendation::generatedAt() const { return m_generatedAt; }
double DynamicPricingRecommendation::basePrice() const { return m_basePrice; }
double DynamicPricingRecommendation::freshnessFactor() const { return m_freshnessFactor; }
double DynamicPricingRecommendation::demandFactor() const { return m_demandFactor; }
double DynamicPricingRecommendation::stockPressureFactor() const { return m_stockPressureFactor; }
double DynamicPricingRecommendation::weatherRiskFactor() const { return m_weatherRiskFactor; }
double DynamicPricingRecommendation::marketTrendFactor() const { return m_marketTrendFactor; }
double DynamicPricingRecommendation::rarityFactor() const { return m_rarityFactor; }
double DynamicPricingRecommendation::algorithmicPrice() const { return m_algorithmicPrice; }
double DynamicPricingRecommendation::suggestedPrice() const { return m_suggestedPrice; }
double DynamicPricingRecommendation::floorPrice() const { return m_floorPrice; }
double DynamicPricingRecommendation::ceilingPrice() const { return m_ceilingPrice; }
double DynamicPricingRecommendation::marginFloorPrice() const { return m_marginFloorPrice; }
double DynamicPricingRecommendation::confidenceScore() const { return m_confidenceScore; }
bool DynamicPricingRecommendation::manualOverrideEnabled() const { return m_manualOverrideEnabled; }
bool DynamicPricingRecommendation::overrideApplied() const { return m_overrideApplied; }
double DynamicPricingRecommendation::manualOverridePrice() const { return m_manualOverridePrice; }
const QString &DynamicPricingRecommendation::decisionSource() const { return m_decisionSource; }
const QString &DynamicPricingRecommendation::explanation() const { return m_explanation; }
void DynamicPricingRecommendation::setGeneratedAt(const QDateTime &generatedAt) { m_generatedAt = generatedAt; }
void DynamicPricingRecommendation::setBasePrice(double basePrice) { m_basePrice = basePrice; }
void DynamicPricingRecommendation::setFreshnessFactor(double freshnessFactor) { m_freshnessFactor = freshnessFactor; }
void DynamicPricingRecommendation::setDemandFactor(double demandFactor) { m_demandFactor = demandFactor; }
void DynamicPricingRecommendation::setStockPressureFactor(double stockPressureFactor) { m_stockPressureFactor = stockPressureFactor; }
void DynamicPricingRecommendation::setWeatherRiskFactor(double weatherRiskFactor) { m_weatherRiskFactor = weatherRiskFactor; }
void DynamicPricingRecommendation::setMarketTrendFactor(double marketTrendFactor) { m_marketTrendFactor = marketTrendFactor; }
void DynamicPricingRecommendation::setRarityFactor(double rarityFactor) { m_rarityFactor = rarityFactor; }
void DynamicPricingRecommendation::setAlgorithmicPrice(double algorithmicPrice) { m_algorithmicPrice = algorithmicPrice; }
void DynamicPricingRecommendation::setSuggestedPrice(double suggestedPrice) { m_suggestedPrice = suggestedPrice; }
void DynamicPricingRecommendation::setFloorPrice(double floorPrice) { m_floorPrice = floorPrice; }
void DynamicPricingRecommendation::setCeilingPrice(double ceilingPrice) { m_ceilingPrice = ceilingPrice; }
void DynamicPricingRecommendation::setMarginFloorPrice(double marginFloorPrice) { m_marginFloorPrice = marginFloorPrice; }
void DynamicPricingRecommendation::setConfidenceScore(double confidenceScore) { m_confidenceScore = confidenceScore; }
void DynamicPricingRecommendation::setManualOverrideEnabled(bool manualOverrideEnabled) { m_manualOverrideEnabled = manualOverrideEnabled; }
void DynamicPricingRecommendation::setOverrideApplied(bool overrideApplied) { m_overrideApplied = overrideApplied; }
void DynamicPricingRecommendation::setManualOverridePrice(double manualOverridePrice) { m_manualOverridePrice = manualOverridePrice; }
void DynamicPricingRecommendation::setDecisionSource(const QString &decisionSource) { m_decisionSource = decisionSource; }
void DynamicPricingRecommendation::setExplanation(const QString &explanation) { m_explanation = explanation; }

DynamicPricingDashboardData::DynamicPricingDashboardData() = default;
const DynamicPricingContext &DynamicPricingDashboardData::context() const { return m_context; }
DynamicPricingContext &DynamicPricingDashboardData::context() { return m_context; }
const DynamicPricingRecommendation &DynamicPricingDashboardData::recommendation() const { return m_recommendation; }
DynamicPricingRecommendation &DynamicPricingDashboardData::recommendation() { return m_recommendation; }
const QVector<DynamicPricingHistoryEntry> &DynamicPricingDashboardData::history() const { return m_history; }
QVector<DynamicPricingHistoryEntry> &DynamicPricingDashboardData::history() { return m_history; }
void DynamicPricingDashboardData::setContext(const DynamicPricingContext &context) { m_context = context; }
void DynamicPricingDashboardData::setRecommendation(const DynamicPricingRecommendation &recommendation) { m_recommendation = recommendation; }
void DynamicPricingDashboardData::setHistory(const QVector<DynamicPricingHistoryEntry> &history) { m_history = history; }
