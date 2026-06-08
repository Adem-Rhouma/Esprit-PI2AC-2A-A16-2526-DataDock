#ifndef EMPLOYESMODULEWIDGET_H
#define EMPLOYESMODULEWIDGET_H

#include <QWidget>

class QTabWidget;

class EmployesModuleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EmployesModuleWidget(QWidget *parent = nullptr);

private:
    QTabWidget *tabWidget;
};

#endif // EMPLOYESMODULEWIDGET_H
