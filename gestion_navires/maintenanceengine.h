#ifndef MAINTENANCEENGINE_H
#define MAINTENANCEENGINE_H

#include <QObject>
#include <QDate>
#include <QList>
#include "NavireConstants.h"
#include "maintenancerule.h"
#include "vesselusage.h"
#include "meteo_data.h"

struct PredictionResult {
    QDate predictedDate;
    int confidenceInterval; // in days
    NavireConstants::Urgency urgency;
    QString suggestion;
};

class MaintenanceEngine : public QObject {
    Q_OBJECT
public:
    explicit MaintenanceEngine(QObject *parent = nullptr);

    static double calculateStrain(const QList<VesselUsage> &usageLogs, double seaStateExposure);
    static PredictionResult predictNextMaintenance(int navId, const MeteoData::MeteoReport &weather);

private:
    static NavireConstants::Urgency calculateUrgency(const QDate &predictedDate, int thresholdRemainingPercent);
};

#endif // MAINTENANCEENGINE_H
