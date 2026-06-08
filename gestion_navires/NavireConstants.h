#ifndef NAVIRECONSTANTS_H
#define NAVIRECONSTANTS_H

#include <QString>

namespace NavireConstants {

// API Endpoints
const QString METEO_MARINE_URL = "https://marine-api.open-meteo.com/v1/marine";
const QString METEO_FORECAST_URL = "https://api.open-meteo.com/v1/forecast";
const int REFRESH_INTERVAL_MS = 3600000; // 1 hour

// Port Base Configuration (Integration Test: Busan, South Korea)
const double TUNIS_LAT = 35.1796;
const double TUNIS_LON = 129.0756;
const QString PORT_BASE_NAME = "Port de Busan";

// Risk Levels
enum class RiskLevel {
    LOW,
    MODERATE,
    HIGH,
    CRITICAL
};

const QString RISK_LOW_COLOR = "#10B981";      // Green
const QString RISK_MODERATE_COLOR = "#F59E0B"; // Amber
const QString RISK_HIGH_COLOR = "#FB923C";     // Orange
const QString RISK_CRITICAL_COLOR = "#EF4444"; // Red

// Maintenance Thresholds
const int UPCOMING_THRESHOLD_DAYS = 30;
const int UPCOMING_THRESHOLD_PERCENT = 20;
const int IMMINENT_THRESHOLD_DAYS = 7;
const int IMMINENT_THRESHOLD_PERCENT = 5;

// Maintenance Rules Types
enum class RuleType {
    CALENDAR_BASED,
    HOURS_BASED,
    COMPOSITE
};

// Urgency Status
enum class Urgency {
    NOMINAL,
    UPCOMING,
    IMMINENT,
    OVERDUE
};

// Maritime Conventions
const double KNOTS_TO_MS = 0.514444;
const double BEAUFORT_THRESHOLDS[] = {0.3, 1.5, 3.3, 5.4, 7.9, 10.7, 13.8, 17.1, 20.7, 24.4, 28.4, 32.6};

} // namespace NavireConstants

#endif // NAVIRECONSTANTS_H
