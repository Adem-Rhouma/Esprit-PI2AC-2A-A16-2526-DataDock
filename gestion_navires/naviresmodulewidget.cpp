#include "naviresmodulewidget.h"

#include "navirespage.h"
#include "naviresstatspage.h"
#include "meteowidget.h"
#include "maintenancepage.h"
#include "NavireDatabaseHelper.h"

#include <QTabWidget>
#include <QVBoxLayout>

NaviresModuleWidget::NaviresModuleWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabWidget = new QTabWidget(this);

    crudPage = new NaviresPage(tabWidget);
    auto *statsPage = new NaviresStatsPage(tabWidget);

    auto *meteoPage = new MeteoWidget(tabWidget);
    auto *maintenancePage = new MaintenancePage(tabWidget);

    tabWidget->addTab(crudPage, tr("Gestion Navires"));
    tabWidget->addTab(statsPage, tr("Statistiques"));
    tabWidget->addTab(meteoPage, tr("Météo & Planification"));
    tabWidget->addTab(maintenancePage, tr("Maintenance Prédictive"));

    // Connect the Weather Strip in stats to the Planificateur tab
    connect(statsPage, &NaviresStatsPage::requestMeteoTab, [this](){
        tabWidget->setCurrentIndex(2); // Index of Météo & Analyse
    });

    // Rename the tab for accuracy
    tabWidget->setTabText(2, tr("Météo & Analyse"));

    // Connect the live weather feed to the maintenance prediction engine
    connect(meteoPage, &MeteoWidget::weatherUpdated, maintenancePage, &MaintenancePage::refreshData);

    NavireDatabaseHelper::initializeSchema();

    layout->addWidget(tabWidget);

    // Apply shared tab styling (similar to Employes/Logistique modules)
    tabWidget->setStyleSheet(
        "QTabWidget::pane { border: none; background: transparent; }"
        "QTabWidget::tab-bar { alignment: left; }"
        "QTabBar::tab { "
        "   background: rgba(255, 255, 255, 180); "
        "   color: #1B1B1B; "
        "   padding: 8px 20px; "
        "   border-top-left-radius: 10px; "
        "   border-top-right-radius: 10px; "
        "   margin-right: 4px; "
        "   font-weight: 600;"
        "}"
        "QTabBar::tab:selected { "
        "   background: #FFFFFF; "
        "   color: #5E60CE; "
        "   border-bottom: 2px solid #5E60CE;"
        "}"
        "QTabBar::tab:hover { "
        "   background: rgba(255, 255, 255, 220); "
        "}"
    );
}

