#include "dynamicpricingrepository.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTime>
#include <QVariant>
#include <QtMath>

namespace {

double computeFreshnessScore(double freshnessHours)
{
    const double boundedHours = qBound(0.0, freshnessHours, 144.0);
    return 100.0 * (1.0 - (boundedHours / 144.0));
}

QString normalizeZone(const QVariant &value)
{
    const QString zone = value.toString().trimmed();
    return zone.isEmpty() ? QStringLiteral("Zone non renseignee") : zone;
}

QString detectCatchLinkColumn()
{
    QSqlQuery probe;
    probe.prepare(QStringLiteral("SELECT * FROM PECHESESPECES WHERE ROWNUM = 1"));
    if (!probe.exec()) {
        return QStringLiteral("IDPECHE");
    }

    const QSqlRecord record = probe.record();
    if (record.indexOf(QStringLiteral("IDPECHE")) >= 0) {
        return QStringLiteral("IDPECHE");
    }
    if (record.indexOf(QStringLiteral("IDPPECHE")) >= 0) {
        return QStringLiteral("IDPPECHE");
    }

    return QStringLiteral("IDPECHE");
}

} // namespace

bool DpRepository::isTableAccessible(const QString &tableName) const
{
    QSqlQuery probe;
    probe.prepare(QStringLiteral("SELECT 1 FROM %1 WHERE ROWNUM = 1").arg(tableName));
    return probe.exec();
}

bool DpRepository::loadSpecies(QVector<DpSpecies> *out, QString *error) const
{
    if (!out) {
        if (error) {
            *error = QStringLiteral("Pointeur de sortie invalide pour loadSpecies.");
        }
        return false;
    }

    out->clear();

    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT IDESPECE, NOMESPECE, DESCRIPTION "
        "FROM ESPECES "
        "ORDER BY NOMESPECE"));

    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    while (query.next()) {
        DpSpecies species;
        species.id = query.value(0).toInt();
        species.name = query.value(1).toString().trimmed();
        species.description = query.value(2).toString().trimmed();
        out->append(species);
    }

    if (out->isEmpty()) {
        if (error) {
            *error = QStringLiteral("Aucune espece trouvee dans ESPECES.");
        }
        return false;
    }

    return true;
}

bool DpRepository::loadPorts(QVector<DpPort> *out, QString *error) const
{
    if (!out) {
        if (error) {
            *error = QStringLiteral("Pointeur de sortie invalide pour loadPorts.");
        }
        return false;
    }

    out->clear();

    if (!isTableAccessible(QStringLiteral("DP_PORT_CONFIG"))) {
        return true;
    }

    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT PORT_ID, PORT_NAME, LATITUDE, LONGITUDE, IS_ACTIVE "
        "FROM DP_PORT_CONFIG "
        "WHERE UPPER(TRIM(NVL(TO_CHAR(IS_ACTIVE), 'Y'))) IN ('Y', '1', 'TRUE', 'T') "
        "ORDER BY PORT_NAME"));

    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    while (query.next()) {
        DpPort port;
        port.portId = query.value(0).toInt();
        port.name = query.value(1).toString().trimmed();
        port.latitude = query.value(2).toDouble();
        port.longitude = query.value(3).toDouble();
        port.isActive = true;
        out->append(port);
    }

    return true;
}

bool DpRepository::loadCatchContext(int speciesId, DpCatchContext *out, QString *error) const
{
    if (!out) {
        if (error) {
            *error = QStringLiteral("Pointeur de sortie invalide pour loadCatchContext.");
        }
        return false;
    }

    *out = DpCatchContext();
    out->speciesId = speciesId;

    auto loadWithCatchLinkColumn = [&](const QString &catchLinkColumn, QString *sqlError) -> bool {
        *out = DpCatchContext();
        out->speciesId = speciesId;

        QSqlQuery aggregateQuery;
        aggregateQuery.prepare(QStringLiteral(
            "SELECT "
            "    NVL(SUM(PE.QUANTITE), 0), "
            "    NVL(AVG(PE.PRIXUNITAIRE), 0), "
            "    MAX(P.DATEPECHE), "
            "    COUNT(DISTINCT P.IDPECHE), "
            "    COUNT(DISTINCT NVL(TRIM(P.ZONEPECHE), 'ZONE_NON_RENSEIGNEE')) "
            "FROM PECHESESPECES PE "
            "JOIN PECHES P ON P.IDPECHE = PE.%1 "
            "WHERE PE.IDESPECE = :speciesId "
            "  AND NVL(P.IS_ARCHIVED, 0) = 0").arg(catchLinkColumn));
        aggregateQuery.bindValue(QStringLiteral(":speciesId"), speciesId);

        if (!aggregateQuery.exec()) {
            if (sqlError) {
                *sqlError = aggregateQuery.lastError().text();
            }
            return false;
        }

        if (aggregateQuery.next()) {
            out->totalQuantity = aggregateQuery.value(0).toDouble();
            out->avgBasePrice = aggregateQuery.value(1).toDouble();
            out->mostRecentCatch = aggregateQuery.value(2).toDate();
            out->activeCatches = aggregateQuery.value(3).toInt();
            out->activeZoneCount = aggregateQuery.value(4).toInt();
        }

        QSqlQuery latestQuery;
        latestQuery.prepare(QStringLiteral(
            "SELECT latest.PRIXUNITAIRE, latest.ZONEPECHE, latest.DATEPECHE "
            "FROM ( "
            "    SELECT PE.PRIXUNITAIRE, P.ZONEPECHE, P.DATEPECHE "
            "    FROM PECHESESPECES PE "
            "    JOIN PECHES P ON P.IDPECHE = PE.%1 "
            "    WHERE PE.IDESPECE = :speciesId "
            "      AND NVL(P.IS_ARCHIVED, 0) = 0 "
            "    ORDER BY P.DATEPECHE DESC, P.IDPECHE DESC "
            ") latest "
            "WHERE ROWNUM = 1").arg(catchLinkColumn));
        latestQuery.bindValue(QStringLiteral(":speciesId"), speciesId);

        if (!latestQuery.exec()) {
            if (sqlError) {
                *sqlError = latestQuery.lastError().text();
            }
            return false;
        }

        if (latestQuery.next()) {
            out->lastBasePrice = latestQuery.value(0).toDouble();
            out->latestZone = normalizeZone(latestQuery.value(1));
            if (!out->mostRecentCatch.isValid()) {
                out->mostRecentCatch = latestQuery.value(2).toDate();
            }
        }

        return true;
    };

    QString sqlError;
    const QString catchLinkColumn = detectCatchLinkColumn();
    if (!loadWithCatchLinkColumn(catchLinkColumn, &sqlError)) {
        if (error) {
            *error = sqlError;
        }
        return false;
    }

    if (out->mostRecentCatch.isValid()) {
        const QDateTime catchDateTime(out->mostRecentCatch, QTime(0, 0), Qt::LocalTime);
        out->freshnessHours =
            static_cast<double>(catchDateTime.secsTo(QDateTime::currentDateTime())) / 3600.0;
    } else {
        out->freshnessHours = 240.0;
    }

    out->freshnessScore = computeFreshnessScore(out->freshnessHours);

    if (out->latestZone.isEmpty()) {
        out->latestZone = QStringLiteral("Zone non renseignee");
    }

    return true;
}

bool DpRepository::loadHistory(int speciesId, QVector<DpHistoryEntry> *out, QString *error) const
{
    if (!out) {
        if (error) {
            *error = QStringLiteral("Pointeur de sortie invalide pour loadHistory.");
        }
        return false;
    }

    out->clear();

    if (!isTableAccessible(QStringLiteral("DP_PRICE_RECOMMENDATION_LOG"))) {
        return true;
    }

    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT * "
        "FROM ( "
        "    SELECT "
        "        L.LOG_ID, "
        "        L.IDESPECE, "
        "        E.NOMESPECE, "
        "        L.PORT_ID, "
        "        NVL(P.PORT_NAME, ''), "
        "        L.BASE_PRICE_TND, "
        "        L.RECOMMENDED_PRICE_TND, "
        "        NVL(L.WEATHER_SCORE, 0), "
        "        NVL(L.RISK_SCORE, 0), "
        "        NVL(L.FRESHNESS_SCORE, 0), "
        "        NVL(L.EXPLANATION_TEXT, ''), "
        "        NVL(L.API_SOURCE, 'LOCAL_DB'), "
        "        L.CREATED_AT "
        "    FROM DP_PRICE_RECOMMENDATION_LOG L "
        "    JOIN ESPECES E ON E.IDESPECE = L.IDESPECE "
        "    LEFT JOIN DP_PORT_CONFIG P ON P.PORT_ID = L.PORT_ID "
        "    WHERE L.IDESPECE = :speciesId "
        "    ORDER BY L.CREATED_AT DESC, L.LOG_ID DESC "
        ") "
        "WHERE ROWNUM <= 50"));
    query.bindValue(QStringLiteral(":speciesId"), speciesId);

    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    while (query.next()) {
        DpHistoryEntry entry;
        entry.logId = query.value(0).toInt();
        entry.speciesId = query.value(1).toInt();
        entry.speciesName = query.value(2).toString().trimmed();
        entry.portId = query.value(3).toInt();
        entry.portName = query.value(4).toString().trimmed();
        entry.basePrice = query.value(5).toDouble();
        entry.recommendedPrice = query.value(6).toDouble();
        entry.weatherScore = query.value(7).toDouble();
        entry.riskScore = query.value(8).toDouble();
        entry.freshnessScore = query.value(9).toDouble();
        entry.explanation = query.value(10).toString().trimmed();
        entry.apiSource = query.value(11).toString().trimmed();
        entry.createdAt = query.value(12).toDateTime();
        out->append(entry);
    }

    return true;
}

bool DpRepository::saveRecommendation(const DpPricingInput &input,
                                      const DpPricingResult &result,
                                      QString *error) const
{
    if (!isTableAccessible(QStringLiteral("DP_PRICE_RECOMMENDATION_LOG"))) {
        if (error) {
            *error = QStringLiteral("La table DP_PRICE_RECOMMENDATION_LOG est inaccessible.");
        }
        return false;
    }

    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO DP_PRICE_RECOMMENDATION_LOG ( "
        "    LOG_ID, IDESPECE, PORT_ID, BASE_PRICE_TND, RECOMMENDED_PRICE_TND, "
        "    WEATHER_SCORE, RISK_SCORE, FRESHNESS_SCORE, EXPLANATION_TEXT, API_SOURCE "
        ") VALUES ( "
        "    (SELECT NVL(MAX(LOG_ID), 0) + 1 FROM DP_PRICE_RECOMMENDATION_LOG), "
        "    :speciesId, :portId, :basePrice, :recommendedPrice, "
        "    :weatherScore, :riskScore, :freshnessScore, :explanation, :apiSource "
        ")"));

    query.bindValue(QStringLiteral(":speciesId"),
                    input.speciesId > 0 ? QVariant(input.speciesId) : QVariant());
    query.bindValue(QStringLiteral(":portId"),
                    input.portId > 0 ? QVariant(input.portId) : QVariant());
    query.bindValue(QStringLiteral(":basePrice"), input.basePrice);
    query.bindValue(QStringLiteral(":recommendedPrice"), result.recommendedPrice);
    query.bindValue(QStringLiteral(":weatherScore"), result.weatherScore);
    query.bindValue(QStringLiteral(":riskScore"), result.riskScore);
    query.bindValue(QStringLiteral(":freshnessScore"), result.freshnessScore);
    query.bindValue(QStringLiteral(":explanation"),
                    result.explanation.isEmpty()
                        ? QVariant()
                        : QVariant(result.explanation.left(500)));
    query.bindValue(QStringLiteral(":apiSource"),
                    result.apiSource.isEmpty()
                        ? QVariant(QStringLiteral("LOCAL_DB"))
                        : QVariant(result.apiSource.left(120)));

    if (!query.exec()) {
        if (error) {
            *error = query.lastError().text();
        }
        return false;
    }

    return true;
}
