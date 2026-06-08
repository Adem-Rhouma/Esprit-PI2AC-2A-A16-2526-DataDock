#include "employesmodulewidget.h"
#include "employespage.h"
#include "employestatspage.h"

#include <QTabWidget>
#include <QVBoxLayout>

EmployesModuleWidget::EmployesModuleWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabWidget = new QTabWidget(this);
    tabWidget->addTab(new EmployesPage(tabWidget), tr("Gestion Employés"));
    tabWidget->addTab(new EmployesStatsPage(tabWidget), tr("Statistiques"));

    layout->addWidget(tabWidget);

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
