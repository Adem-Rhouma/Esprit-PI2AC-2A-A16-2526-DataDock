#ifndef LOGISTIQUEMODULEWIDGET_H
#define LOGISTIQUEMODULEWIDGET_H

#include <QWidget>

class QTabWidget;

class LogistiqueModuleWidget : public QWidget
{
    Q_OBJECT

public:
    enum PageIndex {
        CamionsIdx = 0,
        OperationsIdx = 1,
        StatsIdx = 2,
        MapIdx     = 3,
        EfficiencyIdx = 4,
        ActivityIdx = 5
    };

    explicit LogistiqueModuleWidget(QWidget *parent = nullptr);
    void setPage(int index);

private:
    QTabWidget *tabWidget;
};

#endif // LOGISTIQUEMODULEWIDGET_H
