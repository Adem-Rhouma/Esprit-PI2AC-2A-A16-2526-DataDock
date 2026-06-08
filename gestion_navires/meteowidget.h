#ifndef METEOWIDGET_H
#define METEOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include "meteoclient.h"
#include "meteo_data.h"

namespace Ui {
class MeteoWidget;
}

class MeteoWidget : public QWidget {
    Q_OBJECT
public:
    explicit MeteoWidget(QWidget *parent = nullptr);
    ~MeteoWidget();

signals:
    void weatherUpdated(const MeteoData::MeteoReport &report);

public slots:
    void updateData(const MeteoData::MeteoReport &report);
    void showError(const QString &error);

private:
    void setupUiManual();
    void applyStyle();

    Ui::MeteoWidget *ui;
    QLabel *m_riskGaugeLabel;
    QLabel *m_riskDescription;
    QTableWidget *m_seaMatrix;
    QLabel *m_portConditionLabel;
    QLabel *m_lastUpdateLabel;
};

#endif // METEOWIDGET_H
