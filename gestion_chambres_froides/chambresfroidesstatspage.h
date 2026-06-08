#ifndef CHAMBRESFROIDESSTATSPAGE_H
#define CHAMBRESFROIDESSTATSPAGE_H

#include <QWidget>

namespace Ui {
class ChambresFroidesStatsPage;
}

class ChambresFroidesStatsPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChambresFroidesStatsPage(QWidget *parent = nullptr);
    ~ChambresFroidesStatsPage();

public slots:
    void refreshStats();

private:
    void applyStyles();
    void setupCharts();

    Ui::ChambresFroidesStatsPage *ui;
    
    // Alert System
    class QTimer *m_alertBlinkTimer = nullptr;
    class QPushButton *m_btnAlertInbox = nullptr;
    void setupIntelligenceDashboard();
    void updateIntelligenceUI();

    // Chart pointers for updates
    class QBarSeries *tempSeries = nullptr;
    class QBarSet *tempSet = nullptr;
    class QPieSeries *statusSeries = nullptr;
    class QPieSeries *speciesSeries = nullptr;
};

#endif // CHAMBRESFROIDESSTATSPAGE_H
