#include "camion.h"
#include <QVariant>

Camion::Camion() : m_id(0), m_capacite(0.0), m_latitude(0.0), m_longitude(0.0), m_kilometrage(0.0), m_consommation_totale(0.0), m_rfid("") {}

Camion::Camion(int id, const QString &immat, const QString &type, double cap,
               const QString &zone, const QString &statut, const QString &desc,
               double lat, double lon, double kilo, double conso, const QString &rfid)
    : m_id(id), m_immatriculation(immat), m_type(type), m_capacite(cap),
      m_zone(zone), m_statut(statut), m_description(desc),
      m_latitude(lat), m_longitude(lon), m_kilometrage(kilo), m_consommation_totale(conso), m_rfid(rfid) {}

int Camion::getId() const { return m_id; }
QString Camion::getImmatriculation() const { return m_immatriculation; }
QString Camion::getType() const { return m_type; }
double Camion::getCapacite() const { return m_capacite; }
QString Camion::getZone() const { return m_zone; }
QString Camion::getStatut() const { return m_statut; }
QString Camion::getDescription() const { return m_description; }
double Camion::getLatitude() const { return m_latitude; }
double Camion::getLongitude() const { return m_longitude; }
double Camion::getKilometrage() const { return m_kilometrage; }
double Camion::getConsommationTotale() const { return m_consommation_totale; }
QString Camion::getRfid() const { return m_rfid; }

void Camion::setId(int id) { m_id = id; }
void Camion::setImmatriculation(const QString &immat) { m_immatriculation = immat; }
void Camion::setType(const QString &type) { m_type = type; }
void Camion::setCapacite(double cap) { m_capacite = cap; }
void Camion::setZone(const QString &zone) { m_zone = zone; }
void Camion::setStatut(const QString &statut) { m_statut = statut; }
void Camion::setDescription(const QString &desc) { m_description = desc; }
void Camion::setLatitude(double lat) { m_latitude = lat; }
void Camion::setLongitude(double lon) { m_longitude = lon; }
void Camion::setKilometrage(double kilo) { m_kilometrage = kilo; }
void Camion::setConsommationTotale(double conso) { m_consommation_totale = conso; }
void Camion::setRfid(const QString &rfid) { m_rfid = rfid; }

bool Camion::ajouter()
{
    int newId = 1;
    QSqlQuery qMax("SELECT NVL(MAX(CamionId), 0) + 1 FROM Camions");
    if (qMax.next()) {
        newId = qMax.value(0).toInt();
    }
    m_id = newId;

    QSqlQuery q;
    q.prepare("INSERT INTO Camions (CamionId, Immatriculation, TypeCamion, "
              "CapaciteCharge, ZoneStationnement, Statut, Description, Latitude, Longitude, Kilometrage, Consommation_totale, rfid) "
              "VALUES (:id, :immat, :type, :cap, :zone, :statut, :desc, :lat, :lon, :kilo, :conso, :rfid)");
    q.bindValue(":id",     m_id);
    q.bindValue(":immat",  m_immatriculation);
    q.bindValue(":type",   m_type);
    q.bindValue(":cap",    m_capacite);
    q.bindValue(":zone",   m_zone);
    q.bindValue(":statut", m_statut);
    q.bindValue(":desc",   m_description);
    q.bindValue(":lat",    m_latitude);
    q.bindValue(":lon",    m_longitude);
    q.bindValue(":kilo",   m_kilometrage);
    q.bindValue(":conso",  m_consommation_totale);
    q.bindValue(":rfid",   m_rfid);

    return q.exec();
}

bool Camion::modifier()
{
    QSqlQuery q;
    q.prepare("UPDATE Camions SET Immatriculation = :immat, TypeCamion = :type, "
              "CapaciteCharge = :cap, ZoneStationnement = :zone, "
              "Statut = :statut, Description = :desc, Latitude = :lat, Longitude = :lon, "
              "Kilometrage = :kilo, Consommation_totale = :conso, rfid = :rfid "
              "WHERE CamionId = :id");
    q.bindValue(":immat",  m_immatriculation);
    q.bindValue(":type",   m_type);
    q.bindValue(":cap",    m_capacite);
    q.bindValue(":zone",   m_zone);
    q.bindValue(":statut", m_statut);
    q.bindValue(":desc",   m_description);
    q.bindValue(":lat",    m_latitude);
    q.bindValue(":lon",    m_longitude);
    q.bindValue(":kilo",   m_kilometrage);
    q.bindValue(":conso",  m_consommation_totale);
    q.bindValue(":rfid",   m_rfid);
    q.bindValue(":id",     m_id);

    return q.exec();
}

bool Camion::supprimer(int id)
{
    QSqlQuery q;
    q.prepare("DELETE FROM Camions WHERE CamionId = :id");
    q.bindValue(":id", id);
    return q.exec();
}
