#ifndef PECHEARCHIVEDDIALOG_H
#define PECHEARCHIVEDDIALOG_H

#include "pechebasedialog.h"

class QTableWidget;

namespace Ui {
class PecheArchivedDialog;
}

class PecheArchivedDialog : public PecheBaseDialog
{
    Q_OBJECT

public:
    explicit PecheArchivedDialog(QWidget *parent = nullptr);
    ~PecheArchivedDialog();

private:
    void setupTable();
    void loadArchived();
    void addRow(const QString &idPeche,
                const QString &datePeche,
                const QString &origine,
                const QString &espece,
                double quantite,
                double prixUnitaire,
                const QString &statut);
    QWidget *createActionsWidget(const QString &idPeche);

    Ui::PecheArchivedDialog *ui;
    QTableWidget *table = nullptr;
};

#endif // PECHEARCHIVEDDIALOG_H
