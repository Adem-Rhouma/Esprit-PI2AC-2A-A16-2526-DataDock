#include "passwordresetserver.h"
#include <QTcpSocket>
#include <QUrlQuery>
#include <QDebug>
#include <QUuid>
#include "employe.h"

PasswordResetServer::PasswordResetServer(QObject *parent) : QTcpServer(parent) {}

bool PasswordResetServer::startServer(quint16 port) {
    if (!this->listen(QHostAddress::Any, port)) {
        qDebug() << "Server could not start!";
        return false;
    }
    qDebug() << "Server started on port" << port;
    return true;
}

QString PasswordResetServer::createToken(const QString &email) {
    QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    tokens.insert(token, email);
    return token;
}

void PasswordResetServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    connect(socket, &QTcpSocket::readyRead, this, &PasswordResetServer::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &PasswordResetServer::onDisconnected);
}

void PasswordResetServer::onReadyRead() {
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) return;

    QString data = socket->readAll();
    QStringList lines = data.split("\r\n");
    if (lines.isEmpty()) return;

    QString firstLine = lines.first();
    QStringList parts = firstLine.split(" ");
    if (parts.size() < 2) return;

    QString method = parts[0];
    QString path = parts[1];

    if (method == "GET") {
        handleGet(socket, path);
    } else if (method == "POST") {
        int bodyIndex = data.indexOf("\r\n\r\n");
        if (bodyIndex != -1) {
            QString body = data.mid(bodyIndex + 4);
            handlePost(socket, body);
        }
    }
}

void PasswordResetServer::handleGet(QTcpSocket *socket, const QString &path) {
    QUrl url(path);
    QUrlQuery query(url.query());
    QString token = query.queryItemValue("token");

    if (!tokens.contains(token)) {
        sendResponse(socket, "<h1>Lien invalide ou expire</h1><p>Veuillez faire une nouvelle demande de mot de passe oublie.</p>");
        return;
    }

    QString html = 
        "<!DOCTYPE html>"
        "<html lang='fr'>"
        "<head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<title>Réinitialiser le mot de passe</title>"
        "<style>"
        "body { margin: 0; padding: 0; font-family: 'Segoe UI', Arial, sans-serif; background-color: #2f6bff; display: flex; align-items: center; justify-content: center; min-height: 100vh; }"
        ".card { background: white; padding: 40px; border-radius: 24px; box-shadow: 0 10px 25px rgba(0,0,0,0.2); width: 100%; max-width: 450px; text-align: center; border: 1px solid rgba(0,0,0,0.05); }"
        ".icon-container { background: linear-gradient(135deg, #2f6bff, #174bbd); width: 70px; height: 70px; border-radius: 50%; display: flex; align-items: center; justify-content: center; margin: 0 auto 24px auto; color: #ffcc00; font-size: 30px; }"
        "h1 { color: #2f6bff; margin: 0 0 8px 0; font-size: 28px; font-weight: 700; }"
        "p.subtitle { color: #64748b; margin-bottom: 32px; font-size: 14px; }"
        ".input-group { text-align: left; margin-bottom: 24px; }"
        "label { display: block; margin-bottom: 8px; font-size: 14px; font-weight: 600; color: #1e293b; }"
        "input { width: 100%; padding: 12px 16px; border: 1px solid #e2e8f0; border-radius: 12px; font-size: 14px; box-sizing: border-box; transition: border-color 0.2s; }"
        "input:focus { outline: none; border-color: #2f6bff; }"
        ".rules-box { background: #f8fafc; border-radius: 12px; padding: 16px; text-align: left; margin-bottom: 32px; border: 1px solid #f1f5f9; }"
        ".rules-box h3 { margin: 0 0 12px 0; font-size: 14px; color: #1e293b; font-weight: 700; }"
        ".rules-box ul { margin: 0; padding: 0 0 0 20px; color: #64748b; font-size: 13px; }"
        ".rules-box li { margin-bottom: 6px; }"
        ".btn-reset { background: linear-gradient(90deg, #2f6bff, #174bbd); color: white; border: none; padding: 14px; border-radius: 12px; cursor: pointer; font-size: 16px; font-weight: 700; width: 100%; transition: opacity 0.2s; display: flex; align-items: center; justify-content: center; gap: 8px; }"
        ".btn-reset:hover { opacity: 0.9; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class='card'>"
        "<div class='icon-container'>🔑</div>"
        "<h1>Reset Password</h1>"
        "<p class='subtitle'>Choose a strong new password for your account.</p>"
        "<form action='/update' method='POST' onsubmit='return validateForm()'>"
        "<input type='hidden' name='token' value='" + token + "'>"
        "<div class='input-group'>"
        "<label for='new-password'>New Password</label>"
        "<input type='password' id='new-password' name='password' placeholder='Enter new password' required>"
        "</div>"
        "<div class='input-group'>"
        "<label for='confirm-password'>Confirm Password</label>"
        "<input type='password' id='confirm-password' placeholder='Re-enter new password' required>"
        "</div>"
        "<div class='rules-box'>"
        "<h3>Password must contain:</h3>"
        "<ul>"
        "<li>At least 8 characters</li>"
        "<li>One uppercase letter</li>"
        "<li>One lowercase letter</li>"
        "<li>One number</li>"
        "<li>One special character</li>"
        "</ul>"
        "</div>"
        "<button type='submit' class='btn-reset'><span>✔️</span> Reset Password</button>"
        "</form>"
        "</div>"
        "<script>"
        "function validateForm() {"
        "  var p1 = document.getElementById('new-password').value;"
        "  var p2 = document.getElementById('confirm-password').value;"
        "  if (p1 !== p2) { alert('Les mots de passe ne correspondent pas'); return false; }"
        "  if (p1.length < 8) { alert('Le mot de passe doit faire au moins 8 caracteres'); return false; }"
        "  return true;"
        "}"
        "</script>"
        "</body>"
        "</html>";

    sendResponse(socket, html);
}

void PasswordResetServer::handlePost(QTcpSocket *socket, const QString &body) {
    QUrlQuery query(body);
    QString token = query.queryItemValue("token");
    QString newPassword = query.queryItemValue("password");

    if (tokens.contains(token)) {
        QString email = tokens[token];
        
        // Update database
        Employe e;
        e.set_email(email);
        if (e.employeExiste()) {
            e.set_mdp(newPassword);
            if (e.modifier()) {
                tokens.remove(token);
                QString successHtml = 
                    "<html><head><style>"
                    "body { margin: 0; padding: 0; font-family: 'Segoe UI', Arial, sans-serif; background-color: #2f6bff; display: flex; align-items: center; justify-content: center; min-height: 100vh; }"
                    ".card { background: white; padding: 40px; border-radius: 24px; box-shadow: 0 10px 25px rgba(0,0,0,0.2); width: 100%; max-width: 450px; text-align: center; border: 1px solid rgba(0,0,0,0.05); }"
                    ".icon-success { color: #10b981; font-size: 60px; margin-bottom: 20px; }"
                    "h1 { color: #1e293b; margin: 0 0 16px 0; font-size: 28px; font-weight: 700; }"
                    "p { color: #64748b; line-height: 1.6; margin-bottom: 0; }"
                    "</style></head><body>"
                    "<div class='card'>"
                    "<div class='icon-success'>✅</div>"
                    "<h1>Succès !</h1>"
                    "<p>Votre mot de passe a été mis à jour avec succès. Vous pouvez maintenant fermer cette page et vous connecter sur <strong>DataDock</strong>.</p>"
                    "</div>"
                    "</body></html>";
                sendResponse(socket, successHtml);
            } else {
                sendResponse(socket, "<h1>Erreur</h1><p>Impossible de mettre a jour la base de donnees.</p>");
            }
        } else {
            sendResponse(socket, "<h1>Erreur</h1><p>Employe non trouve.</p>");
        }
    } else {
        sendResponse(socket, "<h1>Session expiree</h1><p>Veuillez recommencer.</p>");
    }
}

void PasswordResetServer::sendResponse(QTcpSocket *socket, const QString &content, const QString &contentType) {
    QByteArray response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType.toUtf8() + "\r\n";
    response += "Content-Length: " + QByteArray::number(content.toUtf8().size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += content.toUtf8();

    socket->write(response);
    socket->flush();
}

void PasswordResetServer::onDisconnected() {
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket) socket->deleteLater();
}
