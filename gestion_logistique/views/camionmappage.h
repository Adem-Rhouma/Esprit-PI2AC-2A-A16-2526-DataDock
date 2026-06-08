#ifndef CAMIONMAPPAGE_H
#define CAMIONMAPPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QQuickWidget>
#include <QSplitter>
#include <QSqlQuery>

class CamionMapPage : public QWidget
{
    Q_OBJECT

public:
    explicit CamionMapPage(QWidget *parent = nullptr);
    ~CamionMapPage();


    void syncMapModel();

public slots:
    void updateCamionPosition(int id, double lat, double lon);

protected:

    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:

    QQuickWidget *mapWidget;
    void setupUI();
};

#endif // CAMIONMAPPAGE_H
