#ifndef PecheMODULEWIDGET_H
#define PecheMODULEWIDGET_H

#include <QWidget>

class QStackedWidget;
class DynamicPricingPage;
class FishVisionPage;
class PecheRecommendationsPage;

class PecheModuleWidget : public QWidget
{
    Q_OBJECT

public:
    enum PageIndex {
        CrudPage                  = 0,
        StatsPage                 = 1,
        SmartServicesPage         = 2,
        DynamicPricingPageIndex   = 3,
        FishVisionPageIndex       = 4,
        RecommendationsPageIndex  = 5
    };

    explicit PecheModuleWidget(QWidget *parent = nullptr);

    void showCrud();
    void showStats();
    void showSmartServices();
    void showDynamicPricing();
    void showFishVision();
    void showRecommendations();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setPage(int index);

    QStackedWidget      *stackedPages          = nullptr;
    DynamicPricingPage  *dynamicPricingPage    = nullptr;
    FishVisionPage      *fishVisionPage        = nullptr;
    PecheRecommendationsPage *recommendationsPage = nullptr;
};

#endif // PecheMODULEWIDGET_H
