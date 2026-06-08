#ifndef PECHECRUDDIALOG_H
#define PECHECRUDDIALOG_H

#include "pechebasedialog.h"
#include <QList>
#include <QMap>
#include <QPair>
#include <QRegularExpression>
#include <QString>

class QComboBox;
class QDateEdit;
class QLabel;
class QPlainTextEdit;
class QTableWidget;

namespace Ui {
class PecheCrudDialog;
}

class PecheCrudDialog : public PecheBaseDialog
{
    Q_OBJECT

public:
    enum Mode {
        AddMode,
        EditMode
    };

    explicit PecheCrudDialog(Mode mode, QWidget *parent = nullptr);
    explicit PecheCrudDialog(Mode mode, int idPeche, QWidget *parent = nullptr);
    ~PecheCrudDialog();

    int pecheId() const;

private slots:
    void onAddEspeceRow();
    void onRemoveEspeceRow();
    void onAddEmployeRow();
    void onRemoveEmployeRow();
    void onAccept();
    void onFieldEdited();
    void onDescriptionChanged();

private:
    // Setup helpers
    void setupTables();
    void loadNavires();
    void loadEspeces();   // populates m_especesList
    void loadEmployes();  // populates m_employesList
    void loadPeche(int idPeche);
    void setupUiHints();
    void setupValidators();
    void setupErrorLabels();
    void setupLiveValidation();

    // Helpers to insert a pre-populated combo-box cell
    void insertEspeceComboCell(int row, int col, int selectedId = -1);
    void insertEmployeComboCell(int row, int col, int selectedId = -1);

    // Save / DB
    bool save();
    bool insertAssociations(int idPeche, QString *err);
    bool deleteAssociations(int idPeche, QString *err);

    // Row readers  – col 0 now returns a foreign-key ID from Qt::UserRole
    bool readEspeceRow(int row, int &idEspece, double &quantite, double &prix, QString &err) const;
    bool readEmployeRow(int row, int &idEmploye, QString &description, QString &err) const;
    bool zoneexiste(QString zone);

    // Error UI
    void applyErrors(const QMap<QWidget *, QString> &errors);
    void applyErrorState(QWidget *field, QLabel *label, const QString &message);
    void clearErrorState(QWidget *field, QLabel *label);
    void focusFirstInvalid(const QMap<QWidget *, QString> &errors);

    // Validation
    QMap<QWidget *, QString> validateAll(bool includeDbChecks);
    bool validateEspecesTable(QString *err, QWidget **firstInvalid) const;
    bool validateEmployesTable(QString *err, QWidget **firstInvalid) const;

    // Live save-button state
    void updateSaveEnabled();

    // Input normalization / parsing helpers
    QString normalizeText(const QString &text) const;
    bool parseDouble(const QString &text, double *out) const;
    bool parseInt(const QString &text, int *out) const;
    void applyNormalizedInputs();


    Ui::PecheCrudDialog *ui;
    Mode m_mode;
    int m_pecheId = 0;

    // Cached lookup lists loaded once from DB
    // QPair<int id, QString displayName>
    QList<QPair<int, QString>> m_especesList;
    QList<QPair<int, QString>> m_employesList;

    // Error labels inserted into the form layout
    QLabel *m_errorDate        = nullptr;
    QLabel *m_errorNavire      = nullptr;
    QLabel *m_errorZone        = nullptr;
    QLabel *m_errorDescription = nullptr;
    QLabel *m_errorEspeces     = nullptr;
    QLabel *m_errorEmployes    = nullptr;

    QRegularExpression m_zoneRegex;
    int m_zoneMaxLen  = 60;
    int m_descMaxLen  = 255;
    int m_empDescMax  = 200;

    bool m_ignoreSignals    = false;
    bool m_validationActive = true;
};

#endif // PECHECRUDDIALOG_H
