#ifndef PRICEHISTORYTABLEMODEL_H
#define PRICEHISTORYTABLEMODEL_H

#include "../models/dynamicpricingmodels.h"

#include <QAbstractTableModel>
#include <QVector>

class PriceHistoryTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column {
        ColDateTime = 0,
        ColPort,
        ColBasePrice,
        ColRecommendedPrice,
        ColDelta,
        ColWeather,
        ColRisk,
        ColFreshness,
        ColSource,
        ColExplanation,
        ColAction,
        ColumnCount
    };

    explicit PriceHistoryTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void setEntries(const QVector<DpHistoryEntry> &entries);
    void clearEntries();
    const DpHistoryEntry *entryAt(int row) const;

private:
    QVector<DpHistoryEntry> m_entries;
};

#endif // PRICEHISTORYTABLEMODEL_H
