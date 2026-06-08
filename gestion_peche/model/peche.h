#ifndef PECHE_MODEL_H
#define PECHE_MODEL_H

#include <QDate>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

class Peche
{
public:
    Peche();
    Peche(int id,
          const QDate &datePeche,
          const QString &zonePeche,
          const QString &description,
          int idNavire);

    int getId() const;
    QDate getDatePeche() const;
    QString getZonePeche() const;
    QString getDescription() const;
    int getIdNavire() const;

    void setId(int id);
    void setDatePeche(const QDate &datePeche);
    void setZonePeche(const QString &zonePeche);
    void setDescription(const QString &description);
    void setIdNavire(int idNavire);

    bool ajouter();
    bool modifier();
    static bool supprimer(int idPeche);
    QString lastError() const;

private:
    void setLastError(const QString &error);

    int m_id;
    QDate m_datePeche;
    QString m_zonePeche;
    QString m_description;
    int m_idNavire;
    QString m_lastError;
};

#endif // PECHE_MODEL_H
