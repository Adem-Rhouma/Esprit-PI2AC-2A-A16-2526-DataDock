#ifndef CHAMBRESFROIDESDIALOG_H
#define CHAMBRESFROIDESDIALOG_H

#include <QDialog>
#include <QString>
#include "chambresfroides.h"

namespace Ui {
class ChambresFroidesDialog;
}

class ChambresFroidesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChambresFroidesDialog(QWidget *parent = nullptr);
    ~ChambresFroidesDialog();

    void setChambre(const ChambresFroides &chambre);

private slots:
    void on_btnSave_clicked();
    void on_btnCancel_clicked();
    void on_txtId_textChanged(const QString &arg1);
    void on_txtCertSan_textChanged(const QString &arg1);
    void on_capacity_changed();

private:
    void loadPecheIds();
    bool validateInputs(QString *firstError = nullptr) const;
    void updateSaveButtonState();

    Ui::ChambresFroidesDialog *ui;
    bool m_isEditMode;
    QString m_oldId;
};

#endif // CHAMBRESFROIDESDIALOG_H
