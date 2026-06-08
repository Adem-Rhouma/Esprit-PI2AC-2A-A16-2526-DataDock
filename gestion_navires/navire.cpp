#include "navire.h"
#include "connection.h"
#include <QVariant>
#include <QStringList>

Navire::Navire() {}

Navire::Navire(int id, QString nom, QString mat, QString type, double longeur,
               double puissance, QString proprio, QString statut, double capacity){
    this->idNavire = id;
    this->nomNavire = nom;
    this->matricule = mat;
    this->typeNavire = type;
    this->longueur = longeur;
    this->puissanceMoteur = puissance;
    this->proprietaire = proprio;
    this->statutNavire = statut;
    this->maxCargoCapacity = capacity;
}

static QString normalizeOrderBy(const QString& orderBy)
{
    const QString ob = orderBy.trimmed().toUpper();

    static const QStringList allowed = {
        "IDNAVIRE", "NOMNAVIRE", "MATRICULE", "TYPENAVIRE",
        "LONGUEUR", "PUISSANCEMOTEUR", "PROPRIETAIRE", "STATUTNAVIRE", "MAX_CARGO_CAPACITY"
    };
    return allowed.contains(ob) ? ob : QStringLiteral("IDNAVIRE");
}

static QString normalizeOrderDir(const QString& orderDir)
{
    const QString od = orderDir.trimmed().toUpper();
    return (od == "DESC") ? QStringLiteral("DESC") : QStringLiteral("ASC");
}

bool Navire::ajouter(){
    Connection& conn = Connection::createInstance();

    // ── Ensure NAVIRE_SEQ exists ──────────────────────────────────
    {
        QSqlQuery check(conn.db);
        check.prepare("SELECT COUNT(*) FROM USER_SEQUENCES WHERE SEQUENCE_NAME = 'NAVIRE_SEQ'");
        if (check.exec() && check.next() && check.value(0).toInt() == 0) {
            // Sequence missing: create it aligned to current MAX(IDNAVIRE)
            QSqlQuery maxQ(conn.db);
            int maxId = 0;
            if (maxQ.exec("SELECT NVL(MAX(IDNAVIRE),0) FROM NAVIRES") && maxQ.next())
                maxId = maxQ.value(0).toInt();

            QSqlQuery createSeq(conn.db);
            const QString seqSql = QString(
                "CREATE SEQUENCE NAVIRE_SEQ "
                "START WITH %1 INCREMENT BY 1 NOCACHE NOCYCLE"
            ).arg(maxId + 1);
            if (!createSeq.exec(seqSql)) {
                m_lastError = "Cannot create NAVIRE_SEQ: " + createSeq.lastError().text();
                qDebug() << "Navire::ajouter -" << m_lastError;
                return false;
            }
            qDebug() << "Navire::ajouter - NAVIRE_SEQ created, START WITH" << (maxId + 1);
        }
    }

    // ── INSERT using sequence ─────────────────────────────────────
    QSqlQuery query(conn.db);
    query.prepare(
        "INSERT INTO NAVIRES (IDNAVIRE, NOMNAVIRE, MATRICULE, TYPENAVIRE, "
        "LONGUEUR, PUISSANCEMOTEUR, PROPRIETAIRE, STATUTNAVIRE, MAX_CARGO_CAPACITY) "
        "VALUES (NAVIRE_SEQ.NEXTVAL, :nom, :matricule, :type, :longueur, :puissance, "
        ":proprietaire, :statut, :capacity)"
    );
    query.bindValue(":nom",          nomNavire);
    query.bindValue(":matricule",    matricule);
    query.bindValue(":type",         typeNavire);
    query.bindValue(":longueur",     longueur);
    query.bindValue(":puissance",    puissanceMoteur);
    query.bindValue(":proprietaire", proprietaire);
    query.bindValue(":statut",       statutNavire);
    query.bindValue(":capacity",     maxCargoCapacity);

    const bool ok = query.exec();
    if (!ok) {
        m_lastError = query.lastError().text();
        qDebug() << "Navire::ajouter INSERT failed:" << m_lastError;
    } else {
        m_lastError.clear();
        // Read back the auto-generated ID from the sequence
        QSqlQuery seqQ(conn.db);
        if (seqQ.exec("SELECT NAVIRE_SEQ.CURRVAL FROM DUAL") && seqQ.next())
            idNavire = seqQ.value(0).toInt();
        qDebug() << "Navire::ajouter OK - new IDNAVIRE =" << idNavire;
    }
    return ok;
}

bool Navire::supprimer(){
    return Navire::supprimer(idNavire);
}

bool Navire::modifier(){
    Connection& conn = Connection::createInstance();
    QSqlQuery query(conn.db);

    query.prepare("UPDATE NAVIRES SET NOMNAVIRE = :nom, MATRICULE = :matricule, "
                  "TYPENAVIRE = :type, LONGUEUR = :longueur, PUISSANCEMOTEUR = :puissance, "
                  "PROPRIETAIRE = :proprietaire, STATUTNAVIRE = :statut, "
                  "MAX_CARGO_CAPACITY = :capacity "
                  "WHERE IDNAVIRE = :id");

    query.bindValue(":id", idNavire);
    query.bindValue(":nom", nomNavire);
    query.bindValue(":matricule", matricule);
    query.bindValue(":type", typeNavire);
    query.bindValue(":longueur", longueur);
    query.bindValue(":puissance", puissanceMoteur);
    query.bindValue(":proprietaire", proprietaire);
    query.bindValue(":statut", statutNavire);

    const bool ok = query.exec();
    if (!ok) {
        m_lastError = query.lastError().text();
        qDebug() << "Navire::modifier failed:" << m_lastError;
    } else {
        m_lastError.clear();
    }
    return ok;
}

bool Navire::charger(int id)
{
    Connection& conn = Connection::createInstance();
    QSqlQuery query(conn.db);
    query.prepare("SELECT IDNAVIRE, NOMNAVIRE, MATRICULE, TYPENAVIRE, LONGUEUR, "
                  "PUISSANCEMOTEUR, PROPRIETAIRE, STATUTNAVIRE, MAX_CARGO_CAPACITY "
                  "FROM NAVIRES WHERE IDNAVIRE = :id");
    query.bindValue(":id", id);
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Navire::charger failed:" << m_lastError;
        return false;
    }
    if (!query.next()) {
        m_lastError = QStringLiteral("Navire introuvable.");
        return false;
    }

    idNavire = query.value(0).toInt();
    nomNavire = query.value(1).toString();
    matricule = query.value(2).toString();
    typeNavire = query.value(3).toString();
    longueur = query.value(4).toDouble();
    puissanceMoteur = query.value(5).toDouble();
    proprietaire = query.value(6).toString();
    statutNavire = query.value(7).toString();
    maxCargoCapacity = query.value(8).toDouble();
    m_lastError.clear();
    return true;
}

QSqlQueryModel* Navire::getNavire(){
    return afficher("IDNAVIRE", "ASC");
}

QSqlQueryModel* Navire::afficher(const QString& orderBy, const QString& orderDir)
{
    Connection& conn = Connection::createInstance();
    QSqlQueryModel* model = new QSqlQueryModel();

    QSqlQuery query(conn.db);
    const QString ob = normalizeOrderBy(orderBy);
    const QString od = normalizeOrderDir(orderDir);
    query.prepare(QStringLiteral("SELECT IDNAVIRE, NOMNAVIRE, MATRICULE, TYPENAVIRE, LONGUEUR, "
                                 "PUISSANCEMOTEUR, PROPRIETAIRE, STATUTNAVIRE, MAX_CARGO_CAPACITY "
                                 "FROM NAVIRES ORDER BY %1 %2").arg(ob, od));
    if (!query.exec()) {
        qDebug() << "Navire::afficher failed:" << query.lastError().text();
    }

    model->setQuery(query);

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Matricule"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Type"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Longueur (m)"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Puissance (CV)"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Propriétaire"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Statut"));
    model->setHeaderData(8, Qt::Horizontal, QObject::tr("Capacité Cargo (T)"));

    return model;
}

QSqlQueryModel* Navire::rechercher(const QString& critere, const QString& orderBy, const QString& orderDir){
    Connection& conn = Connection::createInstance();
    QSqlQueryModel* model = new QSqlQueryModel();

    QSqlQuery query(conn.db);
    const QString ob = normalizeOrderBy(orderBy);
    const QString od = normalizeOrderDir(orderDir);
    query.prepare(QStringLiteral(
        "SELECT IDNAVIRE, NOMNAVIRE, MATRICULE, TYPENAVIRE, LONGUEUR, "
        "PUISSANCEMOTEUR, PROPRIETAIRE, STATUTNAVIRE, MAX_CARGO_CAPACITY "
        "FROM NAVIRES "
        "WHERE NOMNAVIRE LIKE :critere "
        "   OR MATRICULE LIKE :critere "
        "   OR PROPRIETAIRE LIKE :critere "
        "   OR TYPENAVIRE LIKE :critere "
        "   OR STATUTNAVIRE LIKE :critere "
        "ORDER BY %1 %2").arg(ob, od));

    const QString searchCritere = "%" + critere + "%";
    query.bindValue(":critere", searchCritere);
    if (!query.exec()) {
        qDebug() << "Navire::rechercher failed:" << query.lastError().text();
    }

    model->setQuery(query);

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Matricule"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Type"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Longueur (m)"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Puissance (CV)"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Propriétaire"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Statut"));
    model->setHeaderData(8, Qt::Horizontal, QObject::tr("Capacité Cargo (T)"));

    return model;
}

bool Navire::existe(int id)
{
    Connection& conn = Connection::createInstance();
    QSqlQuery query(conn.db);
    query.prepare("SELECT 1 FROM NAVIRES WHERE IDNAVIRE = :id");
    query.bindValue(":id", id);
    if (!query.exec()) {
        qDebug() << "Navire::existe failed:" << query.lastError().text();
        return false;
    }
    return query.next();
}

bool Navire::supprimer(int id)
{
    Connection& conn = Connection::createInstance();
    QSqlQuery query(conn.db);

    query.prepare("DELETE FROM NAVIRES WHERE IDNAVIRE = :id");
    query.bindValue(":id", id);
    const bool ok = query.exec();
    if (!ok) {
        qDebug() << "Navire::supprimer failed:" << query.lastError().text();
    }
    return ok;
}
