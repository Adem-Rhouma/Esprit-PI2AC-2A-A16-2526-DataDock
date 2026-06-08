#ifndef MAINTENANCERULE_H
#define MAINTENANCERULE_H

#include <QString>
#include <QDate>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include "NavireConstants.h"
#include "connection.h"

class MaintenanceRule {
public:
    int idRule;
    int idNavire;
    NavireConstants::RuleType type;
    int thresholdHours;
    int thresholdDays;
    QDate lastMaintenanceDate;

    MaintenanceRule() : idRule(0), idNavire(0), thresholdHours(0), thresholdDays(0) {}

    bool save() {
        QSqlQuery q(Connection::createInstance().db);
        QSqlQuery qMax(Connection::createInstance().db);
        int newId = 1;
        if (qMax.exec("SELECT NVL(MAX(IDRULE), 0) + 1 FROM MAINTENANCE_RULES") && qMax.next()) {
            newId = qMax.value(0).toInt();
        }
        idRule = newId;

        q.prepare(R"(
            INSERT INTO MAINTENANCE_RULES (IDRULE, IDNAVIRE, RULE_TYPE, THRESHOLD_HOURS, THRESHOLD_DAYS, LAST_MAINTENANCE_DATE)
            VALUES (:idRule, :navire, :type, :hr, :day, :last)
        )");
        q.bindValue(":idRule", idRule);
        q.bindValue(":navire", idNavire);
        q.bindValue(":type", typeToString(type));
        q.bindValue(":hr", thresholdHours);
        q.bindValue(":day", thresholdDays);
        q.bindValue(":last", lastMaintenanceDate);
        return q.exec();
    }

    static MaintenanceRule getByNavire(int navId) {
        MaintenanceRule r;
        QSqlQuery q(Connection::createInstance().db);
        q.prepare("SELECT * FROM MAINTENANCE_RULES WHERE IDNAVIRE = :id");
        q.bindValue(":id", navId);
        if (q.exec() && q.next()) {
            r.idRule = q.value("IDRULE").toInt();
            r.idNavire = q.value("IDNAVIRE").toInt();
            r.type = stringToType(q.value("RULE_TYPE").toString());
            r.thresholdHours = q.value("THRESHOLD_HOURS").toInt();
            r.thresholdDays = q.value("THRESHOLD_DAYS").toInt();
            r.lastMaintenanceDate = q.value("LAST_MAINTENANCE_DATE").toDate();
        }
        return r;
    }

private:
    static QString typeToString(NavireConstants::RuleType t) {
        if (t == NavireConstants::RuleType::CALENDAR_BASED) return "Calendar";
        if (t == NavireConstants::RuleType::HOURS_BASED) return "Hours";
        return "Composite";
    }

    static NavireConstants::RuleType stringToType(QString s) {
        if (s == "Calendar") return NavireConstants::RuleType::CALENDAR_BASED;
        if (s == "Hours") return NavireConstants::RuleType::HOURS_BASED;
        return NavireConstants::RuleType::COMPOSITE;
    }
};

#endif // MAINTENANCERULE_H
