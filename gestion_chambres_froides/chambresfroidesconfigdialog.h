#ifndef CHAMBRESFROIDESCONFIGDIALOG_H
#define CHAMBRESFROIDESCONFIGDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QThread>
#include "../arduino.h"

// Project pattern: Forward declarations in global namespace
class QChartView;
class QLineSeries;
class QValueAxis;
class QChart;
class QLabel;
class QSlider;

class ChambresFroidesConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChambresFroidesConfigDialog(const QString &id, const QString &tagNumber, double temp, double humidity, const QString &state, bool betaTest, QWidget *parent = nullptr);
    ~ChambresFroidesConfigDialog();

    double getTemperature() const;
    double getHumidity() const;

private slots:
    void on_sliderTemp_valueChanged(int value);
    void on_sliderHumidity_valueChanged(int value);
    void updateRealtimeData();
    void on_btnSave_clicked();
    void onSyncTimerTimeout();

private:
    void setupManualUI();
    void setupChart();
    void updateLabels();
    
    QString m_id;
    QString m_tag;   // TAG lisible envoyé à l'Arduino (ex: "CF1" au lieu de "1")
    QString m_state;
    double m_currentTemp;
    double m_currentHumidity;
    
    // UI Elements
    QLabel *ui_lblTempVal;
    QLabel *ui_lblDesiredTempVal;
    QLabel *ui_lblHumidityVal;
    QLabel *ui_lblLoadVal;
    QLabel *ui_lblFrostVal;
    QSlider *ui_sliderTemp;
    QSlider *ui_sliderHumidity;
    
    // Chart Pointers (global namespace forward declared)
    QChartView *ui_chartView;
    QLineSeries *m_tempSeries;
    QLineSeries *m_humiditySeries;
    QLineSeries *m_loadSeries;
    QLineSeries *m_frostSeries;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;

    QTimer *m_dataTimer;
    QTimer *m_syncTimer;
    int m_tickCount;
    bool m_betaTest;
    Arduino m_arduino;
    QString m_serialBuffer;
};

#endif // CHAMBRESFROIDESCONFIGDIALOG_H
