#ifndef METEODATA_H
#define METEODATA_H

#include <QString>
#include <QDateTime>
#include <QList>
#include "NavireConstants.h"

namespace MeteoData {

struct CurrentConditions {
    double temp;
    double windSpeed; // knots
    int beaufort;
    double waveHeight; // meters
    double visibility; // km
    double pressure; // hPa
    int seaStateCode;
    QString windDir;
    QDateTime timestamp;
};

struct ForecastDay {
    QDate date;
    double maxTemp;
    double maxWindSpeed;
    double maxWaveHeight;
    double rainProbability;
    int riskScore; // 0-100
    NavireConstants::RiskLevel riskLevel;
    QString advisory;
};

struct OperationalAdvisory {
    NavireConstants::RiskLevel riskLevel;
    int globalRiskScore; // 0-100
    bool unloadingFeasible;
    QString text;
    bool recommendDryDock;
};

struct TideData {
    QDateTime nextHighTide;
    QDateTime nextLowTide;
    double currentLevel;
};

struct MeteoReport {
    CurrentConditions current;
    QList<ForecastDay> forecast;
    TideData tide;
    OperationalAdvisory advisory;
};

} // namespace MeteoData

#endif // METEODATA_H
