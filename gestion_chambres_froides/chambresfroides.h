#ifndef CHAMBRESFROIDES_H
#define CHAMBRESFROIDES_H

#include <QString>
#include <QDate>
#include <QSqlQueryModel>
#include <QMap>

class ChambresFroides
{
public:
    ChambresFroides();
    ChambresFroides(const QString &id,
                    double temp,
                    const QDate &inDate,
                    const QDate &limitDate,
                    const QString &certificateSan,
                    double maxCap,
                    double occCap,
                    const QString &state,
                    const QVariant &idPeche,
                    const QString &tagNumber = "",
                    double humidity = 85.0,
                    bool betaTest = false);

    // Getters
    QString getId() const;
    double getTemp() const;
    QDate getInDate() const;
    QDate getLimitDate() const;
    QString getCertificateSan() const;
    double getMaxCap() const;
    double getOccCap() const;
    QString getState() const;
    QVariant getIdPeche() const;
    QString getTagNumber() const;
    double getHumidity() const;
    bool isBetaTest() const;

    // Setters
    void setId(const QString &id);
    void setTemp(double temp);
    void setInDate(const QDate &inDate);
    void setLimitDate(const QDate &limitDate);
    void setCertificateSan(const QString &certificateSan);
    void setMaxCap(double maxCap);
    void setOccCap(double occCap);
    void setState(const QString &state);
    void setIdPeche(const QVariant &idPeche);
    void setTagNumber(const QString &tagNumber);
    void setHumidity(double humidity);
    void setBetaTest(bool betaTest);

    // CRUD Methods
    bool ajouter_chf(QString *err = nullptr) const;
    bool modifier_chf(const QString &oldId, QString *err = nullptr) const;
    static bool supprimer_chf(const QString &id, QString *err = nullptr);
    static QSqlQueryModel *afficher_chf(QString *err = nullptr);

    // Statistics Methods
    static QMap<QString, double> getStatistics(QString *err = nullptr);
    static QSqlQueryModel* getStatusDistribution(QString *err = nullptr);
    static QSqlQueryModel* getCapacityStats(QString *err = nullptr);
    static QSqlQueryModel* getTemperatureStats(QString *err = nullptr);
    static QMap<QString, QString> getAdvancedIntelligence(QString *err = nullptr);
    static QList<QMap<QString, QString>> getActiveAlerts(QString *err = nullptr);
    static bool updateAlertStatus(const QString &alertId, const QString &status, QString *err = nullptr);
    static bool initAlertSystem(); // Ensure alert table exists
    static QString generateNextId();
    static void seedDatabase(); // Fill with initial sample data

private:
    QString m_id;
    double m_temp;
    QDate m_inDate;
    QDate m_limitDate;
    QString m_certificateSan;
    double m_maxCap;
    double m_occCap;
    QString m_state;
    QVariant m_idPeche; // QVariant to strictly support NULL
    QString m_tagNumber;
    double m_humidity;
    bool m_betaTest;
};

#endif // CHAMBRESFROIDES_H
