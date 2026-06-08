#ifndef NAVIRESPAGE_H
#define NAVIRESPAGE_H

#include <QWidget>

class QComboBox;
class QLineEdit;
class QTableWidget;
class QPushButton;
class QWidget;

namespace Ui {
class NaviresPage;
}

class NaviresPage : public QWidget
{
    Q_OBJECT

public:
    explicit NaviresPage(QWidget *parent = nullptr);
    ~NaviresPage();

private:
    void setupTable();
    void applyStyles();
    void loadFromDb();
    QWidget *createActionsWidget(int idNavire);
    QWidget *createStatusBadge(const QString &status);
    void updateSort();
    void applySearchFilter();
    void refreshTable();
    void exportToPDF(const QString &filePath);
    void exportToCSV(const QString &filePath);

    void openAddDialog();
    void openEditDialog(int idNavire);
    void confirmDelete(int idNavire);

    Ui::NaviresPage *ui;

    QPushButton *addButton = nullptr;
    QPushButton *refreshButton = nullptr;
    QPushButton *exportButton = nullptr;
    QComboBox *sortCombo = nullptr;
    QComboBox *sortOrderCombo = nullptr;
    QComboBox *searchCombo = nullptr;
    QLineEdit *searchEdit = nullptr;
    QWidget *tableCard = nullptr;
    QTableWidget *table = nullptr;
};

#endif // NAVIRESPAGE_H
