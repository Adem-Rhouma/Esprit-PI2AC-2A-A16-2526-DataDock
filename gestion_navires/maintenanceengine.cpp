#include "maintenanceengine.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <cmath>
#include "connection.h"
#include "NavireConstants.h"

MaintenanceEngine::MaintenanceEngine(QObject *parent) : QObject(parent) {}

double MaintenanceEngine::calculateStrain(const QList<VesselUsage> &usageLogs, double seaStateExposure) {
    double totalStrain = 0;
    for (const auto &log : usageLogs) {
        qint64 hours = log.departureTime.secsTo(log.returnTime) / 3600;
        // Formula: hours * load factor * sea state exposure
        totalStrain += hours * log.cargoLoadFactor * (1.0 + seaStateExposure * 0.2);
    }
    return totalStrain;
}

PredictionResult MaintenanceEngine::predictNextMaintenance(int navId, const MeteoData::MeteoReport &weather) {
    PredictionResult result;
    result.predictedDate = QDate::currentDate().addDays(30); // Default
    result.confidenceInterval = 5;
    result.urgency = NavireConstants::Urgency::NOMINAL;

    MaintenanceRule rule = MaintenanceRule::getByNavire(navId);
    if (rule.idRule == 0) return result;

    // Calc remaining days/hours
    int daysSinceMaint = rule.lastMaintenanceDate.daysTo(QDate::currentDate());
    int daysRemaining = rule.thresholdDays - daysSinceMaint;

    // For composite/calendar, we use the earlier of the two
    if (daysRemaining < 30) {
        result.predictedDate = QDate::currentDate().addDays(qMax(0, daysRemaining));
    }

    // Cross-reference with weather forecast
    // Find a window with low risk near the predicted date
    bool windowFound = false;
    for (const auto &day : weather.forecast) {
        if (day.riskLevel == NavireConstants::RiskLevel::LOW && 
            std::abs(day.date.daysTo(result.predictedDate)) <= 3) {
            result.predictedDate = day.date;
            result.suggestion = "Optimal dry-dock window: " + day.date.toString("dd MMM") + " (Calm weather)";
            windowFound = true;
            break;
        }
    }

    if (!windowFound) {
        result.suggestion = "No calm windows near predicted date. Consider earlier docking.";
    }

    // Calc urgency
    double percentRemaining = (daysRemaining / (double)rule.thresholdDays) * 100.0;
    if (daysRemaining <= 0) result.urgency = NavireConstants::Urgency::OVERDUE;
    else if (daysRemaining <= 7 || percentRemaining <= 5) result.urgency = NavireConstants::Urgency::IMMINENT;
    else if (daysRemaining <= 30 || percentRemaining <= 20) result.urgency = NavireConstants::Urgency::UPCOMING;

    return result;
}
