#ifndef EMAIL_H
#define EMAIL_H
#include <QString>

class mailer
{
public:
    mailer();
    mailer(QString, QString, QString);

    static int sendEmail(QString,QString,QString,QString);
 private:
    QString destinataire;
    QString object,body;
};

#endif // EMAIL_H
