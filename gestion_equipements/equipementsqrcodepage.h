#ifndef EQUIPEMENTSQRCODEPAGE_H
#define EQUIPEMENTSQRCODEPAGE_H

#include <QByteArray>
#include <QImage>
#include <QWidget>

#include "materiel.h"

class QComboBox;
class QLabel;
class QPushButton;

class EquipementsQrCodePage : public QWidget
{
    Q_OBJECT

public:
    explicit EquipementsQrCodePage(QWidget *parent = nullptr);

    void refreshData();

private:
    void applyStyling();
    void buildUi();
    void loadMateriels();
    void updatePreview();
    void exportCurrentQr();
    QImage generateQrImage(const QString &text) const;

    QComboBox *comboMateriels;
    QLabel *lblPreview;
    QLabel *lblInfo;
    QPushButton *btnRefresh;
    QPushButton *btnExport;
};

#endif // EQUIPEMENTSQRCODEPAGE_H
