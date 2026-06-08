#include "email.h"
#include <QtNetwork>
#include <QDebug>
mailer::mailer()
{

}

int mailer::sendEmail(QString dist, QString obj, QString bdy, QString name)
{

    qDebug()<<"sslLibraryBuildVersionString: "<<QSslSocket::sslLibraryBuildVersionString();
    qDebug()<<"sslLibraryVersionNumber: "<<QSslSocket::sslLibraryVersionNumber();
     qDebug()<<"supportsSsl: "<<QSslSocket::supportsSsl();;
    qDebug()<<QCoreApplication::libraryPaths();
        // SMTP server information
        QString smtpServer = QString::fromLocal8Bit(qgetenv("SMTP_SERVER"));
        if (smtpServer.isEmpty()) smtpServer = "smtp.gmail.com";
        
        QString smtpPortStr = QString::fromLocal8Bit(qgetenv("SMTP_PORT"));
        int smtpPort = smtpPortStr.isEmpty() ? 465 : smtpPortStr.toInt();
        
        QString username = QString::fromLocal8Bit(qgetenv("SMTP_USER"));
        if (username.isEmpty()) username = "example@gmail.com";
        
        QString password = QString::fromLocal8Bit(qgetenv("SMTP_PASSWORD"));
        if (password.isEmpty()) password = "your_password_here";
        
        // Sender and recipient information
        QString from = QString::fromLocal8Bit(qgetenv("SMTP_FROM"));
        if (from.isEmpty()) from = "ITRUST.audit@gmail.com";
        QString to =dist;
        QString subject = obj;
        QString body = "Reset your password: " + bdy;

        QString htmlBody =
            "<html><body style='margin:0;padding:0;background-color:#f4f4f4;font-family:Helvetica,Arial,sans-serif;'>"
            "<table width='100%' cellpadding='0' cellspacing='0' style='padding:50px 0;'>"
            "<tr><td align='center'>"
            "<table width='600' cellpadding='0' cellspacing='0' style='max-width:600px;background:#ffffff;box-shadow:0 4px 6px rgba(0,0,0,0.1);'>"
            "<tr><td style='background:linear-gradient(to right, #2f6bff, #174bbd);padding:40px 20px;text-align:center;'>"
            "<h1 style='color:#ffffff;margin:0;font-size:24px;font-weight:bold;'>🔓 Password Reset Request</h1>"
            "</td></tr>"
            "<tr><td style='padding:40px 35px;color:#333333;font-size:16px;line-height:1.6;'>"
            "<p style='margin:0 0 20px 0;'>Hi <strong>" + name + "</strong>,</p>"
            "<p style='margin:0 0 20px 0;'>We received a request to reset your password for your <strong>DataDock account</strong>.</p>"
            "<p style='margin:0 0 30px 0;'>Click the button below to choose a new password:</p>"
            "<table width='100%' cellpadding='0' cellspacing='0'>"
            "<tr><td align='center' style='padding:10px 0 35px 0;'>"
            "<a href='" + bdy + "' style='background-color:#2f6bff;color:#ffffff;padding:12px 28px;border-radius:4px;font-weight:bold;display:inline-block;text-decoration:none;'>"
            "Reset Password"
            "</a>"
            "</td></tr>"
            "</table>"
            "<p style='margin:0 0 12px 0;font-weight:bold;'>This link will expire in 1 hour.</p>"
            "<p style='margin:0 0 12px 0;color:#555555;'>If you didn't request this password reset, you can safely ignore this email. Your password will not be changed.</p>"
            "<p style='margin:0;color:#555555;'>For security reasons, never share this link with anyone.</p>"
            "</td></tr>"
            "</table>"
            "</td></tr></table>"
            "</body></html>";

        // Create a TCP socket
        QSslSocket socket;

        // Connect to the SMTP server
        socket.connectToHostEncrypted(smtpServer, smtpPort);
        if (!socket.waitForConnected()) {
            qDebug() << "Error connecting to the server:" << socket.errorString();
            return -1;
        }

        // Wait for the greeting from the server
        if (!socket.waitForReadyRead()) {
            qDebug() << "Error reading from server:" << socket.errorString();
            return -1;
        }

        qDebug() << "Connected to the server.";

        // Send the HELO command
        socket.write("HELO localhost\r\n");
        socket.waitForBytesWritten();

        // Read the response from the server
        if (!socket.waitForReadyRead()) {
            qDebug() << "Error reading from server:" << socket.errorString();
            return -1;
        }

        // Send the authentication information
        socket.write("AUTH LOGIN\r\n");
        socket.waitForBytesWritten();

        if (!socket.waitForReadyRead()) {
            qDebug() << "Error reading from server:" << socket.errorString();
            return -1;
        }

        // Send the username
        socket.write(QByteArray().append(username.toUtf8()).toBase64() + "\r\n");
        socket.waitForBytesWritten();

        if (!socket.waitForReadyRead()) {
            qDebug() << "Error reading from server:" << socket.errorString();
            return -1;
        }

        // Send the password
        socket.write(QByteArray().append(password.toUtf8()).toBase64() + "\r\n");
        socket.waitForBytesWritten();

        if (!socket.waitForReadyRead()) {
            qDebug() << "Error reading from server:" << socket.errorString();
            return -1;
        }

        // Send the MAIL FROM command
        socket.write("MAIL FROM:<" + from.toUtf8() + ">\r\n");
        socket.waitForBytesWritten();

        if (!socket.waitForReadyRead()) {
            qDebug() << "Error reading from server:" << socket.errorString();
            return -1;
        }

        // Send the RCPT TO command
        socket.write("RCPT TO:<" + to.toUtf8() + ">\r\n");
        socket.waitForBytesWritten();

        if (!socket.waitForReadyRead()) {
            qDebug() << "Error reading from server:" << socket.errorString();
            return -1;
        }

        // Send the DATA command
        socket.write("DATA\r\n");
        socket.waitForBytesWritten();

        if (!socket.waitForReadyRead()) {
            qDebug() << "Error reading from server:" << socket.errorString();
            return -1;
        }

        // Send the email content
        socket.write("From: " + from.toUtf8() + "\r\n");
        socket.write("To: " + to.toUtf8() + "\r\n");
        socket.write("Subject: " + subject.toUtf8() + "\r\n");
        socket.write("MIME-Version: 1.0\r\n");
        socket.write("Content-Type: text/html; charset=UTF-8\r\n");
        socket.write("Content-Transfer-Encoding: 8bit\r\n");
        socket.write("\r\n");  // Empty line before the body
        socket.write(htmlBody.toUtf8() + "\r\n");
        socket.write(".\r\n");  // End of email content
        socket.waitForBytesWritten();

        if (!socket.waitForReadyRead()) {
            qDebug() << "Error reading from server:" << socket.errorString();
            return -1;
        }

        // Send the QUIT command
        socket.write("QUIT\r\n");
        socket.waitForBytesWritten();

        if (!socket.waitForReadyRead()) {
            qDebug() << "Error reading from server:" << socket.errorString();
            return -1;
        }

        qDebug() << "Email sent successfully.";

        // Close the socket
        socket.close();

        Q_UNUSED(body);
        return 0;
}
