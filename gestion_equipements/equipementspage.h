#ifndef EQUIPEMENTSPAGE_H
#define EQUIPEMENTSPAGE_H

#include <QStandardItemModel>
#include <QWidget>

#include "materiel.h"

class QLineEdit;

namespace Ui {
class EquipementsPage;
}

class EquipementsPage : public QWidget
{
    Q_OBJECT

public:
    explicit EquipementsPage(QWidget *parent = nullptr);
    ~EquipementsPage();

private:
    void on_ajouter_clicked();
    void on_statistique_clicked();
    void on_rt1_2_clicked();
    void on_rt1_clicked();
    void on_rt1_3_clicked();
    void on_rt1_5_clicked();
    void on_rt1_4_clicked();
    void on_ajouterr_clicked();
    void on_ajouterr_2_clicked();
    void on_annuler_clicked();
    void on_ajouterr_3_clicked();

    void setupEquipementsTable();
    void normalizeFormTexts();
    void refreshTable();
    void setupReparableCombos();
    void openAddDialog();
    void clearAddForm();
    void clearEditForm();
    bool readMaterielFromAddForm(materiel &value) const;
    bool readMaterielFromEditForm(materiel &value) const;
    void showMaterielDetails(const materiel &value);
    void showDeleteConfirm(const materiel &value);
    void openEditForm(const materiel &value);
    void hidePanels();
    void showListGroup();
    void exportTableToExcel();
    void setupInputValidation();
    void clearFormValidationState();
    void setFieldValidity(QLineEdit *field, bool isValid) const;
    bool validateCurrentForm(bool showErrors);
    void updateSaveButtonState();
    void showFormError(const QString &message);

    Ui::EquipementsPage *ui;
    QStandardItemModel *model;
    int m_selectedMaterielId;
    bool m_isEditMode;
    int m_pendingDeleteId;
};

#endif // EQUIPEMENTSPAGE_H
