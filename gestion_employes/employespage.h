#ifndef EMPLOYESPAGE_H
#define EMPLOYESPAGE_H

#include <QWidget>
#include <QStandardItem>
#include "employe.h"
#include <functional>
namespace Ui {
class EmployesPage;
}

class EmployesPage : public QWidget
{
    Q_OBJECT

public:
    explicit EmployesPage(QWidget *parent = nullptr);
    ~EmployesPage();
    void envoyerImageFace(const QString &imagePath, std::function<void(QJsonArray)> callback);

private slots:

    void on_ajouter_clicked();

    void on_statistique_clicked();

    void on_rt1_2_clicked();
    void on_rt1_clicked();
    void on_rt1_3_clicked();
    void on_rt1_5_clicked();
    void on_rt1_4_clicked();

    void on_ajouterr_clicked();   // Confirmer Ajout
    void on_ajouterr_2_clicked(); // Confirmer Modification

    // Slots pour Recherche et Tri
    void on_searchBar_2_textChanged(const QString &arg1);
    void on_comboTrier_currentIndexChanged(int index); // Changed to int to be safer

    void on_affichage_3_clicked(); // Slot pour Exporter PDF et TXT
    void on_reglementBtn_clicked(); // Slot pour Règlement du Port PDF
    void onAttestationClicked();

    void refresh(); // Helper method
    void populateTable(QSqlQueryModel *qModel); // Helper pour remplir le tableau
    bool exporterAttestationParId(const QString &idEmploye);

private:
    Ui::EmployesPage *ui;
    QStandardItemModel *model;
    Employe Etmp;
};

#endif // EMPLOYESPAGE_H
