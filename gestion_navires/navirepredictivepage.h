#ifndef NAVIREPREDICTIVEPAGE_H
#define NAVIREPREDICTIVEPAGE_H

#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

namespace Ui {
class NavirePredictivePage;
}

class NavirePredictivePage : public QWidget
{
    Q_OBJECT

public:
    explicit NavirePredictivePage(int shipId, QWidget *parent = nullptr);
    ~NavirePredictivePage();

private slots:
    void onRegressorFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onClassifierFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onPythonError(QProcess::ProcessError error);

private:
    Ui::NavirePredictivePage *ui;
    int m_shipId;
    QProcess *m_regressorProcess;
    QProcess *m_classifierProcess;

    void runPrediction();
    void updateMaintenanceUI(const QJsonObject &predictions);
    void updateSymptomUI(const QJsonObject &predictions);
    void setupCharts();
};

#endif // NAVIREPREDICTIVEPAGE_H
