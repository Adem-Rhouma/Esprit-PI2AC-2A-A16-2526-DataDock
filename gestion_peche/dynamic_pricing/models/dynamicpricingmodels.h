#ifndef DYNAMICPRICINGMODELS_H
#define DYNAMICPRICINGMODELS_H

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QVector>

struct DpSpecies
{
    int id = 0;
    QString name;
    QString description;
};

struct DpPort
{
    int portId = 0;
    QString name;
    double latitude = 0.0;
    double longitude = 0.0;
    bool isActive = true;
};

struct DpCatchContext
{
    int speciesId = 0;
    double totalQuantity = 0.0;
    double avgBasePrice = 0.0;
    double lastBasePrice = 0.0;
    QDate mostRecentCatch;
    int activeCatches = 0;
    QString latestZone;
    int activeZoneCount = 0;
    double freshnessHours = 0.0;
    double freshnessScore = 0.0;
};

struct DpHistoryEntry
{
    int logId = 0;
    int speciesId = 0;
    QString speciesName;
    int portId = 0;
    QString portName;
    double basePrice = 0.0;
    double recommendedPrice = 0.0;
    double weatherScore = 0.0;
    double riskScore = 0.0;
    double freshnessScore = 0.0;
    QString explanation;
    QString apiSource;
    QDateTime createdAt;
};

struct DpPricingInput
{
    int speciesId = 0;
    QString speciesName;
    int portId = 0;
    QString portName;

    double basePrice = 0.0;
    double averageBasePrice = 0.0;
    double totalQuantityKg = 0.0;
    int activeCatches = 0;

    double freshnessHours = 0.0;
    double freshnessScore = 0.0;
    QDate lastCatchDate;
    QString latestZone;

    double portLatitude = 0.0;
    double portLongitude = 0.0;

    double liveWeatherScore = 0.0;
    double liveRiskScore = 0.0;
    bool hasLiveMarineData = false;

    double externalActivityScore = 0.0;
    double externalFishingHours = 0.0;
    QString externalSummary;
    bool hasExternalIntelligence = false;
    bool gfwTokenConfigured = false;
};

struct DpPricingResult
{
    double recommendedPrice = 0.0;
    double lowerBound = 0.0;
    double upperBound = 0.0;
    double deltaAmount = 0.0;
    double deltaPercent = 0.0;

    double freshnessScore = 0.0;
    double availabilityScore = 0.0;
    double weatherScore = 0.0;
    double riskScore = 0.0;
    double externalActivityScore = 0.0;
    double compositeScore = 0.0;

    double freshnessAdjustment = 0.0;
    double availabilityAdjustment = 0.0;
    double weatherAdjustment = 0.0;
    double riskAdjustment = 0.0;
    double externalAdjustment = 0.0;
    double zoneAdjustment = 0.0;

    QString explanation;
    QString statusLine;
    QString fallbackState;
    QString apiSource;

    bool hasLiveWeatherData = false;
    bool hasExternalIntelligence = false;
};

#endif // DYNAMICPRICINGMODELS_H
