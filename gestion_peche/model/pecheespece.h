#ifndef PECHEESPECE_MODEL_H
#define PECHEESPECE_MODEL_H

#include <QSqlError>
#include <QSqlQuery>
#include <QString>

class PecheEspece
{
public:
    PecheEspece();
    PecheEspece(int idPeche, int idEspece, double quantite, double prixUnitaire);

    int getIdPeche() const;
    int getIdEspece() const;
    double getQuantite() const;
    double getPrixUnitaire() const;

    void setIdPeche(int idPeche);
    void setIdEspece(int idEspece);
    void setQuantite(double quantite);
    void setPrixUnitaire(double prixUnitaire);

    bool ajouter();
    bool modifier();
    static bool supprimer(int idPeche, int idEspece);

private:
    int m_idPeche;
    int m_idEspece;
    double m_quantite;
    double m_prixUnitaire;
};

#endif // PECHEESPECE_MODEL_H
