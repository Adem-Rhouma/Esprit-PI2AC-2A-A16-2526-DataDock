#ifndef PECHESMARTSERVICESPAGE_H
#define PECHESMARTSERVICESPAGE_H

#include <QWidget>
#include <QEvent>

class PecheSmartServicesPage : public QWidget
{
    Q_OBJECT

public:
    explicit PecheSmartServicesPage(QWidget *parent = nullptr);
    ~PecheSmartServicesPage() override = default;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void openDynamicPricingRequested();
    void openRecommendationsRequested();
    void fishVisionRequested();

private:
    void buildUi();
};

#endif // PECHESMARTSERVICESPAGE_H
