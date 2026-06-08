#ifndef MARINEDTO_H
#define MARINEDTO_H

#include <QMetaType>
#include <QString>

struct MarineCurrentData
{
    double waveHeight = 0.0;
    double waveDirection = 0.0;
    double wavePeriod = 0.0;
    double oceanCurrentVelocity = 0.0;
    double oceanCurrentDirection = 0.0;
    double seaSurfaceTemperature = 0.0;
    double requestLatitude = 0.0;
    double requestLongitude = 0.0;
    QString observationTime;
    QString rawJson;
    bool valid = false;
};

struct MarineWeatherScores
{
    double weatherScore = 0.0;
    double riskScore = 0.0;
    QString summary;
};

struct GfwPortIntelligence
{
    QString portName;
    QString zoneHint;
    QString startDate;
    QString endDate;
    double totalFishingHours = 0.0;
    int sampleCells = 0;
    QString rawJson;
    bool valid = false;
    bool tokenConfigured = false;
};

struct GfwIntelligenceScores
{
    double activityScore = 0.0;
    QString summary;
    QString sourceTag;
};

Q_DECLARE_METATYPE(MarineCurrentData)
Q_DECLARE_METATYPE(MarineWeatherScores)
Q_DECLARE_METATYPE(GfwPortIntelligence)
Q_DECLARE_METATYPE(GfwIntelligenceScores)

#endif // MARINEDTO_H
