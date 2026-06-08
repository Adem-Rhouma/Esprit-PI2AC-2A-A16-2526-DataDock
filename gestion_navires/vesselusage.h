#ifndef VESSELUSAGE_H
#define VESSELUSAGE_H

#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include "connection.h"

class VesselUsage {
public:
    int idUsage;
    int idNavire;
    QDateTime departureTime;
    QDateTime returnTime;
    double cargoLoadFactor;
    double strainIndex;

    VesselUsage() : idUsage(0), idNavire(0), cargoLoadFactor(0), strainIndex(0) {}

    bool save() {
        QSqlQuery q(Connection::createInstance().db);
        QSqlQuery qMax(Connection::createInstance().db);
        int newId = 1;
        if (qMax.exec("SELECT NVL(MAX(IDUSAGE), 0) + 1 FROM VESSEL_USAGE") && qMax.next()) {
            newId = qMax.value(0).toInt();
        }
        idUsage = newId;

        q.prepare(R"(
            INSERT INTO VESSEL_USAGE (IDUSAGE, IDNAVIRE, DEPARTURE_TIME, RETURN_TIME, CARGO_LOAD_FACTOR, STRAIN_INDEX)
            VALUES (:idUsage, :navire, :dep, :ret, :load, :strain)
        )");
        q.bindValue(":idUsage", idUsage);
        q.bindValue(":navire", idNavire);
        q.bindValue(":dep", departureTime);
        q.bindValue(":ret", returnTime);
        q.bindValue(":load", cargoLoadFactor);
        q.bindValue(":strain", strainIndex);
        return q.exec();
    }
};

#endif // VESSELUSAGE_H
