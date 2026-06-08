#ifndef NAVIRECRUDDIALOG_H
#define NAVIRECRUDDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>

namespace Ui {
class NavireCrudDialog;
}

class NavireCrudDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode { AddMode, EditMode };

    explicit NavireCrudDialog(Mode mode, QWidget *parent = nullptr);
    explicit NavireCrudDialog(Mode mode, int idNavire, QWidget *parent = nullptr);
    ~NavireCrudDialog();

    int navireId() const;

private slots:
    void onAccept();

private:
    void applyStyle();
    void setupValidators();
    void connectRealTimeValidation();
    bool loadNavire(int idNavire);
    bool validateAll();
    bool save();

    // Validation helpers — plain private (NOT slots) to avoid MOC QLabel metatype issues
    void validateField(QLineEdit *field, QLabel *errLabel, const QString &errorMsg, bool condition);
    void setFieldError(QLineEdit *field, QLabel *errLabel, const QString &msg);
    void clearFieldError(QLineEdit *field, QLabel *errLabel);

    Ui::NavireCrudDialog *ui;
    Mode m_mode;
    int m_navireId = 0;
};

#endif // NAVIRECRUDDIALOG_H
