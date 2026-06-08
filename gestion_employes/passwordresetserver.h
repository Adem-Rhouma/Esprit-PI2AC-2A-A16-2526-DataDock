#ifndef PASSWORDRESETSERVER_H
#define PASSWORDRESETSERVER_H

#include <QTcpServer>
#include <QMap>
#include <QString>

class PasswordResetServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit PasswordResetServer(QObject *parent = nullptr);
    bool startServer(quint16 port = 1705);
    QString createToken(const QString &email);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void handleGet(class QTcpSocket *socket, const QString &path);
    void handlePost(class QTcpSocket *socket, const QString &body);
    void sendResponse(class QTcpSocket *socket, const QString &content, const QString &contentType = "text/html; charset=utf-8");

    QMap<QString, QString> tokens; // token -> email
};

#endif // PASSWORDRESETSERVER_H
