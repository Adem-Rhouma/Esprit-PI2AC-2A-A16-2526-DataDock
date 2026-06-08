#ifndef ALERTSERVICE_H
#define ALERTSERVICE_H

#include <QObject>
#include <QTimer>
#include <QList>
#include "maintenanceengine.h"
#include "emailnotifier.h"
#include "meteo_data.h"

class AlertService : public QObject {
    Q_OBJECT
public:
    explicit AlertService(QObject *parent = nullptr);
    void checkAlerts(const MeteoData::MeteoReport &weather);

signals:
    void alertTriggered(const QString &title, const QString &msg);

private:
    QTimer *checkTimer;
};

#endif // ALERTSERVICE_H
