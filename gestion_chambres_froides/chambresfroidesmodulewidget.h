#ifndef CHAMBRESFROIDESMODULEWIDGET_H
#define CHAMBRESFROIDESMODULEWIDGET_H

#include <QWidget>

class QTabWidget;

class ChambresFroidesModuleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChambresFroidesModuleWidget(QWidget *parent = nullptr);
    ChambresFroidesModuleWidget() : ChambresFroidesModuleWidget(nullptr) {} // Ensure default constructor for QMetaType

private:
    QTabWidget *tabWidget;
};

#endif // CHAMBRESFROIDESMODULEWIDGET_H
