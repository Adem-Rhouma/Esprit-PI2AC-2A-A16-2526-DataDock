#include "logistiquemodulewidget.h"
#include "views/camionspage.h"
#include "views/operationscamionspage.h"
#include "views/logistiquestatspage.h"
#include "views/camionmappage.h"
#include "views/camionefficiencypage.h"
#include "views/camionactivitypage.h"

#include <QTabWidget>
#include <QTabBar>
#include <QVBoxLayout>

LogistiqueModuleWidget::LogistiqueModuleWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabWidget = new QTabWidget(this);
    tabWidget->setStyleSheet("QTabWidget::pane { border: none; background: transparent; }");
    tabWidget->addTab(new CamionsPage(tabWidget),             tr("Camions"));
    tabWidget->addTab(new OperationsCamionsPage(tabWidget),   tr("Opérations Camions"));
    tabWidget->addTab(new LogistiqueStatsPage(tabWidget),     tr("Statistiques"));
    tabWidget->addTab(new CamionMapPage(tabWidget),           tr("Carte Interactive"));
    tabWidget->addTab(new CamionEfficiencyPage(tabWidget),    tr("Analyse & Maintenance"));
    tabWidget->addTab(new CamionActivityPage(tabWidget),      tr("Historique d'Activité"));

    layout->addWidget(tabWidget);
    tabWidget->tabBar()->hide();
    
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background: transparent; border: none;");

    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == StatsIdx) {
            auto *statsPage = qobject_cast<LogistiqueStatsPage*>(tabWidget->widget(index));
            if (statsPage) statsPage->refreshData();
        } else if (index == EfficiencyIdx) {
            auto *effPage = qobject_cast<CamionEfficiencyPage*>(tabWidget->widget(index));
            if (effPage) effPage->loadData();
        } else if (index == ActivityIdx) {
            auto *activityPage = qobject_cast<CamionActivityPage*>(tabWidget->widget(index));
            if (activityPage) activityPage->loadFromDB();
        }
    });


}

void LogistiqueModuleWidget::setPage(int index)
{
    if (tabWidget) {
        tabWidget->setCurrentIndex(index);
    }
}
