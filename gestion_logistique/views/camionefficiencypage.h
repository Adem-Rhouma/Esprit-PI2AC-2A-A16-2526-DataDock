#ifndef CAMIONEFFICIENCYPAGE_H
#define CAMIONEFFICIENCYPAGE_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

class CamionEfficiencyPage : public QWidget
{
    Q_OBJECT
public:
    explicit CamionEfficiencyPage(QWidget *parent = nullptr);

public slots:
    void loadData();

private:
    void setupUI();
    void applyStyles();

    QScrollArea *scrollArea;
    QWidget *cardsContainer;
    QPushButton *refreshBtn;
    class QComboBox *filterCombo;
};

#endif // CAMIONEFFICIENCYPAGE_H
