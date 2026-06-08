#ifndef NAVIRE_H
#define NAVIRE_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QDebug>

class Navire
{
private:
    int idNavire;
    QString nomNavire;
    QString matricule;
    QString typeNavire;
    double longueur;
    double puissanceMoteur;
    QString proprietaire;
    QString statutNavire;
    double maxCargoCapacity;
    QString m_lastError;

public:
    Navire();
    Navire(int id, QString nom, QString mat, QString type, double longeur,
           double puissance, QString proprio, QString statut, double capacity = 0);

    bool ajouter();
    bool supprimer();
    bool modifier();
    bool charger(int id);

    QSqlQueryModel* getNavire();
    QSqlQueryModel* afficher(const QString& orderBy = "IDNAVIRE", const QString& orderDir = "ASC");
    QSqlQueryModel* rechercher(const QString& critere,
                               const QString& orderBy = "IDNAVIRE",
                               const QString& orderDir = "ASC");

    static bool existe(int id);
    static bool supprimer(int id);
    QString lastError() const { return m_lastError; }


    int getIdNavire() const { return idNavire; }
    QString getNomNavire() const { return nomNavire; }
    QString getMatricule() const { return matricule; }
    QString getTypeNavire() const { return typeNavire; }
    double getLongueur() const { return longueur; }
    double getPuissanceMoteur() const { return puissanceMoteur; }
    QString getProprietaire() const { return proprietaire; }
    QString getStatutNavire() const { return statutNavire; }
    double getMaxCargoCapacity() const { return maxCargoCapacity; }

    void setIdNavire(int id) { idNavire = id; }
    void setNomNavire(const QString& nom) { nomNavire = nom; }
    void setMatricule(const QString& mat) { matricule = mat; }
    void setTypeNavire(const QString& type) { typeNavire = type; }
    void setLongueur(double l) { longueur = l; }
    void setPuissanceMoteur(double p) { puissanceMoteur = p; }
    void setProprietaire(const QString& prop) { proprietaire = prop; }
    void setStatutNavire(const QString& statut) { statutNavire = statut; }
    void setMaxCargoCapacity(double c) { maxCargoCapacity = c; }
};

#endif // NAVIRE_H
