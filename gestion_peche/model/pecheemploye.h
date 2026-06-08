#ifndef PECHEEMPLOYE_MODEL_H
#define PECHEEMPLOYE_MODEL_H

#include <QSqlError>
#include <QSqlQuery>
#include <QString>

class PecheEmploye
{
public:
    PecheEmploye();
    PecheEmploye(int idPeche, int idEmploye, const QString &description);

    int getIdPeche() const;
    int getIdEmploye() const;
    QString getDescription() const;

    void setIdPeche(int idPeche);
    void setIdEmploye(int idEmploye);
    void setDescription(const QString &description);

    bool ajouter();
    bool modifier();
    static bool supprimer(int idPeche, int idEmploye);

private:
    int m_idPeche;
    int m_idEmploye;
    QString m_description;
};

#endif // PECHEEMPLOYE_MODEL_H
