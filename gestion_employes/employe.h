#ifndef EMPLOYE_H
#define EMPLOYE_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QDateTime>
#include <QImage>

class Employe
{
    QString IDEMPLOYE;
    QString NOM;
    QString PRENOM;
    QString ROLE;
    QString EMAIL;
    QString MOT_DE_PASSE;
    QString PHOTO_IDENTITE;
    QString SEXE; // New field

public:
    QString HEURE;
    QString HEURE_R;
    QString embedding;
    Employe();
    Employe(QString id, QString nom, QString prenom, QString role, QString email, QString mdp, QString photo, QString sexe = "");

    QString get_id();
    QString get_nom();
    QString get_prenom();
    QString get_role();
    QString get_email();
    QString get_mdp();
    QString get_photo();
    QString get_sexe();

    void set_id(QString id);
    void set_nom(QString n);
    void set_prenom(QString p);
    void set_role(QString r);
    void set_email(QString e);
    void set_mdp(QString m);
    void set_photo(QString p);
    void set_sexe(QString s);

    bool ajouter();
    QSqlQueryModel * afficher();
    bool supprimer(QString id);
    bool modifier();

    QSqlQueryModel * trier(QString critere, QString ordre);
    QSqlQueryModel * rechercher(QString recherche);
    void afficher(QSqlQueryModel *model);

    int getTotalCount();
    int getCountByRole(QString role);
    int getCountByEmailDomain(QString domain);
    int getCountByNameRange(QChar start, QChar end);
    int getCountBySexe(QString sexe);
    bool login();
    bool ajoutFaceID();

    bool verifierFaceAvecEmbedding(const QImage &image, const QString &embeddingJson);
    bool faceid(const QImage &preview);
    int sumEmployeByRole(QString ch);
    bool employeExiste();
    bool employeExisteA();
    bool enregitrement(QString id ,QString zone , int heur);
    bool enregistrerHeureRetour(QString id);
};

#endif // EMPLOYE_H
