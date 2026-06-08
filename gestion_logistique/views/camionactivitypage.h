#ifndef CAMIONACTIVITYPAGE_H
#define CAMIONACTIVITYPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QStackedWidget>
#include <QTextEdit>

class CamionActivityPage : public QWidget
{
    Q_OBJECT

public:
    explicit CamionActivityPage(QWidget *parent = nullptr);
    void logMessage(const QString &msg);

public slots:
    void loadFromDB();

private slots:
    void onRefreshClicked();
    void onSearchChanged(const QString &text);
    void onPrevPage();
    void onNextPage();

private:
    void setupUI();
    void applyStyles();
    void addTableRow(int id, const QString &matricule, const QString &type, const QString &dateTime);
    QWidget *createTypeBadge(const QString &type) const;

    QTableWidget *table;
    QTextEdit *console;
    QPushButton *refreshBtn;
    QLineEdit *searchEdit;
    
    // Pagination
    int currentPage = 0;
    const int pageSize = 50;
    int totalRecords = 0;
    QPushButton *prevBtn;
    QPushButton *nextBtn;
    QLabel *pageInfoLbl;
};

#endif // CAMIONACTIVITYPAGE_H
