#ifndef OPERATIONCAMION_H
#define OPERATIONCAMION_H

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>

class OperationCamion
{
public:
    OperationCamion();
    OperationCamion(int idPeche, int camionId, const QString &typeOp,
                    const QString &zoneOp, const QDateTime &dDebut,
                    const QDateTime &dFin, const QString &statut,
                    int priorite, const QString &desc);

    // Getters
    int getOpId() const;
    int getCamionId() const;
    int getIdPeche() const;
    QString getTypeOperation() const;
    QString getZoneOperation() const;
    QDateTime getDateDebut() const;
    QDateTime getDateFin() const;
    QString getStatut() const;
    int getPriorite() const;
    QString getDescription() const;

    // Setters
    void setOpId(int opId);
    void setCamionId(int camion);
    void setIdPeche(int peche);
    void setTypeOperation(const QString &type);
    void setZoneOperation(const QString &zone);
    void setDateDebut(const QDateTime &debut);
    void setDateFin(const QDateTime &fin);
    void setStatut(const QString &statut);
    void setPriorite(int prio);
    void setDescription(const QString &desc);

    // DB Operations (MVC Model)
    bool ajouter();
    bool modifier();
    static bool supprimer(int opId);

private:
    int m_opId;
    int m_camionId;
    int m_idPeche;
    QString m_typeOperation;
    QString m_zoneOperation;
    QDateTime m_dateDebut;
    QDateTime m_dateFin;
    QString m_statut;
    int m_priorite;
    QString m_description;
};

#endif // OPERATIONCAMION_H
