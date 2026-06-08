#ifndef METEOSTRIPWIDGET_H
#define METEOSTRIPWIDGET_H

#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>
#include "meteo_data.h"

class MeteoStripWidget : public QFrame {
    Q_OBJECT
public:
    explicit MeteoStripWidget(QWidget *parent = nullptr);

public slots:
    void updateData(const MeteoData::MeteoReport &report);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void setupUi();
    void applyStyle();

    QLabel *m_statusLabel;
    QLabel *m_riskOrb;
    QLabel *m_detailsLabel;
    QHBoxLayout *m_forecastLayout;
};

#endif // METEOSTRIPWIDGET_H
