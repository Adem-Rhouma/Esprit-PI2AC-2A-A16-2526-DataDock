#ifndef PECHEDETAILSDIALOG_H
#define PECHEDETAILSDIALOG_H

#include "pechebasedialog.h"

namespace Ui {
class PecheDetailsDialog;
}

class PecheDetailsDialog : public PecheBaseDialog
{
    Q_OBJECT

public:
    explicit PecheDetailsDialog(int idPeche, QWidget *parent = nullptr);
    ~PecheDetailsDialog();

private:
    void loadDetails(int idPeche);
    void loadEspeces(int idPeche);
    void loadEmployes(int idPeche);

    Ui::PecheDetailsDialog *ui;
};

#endif // PECHEDETAILSDIALOG_H
