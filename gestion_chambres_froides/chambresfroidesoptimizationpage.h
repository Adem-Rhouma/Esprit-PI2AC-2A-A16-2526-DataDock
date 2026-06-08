#ifndef CHAMBRESFROIDESOPTIMIZATIONPAGE_H
#define CHAMBRESFROIDESOPTIMIZATIONPAGE_H

#include <QWidget>
#include <QMap>
#include <QVariant>
#include <QVBoxLayout>

namespace Ui {
class ChambresFroidesOptimizationPage;
}

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "scenariodetailsdialog.h"

class ChambresFroidesOptimizationPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChambresFroidesOptimizationPage(QWidget *parent = nullptr);
    ~ChambresFroidesOptimizationPage();

public slots:
    void refreshData();

private slots:
    void on_comboMode_currentIndexChanged(int index);
    void on_btnOptimize_clicked();
    void on_applyStorage_clicked();
    void on_comboPecheId_currentIndexChanged(int index);
    void on_explainScenario_clicked();
    void onAiProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    Ui::ChambresFroidesOptimizationPage *ui;
    QProcess *aiProcess;
    QVBoxLayout *cardsLayout;
    
    struct AISolution {
        QString id;
        double score;
        QString qualitative;
        QJsonArray allocations;
        QJsonObject predictions;
        QString explanation;
        int index;
    };
    QList<AISolution> currentAiSolutions;

    void loadPecheIds();
    void setupTable();
    void applyStyles();
    void performLegacyOptimization();
    void performAIOptimization();
    void displayAiSolution(int index);
    
    bool checkAiDependencies();
    void installDependencies();
    
    struct Recommendation {
        QString chambreId;
        double currentTemp;
        double targetTemp;
        double availableCap;
        QString justification;
        double score;
    };
};

#endif // CHAMBRESFROIDESOPTIMIZATIONPAGE_H
