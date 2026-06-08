#include "alertservice.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include "maintenanceengine.h"
#include "emailnotifier.h"
#include "NavireConstants.h"

AlertService::AlertService(QObject *parent) : QObject(parent) {
    checkTimer = new QTimer(this);
    // In a real app we'd trigger this periodically.
    // connect(checkTimer, &QTimer::timeout, this, &AlertService::someInternalWorker);
}

void AlertService::checkAlerts(const MeteoData::MeteoReport &weather) {
    QSqlQuery q("SELECT IDNAVIRE, NOMNAVIRE, PROPRIETAIRE FROM NAVIRES");
    while (q.next()) {
        int navId = q.value("IDNAVIRE").toInt();
        QString navName = q.value("NOMNAVIRE").toString();
        QString ownerName = q.value("PROPRIETAIRE").toString();

        PredictionResult pred = MaintenanceEngine::predictNextMaintenance(navId, weather);
        if (pred.urgency == NavireConstants::Urgency::IMMINENT || 
            pred.urgency == NavireConstants::Urgency::OVERDUE) {

            QString statusStr = (pred.urgency == NavireConstants::Urgency::IMMINENT) ? "IMMINENT" : "OVERDUE";
            QString title = "Maintenance Alert: " + navName;
            QString msg = QString("Vessel %1 maintenance status: %2. Predicted Date: %3. %4")
                                .arg(navName, statusStr, pred.predictedDate.toString("dd MMM"), pred.suggestion);

            emit alertTriggered(title, msg);

            // Send Emails
            EmailNotifier::sendAlert("manager@dock.com", title, msg);
            EmailNotifier::sendAlert("owner_" + ownerName + "@vessel.com", title, msg);
        }
    }
}
