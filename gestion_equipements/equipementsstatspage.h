#ifndef EQUIPEMENTSSTATSPAGE_H
#define EQUIPEMENTSSTATSPAGE_H

#include <QWidget>

#include "materiel.h"

namespace Ui {
class EquipementsStatsPage;
}

class EquipementsStatsPage : public QWidget
{
    Q_OBJECT

public:
    explicit EquipementsStatsPage(QWidget *parent = nullptr);
    ~EquipementsStatsPage();

    void refreshData();

private:
    void applyStyling();
    void setupIcons();
    void loadStatistics();
    void setupCharts(const QList<materiel> &materiels);
    void setupTable(const QList<materiel> &materiels);

    Ui::EquipementsStatsPage *ui;
};

#endif // EQUIPEMENTSSTATSPAGE_H
