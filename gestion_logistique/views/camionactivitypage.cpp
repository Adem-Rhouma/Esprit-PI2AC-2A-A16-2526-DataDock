#include "camionactivitypage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>

CamionActivityPage::CamionActivityPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    applyStyles();
    loadFromDB();
}

void CamionActivityPage::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Header
    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("Historique d'Activité");
    title->setStyleSheet("font-family: 'Segoe UI', 'Helvetica Neue', Helvetica, Arial, sans-serif; font-size: 28px; font-weight: 900; color: #1B1B2F; letter-spacing: 1px;");

    refreshBtn = new QPushButton("Actualiser");
    refreshBtn->setFixedHeight(36);
    connect(refreshBtn, &QPushButton::clicked, this, &CamionActivityPage::loadFromDB);

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(refreshBtn);
    mainLayout->addLayout(headerLayout);

    // Search bar
    auto *searchBar = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Rechercher par matricule...");
    searchEdit->setFixedHeight(36);
    connect(searchEdit, &QLineEdit::textChanged, this, &CamionActivityPage::onSearchChanged);

    searchBar->addWidget(new QLabel("Recherche :"));
    searchBar->addWidget(searchEdit);
    searchBar->addStretch();
    mainLayout->addLayout(searchBar);

    // Main Content (Table + Console)
    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(15);

    // Table Container
    auto *tableContainer = new QVBoxLayout();
    table = new QTableWidget();
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"ID", "Matricule", "Type Mouvement", "Date & Heure"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setColumnHidden(0, true);
    tableContainer->addWidget(table);

    // Pagination
    auto *paginationLayout = new QHBoxLayout();
    prevBtn = new QPushButton("Précédent");
    nextBtn = new QPushButton("Suivant");
    pageInfoLbl = new QLabel("Page 1/1");
    pageInfoLbl->setStyleSheet("font-weight: bold; color: #1B1B2F;");

    prevBtn->setFixedWidth(100);
    nextBtn->setFixedWidth(100);

    paginationLayout->addStretch();
    paginationLayout->addWidget(prevBtn);
    paginationLayout->addWidget(pageInfoLbl);
    paginationLayout->addWidget(nextBtn);
    paginationLayout->addStretch();
    tableContainer->addLayout(paginationLayout);

    contentLayout->addLayout(tableContainer, 7); // Give table more space

    // Debug Console Container
    auto *debugLayout = new QVBoxLayout();
    auto *debugTitle = new QLabel("Console de Debugging Serial");
    debugTitle->setStyleSheet("font-weight: bold; color: #5390D9; font-size: 14px;");
    
    console = new QTextEdit();
    console->setReadOnly(true);
    console->setStyleSheet("background-color: #1e293b; color: #38bdf8; font-family: 'Consolas', 'Courier New'; font-size: 12px; border-radius: 8px; border: 1px solid #334155;");
    console->setPlaceholderText("Attente de données série...");
    
    debugLayout->addWidget(debugTitle);
    debugLayout->addWidget(console);
    
    contentLayout->addLayout(debugLayout, 3); // 30% width for logs

    mainLayout->addLayout(contentLayout);

    connect(prevBtn, &QPushButton::clicked, this, &CamionActivityPage::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &CamionActivityPage::onNextPage);
}

void CamionActivityPage::logMessage(const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    console->append(QString("[%1] %2").arg(timestamp, msg));
    console->verticalScrollBar()->setValue(console->verticalScrollBar()->maximum());
}

void CamionActivityPage::applyStyles()
{
    setStyleSheet(
        "QTableWidget { background-color: #ffffff; alternate-background-color: #f9fafb; color: #1B1B2F; border: 1px solid #d1d5db; border-radius: 10px; }"
        "QTableWidget::item { background-color: #ffffff; padding: 10px; }"
        "QTableWidget::item:selected { background-color: #e0f2fe; color: #0369a1; }"
        "QHeaderView::section { background-color: #f8faff; color: #1B1B2F; font-weight: bold; border: 1px solid #d1d5db; padding: 10px; }"
        "QPushButton { background: #5390D9; color: white; border: none; border-radius: 8px; padding: 8px 16px; font-weight: 600; }"
        "QPushButton:hover { background: #48BFE3; }"
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 6px; padding: 5px 10px; background: white; color: #1B1B2F; }"
        "QLabel { color: #1B1B2F; }"
    );
    table->setAlternatingRowColors(true);
}

void CamionActivityPage::loadFromDB()
{
    table->setRowCount(0);
    
    QString filter = searchEdit->text().trimmed();
    QString whereCl = "";
    if (!filter.isEmpty()) {
        whereCl = QString("WHERE (matricule LIKE '%%1%') ").arg(filter);
    }

    // Records count
    QSqlQuery countQuery;
    countQuery.prepare("SELECT COUNT(*) FROM camion_activity " + whereCl);
    if (countQuery.exec() && countQuery.next()) totalRecords = countQuery.value(0).toInt();
    
    int totalPages = qMax(1, (totalRecords + pageSize - 1) / pageSize);
    if (currentPage >= totalPages) currentPage = totalPages - 1;
    if (currentPage < 0) currentPage = 0;

    int offset = currentPage * pageSize;
    int maxRow = offset + pageSize;

    // PAGINATION (Oracle ROWNUM)
    QString baseSql = QString("SELECT id_activity, matricule, type_mouvement, date_heure FROM camion_activity %1 ORDER BY date_heure DESC").arg(whereCl);
    QString sql = QString("SELECT * FROM (SELECT a.*, ROWNUM rnum FROM (%1) a WHERE ROWNUM <= %2) WHERE rnum > %3").arg(baseSql).arg(maxRow).arg(offset);

    QSqlQuery q;
    if (!q.exec(sql)) {
        QMessageBox::warning(this, "Erreur BD", "Impossible de charger l'historique :\n" + q.lastError().text());
        return;
    }

    while (q.next()) {
        addTableRow(
            q.value(0).toInt(),
            q.value(1).toString(),
            q.value(2).toString(),
            q.value(3).toDateTime().toString("dd/MM/yyyy HH:mm:ss")
        );
    }

    pageInfoLbl->setText(QString("Page %1 / %2 (%3 enregistrements)").arg(currentPage + 1).arg(totalPages).arg(totalRecords));
    prevBtn->setEnabled(currentPage > 0);
    nextBtn->setEnabled(currentPage < totalPages - 1);
}

void CamionActivityPage::addTableRow(int id, const QString &matricule, const QString &type, const QString &dateTime)
{
    int row = table->rowCount();
    table->insertRow(row);
    table->setRowHeight(row, 50);

    auto *item0 = new QTableWidgetItem(QString::number(id));
    item0->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 0, item0);

    auto *item1 = new QTableWidgetItem(matricule);
    item1->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 1, item1);

    // Type Badge
    table->setCellWidget(row, 2, createTypeBadge(type));

    auto *item3 = new QTableWidgetItem(dateTime);
    item3->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 3, item3);
}

QWidget *CamionActivityPage::createTypeBadge(const QString &type) const
{
    QString bg = (type == "ENTREE") ? "#d1fae5" : "#fee2e2";
    QString color = (type == "ENTREE") ? "#065f46" : "#991b1b";

    auto *wrapper = new QWidget();
    auto *layout = new QHBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);

    auto *lbl = new QLabel(type);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setStyleSheet(QString(
        "QLabel { background: %1; color: %2; border-radius: 10px; padding: 4px 12px; font-weight: 600; font-size: 12px; }"
    ).arg(bg, color));

    layout->addWidget(lbl);
    return wrapper;
}

void CamionActivityPage::onRefreshClicked() { loadFromDB(); }
void CamionActivityPage::onSearchChanged(const QString &) { currentPage = 0; loadFromDB(); }
void CamionActivityPage::onPrevPage() { if (currentPage > 0) { currentPage--; loadFromDB(); } }
void CamionActivityPage::onNextPage() { if ((currentPage + 1) * pageSize < totalRecords) { currentPage++; loadFromDB(); } }
