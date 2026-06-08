#include "equipementsmodulewidget.h"
#include "equipementspage.h"
#include "equipementsstatspage.h" // Functionality to be added
#include "equipementsqrcodepage.h"
#include "equipementssmsalertspage.h"

#include <QTabWidget>
#include <QVBoxLayout>

EquipementsModuleWidget::EquipementsModuleWidget(QWidget *parent)
    : QWidget(parent)
    , qrCodePage(nullptr)
    , smsAlertsPage(nullptr)
    , statsPage(nullptr)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabWidget = new QTabWidget(this);
    tabWidget->addTab(new EquipementsPage(tabWidget), tr("Gestion Matériaux & Terminaux"));
    statsPage = new EquipementsStatsPage(tabWidget);
    tabWidget->addTab(statsPage, tr("Statistiques"));
    qrCodePage = new EquipementsQrCodePage(tabWidget);
    tabWidget->addTab(qrCodePage, tr("QR Code"));
    smsAlertsPage = new EquipementsSmsAlertsPage(tabWidget);
    tabWidget->addTab(smsAlertsPage, tr("Alertes SMS"));

    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 1 && statsPage) {
            statsPage->refreshData();
        } else if (index == 2 && qrCodePage) {
            qrCodePage->refreshData();
        } else if (index == 3 && smsAlertsPage) {
            smsAlertsPage->refreshData();
        }
    });

    layout->addWidget(tabWidget);

    // Apply the same styling as LogistiqueModuleWidget
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
