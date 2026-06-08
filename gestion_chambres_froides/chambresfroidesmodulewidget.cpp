#include "chambresfroidesmodulewidget.h"
#include "chambresfroidespage.h"
#include "chambresfroidesexportpage.h"
#include "chambresfroidesstatspage.h"
#include "chambresfroidesoptimizationpage.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QStackedWidget>

ChambresFroidesModuleWidget::ChambresFroidesModuleWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabWidget = new QTabWidget(this);
    
    // Stack for the first tab to switch between List and Export Page
    QStackedWidget *mainStack = new QStackedWidget(tabWidget);
    auto *page = new ChambresFroidesPage(mainStack);
    auto *exportPage = new ChambresFroidesExportPage(mainStack);
    
    mainStack->addWidget(page);      // Index 0
    mainStack->addWidget(exportPage); // Index 1

    auto *statsPage = new ChambresFroidesStatsPage(tabWidget);
    auto *optiPage = new ChambresFroidesOptimizationPage(tabWidget);

    tabWidget->addTab(mainStack, tr("Gestion Chambres"));
    tabWidget->addTab(statsPage, tr("Statistiques"));
    tabWidget->addTab(optiPage, tr("Optimisation"));

    // Navigation logic
    connect(page, &ChambresFroidesPage::exportRequested, [mainStack]() {
        mainStack->setCurrentIndex(1);
    });
    connect(exportPage, &ChambresFroidesExportPage::backRequested, [mainStack]() {
        mainStack->setCurrentIndex(0);
    });

    connect(page, &ChambresFroidesPage::dataChanged, statsPage, &ChambresFroidesStatsPage::refreshStats);
    connect(page, &ChambresFroidesPage::dataChanged, optiPage, &ChambresFroidesOptimizationPage::refreshData);

    layout->addWidget(tabWidget);

    // Apply shared tab styling (similar to Logistique)
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

