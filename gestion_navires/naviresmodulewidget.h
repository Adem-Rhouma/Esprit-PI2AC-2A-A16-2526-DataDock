#ifndef NAVIRESMODULEWIDGET_H
#define NAVIRESMODULEWIDGET_H

#include <QWidget>

class QTabWidget;
class NaviresPage;

class NaviresModuleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NaviresModuleWidget(QWidget *parent = nullptr);

private:
    QTabWidget *tabWidget = nullptr;
    NaviresPage *crudPage = nullptr;
};

#endif // NAVIRESMODULEWIDGET_H

