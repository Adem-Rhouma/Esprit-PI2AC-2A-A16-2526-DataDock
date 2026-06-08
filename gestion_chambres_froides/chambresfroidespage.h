#ifndef CHAMBRESFROIDESPAGE_H
#define CHAMBRESFROIDESPAGE_H

#include <QWidget>

class QTableWidget;
class QComboBox;
class QLineEdit;
class QPushButton;

namespace Ui {
class ChambresFroidesPage;
}

class ChambresFroidesPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChambresFroidesPage(QWidget *parent = nullptr);
    ~ChambresFroidesPage();
    
signals:
    void dataChanged();
    void exportRequested();

private slots:
    void on_btnAddChambre_clicked();
    void on_searchBar_textChanged(const QString &text);
    void on_comboSortField_currentIndexChanged(int index);

    void onEditButtonClicked();
    void onConfigButtonClicked();
    void onDeleteButtonClicked();

private:
    void refreshTable();
    void setupTable();
    void loadSampleData();
    void applyStyles();
    void addRow(const QString &id, const QString &tagNumber, double temp, 
                const QString &inDate, const QString &limitDate, 
                const QString &certSan, double maxCap, double occCap, 
                const QString &state, const QVariant &idPeche, double humidity, bool betaTest);
    
    QWidget* createStatusBadge(const QString &statut) const;
    QWidget* createActionsWidget(const QString &id, const QString &tagNumber, double humidity, bool betaTest);

    Ui::ChambresFroidesPage *ui;
};

#endif // CHAMBRESFROIDESPAGE_H
