#include "pechemodulewidget.h"

#include "pechepage.h"
#include "pechestatspage.h"
#include "pechesmartservicespage.h"
#include "pecherecommendationspage.h"
#include "dynamic_pricing/ui/dynamicpricingpage.h"
#include "fishvision/fishvisionpage.h"
#include "pechefontutils.h"

#include <QApplication>
#include <QEvent>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWheelEvent>

PecheModuleWidget::PecheModuleWidget(QWidget *parent)
    : QWidget(parent)
{
    PecheFontUtils::applyModuleFont(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    stackedPages = new QStackedWidget(this);
    layout->addWidget(stackedPages);

    // Index 0 – Crud
    stackedPages->addWidget(new PechePage(stackedPages));

    // Index 1 – Stats
    stackedPages->addWidget(new PecheStatsPage(stackedPages));

    // Index 2 – Smart Services
    auto *smartServices = new PecheSmartServicesPage(stackedPages);
    stackedPages->addWidget(smartServices);

    // Index 3 – Dynamic Pricing page (integrated, never a standalone window)
    dynamicPricingPage = new DynamicPricingPage(stackedPages);
    stackedPages->addWidget(dynamicPricingPage);

    // Index 4 – FishVision AI
    fishVisionPage = new FishVisionPage(stackedPages);
    stackedPages->addWidget(fishVisionPage);

    // Index 5 – Recommandations IA
    recommendationsPage = new PecheRecommendationsPage(stackedPages);
    stackedPages->addWidget(recommendationsPage);

    // Navigation: Smart Services button → Dynamic Pricing page
    connect(smartServices, &PecheSmartServicesPage::openDynamicPricingRequested,
            this, &PecheModuleWidget::showDynamicPricing);

    // Navigation: Smart Services button → Recommandations IA page
    connect(smartServices, &PecheSmartServicesPage::openRecommendationsRequested,
            this, &PecheModuleWidget::showRecommendations);

    // Navigation: Smart Services button → FishVision page
    connect(smartServices, &PecheSmartServicesPage::fishVisionRequested,
            this, &PecheModuleWidget::showFishVision);

    // Navigation: Dynamic Pricing back button → Smart Services page
    connect(dynamicPricingPage, &DynamicPricingPage::backRequested,
            this, &PecheModuleWidget::showSmartServices);

    // Navigation: FishVision back button → Smart Services page
    connect(fishVisionPage, &FishVisionPage::backRequested,
            this, &PecheModuleWidget::showSmartServices);

    // Navigation: Recommandations IA back button → Smart Services page
    connect(recommendationsPage, &PecheRecommendationsPage::backRequested,
            this, &PecheModuleWidget::showSmartServices);

    setPage(CrudPage);

    installEventFilter(this);
    const auto children = findChildren<QWidget *>();
    for (QWidget *child : children) {
        if (child) {
            child->installEventFilter(this);
        }
    }
}

void PecheModuleWidget::showCrud()
{
    setPage(CrudPage);
}

void PecheModuleWidget::showStats()
{
    setPage(StatsPage);
}

void PecheModuleWidget::showSmartServices()
{
    setPage(SmartServicesPage);
}

void PecheModuleWidget::showDynamicPricing()
{
    setPage(DynamicPricingPageIndex);
}

void PecheModuleWidget::showFishVision()
{
    setPage(FishVisionPageIndex);
}

void PecheModuleWidget::showRecommendations()
{
    setPage(RecommendationsPageIndex);
}

void PecheModuleWidget::setPage(int index)
{
    if (!stackedPages) {
        return;
    }
    stackedPages->setCurrentIndex(index);
}

bool PecheModuleWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event && event->type() == QEvent::Wheel) {
        if (stackedPages && qobject_cast<PecheSmartServicesPage *>(stackedPages->currentWidget())) {
            return QWidget::eventFilter(watched, event);
        }
        if (stackedPages && qobject_cast<DynamicPricingPage *>(stackedPages->currentWidget())) {
            return QWidget::eventFilter(watched, event);
        }
        if (stackedPages && qobject_cast<FishVisionPage *>(stackedPages->currentWidget())) {
            return QWidget::eventFilter(watched, event);
        }
        if (stackedPages && qobject_cast<PecheRecommendationsPage *>(stackedPages->currentWidget())) {
            return QWidget::eventFilter(watched, event);
        }

        QWidget *widget = qobject_cast<QWidget *>(watched);
        QWidget *parent = widget ? widget->parentWidget() : parentWidget();
        while (parent) {
            auto *scrollArea = qobject_cast<QScrollArea *>(parent);
            if (scrollArea) {
                scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                QWidget *viewport = scrollArea->viewport();
                if (viewport && watched != viewport) {
                    QApplication::sendEvent(viewport, event);
                    return true;
                }
                break;
            }
            parent = parent->parentWidget();
        }
    }

    return QWidget::eventFilter(watched, event);
}
