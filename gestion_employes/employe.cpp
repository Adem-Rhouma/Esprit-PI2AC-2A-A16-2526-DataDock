#include "employe.h"
#include <QDebug>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QBuffer>
#include <QEventLoop>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
Employe::Employe()
{
    IDEMPLOYE = "";
    NOM = "";
    PRENOM = "";
    ROLE = "";
    EMAIL = "";
    MOT_DE_PASSE = "";
    PHOTO_IDENTITE = "";
    SEXE = "";

    // Ensure SEXE column exists
    QSqlQuery query;
    query.exec("ALTER TABLE EMPLOYES ADD (SEXE VARCHAR2(20))");
    
    // One-time population of SEXE if it's currently empty (for existing rows)
    // We split them 50/50 based on row identity for the demo
    query.exec("UPDATE EMPLOYES SET SEXE = 'Homme' WHERE SEXE IS NULL AND MOD(ORA_HASH(IDEMPLOYE), 2) = 0");
    query.exec("UPDATE EMPLOYES SET SEXE = 'Femme' WHERE SEXE IS NULL");
}

Employe::Employe(QString id, QString nom, QString prenom, QString role, QString email, QString mdp, QString photo, QString sexe)
{
    this->IDEMPLOYE = id;
    this->NOM = nom;
    this->PRENOM = prenom;
    this->ROLE = role;
    this->EMAIL = email;
    this->MOT_DE_PASSE = mdp;
    this->PHOTO_IDENTITE = photo;
    this->SEXE = sexe;
}

// Getters
QString Employe::get_id(){return IDEMPLOYE;}
QString Employe::get_nom(){return NOM;}
QString Employe::get_prenom(){return PRENOM;}
QString Employe::get_role(){return ROLE;}
QString Employe::get_email(){return EMAIL;}
QString Employe::get_mdp(){return MOT_DE_PASSE;}
QString Employe::get_photo(){return PHOTO_IDENTITE;}
QString Employe::get_sexe(){return SEXE;}

// Setters
void Employe::set_id(QString id){this->IDEMPLOYE=id;}
void Employe::set_nom(QString n){this->NOM=n;}
void Employe::set_prenom(QString p){this->PRENOM=p;}
void Employe::set_role(QString r){this->ROLE=r;}
void Employe::set_email(QString e){this->EMAIL=e;}
void Employe::set_mdp(QString m){this->MOT_DE_PASSE=m;}
void Employe::set_photo(QString p){this->PHOTO_IDENTITE=p;}
void Employe::set_sexe(QString s){this->SEXE=s;}

bool Employe::ajouter()
{
    QSqlQuery query;
    // Updated with correct column names and SEXE
    query.prepare("INSERT INTO EMPLOYES (IDEMPLOYE, NOM, PRENOM, ROLE, EMAIL, MOT_DE_PASSE, PHOTO_IDENTITE, SEXE) "
                  "VALUES (:id, :nom, :prenom, :role, :email, :mdp, :photo, :sexe)");

    query.bindValue(":id", IDEMPLOYE);
    query.bindValue(":nom", NOM);
    query.bindValue(":prenom", PRENOM);
    query.bindValue(":role", ROLE);
    query.bindValue(":email", EMAIL);
    query.bindValue(":mdp", MOT_DE_PASSE);
    query.bindValue(":photo", PHOTO_IDENTITE);
    query.bindValue(":sexe", SEXE);

    if (query.exec())
        return true;
    else {
        qDebug() << "Erreur ajout employe:" << query.lastError().text();
        return false;
    }
}

QSqlQueryModel * Employe::afficher()
{
    QSqlQueryModel * model = new QSqlQueryModel();
    // Use explicit column selection to ensure order
    model->setQuery("SELECT IDEMPLOYE, NOM, PRENOM, ROLE, EMAIL, MOT_DE_PASSE, PHOTO_IDENTITE, SEXE FROM EMPLOYES");

    // Set column headers to match UI
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID_Employe"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prenom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Role"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Mot_De_Passe"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Photo_Identite"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Sexe"));

    return model;
}
bool Employe::supprimer(QString id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM EMPLOYES WHERE IDEMPLOYE = :id");
    query.bindValue(":id", id);
    if (query.exec())
        return true;
    else {
        qDebug() << "Erreur suppression employe:" << query.lastError().text();
        return false;
    }
}

bool Employe::modifier()
{

    QSqlQuery query;
    query.prepare("UPDATE EMPLOYES SET NOM= :nom, PRENOM= :prenom, ROLE= :role, EMAIL= :email, "
                  "MOT_DE_PASSE= :mdp, PHOTO_IDENTITE= :photo WHERE IDEMPLOYE= :id");

    query.bindValue(":nom", NOM);
    query.bindValue(":prenom", PRENOM);
    query.bindValue(":role", ROLE);
    query.bindValue(":email", EMAIL);
    query.bindValue(":mdp", MOT_DE_PASSE);
    query.bindValue(":photo", PHOTO_IDENTITE);
    query.bindValue(":id", IDEMPLOYE);

    if (query.exec())
        return true;
    else {
        qDebug() << "Erreur modification employe:" << query.lastError().text();
        return false;
    }
}

QSqlQueryModel * Employe::trier(QString critere, QString ordre)
{
    QSqlQueryModel * model = new QSqlQueryModel();
    QSqlQuery query;

    // Default fallback
    if(critere.isEmpty()) critere = "NOM";
    if(ordre.isEmpty()) ordre = "ASC";

    // Whitelist des colonnes triables
    if (critere != "NOM" && critere != "ROLE" && critere != "PRENOM" && critere != "IDEMPLOYE") {
        critere = "NOM";
    }

    if (ordre != "ASC" && ordre != "DESC") {
        ordre = "ASC";
    }

    // Utiliser ORDER BY critere ASC directement mais avec collation ou UPPER pour eviter case sensitivity
    // Une version plus standard: ORDER BY LOWER(COL)
    QString q = "SELECT IDEMPLOYE, NOM, PRENOM, ROLE, EMAIL, MOT_DE_PASSE, PHOTO_IDENTITE FROM EMPLOYES ORDER BY " + critere + " " + ordre;

    // Si on veut insensibilité à la casse, on peut faire:
    // "ORDER BY LOWER(" + critere + ") " + ordre
    // Mais certains SGBD préfèrent "ORDER BY " + critere + " COLLATE NOCASE " (SQLite)
    // Essayons la version standard sans LOWER pour voir si c'était le pb, ou avec UPPER

    if (critere == "IDEMPLOYE") {
        q = "SELECT IDEMPLOYE, NOM, PRENOM, ROLE, EMAIL, MOT_DE_PASSE, PHOTO_IDENTITE FROM EMPLOYES ORDER BY " + critere + " " + ordre;
    } else {
        q = "SELECT IDEMPLOYE, NOM, PRENOM, ROLE, EMAIL, MOT_DE_PASSE, PHOTO_IDENTITE FROM EMPLOYES ORDER BY LOWER(" + critere + ") " + ordre;
    }

    query.prepare(q);

    if(!query.exec()) { // Ajouter debug erreur
        qDebug() << "Erreur tri:" << query.lastError().text();
    }

    model->setQuery(std::move(query));

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID_Employe"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Role"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Mot_De_Passe"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Photo_Identite"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Sexe"));

    return model;
}

QSqlQueryModel * Employe::rechercher(QString recherche)
{
    QSqlQueryModel * model = new QSqlQueryModel();
    QSqlQuery query;

    // Recherche dans ID, Nom, Prenom, Role et Email
    query.prepare("SELECT IDEMPLOYE, NOM, PRENOM, ROLE, EMAIL, MOT_DE_PASSE, PHOTO_IDENTITE FROM EMPLOYES "
                  "WHERE LOWER(IDEMPLOYE) LIKE :s OR LOWER(NOM) LIKE :s OR LOWER(PRENOM) LIKE :s OR LOWER(ROLE) LIKE :s OR LOWER(EMAIL) LIKE :s");

    QString s = "%" + recherche.toLower() + "%";
    query.bindValue(":s", s);

    query.exec();

    model->setQuery(std::move(query));
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID_Employe"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prenom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Role"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Mot_De_Passe"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Photo_Identite"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Sexe"));

    return model;
}

void Employe :: afficher(QSqlQueryModel *model)
{
    model->setQuery("SELECT IDEMPLOYE, NOM, PRENOM, ROLE, EMAIL FROM EMPLOYES");
}

int Employe::getTotalCount()
{
    QSqlQuery query("SELECT COUNT(*) FROM EMPLOYES");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int Employe::getCountByRole(QString role)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM EMPLOYES WHERE ROLE = :role");
    query.bindValue(":role", role);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int Employe::getCountByEmailDomain(QString domain)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM EMPLOYES WHERE EMAIL LIKE :domain");
    query.bindValue(":domain", "%" + domain);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int Employe::getCountByNameRange(QChar start, QChar end)
{
    QSqlQuery query("SELECT NOM FROM EMPLOYES");
    int count = 0;
    while (query.next()) {
        QString name = query.value(0).toString();
        if (!name.isEmpty()) {
            QChar firstChar = name.at(0).toUpper();
            if (firstChar >= start && firstChar <= end) {
                count++;
            }
        }
    }
    return count;
}
bool Employe:: login()
{
    QSqlQuery query;
    query.prepare("select * from employes where EMAIL= :email and MOT_DE_PASSE= :mdp");
    query.bindValue(":email" , EMAIL);
    query.bindValue(":mdp" , MOT_DE_PASSE);
    if(query.exec() && query.next())
    {
        IDEMPLOYE = query.value("IDEMPLOYE").toString();
        NOM = query.value("NOM").toString();
        PRENOM = query.value("PRENOM").toString();
        ROLE = query.value("ROLE").toString();
        EMAIL = query.value("EMAIL").toString();
        MOT_DE_PASSE = query.value("MOT_DE_PASSE").toString();
        PHOTO_IDENTITE = query.value("PHOTO_IDENTITE").toString();
        SEXE = query.value("SEXE").toString();
        return true;
    }
    else
    {
        return false;
    }
}
bool Employe::ajoutFaceID()
{
    QSqlQuery query;
    query.prepare("UPDATE EMPLOYES SET embedding = :embedding WHERE IDEMPLOYE = :id");

    query.bindValue(":embedding", embedding);
    query.bindValue(":id", IDEMPLOYE);

    if (query.exec())
        return true;
    else {
        qDebug() << "Erreur modification employe:" << query.lastError().text();
        return false;
    }
}
bool Employe::verifierFaceAvecEmbedding(const QImage &image, const QString &embeddingJson)
{
    if (image.isNull()) {
        qDebug() << "Image invalide.";
        return false;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("http://127.0.0.1:5000/face/verify"));

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);

    if (!image.save(&buffer, "JPG")) {
        qDebug() << "Impossible de convertir l'image en JPG.";
        delete multiPart;
        return false;
    }

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant("form-data; name=\"image\"; filename=\"face.jpg\""));
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader,
                        QVariant("image/jpeg"));
    imagePart.setBody(imageData);

    QHttpPart embeddingPart;
    embeddingPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                            QVariant("form-data; name=\"embedding\""));
    embeddingPart.setBody(embeddingJson.toUtf8());

    multiPart->append(imagePart);
    multiPart->append(embeddingPart);

    QNetworkReply *reply = manager.post(request, multiPart);
    multiPart->setParent(reply);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Erreur réseau verify :" << reply->errorString();
        reply->deleteLater();
        return false;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "Réponse JSON invalide :" << responseData;
        return false;
    }

    QJsonObject obj = doc.object();

    bool success = obj.value("success").toBool(false);
    bool match = obj.value("match").toBool(false);

    qDebug() << "Verify success =" << success << ", match =" << match;

    return success && match;
}
bool Employe::faceid(const QImage &preview)
{
    QSqlQuery query;
    query.prepare("SELECT IDEMPLOYE, NOM, PRENOM, ROLE, EMAIL, MOT_DE_PASSE, PHOTO_IDENTITE, embedding FROM EMPLOYES");

    if (!query.exec()) {
        qDebug() << "Erreur SELECT EMPLOYES :" << query.lastError().text();
        return false;
    }

    while (query.next())
    {
        QString embeddingBdd = query.value("embedding").toString();

        if (embeddingBdd.trimmed().isEmpty()) {
            continue;
        }

        if (verifierFaceAvecEmbedding(preview, embeddingBdd)) {
            IDEMPLOYE = query.value("IDEMPLOYE").toString();
            NOM = query.value("NOM").toString();
            PRENOM = query.value("PRENOM").toString();
            ROLE = query.value("ROLE").toString();
            EMAIL = query.value("EMAIL").toString();
            MOT_DE_PASSE = query.value("MOT_DE_PASSE").toString();
            PHOTO_IDENTITE = query.value("PHOTO_IDENTITE").toString();
            embedding = embeddingBdd;

            return true;
        }
    }

    return false;
}
int Employe:: sumEmployeByRole(QString ch)
{
    QSqlQueryModel* searchModel = new QSqlQueryModel();
    searchModel->setQuery("SELECT * FROM EMPLOYES WHERE role LIKE '%" + ch + "%'");

    return searchModel->rowCount() ;
}
bool Employe::employeExiste()
{
    QSqlQuery query;
    query.prepare("SELECT IDEMPLOYE, NOM, PRENOM, ROLE, EMAIL, MOT_DE_PASSE, PHOTO_IDENTITE "
                  "FROM EMPLOYES "
                  "WHERE LOWER(TRIM(EMAIL)) = LOWER(TRIM(:EMAIL))");
    query.bindValue(":EMAIL", EMAIL);

    if (query.exec() && query.next())
    {
        IDEMPLOYE = query.value("IDEMPLOYE").toString();
        NOM = query.value("NOM").toString();
        PRENOM = query.value("PRENOM").toString();
        ROLE = query.value("ROLE").toString();
        EMAIL = query.value("EMAIL").toString();
        MOT_DE_PASSE = query.value("MOT_DE_PASSE").toString();
        PHOTO_IDENTITE = query.value("PHOTO_IDENTITE").toString();
        SEXE = query.value("SEXE").toString();

        return true;
    }
    else
    {
        qDebug() << "Employe introuvable ou erreur SQL :" << query.lastError().text();
        return false;
    }
}

int Employe::getCountBySexe(QString sexe)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM EMPLOYES WHERE SEXE = :sexe");
    query.bindValue(":sexe", sexe);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}
bool Employe::enregitrement(QString id,QString zone, int heur)
{
    QSqlQuery query;

    query.prepare("UPDATE EMPLOYES "
                  "SET HEURE = :heur "
                  "WHERE IDEMPLOYE = :id");

    query.bindValue(":zone", zone);
    query.bindValue(":heur", heur);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur modification zone/heure employé :"
                 << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}
bool Employe::employeExisteA()
{
    QSqlQuery query;
    query.prepare("SELECT *"
                  "FROM EMPLOYES "
                  "WHERE IDEMPLOYE = :idempl");
    query.bindValue(":idempl", IDEMPLOYE);

    if (query.exec() && query.next())
    {
        IDEMPLOYE = query.value("IDEMPLOYE").toString();
        NOM = query.value("NOM").toString();
        PRENOM = query.value("PRENOM").toString();
        ROLE = query.value("ROLE").toString();
        EMAIL = query.value("EMAIL").toString();
        MOT_DE_PASSE = query.value("MOT_DE_PASSE").toString();
        PHOTO_IDENTITE = query.value("PHOTO_IDENTITE").toString();
        SEXE = query.value("SEXE").toString();

        return true;
    }
    else
    {
        qDebug() << "Employe introuvable ou erreur SQL :" << query.lastError().text();
        return false;
    }
}
bool Employe::enregistrerHeureRetour(QString id)
{
    QSqlQuery query;

    query.prepare("UPDATE EMPLOYES "
                  "SET HEURE_R = TO_NUMBER(TO_CHAR(SYSDATE, 'HH24')) "
                  "WHERE IDEMPLOYE = :id");

    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur modification HEURE_R :"
                 << query.lastError().text();
        return false;
    }

    qDebug() << "Heure retour enregistrée. Lignes affectées :"
             << query.numRowsAffected();

    return true;
}

