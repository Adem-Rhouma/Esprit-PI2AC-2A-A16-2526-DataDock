#ifndef EMAILNOTIFIER_H
#define EMAILNOTIFIER_H

#include <QString>
#include <QObject>
#include <QDebug>

class EmailNotifier : public QObject {
    Q_OBJECT
public:
    explicit EmailNotifier(QObject *parent = nullptr) : QObject(parent) {}

    static bool sendAlert(const QString &to, const QString &subject, const QString &body) {
        // Since we are not using a real SMTP server here for simplicity,
        // we simulate the email sending.
        qDebug() << "------------------------------------------";
        qDebug() << "EMAIL ALERT";
        qDebug() << "TO:      " << to;
        qDebug() << "SUBJECT: " << subject;
        qDebug() << "BODY:    " << body;
        qDebug() << "------------------------------------------";
        // Implementation might use QNetworkAccessManager with a Mailgun or simple SMTP via SmtpClient
        return true;
    }
};

#endif // EMAILNOTIFIER_H
