#ifndef NAVIREDETAILPAGE_H
#define NAVIREDETAILPAGE_H

#include <QWidget>
#include <QSqlQueryModel>
#include "navire.h"

namespace Ui {
class NavireDetailPage;
}

class NavireDetailPage : public QWidget
{
    Q_OBJECT

public:
    explicit NavireDetailPage(int shipId, QWidget *parent = nullptr);
    ~NavireDetailPage();

private slots:
    void on_btnAddVoyage_clicked();
private:
    void loadShipData();
    void loadVoyageLogs();


private:
    Ui::NavireDetailPage *ui;
    int m_shipId;
    Navire m_navire;
};

#endif // NAVIREDETAILPAGE_H
