#ifndef NAVIRESSTATSPAGE_H
#define NAVIRESSTATSPAGE_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QFrame>



namespace Ui {
class NaviresStatsPage;
}

class NaviresStatsPage : public QWidget {
    Q_OBJECT
public:
    explicit NaviresStatsPage(QWidget *parent = nullptr);
    ~NaviresStatsPage();

signals:
    void requestMeteoTab();

private slots:
    void loadStatistics();
    void showContextMenu(const QPoint &pos);
    void openPredictiveAnalysis(int shipId, const QString &shipName);

private:
    Ui::NaviresStatsPage *ui;
    void applyStyling();

    QChart *lineChart;
    QChart *donutChart;
    QChart *barChart;
};

#endif // NAVIRESSTATSPAGE_H
