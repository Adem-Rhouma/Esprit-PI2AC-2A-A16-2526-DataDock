#ifndef LOGISTIQUESTATSPAGE_H
#define LOGISTIQUESTATSPAGE_H

#include <QWidget>

namespace Ui {
class LogistiqueStatsPage;
}

class LogistiqueStatsPage : public QWidget
{
    Q_OBJECT

public:
    explicit LogistiqueStatsPage(QWidget *parent = nullptr);
    ~LogistiqueStatsPage();

private:
    void applyStyling();
    void setupIcons();
    void setupCharts();
    void setupTable();
    void setupPredictionsChart();
    void updatePredictions();

public slots:
    void refreshData();

private:
    Ui::LogistiqueStatsPage *ui;
};

#endif // LOGISTIQUESTATSPAGE_H
