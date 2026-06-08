#include "operationcamion.h"
#include <QVariant>

OperationCamion::OperationCamion()
    : m_opId(0), m_camionId(0), m_idPeche(0), m_priorite(5) {}

OperationCamion::OperationCamion(int idPeche, int camionId, const QString &typeOp,
                                 const QString &zoneOp, const QDateTime &dDebut,
                                 const QDateTime &dFin, const QString &statut,
                                 int priorite, const QString &desc)
    : m_opId(0), m_camionId(camionId), m_idPeche(idPeche),
      m_typeOperation(typeOp), m_zoneOperation(zoneOp),
      m_dateDebut(dDebut), m_dateFin(dFin), m_statut(statut),
      m_priorite(priorite), m_description(desc) {}

int OperationCamion::getOpId() const { return m_opId; }
int OperationCamion::getCamionId() const { return m_camionId; }
int OperationCamion::getIdPeche() const { return m_idPeche; }
QString OperationCamion::getTypeOperation() const { return m_typeOperation; }
QString OperationCamion::getZoneOperation() const { return m_zoneOperation; }
QDateTime OperationCamion::getDateDebut() const { return m_dateDebut; }
QDateTime OperationCamion::getDateFin() const { return m_dateFin; }
QString OperationCamion::getStatut() const { return m_statut; }
int OperationCamion::getPriorite() const { return m_priorite; }
QString OperationCamion::getDescription() const { return m_description; }

void OperationCamion::setOpId(int opId) { m_opId = opId; }
void OperationCamion::setCamionId(int camion) { m_camionId = camion; }
void OperationCamion::setIdPeche(int peche) { m_idPeche = peche; }
void OperationCamion::setTypeOperation(const QString &type) { m_typeOperation = type; }
void OperationCamion::setZoneOperation(const QString &zone) { m_zoneOperation = zone; }
void OperationCamion::setDateDebut(const QDateTime &debut) { m_dateDebut = debut; }
void OperationCamion::setDateFin(const QDateTime &fin) { m_dateFin = fin; }
void OperationCamion::setStatut(const QString &statut) { m_statut = statut; }
void OperationCamion::setPriorite(int prio) { m_priorite = prio; }
void OperationCamion::setDescription(const QString &desc) { m_description = desc; }

bool OperationCamion::ajouter()
{
    int newId = 1;
    QSqlQuery qMax("SELECT NVL(MAX(OpCamId), 0) + 1 FROM OperationsCamions");
    if (qMax.next()) {
        newId = qMax.value(0).toInt();
    }
    m_opId = newId;

    QSqlQuery q;
    q.prepare(
        "INSERT INTO OperationsCamions "
        "(OpCamId, CamionId, idPeche, TypeOperation, ZoneOperation, "
        " DateDebut, DateFin, Statut, Priorite, Description) "
        "VALUES (:opId, :camId, :peche, :typeOp, :zoneOp, "
        "  :dDebut, :dFin, :statut, :prio, :desc)"
    );
    q.bindValue(":opId",   m_opId);
    q.bindValue(":camId",  m_camionId);
    q.bindValue(":peche",  m_idPeche);
    q.bindValue(":typeOp", m_typeOperation);
    q.bindValue(":zoneOp", m_zoneOperation);
    q.bindValue(":dDebut", m_dateDebut);
    q.bindValue(":dFin",   m_dateFin);
    q.bindValue(":statut", m_statut);
    q.bindValue(":prio",   m_priorite);
    q.bindValue(":desc",   m_description);

    return q.exec();
}

bool OperationCamion::modifier()
{
    QSqlQuery q;
    q.prepare(
        "UPDATE OperationsCamions SET "
        "CamionId = :camId, idPeche = :peche, TypeOperation = :typeOp, "
        "ZoneOperation = :zoneOp, DateDebut = :dDebut, "
        "DateFin = :dFin, Statut = :statut, "
        "Priorite = :prio, Description = :desc "
        "WHERE OpCamId = :opId"
    );
    q.bindValue(":camId",  m_camionId);
    q.bindValue(":peche",  m_idPeche);
    q.bindValue(":typeOp", m_typeOperation);
    q.bindValue(":zoneOp", m_zoneOperation);
    q.bindValue(":dDebut", m_dateDebut);
    q.bindValue(":dFin",   m_dateFin);
    q.bindValue(":statut", m_statut);
    q.bindValue(":prio",   m_priorite);
    q.bindValue(":desc",   m_description);
    q.bindValue(":opId",   m_opId);

    return q.exec();
}

bool OperationCamion::supprimer(int opId)
{
    QSqlQuery q;
    q.prepare("DELETE FROM OperationsCamions WHERE OpCamId = :id");
    q.bindValue(":id", opId);
    return q.exec();
}
