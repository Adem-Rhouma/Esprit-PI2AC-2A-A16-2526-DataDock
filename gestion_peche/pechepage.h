#ifndef PechePAGE_H
#define PechePAGE_H

#include <QWidget>
#include <QVector>

class QComboBox;
class QLineEdit;
class QTableWidget;
class QPushButton;
class QWidget;

namespace Ui {
class PechePage;
}

class PechePage : public QWidget
{
    Q_OBJECT

public:
    explicit PechePage(QWidget *parent = nullptr);
    ~PechePage();

private:
    void setupTable();
    void applyStyles();
    void loadFromDb();
    void addRow(const QString &idPeche,
                const QString &datePeche,
                const QString &typeOperation,
                const QString &typeProduit,
                double quantite,
                double prixUnitaire,
                const QString &statutPeche);
    QWidget *createStatusBadge(const QString &statut) const;
    QWidget *createActionsWidget(const QString &idPeche);
    int findRowById(const QString &idPeche) const;
    void updateSort();
    void applySearchFilter();
    void refreshTable();
    QVector<int> chooseExportColumns(bool *accepted, bool *visibleRowsOnly);

    void onExportPdf();
    void onExportCsv();

    Ui::PechePage *ui;

    QPushButton *addButton = nullptr;
    QPushButton *archivedButton = nullptr;
    QComboBox *sortCombo = nullptr;
    QComboBox *sortOrderCombo = nullptr;
    QComboBox *searchCombo = nullptr;
    QLineEdit *searchEdit = nullptr;
    QPushButton *exportButton = nullptr;
    QWidget *tableCard = nullptr;
    QTableWidget *table = nullptr;
};

#endif // PechePAGE_H

