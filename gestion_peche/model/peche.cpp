#include "peche.h"

#include <QDebug>
#include <QVariant>

Peche::Peche()
    : m_id(0)
    , m_idNavire(0)
{
}

Peche::Peche(int id,
             const QDate &datePeche,
             const QString &zonePeche,
             const QString &description,
             int idNavire)
    : m_id(id)
    , m_datePeche(datePeche)
    , m_zonePeche(zonePeche)
    , m_description(description)
    , m_idNavire(idNavire)
{
}

int Peche::getId() const { return m_id; }
QDate Peche::getDatePeche() const { return m_datePeche; }
QString Peche::getZonePeche() const { return m_zonePeche; }
QString Peche::getDescription() const { return m_description; }
int Peche::getIdNavire() const { return m_idNavire; }

void Peche::setId(int id) { m_id = id; }
void Peche::setDatePeche(const QDate &datePeche) { m_datePeche = datePeche; }
void Peche::setZonePeche(const QString &zonePeche) { m_zonePeche = zonePeche; }
void Peche::setDescription(const QString &description) { m_description = description; }
void Peche::setIdNavire(int idNavire) { m_idNavire = idNavire; }

bool Peche::ajouter()
{
    m_lastError.clear();
    int newId = 1;
    QSqlQuery qMax;
    qMax.prepare(QStringLiteral("SELECT NVL(MAX(IDPECHE), 0) + 1 FROM PECHES"));
    if (qMax.exec() && qMax.next()) {
        newId = qMax.value(0).toInt();
    }
    m_id = newId;

    QSqlQuery q;
    q.prepare(
        "INSERT INTO PECHES (IDPECHE, DATEPECHE, ZONEPECHE, DESCRIPTION, IDNAVIRE) "
        "VALUES (:id, :datePeche, :zonePeche, :description, :idNavire)");
    q.bindValue(":id", m_id);
    q.bindValue(":datePeche", m_datePeche.isValid() ? QVariant(m_datePeche) : QVariant());
    q.bindValue(":zonePeche", m_zonePeche);
    q.bindValue(":description", m_description);
    q.bindValue(":idNavire", m_idNavire);

    if (!q.exec()) {
        setLastError(q.lastError().text());
        qDebug() << "Peche::ajouter failed:" << m_lastError;
        return false;
    }
    return true;
}

bool Peche::modifier()
{
    m_lastError.clear();
    QSqlQuery q;
    q.prepare(
        "UPDATE PECHES SET DATEPECHE = :datePeche, ZONEPECHE = :zonePeche, "
        "DESCRIPTION = :description, IDNAVIRE = :idNavire WHERE IDPECHE = :id");
    q.bindValue(":datePeche", m_datePeche.isValid() ? QVariant(m_datePeche) : QVariant());
    q.bindValue(":zonePeche", m_zonePeche);
    q.bindValue(":description", m_description);
    q.bindValue(":idNavire", m_idNavire);
    q.bindValue(":id", m_id);

    if (!q.exec()) {
        setLastError(q.lastError().text());
        qDebug() << "Peche::modifier failed:" << m_lastError;
        return false;
    }
    return true;
}

bool Peche::supprimer(int idPeche)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        qDebug() << "Peche::supprimer transaction failed:" << db.lastError().text();
        return false;
    }

    QSqlQuery qDelEspeces(db);
    qDelEspeces.prepare("DELETE FROM PECHESESPECES WHERE IDPECHE = :id");
    qDelEspeces.bindValue(":id", idPeche);
    if (!qDelEspeces.exec()) {
        db.rollback();
        qDebug() << "Peche::supprimer delete PECHESESPECES failed:" << qDelEspeces.lastError().text();
        return false;
    }

    QSqlQuery qDelEmployes(db);
    qDelEmployes.prepare("DELETE FROM PECHESEMPLOYES WHERE IDPECHE = :id");
    qDelEmployes.bindValue(":id", idPeche);
    if (!qDelEmployes.exec()) {
        db.rollback();
        qDebug() << "Peche::supprimer delete PECHESEMPLOYES failed:" << qDelEmployes.lastError().text();
        return false;
    }

    QSqlQuery qDelPeche(db);
    qDelPeche.prepare("DELETE FROM PECHES WHERE IDPECHE = :id");
    qDelPeche.bindValue(":id", idPeche);
    if (!qDelPeche.exec()) {
        db.rollback();
        qDebug() << "Peche::supprimer delete PECHES failed:" << qDelPeche.lastError().text();
        return false;
    }

    if (!db.commit()) {
        qDebug() << "Peche::supprimer commit failed:" << db.lastError().text();
        return false;
    }
    return true;
}

QString Peche::lastError() const
{
    return m_lastError;
}

void Peche::setLastError(const QString &error)
{
    m_lastError = error;
}
