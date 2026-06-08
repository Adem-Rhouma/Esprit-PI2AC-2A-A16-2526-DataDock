#include "pechedao.h"

#include "peche.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

PecheDAO::PecheDAO() = default;

bool PecheDAO::nextId(int *outId, QString *err) const
{
    if (!outId) {
        return false;
    }

    QSqlQuery query;
    query.prepare(QStringLiteral("SELECT NVL(MAX(IDPECHE), 0) + 1 FROM PECHES"));
    if (!query.exec()) {
        if (err) {
            *err = query.lastError().text();
        }
        return false;
    }
    if (!query.next()) {
        if (err) {
            *err = QStringLiteral("Impossible de g\u00e9n\u00e9rer un nouvel ID.");
        }
        return false;
    }
    *outId = query.value(0).toInt();
    return true;
}

bool PecheDAO::insert(const Peche &peche, QString *err) const
{
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "INSERT INTO PECHES (IDPECHE, DATEPECHE, ZONEPECHE, DESCRIPTION, IDNAVIRE) "
        "VALUES (:id, :datePeche, :zonePeche, :description, :idNavire)"));
    query.bindValue(QStringLiteral(":id"), peche.getId());
    query.bindValue(QStringLiteral(":datePeche"),
                    peche.getDatePeche().isValid() ? QVariant(peche.getDatePeche()) : QVariant());
    query.bindValue(QStringLiteral(":zonePeche"), peche.getZonePeche());
    query.bindValue(QStringLiteral(":description"), peche.getDescription());
    query.bindValue(QStringLiteral(":idNavire"), peche.getIdNavire());

    if (!query.exec()) {
        if (err) {
            *err = query.lastError().text();
        }
        return false;
    }
    return true;
}

bool PecheDAO::update(const Peche &peche, QString *err) const
{
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "UPDATE PECHES SET DATEPECHE = :datePeche, ZONEPECHE = :zonePeche, "
        "DESCRIPTION = :description, IDNAVIRE = :idNavire WHERE IDPECHE = :id"));
    query.bindValue(QStringLiteral(":datePeche"),
                    peche.getDatePeche().isValid() ? QVariant(peche.getDatePeche()) : QVariant());
    query.bindValue(QStringLiteral(":zonePeche"), peche.getZonePeche());
    query.bindValue(QStringLiteral(":description"), peche.getDescription());
    query.bindValue(QStringLiteral(":idNavire"), peche.getIdNavire());
    query.bindValue(QStringLiteral(":id"), peche.getId());

    if (!query.exec()) {
        if (err) {
            *err = query.lastError().text();
        }
        return false;
    }
    return true;
}

bool PecheDAO::loadById(int idPeche, Peche *outPeche, QString *err) const
{
    if (!outPeche) {
        return false;
    }

    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT IDPECHE, DATEPECHE, ZONEPECHE, DESCRIPTION, IDNAVIRE "
        "FROM PECHES WHERE IDPECHE = :id"));
    query.bindValue(QStringLiteral(":id"), idPeche);
    if (!query.exec()) {
        if (err) {
            *err = query.lastError().text();
        }
        return false;
    }
    if (!query.next()) {
        if (err) {
            *err = QStringLiteral("Lot de p\u00eache introuvable.");
        }
        return false;
    }

    outPeche->setId(query.value(0).toInt());
    outPeche->setDatePeche(query.value(1).toDate());
    outPeche->setZonePeche(query.value(2).toString());
    outPeche->setDescription(query.value(3).toString());
    outPeche->setIdNavire(query.value(4).toInt());
    return true;
}

bool PecheDAO::existsDuplicate(const QDate &datePeche,
                               const QString &zonePeche,
                               int idNavire,
                               int excludeId,
                               QString *err) const
{
    QSqlQuery query;
    query.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM PECHES "
        "WHERE DATEPECHE = :datePeche AND UPPER(ZONEPECHE) = UPPER(:zonePeche) "
        "AND IDNAVIRE = :idNavire AND IDPECHE <> :excludeId"));
    query.bindValue(QStringLiteral(":datePeche"), datePeche);
    query.bindValue(QStringLiteral(":zonePeche"), zonePeche.trimmed());
    query.bindValue(QStringLiteral(":idNavire"), idNavire);
    query.bindValue(QStringLiteral(":excludeId"), excludeId);

    if (!query.exec()) {
        if (err) {
            *err = query.lastError().text();
        }
        return false;
    }
    if (!query.next()) {
        if (err) {
            *err = QStringLiteral("V\u00e9rification d'unicit\u00e9 impossible.");
        }
        return false;
    }
    return query.value(0).toInt() > 0;
}
