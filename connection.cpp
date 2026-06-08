#include "connection.h"
#include <QDebug>
#include <QSqlError>
#include <QCoreApplication>

Connection::Connection() {
    db = QSqlDatabase::addDatabase("QODBC");
}

Connection::~Connection() {
    if (db.isOpen())
        db.close();
}

Connection& Connection::createInstance() {
    static Connection instance;
    return instance;
}

bool Connection::createConnection() {

    QString host = QString::fromLocal8Bit(qgetenv("DB_HOST"));
    if (host.isEmpty()) host = "127.0.0.1";

    QString port = QString::fromLocal8Bit(qgetenv("DB_PORT"));
    if (port.isEmpty()) port = "1521";

    QString sid = QString::fromLocal8Bit(qgetenv("DB_SID"));
    if (sid.isEmpty()) sid = "XEPDB1";

    QString uid = QString::fromLocal8Bit(qgetenv("DB_USER"));
    if (uid.isEmpty()) uid = "admin";

    QString pwd = QString::fromLocal8Bit(qgetenv("DB_PASSWORD"));
    if (pwd.isEmpty()) pwd = "Admin0000";

    QString connectionString;

#ifdef Q_OS_WIN
    // Windows typically uses installed Oracle ODBC driver name

    qDebug() << "This is a testing Ground";

    db.setDatabaseName(sid);
    db.setUserName(uid);
    db.setPassword(pwd);
    db.setHostName(host);
    db.setPort(port.toInt());

#else
    // Linux (unixODBC style)
    connectionString =
        "DRIVER={Oracle ODBC Driver};"
        "HOST=" + host + ";"
                 "PORT=" + port + ";"
                 "SID=" + sid + ";"
                "UID=" + uid + ";"
                "PWD=" + pwd + ";";
    db.setDatabaseName(connectionString);
#endif



    if (db.open()) {
        qDebug() << "Connection Created";
        return true;
    }

    qDebug() << "Error:" << db.lastError().text();
    return false;
}
