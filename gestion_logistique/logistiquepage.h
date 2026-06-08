#ifndef LOGISTIQUEPAGE_H
#define LOGISTIQUEPAGE_H

#include <QWidget>

class QComboBox;
class QLineEdit;
class QTableWidget;
class QPushButton;
class QWidget;

namespace Ui {
class LogistiquePage;
}

class LogistiquePage : public QWidget
{
    Q_OBJECT

public:
    explicit LogistiquePage(QWidget *parent = nullptr);
    ~LogistiquePage();

private:
    void setupTable();
    void applyStyles();
    void loadSampleData();
    void addRow(const QString &idCamion,
                const QString &immatriculation,
                const QString &typeCamion,
                double capacite,
                double chargeActuelle,
                const QString &zone,
                const QString &statut,
                const QString &heureEntree,
                const QString &heureSortie);
    QWidget *createStatusBadge(const QString &statut) const;
    QWidget *createActionsWidget(const QString &idCamion);
    int findRowById(const QString &idCamion) const;
    void updateSort();
    void applySearchFilter();

    void onExportPdf();
    void onExportExcel();
    void onExportTxt();

    Ui::LogistiquePage *ui;

    QPushButton *addButton = nullptr;
    QComboBox *sortCombo = nullptr;
    QComboBox *sortOrderCombo = nullptr;
    QComboBox *searchCombo = nullptr;
    QLineEdit *searchEdit = nullptr;
    QPushButton *exportPdfButton = nullptr;
    QPushButton *exportExcelButton = nullptr;
    QPushButton *exportTxtButton = nullptr;
    QWidget *tableCard = nullptr;
    QTableWidget *table = nullptr;
};

#endif // LOGISTIQUEPAGE_H
