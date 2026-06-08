#ifndef CAMIONSPAGE_H
#define CAMIONSPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDialog>
#include <QSqlQuery>
#include <QStackedWidget>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QQuickWidget>
#include <QQuickItem>
#include <QVariantList>
#include "../models/camion.h"

// ---------------------------------------------------------------------------
// Helper: table item that sorts by its Qt::UserRole numeric value instead of
// display text. Used for the Capacité column to avoid lexicographic ordering
// ("10.0 T" < "2.0 T" alphabetically but 10.0 > 2.0 numerically).
// ---------------------------------------------------------------------------
class NumericTableItem : public QTableWidgetItem
{
public:
    explicit NumericTableItem(const QString &display, double sortValue)
        : QTableWidgetItem(display)
    {
        setData(Qt::UserRole, sortValue);
        setTextAlignment(Qt::AlignCenter);
    }

    bool operator<(const QTableWidgetItem &other) const override
    {
        return data(Qt::UserRole).toDouble() < other.data(Qt::UserRole).toDouble();
    }
};

class CamionsPage : public QWidget
{
    Q_OBJECT

public:
    explicit CamionsPage(QWidget *parent = nullptr);
    ~CamionsPage();



private slots:
    // List page slots
    void onAddClicked();
    void onSearchChanged(const QString &text);
    void onSortChanged(int index);
    void onRefreshClicked();
    void onEditClicked(int id);
    void onViewClicked(int id);
    void onDeleteClicked(int id);
    void onDeleteSelectedClicked();

    // Form page slots
    void onSaveClicked();
    void onCancelClicked();

    // Export
    void exportToExcel();
    void exportToPdf();

    // Map slot
    void onMapClicked(double lat, double lon);
    void onMarkerMoved(int id, double lat, double lon);

    // Pagination
    void onPrevPage();
    void onNextPage();

private:
    // UI helpers
    void setupListPage();
    void setupFormPage();
    void setupTable();
    void applyStyles();
    void populateForm(int id);
    void updateTruckImage(const QString &type);


    // DB operations
    void loadFromDB();
    bool insertCamion(const QString &immat, const QString &type,
                      double capacite, const QString &zone,
                      const QString &statut, const QString &desc,
                      double lat, double lon, double kilo, double conso, const QString &rfid);
    bool updateCamion(int id, const QString &immat, const QString &type,
                      double capacite, const QString &zone,
                      const QString &statut, const QString &desc,
                      double lat, double lon, double kilo, double conso, const QString &rfid);
    bool deleteCamion(int id);

    // Row helpers
    void addTableRow(int id, const QString &immat, const QString &type,
                     double capacite, const QString &zone,
                     const QString &statut, const QString &desc,
                     double kilo, double conso, const QString &rfid);
    QWidget *createStatusBadge(const QString &statut) const;
    QWidget *createActionsWidget(int camionId);

    // Widgets
    QStackedWidget *stackedWidget = nullptr;
    QWidget *listPage = nullptr;
    QWidget *formPage = nullptr;

    // List page widgets
    QTableWidget  *table       = nullptr;
    QLineEdit     *searchEdit  = nullptr;
    QComboBox     *sortCombo   = nullptr;
    QComboBox     *sortOrderCombo = nullptr;
    QPushButton   *addBtn      = nullptr;
    QPushButton   *refreshBtn  = nullptr;
    QPushButton   *deleteSelectedBtn = nullptr;
    QPushButton   *exportBtn   = nullptr;

    // Pagination elements
    QPushButton *prevBtn      = nullptr;
    QPushButton *nextBtn      = nullptr;
    QLabel      *pageInfoLbl  = nullptr;
    

    // Form page widgets (input fields)
    QLabel *lblTitle = nullptr;
    QLineEdit *edImmat = nullptr;
    QLineEdit *edRFID = nullptr;
    QComboBox *cbType = nullptr;
    QDoubleSpinBox *sbCapacite = nullptr;
    QLineEdit *edZone = nullptr;
    QComboBox *cbStatut = nullptr;
    QLineEdit *edDesc = nullptr;
    QDoubleSpinBox *sbKilometrage = nullptr;
    QDoubleSpinBox *sbConsommation = nullptr;
    QLabel *lblCamionImg = nullptr;
    QPushButton *saveBtn = nullptr;
    QPushButton *cancelBtn = nullptr;


    // State variables for form
    int currentEditId = -1; // -1 for Add mode, valid ID for Edit mode
    bool isViewMode = false;   // true if only viewing (read-only)
    double currentLat = 0.0;
    double currentLon = 0.0;

    // Mini map for form
    QQuickWidget *formMapWidget = nullptr;

    // Pagination State
    int currentPage = 0;
    const int pageSize = 50;
    int totalRecords = 0;
};

#endif // CAMIONSPAGE_H
