#ifndef DYNAMICPRICINGDIALOG_H
#define DYNAMICPRICINGDIALOG_H

#include "pechebasedialog.h"

namespace Ui {
class DynamicPricingDialog;
}

class DynamicPricingDialog : public PecheBaseDialog
{
    Q_OBJECT

public:
    explicit DynamicPricingDialog(QWidget *parent = nullptr);
    ~DynamicPricingDialog();

private:
    void applyStyling();
    void setupHeader();
    void setupTable();

    Ui::DynamicPricingDialog *ui;
};

#endif // DYNAMICPRICINGDIALOG_H
