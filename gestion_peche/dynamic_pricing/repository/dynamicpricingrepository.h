#ifndef DYNAMICPRICINGREPOSITORY_H
#define DYNAMICPRICINGREPOSITORY_H

// =============================================================================
//  DpRepository – Data-access layer for the Dynamic Pricing module
//
//  Tables READ:
//    ESPECES                      (species catalogue)
//    PECHES + PECHESESPECES       (catch context, quantity, freshness, base price)
//    DP_PORT_CONFIG               (port coordinates for future Marine API)
//    DP_PRICE_RECOMMENDATION_LOG  (recommendation history)
//
//  Tables WRITTEN:
//    DP_PRICE_RECOMMENDATION_LOG  (one lightweight INSERT per saved recommendation)
//
//  Nothing else.  No DP_PRODUCTS.  No DP_INVENTORY_STOCK.  No new tables.
// =============================================================================

#include "../models/dynamicpricingmodels.h"

#include <QString>
#include <QVector>

class DpRepository
{
public:
    DpRepository() = default;

    // ------------------------------------------------------------------
    // READ operations
    // ------------------------------------------------------------------

    /// Load all species from ESPECES ordered by NOMESPECE.
    bool loadSpecies(QVector<DpSpecies> *out, QString *error) const;

    /// Load active ports from DP_PORT_CONFIG (IS_ACTIVE = 'Y').
    /// Returns true with an empty list if the table is not yet accessible.
    bool loadPorts(QVector<DpPort> *out, QString *error) const;

    /// Build a DpCatchContext for one species by joining PECHES + PECHESESPECES.
    /// Freshness fields are computed in C++ after the DB round-trip.
    bool loadCatchContext(int speciesId,
                          DpCatchContext *out,
                          QString *error) const;

    /// Return the 50 most-recent rows from DP_PRICE_RECOMMENDATION_LOG
    /// for the given species, joined to ESPECES for the display name.
    /// Returns true with an empty list if the table is not yet accessible.
    bool loadHistory(int speciesId,
                     QVector<DpHistoryEntry> *out,
                     QString *error) const;

    // ------------------------------------------------------------------
    // WRITE operations
    // ------------------------------------------------------------------

    /// INSERT one row into DP_PRICE_RECOMMENDATION_LOG.
    /// CREATED_AT is left to the column DEFAULT (SYSTIMESTAMP).
    bool saveRecommendation(const DpPricingInput  &input,
                            const DpPricingResult &result,
                            QString               *error) const;

private:
    /// Quick probe: tries "SELECT 1 FROM <table> WHERE ROWNUM=1".
    /// Returns false (and does NOT set *error) when the table simply
    /// does not exist yet – callers treat that as "return empty, no error".
    bool isTableAccessible(const QString &tableName) const;
};

#endif // DYNAMICPRICINGREPOSITORY_H