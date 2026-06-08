#include "pecheespece.h"

#include <QDebug>
#include <QVariant>

PecheEspece::PecheEspece()
    : m_idPeche(0)
    , m_idEspece(0)
    , m_quantite(0.0)
    , m_prixUnitaire(0.0)
{
}

PecheEspece::PecheEspece(int idPeche, int idEspece, double quantite, double prixUnitaire)
    : m_idPeche(idPeche)
    , m_idEspece(idEspece)
    , m_quantite(quantite)
    , m_prixUnitaire(prixUnitaire)
{
}

int PecheEspece::getIdPeche() const { return m_idPeche; }
int PecheEspece::getIdEspece() const { return m_idEspece; }
double PecheEspece::getQuantite() const { return m_quantite; }
double PecheEspece::getPrixUnitaire() const { return m_prixUnitaire; }

void PecheEspece::setIdPeche(int idPeche) { m_idPeche = idPeche; }
void PecheEspece::setIdEspece(int idEspece) { m_idEspece = idEspece; }
void PecheEspece::setQuantite(double quantite) { m_quantite = quantite; }
void PecheEspece::setPrixUnitaire(double prixUnitaire) { m_prixUnitaire = prixUnitaire; }

bool PecheEspece::ajouter()
{
    QSqlQuery q;
    q.prepare("INSERT INTO PECHESESPECES (IDPECHE, IDESPECE, QUANTITE, PRIXUNITAIRE) "
              "VALUES (:idPeche, :idEspece, :quantite, :prix)");
    q.bindValue(":idPeche", m_idPeche);
    q.bindValue(":idEspece", m_idEspece);
    q.bindValue(":quantite", m_quantite);
    q.bindValue(":prix", m_prixUnitaire);

    if (!q.exec()) {
        qDebug() << "PecheEspece::ajouter failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool PecheEspece::modifier()
{
    QSqlQuery q;
    q.prepare("UPDATE PECHESESPECES SET QUANTITE = :quantite, PRIXUNITAIRE = :prix "
              "WHERE IDPECHE = :idPeche AND IDESPECE = :idEspece");
    q.bindValue(":quantite", m_quantite);
    q.bindValue(":prix", m_prixUnitaire);
    q.bindValue(":idPeche", m_idPeche);
    q.bindValue(":idEspece", m_idEspece);

    if (!q.exec()) {
        qDebug() << "PecheEspece::modifier failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool PecheEspece::supprimer(int idPeche, int idEspece)
{
    QSqlQuery q;
    q.prepare("DELETE FROM PECHESESPECES WHERE IDPECHE = :idPeche AND IDESPECE = :idEspece");
    q.bindValue(":idPeche", idPeche);
    q.bindValue(":idEspece", idEspece);

    if (!q.exec()) {
        qDebug() << "PecheEspece::supprimer failed:" << q.lastError().text();
        return false;
    }
    return true;
}
