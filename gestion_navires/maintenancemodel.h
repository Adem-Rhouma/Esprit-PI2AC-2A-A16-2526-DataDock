#ifndef MAINTENANCEMODEL_H
#define MAINTENANCEMODEL_H

#include <QSqlQueryModel>
#include <QVariant>
#include "NavireConstants.h"

class MaintenanceModel : public QSqlQueryModel {
    Q_OBJECT
public:
    explicit MaintenanceModel(QObject *parent = nullptr) : QSqlQueryModel(parent) {}

    void refresh() {
        setQuery(R"(
            SELECT n.IDNAVIRE, n.NOMNAVIRE, r.RULE_TYPE, r.THRESHOLD_HOURS, r.THRESHOLD_DAYS, r.LAST_MAINTENANCE_DATE
            FROM NAVIRES n
            LEFT JOIN MAINTENANCE_RULES r ON n.IDNAVIRE = r.IDNAVIRE
        )");
    }

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const override {
        // Here we could handle colored backgrounds for status but that might move to a Delegate
        return QSqlQueryModel::data(item, role);
    }
};

#endif // MAINTENANCEMODEL_H
