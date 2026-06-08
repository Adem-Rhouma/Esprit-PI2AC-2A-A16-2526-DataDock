#include "pecheemploye.h"

#include <QDebug>
#include <QVariant>

PecheEmploye::PecheEmploye()
    : m_idPeche(0)
    , m_idEmploye(0)
{
}

PecheEmploye::PecheEmploye(int idPeche, int idEmploye, const QString &description)
    : m_idPeche(idPeche)
    , m_idEmploye(idEmploye)
    , m_description(description)
{
}

int PecheEmploye::getIdPeche() const { return m_idPeche; }
int PecheEmploye::getIdEmploye() const { return m_idEmploye; }
QString PecheEmploye::getDescription() const { return m_description; }

void PecheEmploye::setIdPeche(int idPeche) { m_idPeche = idPeche; }
void PecheEmploye::setIdEmploye(int idEmploye) { m_idEmploye = idEmploye; }
void PecheEmploye::setDescription(const QString &description) { m_description = description; }

bool PecheEmploye::ajouter()
{
    QSqlQuery q;
    q.prepare("INSERT INTO PECHESEMPLOYES (IDPECHE, IDEMPLOYE, DESCRIPTION) "
              "VALUES (:idPeche, :idEmploye, :description)");
    q.bindValue(":idPeche", m_idPeche);
    q.bindValue(":idEmploye", m_idEmploye);
    q.bindValue(":description", m_description);

    if (!q.exec()) {
        qDebug() << "PecheEmploye::ajouter failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool PecheEmploye::modifier()
{
    QSqlQuery q;
    q.prepare("UPDATE PECHESEMPLOYES SET DESCRIPTION = :description "
              "WHERE IDPECHE = :idPeche AND IDEMPLOYE = :idEmploye");
    q.bindValue(":description", m_description);
    q.bindValue(":idPeche", m_idPeche);
    q.bindValue(":idEmploye", m_idEmploye);

    if (!q.exec()) {
        qDebug() << "PecheEmploye::modifier failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool PecheEmploye::supprimer(int idPeche, int idEmploye)
{
    QSqlQuery q;
    q.prepare("DELETE FROM PECHESEMPLOYES WHERE IDPECHE = :idPeche AND IDEMPLOYE = :idEmploye");
    q.bindValue(":idPeche", idPeche);
    q.bindValue(":idEmploye", idEmploye);

    if (!q.exec()) {
        qDebug() << "PecheEmploye::supprimer failed:" << q.lastError().text();
        return false;
    }
    return true;
}
