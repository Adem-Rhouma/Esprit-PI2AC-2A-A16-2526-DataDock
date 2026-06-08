#include "materiel.h"

#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

materiel::materiel()
	: m_idMateriel(0)
	, m_reparable(false)
	, m_idEmploye(0)
{
}

materiel::materiel(int idMateriel,
				   const QString &libelle,
				   const QString &type,
				   const QString &etat,
				   bool reparable,
				   int idEmploye)
	: m_idMateriel(idMateriel)
	, m_libelle(libelle)
	, m_type(type)
	, m_etat(etat)
	, m_reparable(reparable)
	, m_idEmploye(idEmploye)
{
}

int materiel::idMateriel() const
{
	return m_idMateriel;
}

QString materiel::libelle() const
{
	return m_libelle;
}

QString materiel::type() const
{
	return m_type;
}

QString materiel::etat() const
{
	return m_etat;
}

bool materiel::reparable() const
{
	return m_reparable;
}

int materiel::idEmploye() const
{
	return m_idEmploye;
}

void materiel::setIdMateriel(int idMateriel)
{
	m_idMateriel = idMateriel;
}

void materiel::setLibelle(const QString &libelle)
{
	m_libelle = libelle;
}

void materiel::setType(const QString &type)
{
	m_type = type;
}

void materiel::setEtat(const QString &etat)
{
	m_etat = etat;
}

void materiel::setReparable(bool reparable)
{
	m_reparable = reparable;
}

void materiel::setIdEmploye(int idEmploye)
{
	m_idEmploye = idEmploye;
}

bool materiel::isValid() const
{
	return m_idMateriel > 0
		   && !m_libelle.trimmed().isEmpty()
		   && !m_type.trimmed().isEmpty()
		   && !m_etat.trimmed().isEmpty()
		   && m_idEmploye > 0;
}

bool materiel::ajouter() const
{
    //QSqlDatabase database = QSqlDatabase::database();
    //if (!database.isValid() || !database.isOpen()) {
    //	qWarning() << "[materiel::ajouter] Base de donnees non disponible.";
    //	return false;
    //}

    QSqlQuery query;
	query.prepare("INSERT INTO materiel(IDMATERIEL, LABELLE, TYPEMAT, ETAT, REPARABLE, IDEMPLOYE) "
				  "VALUES(:idmateriel, :labelle, :typemat, :etat, :reparable, :idemploye)");
	query.bindValue(":idmateriel", m_idMateriel);
	query.bindValue(":labelle", m_libelle);
	query.bindValue(":typemat", m_type);
	query.bindValue(":etat", m_etat);
	query.bindValue(":reparable", m_reparable ? 1 : 0);
	query.bindValue(":idemploye", m_idEmploye);

	if (!query.exec()) {
		qWarning() << "[materiel::ajouter]" << query.lastError().text();
		return false;
	}

	return true;
}

bool materiel::modifier() const
{
    //QSqlDatabase database = QSqlDatabase::database();
    //if (!database.isValid() || !database.isOpen()) {
    //	qWarning() << "[materiel::modifier] Base de donnees non disponible.";
    //	return false;
    //}

    QSqlQuery query;
	query.prepare("UPDATE materiel SET LABELLE = :labelle, TYPEMAT = :typemat, ETAT = :etat, REPARABLE = :reparable, "
				  "IDEMPLOYE = :idemploye WHERE IDMATERIEL = :idmateriel");
	query.bindValue(":idmateriel", m_idMateriel);
	query.bindValue(":labelle", m_libelle);
	query.bindValue(":typemat", m_type);
	query.bindValue(":etat", m_etat);
	query.bindValue(":reparable", m_reparable ? 1 : 0);
	query.bindValue(":idemploye", m_idEmploye);

	if (!query.exec()) {
		qWarning() << "[materiel::modifier]" << query.lastError().text();
		return false;
	}

	return query.numRowsAffected() > 0;
}

bool materiel::supprimer(int idMateriel)
{
    //QSqlDatabase database = QSqlDatabase::database();
    //if (!database.isValid() || !database.isOpen()) {
    //	qWarning() << "[materiel::supprimer] Base de donnees non disponible.";
    //	return false;
    //}

    QSqlQuery query;
	query.prepare("DELETE FROM materiel WHERE IDMATERIEL = :idmateriel");
	query.bindValue(":idmateriel", idMateriel);

	if (!query.exec()) {
		qWarning() << "[materiel::supprimer]" << query.lastError().text();
		return false;
	}

	return query.numRowsAffected() > 0;
}

bool materiel::existe(int idMateriel)
{
    //QSqlDatabase database = QSqlDatabase::database();
    //if (!database.isValid() || !database.isOpen()) {
    //	qWarning() << "[materiel::existe] Base de donnees non disponible.";
    //	return false;
    //}

    QSqlQuery query;
	query.prepare("SELECT COUNT(1) FROM materiel WHERE IDMATERIEL = :idmateriel");
	query.bindValue(":idmateriel", idMateriel);

	if (!query.exec() || !query.next()) {
		qWarning() << "[materiel::existe]" << query.lastError().text();
		return false;
	}

	return query.value(0).toInt() > 0;
}

bool materiel::recupererParId(int idMateriel, materiel &value)
{
    //QSqlDatabase database = QSqlDatabase::database();
    //if (!database.isValid() || !database.isOpen()) {
    //	qWarning() << "[materiel::recupererParId] Base de donnees non disponible.";
    //	return false;
    //}

    QSqlQuery query;
	query.prepare("SELECT IDMATERIEL, LABELLE, TYPEMAT, ETAT, REPARABLE, IDEMPLOYE FROM materiel "
				  "WHERE IDMATERIEL = :idmateriel");
	query.bindValue(":idmateriel", idMateriel);

	if (!query.exec() || !query.next()) {
		if (query.lastError().isValid()) {
			qWarning() << "[materiel::recupererParId]" << query.lastError().text();
		}
		return false;
	}

	value = materiel(
		query.value(0).toInt(),
		query.value(1).toString(),
		query.value(2).toString(),
		query.value(3).toString(),
		query.value(4).toInt() == 1,
		query.value(5).toInt()
	);

	return true;
}

QList<materiel> materiel::afficherTous()
{
	QList<materiel> result;

    //QSqlDatabase database = QSqlDatabase::database();
    //if (!database.isValid() || !database.isOpen()) {
    //	qWarning() << "[materiel::afficherTous] Base de donnees non disponible.";
    //	return result;
    //}

    QSqlQuery query;
	if (!query.exec("SELECT IDMATERIEL, LABELLE, TYPEMAT, ETAT, REPARABLE, IDEMPLOYE FROM materiel ORDER BY IDMATERIEL")) {
		qWarning() << "[materiel::afficherTous]" << query.lastError().text();
		return result;
	}

	while (query.next()) {
		result.append(materiel(
			query.value(0).toInt(),
			query.value(1).toString(),
			query.value(2).toString(),
			query.value(3).toString(),
			query.value(4).toInt() == 1,
			query.value(5).toInt()
		));
	}

	return result;
}
