#ifndef OPERATIONSCAMIONSPAGE_H
#define OPERATIONSCAMIONSPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QStackedWidget>
#include "../models/operationcamion.h"
#include <QSqlQuery>

class OperationsCamionsPage : public QWidget
{
    Q_OBJECT

public:
    explicit OperationsCamionsPage(QWidget *parent = nullptr);
    ~OperationsCamionsPage();

private slots:
    // List actions
    void onAddClicked();
    void onSearchChanged(const QString &text);
    void onSortChanged(int index);
    void onEditClicked(int id);
    void onViewClicked(int id);
    void onDeleteClicked(int id);
    void onDeleteSelectedClicked();

    // Form actions
    void onSaveClicked();
    void onCancelClicked();

    // Pagination
    void onPrevPage();
    void onNextPage();

    // Export
    void exportToExcel();
    void exportToPdf();
    void onRefreshClicked();

private:
    // UI Helpers
    void setupListPage();
    void setupFormPage();
    void setupTable();
    void applyStyles();
    void populateForm(int id);
    void refreshCamionsCombo();

    // DB Operations
    void loadFromDB();
    bool insertOperation(int camionId, int idPeche, const QString &typeOp,
                         const QString &zoneOp, const QDateTime &dateDebut,
                         const QDateTime &dateFin, const QString &statut,
                         int priorite, const QString &desc);
    bool updateOperation(int opId, int camionId, int idPeche,
                         const QString &typeOp, const QString &zoneOp,
                         const QDateTime &dateDebut, const QDateTime &dateFin,
                         const QString &statut, int priorite, const QString &desc);
    bool deleteOperation(int opId);
    void syncToPredictorCSV();

    // Row helpers
    void addTableRow(int opId, int camionId, int idPeche, const QString &immat,
                     const QString &typeOp, const QString &zoneOp,
                     const QDateTime &dateDebut, const QDateTime &dateFin,
                     const QString &statut, int priorite, const QString &desc);
    QWidget *createStatusBadge(const QString &statut) const;
    QWidget *createPriorityBadge(int prio) const;
    QWidget *createActionsWidget(int opId);

    // Widgets
    QStackedWidget *stackedWidget = nullptr;
    QWidget *listPage  = nullptr;
    QWidget *formPage  = nullptr;

    // List Page Widgets
    QTableWidget *table           = nullptr;
    QLineEdit    *searchEdit      = nullptr;
    QComboBox    *sortCombo       = nullptr;
    QComboBox    *sortOrderCombo  = nullptr;
    QPushButton  *addBtn          = nullptr;
    QPushButton  *refreshBtn      = nullptr;
    QPushButton  *deleteSelectedBtn = nullptr;
    QPushButton  *exportBtn       = nullptr;

    // Pagination Widgets
    QPushButton *prevBtn      = nullptr;
    QPushButton *nextBtn      = nullptr;
    QLabel      *pageInfoLbl  = nullptr;

    // Form Page Widgets
    QLabel    *lblTitle      = nullptr;
    QComboBox *cbCamionImmat = nullptr;
    QSpinBox  *sbIdPeche     = nullptr;
    QComboBox *cbType     = nullptr;
    QLineEdit *edZone     = nullptr;
    QDateTimeEdit *deDebut    = nullptr;
    QDateTimeEdit *deFin      = nullptr;
    QComboBox *cbStatut   = nullptr;
    QSpinBox  *sbPriorite = nullptr;
    QLineEdit *edDesc     = nullptr;

    QPushButton *btnSave   = nullptr;
    QPushButton *btnCancel = nullptr;

    int  currentEditId = -1;
    bool isViewMode    = false;

    // Pagination State
    int currentPage = 0;
    const int pageSize = 50;
    int totalRecords = 0;
};

#endif // OPERATIONSCAMIONSPAGE_H
