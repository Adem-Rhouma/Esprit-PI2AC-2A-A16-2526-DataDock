#ifndef MATERIEL_H
#define MATERIEL_H

#include <QList>
#include <QString>

class materiel
{
public:
    materiel();

    materiel(int idMateriel,
             const QString &libelle,
             const QString &type,
             const QString &etat,
             bool reparable,
             int idEmploye);

    int idMateriel() const;
    QString libelle() const;
    QString type() const;
    QString etat() const;
    bool reparable() const;
    int idEmploye() const;

    void setIdMateriel(int idMateriel);
    void setLibelle(const QString &libelle);
    void setType(const QString &type);
    void setEtat(const QString &etat);
    void setReparable(bool reparable);
    void setIdEmploye(int idEmploye);

    bool isValid() const;

    bool ajouter() const;
    bool modifier() const;

    static bool supprimer(int idMateriel);
    static bool existe(int idMateriel);
    static bool recupererParId(int idMateriel, materiel &value);
    static QList<materiel> afficherTous();

private:
    int m_idMateriel;
    QString m_libelle;
    QString m_type;
    QString m_etat;
    bool m_reparable;
    int m_idEmploye;
};

#endif // MATERIEL_H
