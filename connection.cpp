#include "connection.h"

Connection::Connection(){
    db = QSqlDatabase::addDatabase("QODBC");
}



Connection::~Connection(){
    if(db.isOpen()) db.close();
}

Connection& Connection::createInstance(){
    static Connection instance;
    return instance;
}


bool Connection::createConnection(){
    QString host = QString::fromLocal8Bit(qgetenv("DB_HOST"));
    if (host.isEmpty()) host = "localhost";
    
    QString port = QString::fromLocal8Bit(qgetenv("DB_PORT"));
    if (port.isEmpty()) port = "1521";
    
    QString sid = QString::fromLocal8Bit(qgetenv("DB_SID"));
    if (sid.isEmpty()) sid = "XE";
    
    QString uid = QString::fromLocal8Bit(qgetenv("DB_USER"));
    if (uid.isEmpty()) uid = "admin";
    
    QString pwd = QString::fromLocal8Bit(qgetenv("DB_PASSWORD"));
    if (pwd.isEmpty()) pwd = "Admin0000";

    QString connectionString =
        "DRIVER={Oracle ODBC Driver};"
        "HOST=" + host + ";"
        "PORT=" + port + ";"
        "SID=" + sid + ";"
        "UID=" + uid + ";"
        "PWD=" + pwd + ";";

    db.setDatabaseName(connectionString);




    if(db.open()){
        qDebug() << "Connection Created";
        return true;
    }

    qDebug() << "Error: " << db.lastError().text();
    return false;

}
