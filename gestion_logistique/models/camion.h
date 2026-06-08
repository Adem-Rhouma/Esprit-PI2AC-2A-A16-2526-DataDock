#ifndef CAMION_H
#define CAMION_H

#include <QString>
#include <QSqlQuery>
#include <QSqlError>

class Camion
{
private:
    int m_id;
    QString m_immatriculation;
    QString m_type;
    double m_capacite;
    QString m_zone;
    QString m_statut;
    QString m_description;
    double m_latitude;
    double m_longitude;
    double m_kilometrage;
    double m_consommation_totale;
    QString m_rfid;
public:
    Camion();
    Camion(int id, const QString &immat, const QString &type, double cap,
           const QString &zone, const QString &statut, const QString &desc,
           double lat = 0.0, double lon = 0.0, double kilo = 0.0, double conso = 0.0,
           const QString &rfid = "");


    int getId() const;
    QString getImmatriculation() const;
    QString getType() const;
    double getCapacite() const;
    QString getZone() const;
    QString getStatut() const;
    QString getDescription() const;
    double getLatitude() const;
    double getLongitude() const;
    double getKilometrage() const;
    double getConsommationTotale() const;
    QString getRfid() const;


    void setId(int id);
    void setImmatriculation(const QString &immat);
    void setType(const QString &type);
    void setCapacite(double cap);
    void setZone(const QString &zone);
    void setStatut(const QString &statut);
    void setDescription(const QString &desc);
    void setLatitude(double lat);
    void setLongitude(double lon);
    void setKilometrage(double kilo);
    void setConsommationTotale(double conso);
    void setRfid(const QString &rfid);

    bool ajouter();
    bool modifier();
    static bool supprimer(int id);

};

#endif // CAMION_H
