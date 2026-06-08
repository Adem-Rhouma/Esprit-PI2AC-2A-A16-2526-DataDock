#ifndef NAVIRETESTS_H
#define NAVIRETESTS_H

#include <QtTest>
#include "meteotranslator.h"
#include "maintenanceengine.h"

class NavireTests : public QObject {
    Q_OBJECT
private slots:
    void testBeaufortCalculation() {
        QCOMPARE(MeteoTranslator::calculateBeaufort(5.0), 2); // 5 knots ~ 2.5 m/s
        QCOMPARE(MeteoTranslator::calculateBeaufort(25.0), 6); // 25 knots ~ 12.8 m/s
    }

    void testRiskAdvisory() {
        auto adv = MeteoTranslator::getAdvisory(5, 0.2);
        QCOMPARE(adv.riskLevel, NavireConstants::RiskLevel::LOW);

        adv = MeteoTranslator::getAdvisory(40, 5.0);
        QCOMPARE(adv.riskLevel, NavireConstants::RiskLevel::CRITICAL);
    }

    void testStrainCalculation() {
        QList<VesselUsage> logs;
        VesselUsage u;
        u.departureTime = QDateTime(QDate(2026, 1, 1), QTime(8,0));
        u.returnTime = QDateTime(QDate(2026, 1, 1), QTime(12,0)); // 4 hours
        u.cargoLoadFactor = 0.8;
        logs.append(u);

        double strain = MaintenanceEngine::calculateStrain(logs, 1.0); // SeaState 1.0 -> 1.0 + 0.2 = 1.2 coef
        // 4 * 0.8 * 1.2 = 3.84
        QVERIFY(std::abs(strain - 3.84) < 0.01);
    }
};

#endif // NAVIRETESTS_H
