#ifndef EMPLOYESSTATSPAGE_H
#define EMPLOYESSTATSPAGE_H

#include <QWidget>
#include <QPair>
#include <QStringList>
#include <QVector>
class QShowEvent;

namespace Ui {
class EmployesStatsPage;
}

class EmployesStatsPage : public QWidget
{
    Q_OBJECT

public:
    explicit EmployesStatsPage(QWidget *parent = nullptr);
    ~EmployesStatsPage();

public slots:
    void refreshData();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void applyStyling();
    void setupIcons();
    void setupCharts();
    void setupTable();
    void exportStatisticsPdf();
    void updateKpis();
    void detectAvailableColumns();
    QString resolveColumn(const QStringList &candidates) const;
    QVector<QPair<QString, int>> fetchGroupedCounts(const QString &columnName) const;
    int fetchCountByRole(const QString &role) const;
    int fetchTotalEmployees() const;
    void rebuildRoleLineChart();
    void rebuildFunctionDonutChart();
    void rebuildServiceBarChart();
    void rebuildStatusBarChart();
    void clearChartContainer(QWidget *container);

    Ui::EmployesStatsPage *ui;
    QStringList m_tableColumns;
    QString m_roleColumn;
    QString m_functionColumn;
    QString m_serviceColumn;
    QString m_statusColumn;
    QString m_hireDateColumn;
};

#endif // EMPLOYESSTATSPAGE_H
