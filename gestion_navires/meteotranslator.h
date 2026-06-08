#ifndef METEOTRANSLATOR_H
#define METEOTRANSLATOR_H

#include "meteo_data.h"
#include "NavireConstants.h"

class MeteoTranslator {
public:
    static int calculateBeaufort(double windKnots) {
        double windMs = windKnots * NavireConstants::KNOTS_TO_MS;
        const auto& thresholds = NavireConstants::BEAUFORT_THRESHOLDS;
        for (int i = 0; i < 12; ++i) {
            if (windMs < thresholds[i]) return i;
        }
        return 12;
    }

    static int calculateSeaState(double waveHeight) {
        if (waveHeight < 0.1) return 0;
        if (waveHeight < 0.5) return 1;
        if (waveHeight < 1.25) return 2;
        if (waveHeight < 2.5) return 3;
        if (waveHeight < 4.0) return 4;
        if (waveHeight < 6.0) return 5;
        if (waveHeight < 9.0) return 6;
        if (waveHeight < 14.0) return 7;
        return 9;
    }

    static MeteoData::OperationalAdvisory getAdvisory(double windKnots, double waveHeight) {
        MeteoData::OperationalAdvisory adv;
        adv.riskLevel = NavireConstants::RiskLevel::LOW;
        adv.unloadingFeasible = true;
        adv.recommendDryDock = false;

        if (windKnots > 35 || waveHeight > 4.0) {
            adv.riskLevel = NavireConstants::RiskLevel::CRITICAL;
            adv.unloadingFeasible = false;
            adv.text = "CRITICAL: Severe marine conditions. Docking suspended.";
        } else if (windKnots > 25 || waveHeight > 2.5) {
            adv.riskLevel = NavireConstants::RiskLevel::HIGH;
            adv.unloadingFeasible = false;
            adv.text = "HIGH: Stormy conditions. Manual unloading only.";
        } else if (windKnots > 15 || waveHeight > 1.5) {
            adv.riskLevel = NavireConstants::RiskLevel::MODERATE;
            adv.unloadingFeasible = true;
            adv.text = "MODERATE: Choppy sea. Exercise caution.";
        } else {
            adv.riskLevel = NavireConstants::RiskLevel::LOW;
            adv.unloadingFeasible = true;
            adv.text = "LOW: Calm conditions. Optimal for operations.";
            if (waveHeight < 0.5 && windKnots < 10) adv.recommendDryDock = true;
        }

        return adv;
    }
};

#endif // METEOTRANSLATOR_H
