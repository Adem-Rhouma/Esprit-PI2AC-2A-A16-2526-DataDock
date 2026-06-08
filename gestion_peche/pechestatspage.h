#ifndef PecheSTATSPAGE_H
#define PecheSTATSPAGE_H

#include <QWidget>

class QTimer;
class QShowEvent;
class QElapsedTimer;
class QTableWidget;

namespace Ui {
class PecheStatsPage;
}

class QBarCategoryAxis;
class QBarSeries;
class QCategoryAxis;
class QChart;
class QLineSeries;
class QSplineSeries;
class QPieSeries;
class QValueAxis;

class PecheStatsPage : public QWidget
{
    Q_OBJECT

public:
    explicit PecheStatsPage(QWidget *parent = nullptr);
    ~PecheStatsPage();

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void applyStyling();
    void setupIcons();
    void setupCharts();
    void setupTable();
    void populateRecentTable(QTableWidget *table, int rowLimit);
    void refreshAll();
    void refreshKpis();
    void refreshLineChart();
    void refreshDonutChart();
    void refreshBarChart();
    void refreshBottomChart();
    void refreshRecentTable();
    void showFullHistoryDialog();

    void onFilterClicked();
    void onCalendarClicked();
    void onExportClicked();
    void onVoirPlusClicked();

    Ui::PecheStatsPage *ui;
    QTimer *m_refreshTimer = nullptr;
    bool m_refreshing = false;
    QElapsedTimer *m_lastRefresh = nullptr;
    int m_monthsFilter = 1; // Used for filtering KPIs and charts (1 month by default)

    QChart *m_lineChart = nullptr;
    QSplineSeries *m_lineSeries = nullptr;
    QCategoryAxis *m_lineAxisX = nullptr;
    QValueAxis *m_lineAxisY = nullptr;

    QChart *m_donutChart = nullptr;
    QPieSeries *m_donutSeries = nullptr;

    QChart *m_barChart = nullptr;
    QBarSeries *m_barSeries = nullptr;
    QBarCategoryAxis *m_barAxisX = nullptr;
    QValueAxis *m_barAxisY = nullptr;

    QChart *m_bottomChart = nullptr;
    QBarSeries *m_bottomSeries = nullptr;
    QBarCategoryAxis *m_bottomAxisX = nullptr;
    QValueAxis *m_bottomAxisY = nullptr;
};

#endif // PecheSTATSPAGE_H

