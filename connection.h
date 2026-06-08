#ifndef CONNECTION_H
#define CONNECTION_H
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

class Connection
{
public:
    QSqlDatabase db;
    static Connection& createInstance();
    bool createConnection();
private:

    Connection();
    ~Connection();
    Connection(const Connection&)=delete;


    Connection& operator=(const Connection&)=delete;


};

#endif // CONNECTION_H
