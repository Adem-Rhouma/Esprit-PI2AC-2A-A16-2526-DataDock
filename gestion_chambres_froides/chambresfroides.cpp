#include "chambresfroides.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QTime>

ChambresFroides::ChambresFroides() : m_temp(0.0), m_maxCap(0.0), m_occCap(0.0), m_humidity(85.0), m_betaTest(false) {}

ChambresFroides::ChambresFroides(const QString &id, double temp,
                                 const QDate &inDate, const QDate &limitDate,
                                 const QString &certificateSan, double maxCap, double occCap,
                                 const QString &state, const QVariant &idPeche, const QString &tagNumber, double humidity, bool betaTest)
    : m_id(id), m_temp(temp), m_inDate(inDate), m_limitDate(limitDate),
      m_certificateSan(certificateSan), m_maxCap(maxCap), m_occCap(occCap), m_state(state), m_idPeche(idPeche), m_tagNumber(tagNumber), m_humidity(humidity), m_betaTest(betaTest)
{}

// Getters
QString ChambresFroides::getId() const { return m_id; }
double ChambresFroides::getTemp() const { return m_temp; }
QDate ChambresFroides::getInDate() const { return m_inDate; }
QDate ChambresFroides::getLimitDate() const { return m_limitDate; }
QString ChambresFroides::getCertificateSan() const { return m_certificateSan; }
double ChambresFroides::getMaxCap() const { return m_maxCap; }
double ChambresFroides::getOccCap() const { return m_occCap; }
QString ChambresFroides::getState() const { return m_state; }
QVariant ChambresFroides::getIdPeche() const { return m_idPeche; }
QString ChambresFroides::getTagNumber() const { return m_tagNumber; }
double ChambresFroides::getHumidity() const { return m_humidity; }
bool ChambresFroides::isBetaTest() const { return m_betaTest; }

// Setters
void ChambresFroides::setId(const QString &id) { m_id = id; }
void ChambresFroides::setTemp(double temp) { m_temp = temp; }
void ChambresFroides::setInDate(const QDate &inDate) { m_inDate = inDate; }
void ChambresFroides::setLimitDate(const QDate &limitDate) { m_limitDate = limitDate; }
void ChambresFroides::setCertificateSan(const QString &certificateSan) { m_certificateSan = certificateSan; }
void ChambresFroides::setMaxCap(double maxCap) { m_maxCap = maxCap; }
void ChambresFroides::setOccCap(double occCap) { m_occCap = occCap; }
void ChambresFroides::setState(const QString &state) { m_state = state; }
void ChambresFroides::setIdPeche(const QVariant &idPeche) { m_idPeche = idPeche; }
void ChambresFroides::setTagNumber(const QString &tagNumber) { m_tagNumber = tagNumber; }
void ChambresFroides::setHumidity(double humidity) { m_humidity = humidity; }
void ChambresFroides::setBetaTest(bool betaTest) { m_betaTest = betaTest; }

// CRUD Methods
bool ChambresFroides::ajouter_chf(QString *err) const
{
    QString finalId = m_id;
    if (finalId.isEmpty()) {
        finalId = generateNextId();
    }

    QSqlQuery query;
    // Singleton rule: only one Beta Test chamber at a time
    if (m_betaTest) {
        query.exec("UPDATE CHAMBRESFROIDES SET BETA_TEST = 0");
    }

    // We use QString for numbers to bypass any locale-specific decimal separator issues (dot vs comma)
    // and TO_DATE for absolute date format certainty.
    query.prepare("INSERT INTO CHAMBRESFROIDES (CF_ID, TEMP, INDATE, DATELIMITE, CERTIFICATESAN, MAXCAP, OCCCAP, STATUT, IDPECHE, TAG_NUMBER, HUMIDITY, BETA_TEST) "
                  "VALUES (:id, :temp, TO_DATE(:indate, 'YYYY-MM-DD'), TO_DATE(:limitdate, 'YYYY-MM-DD'), :cert, :max, :occ, :statut, :peche, :tagNumber, :humidity, :betaTest)");

    query.bindValue(":id", finalId); // String is fine for NUMBER in Oracle
    query.bindValue(":tagNumber", "CHF-" + finalId);
    query.bindValue(":temp", m_temp);
    query.bindValue(":indate", m_inDate.isValid() ? m_inDate.toString("yyyy-MM-dd") : QVariant(QMetaType::fromType<QString>()));
    query.bindValue(":limitdate", m_limitDate.isValid() ? m_limitDate.toString("yyyy-MM-dd") : QVariant(QMetaType::fromType<QString>()));
    query.bindValue(":cert", m_certificateSan);
    query.bindValue(":max", m_maxCap);
    query.bindValue(":occ", m_occCap);
    query.bindValue(":statut", m_state);
    
    if (m_idPeche.isNull() || !m_idPeche.isValid() || m_idPeche.toString().isEmpty() || m_idPeche.toString() == "0") {
        query.bindValue(":peche", QVariant(QMetaType::fromType<int>())); // Clean NULL
    } else {
        query.bindValue(":peche", m_idPeche.toInt()); // Bind as int to avoid type issues
    }
    
    query.bindValue(":humidity", m_humidity);
    query.bindValue(":betaTest", m_betaTest ? 1 : 0);

    if (!query.exec()) {
        if (err) {
            *err = QString("Database Error: %1\nDriver Error: %2\nNative Code: %3")
                      .arg(query.lastError().databaseText().isEmpty() ? "(none)" : query.lastError().databaseText(),
                           query.lastError().driverText().isEmpty() ? "QODBC: unknown error" : query.lastError().driverText(),
                           query.lastError().nativeErrorCode().isEmpty() ? "(none)" : query.lastError().nativeErrorCode());
        }
        qDebug() << "INSERT failed. Attempted values:" << finalId << m_temp << m_inDate << m_limitDate << m_certificateSan << m_maxCap << m_occCap << m_state << m_idPeche;
        return false;
    }
    return true;
}

QString ChambresFroides::generateNextId()
{
    QSqlQuery query;
    long long nextId = 1;

    // Get max ID
    if (query.exec("SELECT MAX(CAST(CF_ID AS NUMBER)) FROM CHAMBRESFROIDES")) {
        if (query.next() && !query.value(0).isNull()) {
            nextId = query.value(0).toLongLong() + 1;
        }
    }

    // Double check it doesn't exist (safety loop if IDs were deleted or manually added)
    bool exists = true;
    while (exists) {
        query.prepare("SELECT COUNT(*) FROM CHAMBRESFROIDES WHERE CF_ID = :id");
        query.bindValue(":id", QString::number(nextId));
        if (query.exec() && query.next()) {
            if (query.value(0).toInt() == 0) {
                exists = false;
            } else {
                nextId++;
            }
        } else {
            break; // Query error
        }
    }

    return QString::number(nextId);
}

bool ChambresFroides::modifier_chf(const QString &oldId, QString *err) const
{
    QSqlQuery query;
    // Singleton rule: only one Beta Test chamber at a time
    if (m_betaTest) {
        query.exec("UPDATE CHAMBRESFROIDES SET BETA_TEST = 0");
    }

    query.prepare("UPDATE CHAMBRESFROIDES SET "
                  "TEMP = :temp, INDATE = TO_DATE(:indate, 'YYYY-MM-DD'), DATELIMITE = TO_DATE(:limitdate, 'YYYY-MM-DD'), "
                  "CERTIFICATESAN = :cert, MAXCAP = :max, OCCCAP = :occ, STATUT = :statut, IDPECHE = :peche, "
		  "TAG_NUMBER = :tagNumber, HUMIDITY = :humidity, BETA_TEST = :betaTest "
                  "WHERE CF_ID = :oldId");
    
    // Ensure tag number is never empty
    QString tag = m_tagNumber;
    if (tag.isEmpty()) tag = "CHF-" + m_id;
    query.bindValue(":tagNumber", tag);

    query.bindValue(":temp", m_temp);
    query.bindValue(":indate", m_inDate.isValid() ? m_inDate.toString("yyyy-MM-dd") : QVariant(QMetaType::fromType<QString>()));
    query.bindValue(":limitdate", m_limitDate.isValid() ? m_limitDate.toString("yyyy-MM-dd") : QVariant(QMetaType::fromType<QString>()));
    query.bindValue(":cert", m_certificateSan);
    query.bindValue(":max", m_maxCap);
    query.bindValue(":occ", m_occCap);
    query.bindValue(":statut", m_state);
    
    if (m_idPeche.isNull() || !m_idPeche.isValid() || m_idPeche.toString().isEmpty() || m_idPeche.toString() == "0") {
        query.bindValue(":peche", QVariant(QMetaType::fromType<int>()));
    } else {
        query.bindValue(":peche", m_idPeche.toInt());
    }
    
    query.bindValue(":humidity", m_humidity);
    query.bindValue(":betaTest", m_betaTest ? 1 : 0);
    query.bindValue(":oldId", oldId);

    if (!query.exec()) {
        if (err) {
            *err = QString("Database Error: %1\nDriver Error: %2\nNative Code: %3")
                      .arg(query.lastError().databaseText().isEmpty() ? "(none)" : query.lastError().databaseText(),
                           query.lastError().driverText().isEmpty() ? "QODBC: unknown error" : query.lastError().driverText(),
                           query.lastError().nativeErrorCode().isEmpty() ? "(none)" : query.lastError().nativeErrorCode());
        }
        return false;
    }
    return true;
}

bool ChambresFroides::supprimer_chf(const QString &id, QString *err)
{
    QSqlQuery query;
    query.prepare("DELETE FROM CHAMBRESFROIDES WHERE CF_ID = :id");
    query.bindValue(":id", id); // Using string for consistency with ajouter/modifier

    if (!query.exec()) {
        if (err) {
            *err = QString("Database Error: %1\nDriver Error: %2\nNative Code: %3")
                      .arg(query.lastError().databaseText().isEmpty() ? "(none)" : query.lastError().databaseText(),
                           query.lastError().driverText().isEmpty() ? "QODBC: unknown error" : query.lastError().driverText(),
                           query.lastError().nativeErrorCode().isEmpty() ? "(none)" : query.lastError().nativeErrorCode());
        }
        qDebug() << "DELETE failed for ID:" << id << "Error:" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQueryModel *ChambresFroides::afficher_chf(QString *err)
{
    QSqlQuery alterQuery;
    alterQuery.exec("ALTER TABLE CHAMBRESFROIDES ADD TAG_NUMBER VARCHAR2(50)");
    alterQuery.exec("ALTER TABLE CHAMBRESFROIDES ADD HUMIDITY NUMBER DEFAULT 85");
    alterQuery.exec("ALTER TABLE CHAMBRESFROIDES ADD BETA_TEST NUMBER(1) DEFAULT 0");
    alterQuery.exec("ALTER TABLE CHAMBRES_ALERTS ADD HANDLED_BY VARCHAR2(100) DEFAULT 'System'");
    
    // Auto-populate missing tag numbers for existing records, or fix tags that missing the prefix
    alterQuery.exec("UPDATE CHAMBRESFROIDES SET TAG_NUMBER = 'CHF-' || CF_ID WHERE TAG_NUMBER IS NULL OR TAG_NUMBER NOT LIKE 'CHF-%'");

    QSqlQuery checkQuery;
    if (checkQuery.exec("SELECT COUNT(*) FROM CHAMBRESFROIDES") && checkQuery.next()) {
        if (checkQuery.value(0).toInt() == 0) {
            seedDatabase();
        }
    }

    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT CF_ID, TEMP, INDATE, DATELIMITE, CERTIFICATESAN, MAXCAP, OCCCAP, STATUT, IDPECHE, TAG_NUMBER, HUMIDITY, BETA_TEST FROM CHAMBRESFROIDES");

    if (model->lastError().isValid()) {
        if (err) *err = model->lastError().text();
        qDebug() << "Error in afficher:" << model->lastError().text();
        delete model;
        return nullptr;
    }

    return model;
}

QMap<QString, double> ChambresFroides::getStatistics(QString *err)
{
    QMap<QString, double> stats;
    QSqlQuery query;

    // Average Temperature
    if (query.exec("SELECT AVG(TEMP), MIN(TEMP), MAX(TEMP) FROM CHAMBRESFROIDES")) {
        if (query.next()) {
            stats["avg_temp"] = query.value(0).toDouble();
            stats["min_temp"] = query.value(1).toDouble();
            stats["max_temp"] = query.value(2).toDouble();
        }
    } else if (err) *err += "Temp error: " + query.lastError().text() + "; ";

    // Total Stock and Occupancy Rate
    if (query.exec("SELECT SUM(OCCCAP), SUM(MAXCAP) FROM CHAMBRESFROIDES")) {
        if (query.next()) {
            stats["total_stock"] = query.value(0).toDouble();
            double totalMax = query.value(1).toDouble();
            stats["occ_rate"] = (totalMax > 0) ? (stats["total_stock"] / totalMax * 100.0) : 0.0;
        }
    } else if (err) *err += "Stock error: " + query.lastError().text() + "; ";

    // Active and Maintenance Count
    auto activeAlerts = getActiveAlerts();
    if (query.exec("SELECT "
                   "SUM(CASE WHEN STATUT = 'Active' THEN 1 ELSE 0 END), "
                   "COUNT(*) "
                   "FROM CHAMBRESFROIDES")) {
        if (query.next()) {
            stats["active_count"] = query.value(0).toDouble();
            stats["alert_count"] = activeAlerts.size(); // Sychronized with Inbox
            stats["total_count"] = query.value(1).toDouble();
        }
    } else if (err) *err += "Counts error: " + query.lastError().text() + "; ";

    return stats;
}

QSqlQueryModel* ChambresFroides::getStatusDistribution(QString *err)
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT STATUT, COUNT(*) FROM CHAMBRESFROIDES GROUP BY STATUT");
    if (model->lastError().isValid()) {
        if (err) *err = model->lastError().text();
        delete model;
        return nullptr;
    }
    return model;
}

QSqlQueryModel* ChambresFroides::getCapacityStats(QString *err)
{
    QSqlQueryModel *model = new QSqlQueryModel();
    // Fetch last 10 chambers to avoid overcrowding chart
    model->setQuery("SELECT TAG_NUMBER, MAXCAP, OCCCAP FROM (SELECT * FROM CHAMBRESFROIDES ORDER BY CF_ID DESC) WHERE ROWNUM <= 10");
    if (model->lastError().isValid()) {
        if (err) *err = model->lastError().text();
        delete model;
        return nullptr;
    }
    return model;
}

QSqlQueryModel* ChambresFroides::getTemperatureStats(QString *err)
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT TAG_NUMBER, TEMP FROM (SELECT * FROM CHAMBRESFROIDES ORDER BY CF_ID DESC) WHERE ROWNUM <= 10");
    if (model->lastError().isValid()) {
        if (err) *err = model->lastError().text();
        delete model;
        return nullptr;
    }
    return model;
}

bool ChambresFroides::initAlertSystem()
{
    QSqlQuery query;
    // Oracle friendly initialization - No IF NOT EXISTS
    // We try to create, if it fails because it exists, that's fine.
    QString sql = "CREATE TABLE CHAMBRES_ALERTS ("
                  "ALERT_ID VARCHAR2(100) PRIMARY KEY, "
                  "STATUS VARCHAR2(20) DEFAULT 'New', "
                  "LAST_UPDATE DATE)";
    
    if (!query.exec(sql)) {
        // If error is "name is already used by an existing object" (ORA-00955), it's not a real error for us
        if (query.lastError().nativeErrorCode() != "955") {
            qDebug() << "InitAlertSystem Table Error:" << query.lastError().text();
            return false;
        }
    }
    query.exec("ALTER TABLE CHAMBRES_ALERTS ADD DATE_CREATED DATE");
    query.exec("ALTER TABLE CHAMBRES_ALERTS ADD DATE_WORKING DATE");
    query.exec("ALTER TABLE CHAMBRES_ALERTS ADD DATE_RESOLVED DATE");
    return true;
}

bool ChambresFroides::updateAlertStatus(const QString &alertId, const QString &status, QString *err)
{
    QSqlQuery query;
    QString updateSuffix = "";
    if (status == "Working") updateSuffix = ", t.DATE_WORKING = SYSDATE";
    else if (status == "Done" || status == "Ignored") updateSuffix = ", t.DATE_RESOLVED = SYSDATE";
    
    QString sql = QString("MERGE INTO CHAMBRES_ALERTS t "
                  "USING (SELECT :id as id, :status as status FROM DUAL) s "
                  "ON (t.ALERT_ID = s.id) "
                  "WHEN MATCHED THEN UPDATE SET t.STATUS = s.status, t.LAST_UPDATE = SYSDATE %1 "
                  "WHEN NOT MATCHED THEN INSERT (ALERT_ID, STATUS, LAST_UPDATE, DATE_CREATED) VALUES (s.id, s.status, SYSDATE, SYSDATE)")
                  .arg(updateSuffix);
    query.prepare(sql);
    query.bindValue(":id", alertId);
    query.bindValue(":status", status);
    
    if (!query.exec()) {
        if (err) *err = query.lastError().text();
        return false;
    }
    return true;
}

QList<QMap<QString, QString>> ChambresFroides::getActiveAlerts(QString *err)
{
    QList<QMap<QString, QString>> alerts;
    initAlertSystem(); // Fail-safe

    // 1. Fetch live data for potential alerts
    QSqlQuery query;
    // Standardize with explicit aliases to ensure query.value("ALIAS") always works
    QString sql = 
        "SELECT c.CF_ID as ID, c.TAG_NUMBER as TAG, c.TEMP as T, c.OCCCAP as O, c.MAXCAP as M, c.STATUT as S, c.DATELIMITE as L, "
        "NVL(a.STATUS, 'New') as AS_, "
        "a.DATE_CREATED as DC, a.DATE_WORKING as DW, a.DATE_RESOLVED as DR "
        "FROM CHAMBRESFROIDES c "
        "LEFT JOIN CHAMBRES_ALERTS a ON (TRIM(CAST(c.CF_ID AS VARCHAR2(50))) || '_GENERIC' = a.ALERT_ID)"; 
    
    if (!query.exec(sql)) {
        if (err) *err = "Alert SQL Error: " + query.lastError().text();
        qDebug() << "CRITICAL: getActiveAlerts query failed:" << query.lastError().text();
        return alerts;
    }

    QDate today = QDate::currentDate();
    int rowCount = 0;

    while (query.next()) {
        rowCount++;
        QString id = query.value("ID").toString().trimmed();
        QString tag = query.value("TAG").toString().trimmed();
        if (tag.isEmpty()) tag = "ID " + id;
        double temp = query.value("T").toDouble();
        double occ = query.value("O").toDouble();
        double max = query.value("M").toDouble();
        QString status = query.value("S").toString().trimmed();
        QDate limit = query.value("L").toDate();
        QString alertStatus = query.value("AS_").toString();

        QString dc = query.value("DC").toDateTime().toString("dd/MM/yyyy HH:mm");
        QString dw = query.value("DW").toDateTime().toString("dd/MM/yyyy HH:mm");
        QString dr = query.value("DR").toDateTime().toString("dd/MM/yyyy HH:mm");
        bool needsTracking = query.value("DC").isNull();
        if (needsTracking) dc = QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm");

        auto applyDates = [&](QMap<QString, QString> &alert) {
            if (!dc.trimmed().isEmpty()) alert["date_created"] = dc;
            if (!dw.trimmed().isEmpty()) alert["date_working"] = dw;
            if (!dr.trimmed().isEmpty()) alert["date_resolved"] = dr;
            if (needsTracking) alert["needs_tracking"] = "true";
        };

        // Condition 1: Overcapacity (>100%)
        if (occ > max && max > 0) {
            QMap<QString, QString> alert;
            alert["id"] = id;
            alert["tag"] = tag;
            alert["type"] = "Overcapacity";
            alert["severity"] = "Critical";
            alert["msg"] = QString("Chambre %1 surchargée (%2/%3 kg)").arg(tag).arg(occ).arg(max);
            alert["status"] = alertStatus;
            applyDates(alert);
            alerts.append(alert);
        }

        // Condition 2: Critical Temp (> 5°C for fish safety)
        if (temp >= 5.0) { 
            QMap<QString, QString> alert;
            alert["id"] = id;
            alert["tag"] = tag;
            alert["type"] = "Risque Thermique";
            alert["severity"] = (temp >= 10.0) ? "Critical" : "High";
            alert["msg"] = QString("Chambre %1 : Température dangereuse (%2°C)").arg(tag).arg(temp);
            alert["status"] = alertStatus;
            applyDates(alert);
            alerts.append(alert);
        }

        // Condition 3: Status check (Maintenance/Broken)
        QString statusUpper = status.toUpper();
        if (statusUpper != "ACTIVE") {
            QMap<QString, QString> alert;
            alert["id"] = id;
            alert["tag"] = tag;
            alert["type"] = (statusUpper == "BROKEN") ? "Panne Matérielle" : "Maintenance / Inactive";
            alert["severity"] = (statusUpper == "BROKEN") ? "Critical" : "Medium";
            alert["msg"] = QString("Chambre %1 : État critique détecté (%2)").arg(tag).arg(status);
            alert["status"] = alertStatus;
            applyDates(alert);
            alerts.append(alert);
        }
        if (limit.isValid()) {
            qint64 daysToExpiry = today.daysTo(limit);
            if (daysToExpiry < 0) {
                QMap<QString, QString> alert;
                alert["id"] = id;
                alert["tag"] = tag;
                alert["type"] = "Expiré";
                alert["severity"] = "Critique";
                alert["msg"] = QString("Chambre %1 : Date limite dépassée !").arg(tag);
                alert["status"] = alertStatus;
                applyDates(alert);
                alerts.append(alert);
            } else if (daysToExpiry <= 2) {
                QMap<QString, QString> alert;
                alert["id"] = id;
                alert["tag"] = tag;
                alert["type"] = "Expiration Proche";
                alert["severity"] = "Haute";
                alert["msg"] = QString("Chambre %1 : Expire dans %2 jours.").arg(tag).arg(daysToExpiry);
                alert["status"] = alertStatus;
                applyDates(alert);
                alerts.append(alert);
            }
        }
    }

    // 2. Global Bias/Imbalance Check
    if (query.exec("SELECT "
                   "SUM(CASE WHEN OCCCAP > MAXCAP * 0.9 THEN 1 ELSE 0 END), "
                   "SUM(CASE WHEN OCCCAP < MAXCAP * 0.1 THEN 1 ELSE 0 END), "
                   "COUNT(*) FROM CHAMBRESFROIDES")) {
        if (query.next()) {
            int overloaded = query.value(0).toInt();
            int empty = query.value(1).toInt();
            if (overloaded > 0 && empty > 0) {
                QMap<QString, QString> alert;
                alert["id"] = "GLOBAL_BIAS";
                alert["type"] = "Déséquilibre de Charge";
                alert["severity"] = "Moyenne";
                alert["msg"] = QString("Logistique inefficace: %1 chambres saturées alors que %2 sont presque vides.").arg(overloaded).arg(empty);
                
                QString gbStatus = "New";
                QSqlQuery gbQuery("SELECT STATUS, DATE_CREATED, DATE_WORKING, DATE_RESOLVED, HANDLED_BY FROM CHAMBRES_ALERTS WHERE ALERT_ID = 'GLOBAL_BIAS_GENERIC'");
                if (gbQuery.next()) {
                    gbStatus = gbQuery.value(0).toString();
                    alert["date_created"] = gbQuery.value(1).toDateTime().toString("dd/MM/yyyy HH:mm");
                    alert["date_working"] = gbQuery.value(2).toDateTime().toString("dd/MM/yyyy HH:mm");
                    alert["date_resolved"] = gbQuery.value(3).toDateTime().toString("dd/MM/yyyy HH:mm");
                    alert["handled_by"] = gbQuery.value(4).toString();
                } else {
                    alert["needs_tracking"] = "true";
                    alert["date_created"] = QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm");
                }
                alert["status"] = gbStatus; 
                alerts.append(alert);
            }
        }
    }

    for (const auto &a : alerts) {
        if (a.contains("needs_tracking") && a["needs_tracking"] == "true") {
            QString id = a["id"] + "_GENERIC";
            if (a["id"] == "GLOBAL_BIAS") id = "GLOBAL_BIAS_GENERIC";
            QSqlQuery iq;
            iq.prepare("INSERT INTO CHAMBRES_ALERTS (ALERT_ID, STATUS, LAST_UPDATE, DATE_CREATED) VALUES (:id, 'New', SYSDATE, SYSDATE)");
            iq.bindValue(":id", id);
            iq.exec();
        }
    }

    return alerts;
}

QMap<QString, QString> ChambresFroides::getAdvancedIntelligence(QString *)
{
    QMap<QString, QString> intel;
    QSqlQuery query;

    // 1. Extreme Profiles (Thermal Outliers)
    if (query.exec("SELECT TAG_NUMBER, TEMP FROM (SELECT TAG_NUMBER, TEMP FROM CHAMBRESFROIDES ORDER BY TEMP DESC) WHERE ROWNUM = 1")) {
        if (query.next()) intel["hottest"] = QString("%1 (%2°C)").arg(query.value(0).toString()).arg(query.value(1).toDouble());
    }
    if (query.exec("SELECT TAG_NUMBER, TEMP FROM (SELECT TAG_NUMBER, TEMP FROM CHAMBRESFROIDES ORDER BY TEMP ASC) WHERE ROWNUM = 1")) {
        if (query.next()) intel["coldest"] = QString("%1 (%2°C)").arg(query.value(0).toString()).arg(query.value(1).toDouble());
    }

    // 2. Capacity Hub (Overloaded / Underused)
    if (query.exec("SELECT COUNT(*) FROM CHAMBRESFROIDES WHERE OCCCAP > MAXCAP * 0.85")) {
        if (query.next()) intel["critical_load_count"] = query.value(0).toString();
    }
    if (query.exec("SELECT COUNT(*) FROM CHAMBRESFROIDES WHERE OCCCAP < MAXCAP * 0.30")) {
        if (query.next()) intel["underused_count"] = query.value(0).toString();
    }

    // 3. Health Score Calculation
    // Logic: Start at 100%. Deduct points for alerts.
    // NOTE: Only deduct for 'New' or 'Working' alerts. 'Done' and 'Ignored' are resolved.
    double health = 100.0;
    auto alerts = getActiveAlerts();
    for (const auto &a : alerts) {
        if (a["status"] == "Done") continue;
        
        if (a["severity"] == "Critical") health -= 15;
        else if (a["severity"] == "High") health -= 10;
        else health -= 5;
    }
    intel["health_score"] = QString::number(qMax(0.0, health), 'f', 1);

    return intel;
}

void ChambresFroides::seedDatabase()
{
    QSqlQuery query;
    // Check again inside
    query.exec("SELECT COUNT(*) FROM CHAMBRESFROIDES");
    if (query.next() && query.value(0).toInt() >= 5) return;

    struct Sample {
        double temp;
        int daysOffset;
        QString cert;
        double max;
        double occ;
        QString status;
    };

    QList<Sample> samples = {
        {-2.5, -5, "https://cert.com/101.jpg", 5000, 3200, "Active"},
        {-1.0, -2, "https://cert.com/102.jpg", 3000, 2800, "Active"},
        {-5.0, -10, "https://cert.com/103.jpg", 8000, 1500, "Maintenance"},
        {2.0, -1, "https://cert.com/104.jpg", 4000, 3900, "Active"},
        {-18.0, -20, "https://cert.com/105.jpg", 10000, 0, "Active"},
        {6.5, -15, "https://cert.com/106.jpg", 2000, 2100, "Broken"}
    };

    QDate today = QDate::currentDate();
    for (const auto &s : samples) {
        ChambresFroides ch("", s.temp, today.addDays(s.daysOffset), today.addDays(s.daysOffset + 30), 
                           s.cert, s.max, s.occ, s.status, QVariant(), "", 85.0);
        ch.ajouter_chf();
    }
}

