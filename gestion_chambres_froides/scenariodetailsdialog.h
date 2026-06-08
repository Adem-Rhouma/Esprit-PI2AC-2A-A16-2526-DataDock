#ifndef SCENARIODETAILSDIALOG_H
#define SCENARIODETAILSDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>
#include <QVBoxLayout>

// Forward declarations
class QChartView;
class QLineSeries;
class QPieSeries;
class QChart;
class QSlider;
class QLabel;

class ScenarioDetailsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScenarioDetailsDialog(const QJsonObject &solution, int scenarioIndex, QWidget *parent = nullptr);
    ~ScenarioDetailsDialog();

private slots:
    void updatePrediction(int day);

private:
    void setupUI();
    void populateData(const QJsonObject &solution, int index);
    void createFreshnessChart(const QJsonArray &curve, QVBoxLayout *layout);
    void createAllocationChart(const QJsonArray &allocs, QVBoxLayout *layout);
    QWidget* createMetricCard(const QString &title, const QString &value, const QString &prefix, const QString &color);
    QWidget* createAdviceCard(const QString &title, const QString &content, const QString &icon, const QString &color);

    QVBoxLayout *m_chartsLayout;
    QVBoxLayout *m_allocLayout;
    QVBoxLayout *m_adviceLayout;
    QSlider *m_daySlider;
    QLabel *m_predictionValue;
    QLabel *m_predictionVerdict;
    QLabel* m_stateIcon;
    QJsonArray m_freshnessCurve;
    QChartView* m_freshnessView;
    QChart* m_freshnessChart;
    QLineSeries* m_markerLine;
    double m_estimatedShelfLife;
};

#endif // SCENARIODETAILSDIALOG_H
