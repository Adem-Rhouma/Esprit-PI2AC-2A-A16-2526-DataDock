#ifndef DYNAMICPRICINGSERVICE_H
#define DYNAMICPRICINGSERVICE_H

#include "../engine/dynamicpricingengine.h"
#include "../repository/dynamicpricingrepository.h"

#include <QDateTime>

class DynamicPricingService
{
public:
    DynamicPricingService();

    bool loadSpeciesCatalog(QVector<DynamicPricingSpecies> *speciesList, QString *errorMessage) const;
    bool buildDashboard(int productId,
                        bool manualOverrideEnabled,
                        double manualOverridePrice,
                        DynamicPricingDashboardData *dashboardData,
                        QString *errorMessage) const;
    bool saveRecommendation(const DynamicPricingDashboardData &dashboardData, QString *errorMessage) const;
    bool saveManualOverride(int productId,
                            double overridePrice,
                            const QString &operatorName,
                            const QString &reasonText,
                            const QDateTime &startsAt,
                            const QDateTime &endsAt,
                            QString *errorMessage) const;
    bool cancelManualOverride(int overrideId, QString *errorMessage) const;

private:
    DynamicPricingRepository m_repository;
    DynamicPricingEngine m_engine;
};

#endif // DYNAMICPRICINGSERVICE_H
