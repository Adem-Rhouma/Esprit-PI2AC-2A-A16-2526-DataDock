#include "operationscamionspage.h"
#include "../../connection.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QToolButton>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFrame>
#include <QDate>
#include <QStackedWidget>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QIcon>
#include <QTimer>
#include <QFileDialog>
#include <QTextStream>
#include <QPrinter>
#include <QPainter>
#include <QFont>
#include <QMenu>
#include <QScrollBar>
#include <QRegularExpressionValidator>
#include <QRegularExpression>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────
OperationsCamionsPage::OperationsCamionsPage(QWidget *parent)
    : QWidget(parent), currentEditId(-1), isViewMode(false)
{
    setObjectName("OperationsCamionsPage");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(stackedWidget);

    setupListPage();
    setupFormPage();

    stackedWidget->addWidget(listPage);
    stackedWidget->addWidget(formPage);

    applyStyles();
    loadFromDB();
}

OperationsCamionsPage::~OperationsCamionsPage() {}

// ─────────────────────────────────────────────────────────────────────────────
// UI Setup
// ─────────────────────────────────────────────────────────────────────────────
void OperationsCamionsPage::setupListPage()
{
    listPage = new QWidget();
    auto *layout = new QVBoxLayout(listPage);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(20);

    // ── Header ───────────────────────────────────────────────────────────────
    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("Opérations Logistiques");
    title->setStyleSheet("font-family: 'Segoe UI', 'Helvetica Neue', Helvetica, Arial, sans-serif; font-size: 28px; font-weight: 900; color: #1B1B2F; letter-spacing: 1px;");

    refreshBtn = new QPushButton(QIcon(":/icons/refresh.png"), "Rafraîchir");
    deleteSelectedBtn = new QPushButton("Supprimer la sélection");
    deleteSelectedBtn->setObjectName("btnDeleteSelected");
    addBtn     = new QPushButton(QIcon(":/icons/add.png"),     "Nouvelle Opération");
    exportBtn  = new QPushButton(QIcon(":/icons/export.png"),  "Exporter ▾");

    connect(refreshBtn, &QPushButton::clicked, this, &OperationsCamionsPage::onRefreshClicked);
    connect(deleteSelectedBtn, &QPushButton::clicked, this, &OperationsCamionsPage::onDeleteSelectedClicked);
    connect(addBtn,     &QPushButton::clicked, this, &OperationsCamionsPage::onAddClicked);
    connect(exportBtn,  &QPushButton::clicked, this, [this]() {
        QMenu menu(this);
        menu.addAction("Exporter en PDF",   this, &OperationsCamionsPage::exportToPdf);
        menu.addAction("Exporter en Excel (CSV)", this, &OperationsCamionsPage::exportToExcel);
        menu.exec(exportBtn->mapToGlobal(exportBtn->rect().bottomLeft()));
    });

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(refreshBtn);
    headerLayout->addWidget(deleteSelectedBtn);
    headerLayout->addWidget(exportBtn);
    headerLayout->addWidget(addBtn);
    layout->addLayout(headerLayout);

    // ── Top bar (sort + search) ───────────────────────────────────────────────
    auto *topBar = new QHBoxLayout();
    topBar->setSpacing(10);

    auto *lblSort = new QLabel("Trier :");
    lblSort->setObjectName("lblBar");

    sortCombo = new QComboBox();
    sortCombo->addItems({"Camion", "Type", "Zone", "Date Début", "Date Fin", "Statut", "Priorité"});
    sortCombo->setFixedHeight(36);
    connect(sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OperationsCamionsPage::onSortChanged);

    sortOrderCombo = new QComboBox();
    sortOrderCombo->addItems({"↑ Asc", "↓ Desc"});
    sortOrderCombo->setFixedHeight(36);
    connect(sortOrderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OperationsCamionsPage::onSortChanged);

    auto *lblSearch = new QLabel("Recherche :");
    lblSearch->setObjectName("lblBar");

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Type, zone, statut, description...");
    searchEdit->setFixedHeight(36);
    searchEdit->setMinimumWidth(200);
    connect(searchEdit, &QLineEdit::textChanged, this, &OperationsCamionsPage::onSearchChanged);

    topBar->addStretch();
    topBar->addWidget(lblSort);
    topBar->addWidget(sortCombo);
    topBar->addWidget(sortOrderCombo);
    topBar->addSpacing(12);
    topBar->addWidget(lblSearch);
    topBar->addWidget(searchEdit);
    layout->addLayout(topBar);

    // ── Table card ───────────────────────────────────────────────────────────
    auto *card = new QFrame();
    card->setObjectName("tableCard");
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 12, 12, 12);

    table = new QTableWidget();
    table->setObjectName("tableOperations");
    cardLayout->addWidget(table);
    setupTable();

    layout->addWidget(card);

    // ── Pagination Controls ─────────────────────────────────────────────────
    auto *paginationLayout = new QHBoxLayout();
    prevBtn = new QPushButton("Précédent");
    nextBtn = new QPushButton("Suivant");
    pageInfoLbl = new QLabel("Page 1/1");
    pageInfoLbl->setStyleSheet("font-weight: bold; color: #555;");

    prevBtn->setFixedWidth(100);
    prevBtn->setObjectName("btnPagination");
    nextBtn->setFixedWidth(100);
    nextBtn->setObjectName("btnPagination");

    paginationLayout->addStretch();
    paginationLayout->addWidget(prevBtn);
    paginationLayout->addWidget(pageInfoLbl);
    paginationLayout->addWidget(nextBtn);
    paginationLayout->addStretch();
    layout->addLayout(paginationLayout);

    connect(prevBtn, &QPushButton::clicked, this, &OperationsCamionsPage::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &OperationsCamionsPage::onNextPage);
}

void OperationsCamionsPage::setupFormPage()
{
    formPage = new QWidget();
    formPage->setObjectName("formPage");
    auto *layout = new QVBoxLayout(formPage);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(20);

    lblTitle = new QLabel("Titre");
    lblTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #1B1B2F; margin-bottom: 20px;");
    layout->addWidget(lblTitle);

    auto *form = new QFormLayout();
    form->setSpacing(15);

    cbCamionImmat = new QComboBox(); cbCamionImmat->setFixedHeight(36);
    cbCamionImmat->setEditable(true);
    cbCamionImmat->setInsertPolicy(QComboBox::NoInsert);

    sbIdPeche  = new QSpinBox(); sbIdPeche->setRange(0, 99999);  sbIdPeche->setFixedHeight(36);
    sbIdPeche->setSpecialValueText("Aucune (0)");

    cbType = new QComboBox(); cbType->setFixedHeight(36);
    cbType->addItems({"Chargement", "Déchargement", "Transport", "Livraison", "Collecte", "Autre"});

    edZone = new QLineEdit(); edZone->setFixedHeight(36);
    edZone->setPlaceholderText("Zone de l'opération");
    edZone->setValidator(new QRegularExpressionValidator(QRegularExpression("[a-zA-Z0-9 ]+"), this));

    deDebut = new QDateTimeEdit(QDateTime::currentDateTime());
    deDebut->setCalendarPopup(true); deDebut->setFixedHeight(36);
    deDebut->setDisplayFormat("yyyy-MM-dd HH:mm");

    deFin = new QDateTimeEdit(QDateTime::currentDateTime());
    deFin->setCalendarPopup(true); deFin->setFixedHeight(36);
    deFin->setDisplayFormat("yyyy-MM-dd HH:mm");

    cbStatut = new QComboBox(); cbStatut->setFixedHeight(36);
    cbStatut->addItems({"Planifié", "En cours", "Terminé", "Annulé"});

    sbPriorite = new QSpinBox(); sbPriorite->setRange(1, 10); sbPriorite->setFixedHeight(36);
    sbPriorite->setValue(5);

    edDesc = new QLineEdit(); edDesc->setFixedHeight(36);
    edDesc->setPlaceholderText("Description de l'opération");

    form->addRow("Camion (Immat) :",   cbCamionImmat);
    form->addRow("ID Pêche (Réf) :",   sbIdPeche);
    form->addRow("Type d'Opération :", cbType);
    form->addRow("Zone * :",           edZone);
    form->addRow("Date Début :",       deDebut);
    form->addRow("Date Fin :",         deFin);
    form->addRow("Statut :",           cbStatut);
    form->addRow("Priorité (1-10) :",  sbPriorite);
    form->addRow("Description :",      edDesc);

    layout->addLayout(form);

    // Buttons sit directly below the last input field
    auto *btnLayout = new QHBoxLayout();
    btnSave   = new QPushButton("Enregistrer");
    btnCancel = new QPushButton("Retour");
    btnSave->setFixedHeight(40);
    btnCancel->setFixedHeight(40);

    connect(btnSave,   &QPushButton::clicked, this, &OperationsCamionsPage::onSaveClicked);
    connect(btnCancel, &QPushButton::clicked, this, &OperationsCamionsPage::onCancelClicked);

    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);
    layout->addLayout(btnLayout);

    layout->addStretch(); // fill remaining space below buttons
}

void OperationsCamionsPage::setupTable()
{
    table->setColumnCount(13);
    table->setHorizontalHeaderLabels({
        "ID Op.", "Camion ID", "ID Pêche", "#", "Camion", "Type Opération",
        "Zone Opération", "Date Début", "Date Fin",
        "Statut", "Priorité", "Description", "Actions"
    });

    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(false);
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(false);
    table->setFrameShape(QFrame::NoFrame);

    QHeaderView *h = table->horizontalHeader();
    h->setDefaultAlignment(Qt::AlignCenter);
    h->setSectionResizeMode(QHeaderView::ResizeToContents);
    h->setSectionResizeMode(11, QHeaderView::Stretch);   // Description stretches
    h->setSectionResizeMode(12, QHeaderView::ResizeToContents);
    table->setColumnHidden(0, true);
    table->setColumnHidden(1, true); // Camion ID
    table->setColumnHidden(2, true); // ID Pêche
}

void OperationsCamionsPage::applyStyles()
{
    setStyleSheet(
        // Page background
        "QWidget#OperationsCamionsPage { background: transparent; }"

        // Table card
        "QFrame#tableCard {"
        "  background: #ffffff;"
        "  border: 1px solid rgba(0,0,0,0.08);"
        "  border-radius: 14px;"
        "}"

        // Table
        "QTableWidget#tableOperations {"
        "  background: #ffffff;"
        "  alternate-background-color: #f8faff;"
        "  color: #1B1B2F;"
        "  border: none;"
        "  gridline-color: rgba(0,0,0,0.06);"
        "}"
        "QTableWidget#tableOperations::item { padding: 8px; }"
        "QTableWidget#tableOperations::item:selected { background: rgba(83,144,217,0.15); }"
        "QTableWidget#tableOperations::item:hover { background: rgba(86,207,225,0.12); }"
        "QHeaderView::section {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "    stop:0 #7400B8, stop:0.5 #5390D9, stop:1 #56CFE1);"
        "  color: #ffffff;"
        "  font-weight: 700;"
        "  padding: 10px;"
        "  border: none;"
        "}"
        "QTableCornerButton::section {"
        "  background: #7400B8; border: none;"
        "}"

        // Primary button
        "QPushButton#btnPrimary {"
        "  color: white;"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "    stop:0 #7400B8, stop:0.5 #5390D9, stop:1 #56CFE1);"
        "  border: none; border-radius: 10px; padding: 7px 18px;"
        "  font-weight: 600;"
        "}"
        "QPushButton#btnPrimary:hover {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "    stop:0 #6930C3, stop:1 #72EFDD);"
        "}"
        "QPushButton#btnPrimary:pressed { background: #5E60CE; }"

        // Secondary button
        "QPushButton#btnSecondary {"
        "  color: #5390D9;"
        "  background: rgba(83,144,217,0.1);"
        "  border: 1px solid #5390D9; border-radius: 10px; padding: 7px 18px;"
        "  font-weight: 600;"
        "}"
        "QPushButton#btnPagination {"
        "  color: #5390D9;"
        "  background: #ffffff;"
        "  border: 1px solid #d1d5db; border-radius: 10px; padding: 6px 16px;"
        "  font-weight: 600;"
        "}"
        "QPushButton#btnPagination:hover { background: #f9fafb; border-color: #5390D9; }"
        "QPushButton#btnPagination:disabled { background: #f3f4f6; color: #9ca3af; border-color: #e5e7eb; }"
        "QPushButton#btnSecondary:hover { background: rgba(83,144,217,0.2); }"

        // Delete Selected button
        "QPushButton#btnDeleteSelected {"
        "  color: white;"
        "  background: #ef4444;"
        "  border: none; border-radius: 10px; padding: 7px 18px;"
        "  font-weight: 600;"
        "}"
        "QPushButton#btnDeleteSelected:hover { background: #dc2626; }"
        "QPushButton#btnDeleteSelected:pressed { background: #991b1b; }"

        // Inputs (list page)
        "QLineEdit, QComboBox {"
        "  background: #ffffff; color: #1B1B2F;"
        "  border: 1px solid #5390D9; border-radius: 10px; padding: 6px 10px;"
        "}"
        "QLineEdit:focus, QComboBox:focus { border: 2px solid #48BFE3; }"
        "QComboBox::drop-down { width: 18px; border-left: 1px solid #5E60CE; }"
        "QComboBox QAbstractItemView { background: #ffffff; color: #1B1B2F; }"

        // Bar labels
        "QLabel#lblBar { color: #555; font-weight: 600; }"

        // Action buttons inside table
        "QToolButton {"
        "  background: rgba(116,0,184,0.08);"
        "  border: 1px solid rgba(116,0,184,0.5);"
        "  border-radius: 7px; min-width:28px; max-width:28px;"
        "  min-height:28px; max-height:28px; color:#5E60CE;"
        "}"
        "QToolButton:hover {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "    stop:0 #6930C3, stop:1 #72EFDD); color:white; border-color: transparent;"
        "}"

        // Form page
        "QWidget#formPage { background-color: #f8faff; }"
        "QLabel { color: #1B1B2F; }"
        "QPushButton {"
        "  background: #5390D9; color: white; border: none; border-radius: 8px;"
        "  padding: 8px 16px; font-weight: 600;"
        "}"
        "QPushButton:hover { background: #48BFE3; }"
        "QPushButton[text='Retour'] { background: #e5e7eb; color: #374151; }"
        "QPushButton[text='Retour']:hover { background: #d1d5db; }"
        "QLineEdit, QComboBox, QSpinBox, QDateEdit, QDateTimeEdit {"
        "  border: 1px solid #d1d5db; border-radius: 6px; padding: 5px 10px;"
        "  background: white; color: #000000;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDateEdit:focus, QDateTimeEdit:focus { border: 2px solid #5390D9; }"
        "QSpinBox, QDateEdit, QDateTimeEdit { color: #000000; }"

        // Export dropdown menu
        "QMenu {"
        "  background-color: #ffffff;"
        "  border: 1px solid #d1d5db;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "}"
        "QMenu::item {"
        "  color: #1B1B2F;"
        "  padding: 8px 20px;"
        "  border-radius: 6px;"
        "  font-weight: 500;"
        "}"
        "QMenu::item:selected {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "    stop:0 #7400B8, stop:0.5 #5390D9, stop:1 #56CFE1);"
        "  color: white;"
        "}"

        // Dark mini-windows (Dialogs / Message Boxes)
        "QDialog, QMessageBox {"
        "  background-color: #1E293B;"
        "  border: 1px solid #5390D9;"
        "  border-radius: 12px;"
        "}"
        "QDialog QLabel, QMessageBox QLabel {"
        "  color: #FFFFFF;"
        "  font-size: 14px;"
        "}"
        "QDialog QPushButton, QMessageBox QPushButton {"
        "  background-color: #5390D9;"
        "  color: #ffffff;"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 8px 16px;"
        "  font-weight: 600;"
        "  min-width: 80px;"
        "}"
        "QDialog QPushButton:hover, QMessageBox QPushButton:hover {"
        "  background-color: #48BFE3;"
        "}"
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// DB Operations
// ─────────────────────────────────────────────────────────────────────────────
void OperationsCamionsPage::loadFromDB()
{
    // Save scroll position for seamless experience
    int vScroll = table->verticalScrollBar()->value();
    table->setUpdatesEnabled(false);

    table->setSortingEnabled(false);
    table->setRowCount(0);

    // 1. Build Search Filter
    QString filter = searchEdit->text().trimmed();
    QString whereClause = "";
    if (!filter.isEmpty()) {
        whereClause = QString("WHERE (o.TypeOperation LIKE '%%1%' OR o.ZoneOperation LIKE '%%1%' "
                              "OR o.Statut LIKE '%%1%' OR o.Description LIKE '%%1%' "
                              "OR c.Immatriculation LIKE '%%1%') ").arg(filter);
    }

    // 2. Count total records for pagination
    QSqlQuery countQuery;
    countQuery.prepare("SELECT COUNT(*) FROM OperationsCamions o LEFT JOIN Camions c ON o.CamionId = c.CamionId " + whereClause);
    if (countQuery.exec() && countQuery.next()) {
        totalRecords = countQuery.value(0).toInt();
    } else {
        totalRecords = 0;
    }

    int totalPages = qMax(1, (totalRecords + pageSize - 1) / pageSize);
    if (currentPage >= totalPages) currentPage = totalPages - 1;
    if (currentPage < 0) currentPage = 0;

    // 3. Determine Sorting
    QString sortColumn = "o.OpCamId";
    int sortIdx = sortCombo->currentIndex();
    if (sortIdx == 0) sortColumn = "c.Immatriculation";
    else if (sortIdx == 1) sortColumn = "o.TypeOperation";
    else if (sortIdx == 2) sortColumn = "o.ZoneOperation";
    else if (sortIdx == 3) sortColumn = "o.DateDebut";
    else if (sortIdx == 4) sortColumn = "o.DateFin";
    else if (sortIdx == 5) sortColumn = "o.Statut";
    else if (sortIdx == 6) sortColumn = "o.Priorite";

    QString sortOrder = (sortOrderCombo->currentIndex() == 0) ? "ASC" : "DESC";

    // 4. Load Paginated Data with ROWNUM (Oracle subquery pagination)
    int offset = currentPage * pageSize;
    int maxRow = offset + pageSize;
    
    QString baseQueryStr = QString(
        "SELECT o.OpCamId, o.CamionId, o.idPeche, c.Immatriculation, o.TypeOperation, "
        "o.ZoneOperation, o.DateDebut, o.DateFin, o.Statut, o.Priorite, o.Description "
        "FROM OperationsCamions o "
        "LEFT JOIN Camions c ON o.CamionId = c.CamionId "
        "%1 ORDER BY %2 %3"
    ).arg(whereClause, sortColumn, sortOrder);
    
    QString sql = QString("SELECT * FROM ( "
                          "  SELECT a.*, ROWNUM rnum FROM ( %1 ) a "
                          "  WHERE ROWNUM <= %2 "
                          ") WHERE rnum > %3")
                      .arg(baseQueryStr).arg(maxRow).arg(offset);

    QSqlQuery q;
    if (!q.exec(sql)) {
        QMessageBox::warning(this, "Erreur BD",
            "Impossible de charger les opérations :\n" + q.lastError().text());
        return;
    }

    while (q.next()) {
        addTableRow(
            q.value(0).toInt(),
            q.value(1).toInt(),
            q.value(2).toInt(),
            q.value(3).toString(),
            q.value(4).toString(),
            q.value(5).toString(),
            q.value(6).toDateTime(),
            q.value(7).toDateTime(),
            q.value(8).toString(),
            q.value(9).toInt(),
            q.value(10).toString()
        );
    }

    // Update Pagination UI
    pageInfoLbl->setText(QString("Page %1 / %2 (%3 records)").arg(currentPage + 1).arg(totalPages).arg(totalRecords));
    prevBtn->setEnabled(currentPage > 0);
    nextBtn->setEnabled(currentPage < totalPages - 1);

    // Restore scroll position
    table->verticalScrollBar()->setValue(vScroll);
    table->setUpdatesEnabled(true);

    // Automatically sync with the AI predictor's data source
    syncToPredictorCSV();
}

bool OperationsCamionsPage::insertOperation(int camionId, int idPeche,
                                             const QString &typeOp, const QString &zoneOp,
                                             const QDateTime &dateDebut, const QDateTime &dateFin,
                                             const QString &statut, int priorite, const QString &desc)
{
    OperationCamion op(idPeche, camionId, typeOp, zoneOp, dateDebut, dateFin, statut, priorite, desc);
    return op.ajouter();
}

bool OperationsCamionsPage::updateOperation(int opId, int camionId, int idPeche,
                                             const QString &typeOp, const QString &zoneOp,
                                             const QDateTime &dateDebut, const QDateTime &dateFin,
                                             const QString &statut, int priorite, const QString &desc)
{
    OperationCamion op(idPeche, camionId, typeOp, zoneOp, dateDebut, dateFin, statut, priorite, desc);
    op.setOpId(opId);
    return op.modifier();
}

bool OperationsCamionsPage::deleteOperation(int opId)
{
    return OperationCamion::supprimer(opId);
}

// ─────────────────────────────────────────────────────────────────────────────
// Row helpers
// ─────────────────────────────────────────────────────────────────────────────
void OperationsCamionsPage::addTableRow(int opId, int camionId, int idPeche, const QString &immat,
                                         const QString &typeOp, const QString &zoneOp,
                                         const QDateTime &dateDebut, const QDateTime &dateFin,
                                         const QString &statut, int priorite, const QString &desc)
{
    const int row = table->rowCount();
    table->insertRow(row);
    table->setRowHeight(row, 50);

    auto cell = [&](int col, const QString &val) {
        auto *item = new QTableWidgetItem(val);
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, col, item);
    };

    // Numeric columns: store as int in UserRole for correct numeric sort
    auto numCell = [&](int col, int val) {
        auto *item = new QTableWidgetItem();
        item->setData(Qt::DisplayRole, val);
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, col, item);
    };

    numCell(0, opId);
    numCell(1, camionId);
    numCell(2, idPeche);
    numCell(3, (currentPage * pageSize) + row + 1); // Global record number
    cell(4, immat);
    cell(5, typeOp);
    cell(6, zoneOp);
    cell(7, dateDebut.toString("yyyy-MM-dd HH:mm"));
    cell(8, dateFin.toString("yyyy-MM-dd HH:mm"));

    // Statut badge (col 9)
    auto *sItem = new QTableWidgetItem(statut);
    sItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 9, sItem);
    table->setCellWidget(row, 9, createStatusBadge(statut));

    // Priorité badge (col 10)
    auto *pItem = new QTableWidgetItem();
    pItem->setData(Qt::DisplayRole, priorite);
    pItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 10, pItem);
    table->setCellWidget(row, 10, createPriorityBadge(priorite));

    cell(11, desc);

    table->setCellWidget(row, 12, createActionsWidget(opId));
}

QWidget *OperationsCamionsPage::createStatusBadge(const QString &statut) const
{
    QString color, bg;
    if (statut.compare("En cours", Qt::CaseInsensitive) == 0) {
        bg = "#dbeafe"; color = "#1e40af";
    } else if (statut.compare("Terminé", Qt::CaseInsensitive) == 0 ||
               statut.compare("Terminee", Qt::CaseInsensitive) == 0) {
        bg = "#d1fae5"; color = "#065f46";
    } else if (statut.compare("Annulé", Qt::CaseInsensitive) == 0 ||
               statut.compare("Annulée", Qt::CaseInsensitive) == 0) {
        bg = "#fee2e2"; color = "#991b1b";
    } else if (statut.compare("Planifié", Qt::CaseInsensitive) == 0 ||
               statut.compare("Planifiée", Qt::CaseInsensitive) == 0) {
        bg = "#fef3c7"; color = "#92400e";
    } else {
        bg = "#e5e7eb"; color = "#374151";
    }

    auto *wrapper = new QWidget();
    auto *layout  = new QHBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);

    auto *lbl = new QLabel(statut);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setStyleSheet(QString(
        "QLabel { background:%1; color:%2; border-radius:10px;"
        "         padding:3px 10px; font-weight:600; font-size:11px; }"
    ).arg(bg, color));
    layout->addWidget(lbl);
    return wrapper;
}

QWidget *OperationsCamionsPage::createPriorityBadge(int priorite) const
{
    QString bg, color, label;
    if (priorite >= 1 && priorite <= 3) {
        bg = "#d1fae5"; color = "#065f46"; label = QString("P%1 ▼").arg(priorite);
    } else if (priorite >= 4 && priorite <= 6) {
        bg = "#fef3c7"; color = "#92400e"; label = QString("P%1 ■").arg(priorite);
    } else {
        bg = "#fee2e2"; color = "#991b1b"; label = QString("P%1 ▲").arg(priorite);
    }

    auto *wrapper = new QWidget();
    auto *layout  = new QHBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);

    auto *lbl = new QLabel(label);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setStyleSheet(QString(
        "QLabel { background:%1; color:%2; border-radius:10px;"
        "         padding:3px 10px; font-weight:700; font-size:11px; }"
    ).arg(bg, color));
    layout->addWidget(lbl);
    return wrapper;
}

QWidget *OperationsCamionsPage::createActionsWidget(int opId)
{
    auto *wrapper = new QWidget();
    auto *layout  = new QHBoxLayout(wrapper);
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignCenter);

    auto *btnView = new QToolButton();
    auto *btnEdit = new QToolButton();
    auto *btnDel  = new QToolButton();

    btnView->setText("👁");   btnView->setToolTip("Consulter");
    btnEdit->setText("✏️");  btnEdit->setToolTip("Modifier");
    btnDel->setText("🗑");   btnDel->setToolTip("Supprimer");

    connect(btnView, &QToolButton::clicked, this, [=]() { onViewClicked(opId); });
    connect(btnEdit, &QToolButton::clicked, this, [=]() { onEditClicked(opId); });
    connect(btnDel,  &QToolButton::clicked, this, [=]() { onDeleteClicked(opId); });

    layout->addWidget(btnView);
    layout->addWidget(btnEdit);
    layout->addWidget(btnDel);

    return wrapper;
}
void OperationsCamionsPage::refreshCamionsCombo()
{
    cbCamionImmat->clear();
    cbCamionImmat->addItem("-- Sélectionner un camion --", -1);
    QSqlQuery q("SELECT CamionId, Immatriculation FROM Camions ORDER BY Immatriculation");
    while (q.next()) {
        cbCamionImmat->addItem(q.value(1).toString(), q.value(0).toInt());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Stacked Widget UI Interactions
// ─────────────────────────────────────────────────────────────────────────────

void OperationsCamionsPage::populateForm(int id)
{
    refreshCamionsCombo();

    cbCamionImmat->setCurrentIndex(0);
    sbIdPeche->setValue(0);
    cbType->setCurrentIndex(0);
    edZone->clear();
    deDebut->setDateTime(QDateTime::currentDateTime());
    deFin->setDateTime(QDateTime::currentDateTime());
    cbStatut->setCurrentIndex(0);
    sbPriorite->setValue(5);
    edDesc->clear();

    if (id != -1) {
        QSqlQuery q;
        q.prepare("SELECT CamionId, idPeche, TypeOperation, ZoneOperation, "
                  "DateDebut, DateFin, Statut, Priorite, Description "
                  "FROM OperationsCamions WHERE OpCamId = :id");
        q.bindValue(":id", id);
        if (q.exec() && q.next()) {
            int camionId = q.value(0).toInt();
            int idx = cbCamionImmat->findData(camionId);
            if (idx != -1) cbCamionImmat->setCurrentIndex(idx);

            sbIdPeche->setValue(q.value(1).toInt());
            cbType->setCurrentText(q.value(2).toString());
            edZone->setText(q.value(3).toString());
            deDebut->setDateTime(q.value(4).toDateTime());
            deFin->setDateTime(q.value(5).toDateTime());
            cbStatut->setCurrentText(q.value(6).toString());
            sbPriorite->setValue(q.value(7).toInt());
            edDesc->setText(q.value(8).toString());
        }
    }
}

void OperationsCamionsPage::onAddClicked()
{
    currentEditId = -1;
    isViewMode    = false;
    lblTitle->setText("Nouvelle Opération");
    populateForm(-1);

    cbCamionImmat->setEnabled(true); sbIdPeche->setReadOnly(false);
    cbType->setEnabled(true);       edZone->setReadOnly(false);
    deDebut->setReadOnly(false);    deFin->setReadOnly(false);
    cbStatut->setEnabled(true);     sbPriorite->setReadOnly(false);
    edDesc->setReadOnly(false);     btnSave->setVisible(true);

    stackedWidget->setCurrentWidget(formPage);
}

void OperationsCamionsPage::onEditClicked(int id)
{
    currentEditId = id;
    isViewMode    = false;
    lblTitle->setText("Modifier Opération");
    populateForm(id);

    cbCamionImmat->setEnabled(true); sbIdPeche->setReadOnly(false);
    cbType->setEnabled(true);       edZone->setReadOnly(false);
    deDebut->setReadOnly(false);    deFin->setReadOnly(false);
    cbStatut->setEnabled(true);     sbPriorite->setReadOnly(false);
    edDesc->setReadOnly(false);     btnSave->setVisible(true);

    stackedWidget->setCurrentWidget(formPage);
}

void OperationsCamionsPage::onViewClicked(int id)
{
    currentEditId = id;
    isViewMode    = true;
    lblTitle->setText("Détails Opération");
    populateForm(id);

    cbCamionImmat->setEnabled(false); sbIdPeche->setReadOnly(true);
    cbType->setEnabled(false);       edZone->setReadOnly(true);
    deDebut->setReadOnly(true);      deFin->setReadOnly(true);
    cbStatut->setEnabled(false);     sbPriorite->setReadOnly(true);
    edDesc->setReadOnly(true);       btnSave->setVisible(false);

    stackedWidget->setCurrentWidget(formPage);
}

void OperationsCamionsPage::onDeleteClicked(int id)
{
    auto reply = QMessageBox::question(
        this, "Confirmer la suppression",
        QString("Voulez-vous supprimer l'opération ID %1 ?").arg(id),
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        if (deleteOperation(id)) {
            // Defer reload so the cell-widget button slot fully returns first
            QTimer::singleShot(0, this, [this]() {
                loadFromDB();
                QMessageBox::information(this, "Succès", "Opération supprimée avec succès.");
            });
        } else {
            QMessageBox::critical(this, "Erreur", "La suppression a échoué.");
        }
    }
}

void OperationsCamionsPage::onDeleteSelectedClicked()
{
    QList<QTableWidgetItem*> selectedItems = table->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "Sélection vide", "Veuillez sélectionner au moins une opération à supprimer.");
        return;
    }

    // Get unique row indices
    QSet<int> rows;
    for (auto* item : selectedItems) {
        rows.insert(item->row());
    }

    auto reply = QMessageBox::question(
        this, "Confirmation",
        QString("Voulez-vous vraiment supprimer les %1 opérations sélectionnées ?").arg(rows.size()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    int successCount = 0;
    int failCount = 0;

    for (int row : rows) {
        int id = table->item(row, 0)->text().toInt();
        if (deleteOperation(id)) {
            successCount++;
        } else {
            failCount++;
        }
    }

    loadFromDB();

    if (failCount == 0) {
        QMessageBox::information(this, "Succès", QString("%1 opération(s) supprimée(s) avec succès.").arg(successCount));
    } else {
        QMessageBox::warning(this, "Suppression partielle",
            QString("%1 opération(s) supprimée(s), %2 échec(s).").arg(successCount).arg(failCount));
    }
}

void OperationsCamionsPage::onSaveClicked()
{
    if (isViewMode) {
        stackedWidget->setCurrentWidget(listPage);
        return;
    }

    if (deFin->dateTime() < deDebut->dateTime()) {
        QMessageBox::warning(this, "Validation",
                             "La date de fin doit être après ou égale à la date de début.");
        return;
    }

    QString zone = edZone->text().trimmed();
    QString desc = edDesc->text().trimmed();

    if (zone.isEmpty()) {
        QMessageBox::warning(this, "Validation", "La zone de l'opération est obligatoire.");
        edZone->setFocus();
        return;
    }

    // 1. Validate Camion Immatriculation
    int camionId = cbCamionImmat->currentData().toInt();
    QString immatTyped = cbCamionImmat->currentText().trimmed();

    if (camionId <= 0) {
        // Double check if the typed text exists in the database
        QSqlQuery qCam;
        qCam.prepare("SELECT CamionId FROM Camions WHERE Immatriculation = :immat");
        qCam.bindValue(":immat", immatTyped);
        if (qCam.exec() && qCam.next()) {
            camionId = qCam.value(0).toInt();
        } else {
            QMessageBox::warning(this, "Validation",
                                 "Le matricule du camion est invalide ou n'existe pas dans la base de données.");
            return;
        }
    }

    // 2. Validate ID Pêche (if provided)
    int idPecheRef = sbIdPeche->value();
    if (idPecheRef > 0) {
        QSqlQuery qPeche;
        qPeche.prepare("SELECT COUNT(*) FROM PECHES WHERE IDPECHE = :id");
        qPeche.bindValue(":id", idPecheRef);
        if (qPeche.exec() && qPeche.next()) {
            if (qPeche.value(0).toInt() == 0) {
                QMessageBox::warning(this, "Validation",
                                     QString("L'ID Pêche (Ref) %1 n'existe pas").arg(idPecheRef));
                return;
            }
        }
    }

    bool success = false;
    if (currentEditId == -1) {
        success = insertOperation(camionId, sbIdPeche->value(),
                                  cbType->currentText(), zone,
                                  deDebut->dateTime(),
                                  deFin->dateTime(),
                                  cbStatut->currentText(), sbPriorite->value(),
                                  desc);
        if (success) {
            QMessageBox::information(this, "Succès", "Opération ajoutée avec succès !");
        }
    } else {
        success = updateOperation(currentEditId, camionId, sbIdPeche->value(),
                                  cbType->currentText(), zone,
                                  deDebut->dateTime(),
                                  deFin->dateTime(),
                                  cbStatut->currentText(), sbPriorite->value(),
                                  desc);
        if (success) {
            QMessageBox::information(this, "Succès", "Opération modifiée avec succès !");
        }
    }

    if (success) {
        loadFromDB();
        stackedWidget->setCurrentWidget(listPage);
    } else {
        QMessageBox::critical(this, "Erreur", "L'opération en BD a échoué.");
    }
}

void OperationsCamionsPage::onCancelClicked()
{
    stackedWidget->setCurrentWidget(listPage);
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────

void OperationsCamionsPage::onSearchChanged(const QString &/*text*/)
{
    currentPage = 0;
    loadFromDB();
}

void OperationsCamionsPage::onRefreshClicked()
{
    // Total Reset
    currentPage = 0;
    searchEdit->clear();
    
    QSignalBlocker b1(sortCombo);
    QSignalBlocker b2(sortOrderCombo);
    sortCombo->setCurrentIndex(0);
    sortOrderCombo->setCurrentIndex(0);
    
    // Clear scroll position
    table->verticalScrollBar()->setValue(0);
    
    loadFromDB();
}

void OperationsCamionsPage::onSortChanged(int /*index*/)
{
    currentPage = 0;
    loadFromDB();
}

void OperationsCamionsPage::onPrevPage()
{
    if (currentPage > 0) {
        currentPage--;
        loadFromDB();
    }
}

void OperationsCamionsPage::onNextPage()
{
    int totalPages = qMax(1, (totalRecords + pageSize - 1) / pageSize);
    if (currentPage < totalPages - 1) {
        currentPage++;
        loadFromDB();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Export
// ─────────────────────────────────────────────────────────────────────────────
void OperationsCamionsPage::exportToExcel()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Exporter en Excel", "operations_camions.csv",
        "CSV Files (*.csv);;All Files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Erreur", "Impossible d'écrire dans le fichier.");
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Header row (Database names)
    out << "Immatriculation;TypeOperation;ZoneOperation;DateDebut;DateFin;Statut;Priorite;Description\n";

    // Data rows from database (Export All matching filter)
    QString filter = searchEdit->text().trimmed();
    QString whereClause = "";
    if (!filter.isEmpty()) {
        whereClause = QString("WHERE (o.TypeOperation LIKE '%%1%' OR o.ZoneOperation LIKE '%%1%' "
                              "OR o.Statut LIKE '%%1%' OR o.Description LIKE '%%1%' "
                              "OR c.Immatriculation LIKE '%%1%') ").arg(filter);
    }
    
    QSqlQuery q;
    QString queryStr = QString(
        "SELECT c.Immatriculation, o.TypeOperation, o.ZoneOperation, "
        "TO_CHAR(o.DateDebut, 'YYYY-MM-DD HH24:MI:SS'), TO_CHAR(o.DateFin, 'YYYY-MM-DD HH24:MI:SS'), "
        "o.Statut, o.Priorite, o.Description "
        "FROM OperationsCamions o "
        "LEFT JOIN Camions c ON o.CamionId = c.CamionId "
        "%1"
    ).arg(whereClause);
    
    if (q.exec(queryStr)) {
        while (q.next()) {
            QStringList row;
            for (int i = 0; i < 8; ++i) {
                QString val = q.value(i).toString();
                if (val.contains(';')) val = '"' + val + '"';
                row << val;
            }
            out << row.join(";") << "\n";
        }
    }
    file.close();

    QMessageBox::information(this, "Export réussi",
        QString("Fichier exporté avec succès :\n%1").arg(path));
}

void OperationsCamionsPage::exportToPdf()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Exporter en PDF", "operations_camions.pdf",
        "PDF Files (*.pdf);;All Files (*)");
    if (path.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setPageSize(QPageSize::A4);

    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::critical(this, "Erreur", "Impossible d'initialiser le PDF.");
        return;
    }

    const int dpi    = printer.resolution();
    const int margin = dpi / 4;   // ~6 mm
    QRect pageRect   = printer.pageRect(QPrinter::DevicePixel).toRect();
    int x = pageRect.left()  + margin;
    int y = pageRect.top()   + margin;
    int pageW = pageRect.width()  - 2 * margin;

    // Title
    QFont titleFont("Arial", 16, QFont::Bold);
    painter.setFont(titleFont);
    painter.drawText(x, y, "Rapport — Opérations Logistiques");
    y += dpi / 4;

    // --- Statistics Summary ---
    int globalCamions = 0;
    int opsMonth = 0;
    QSqlQuery qStats;
    if (qStats.exec("SELECT COUNT(*) FROM Camions")) {
        if (qStats.next()) globalCamions = qStats.value(0).toInt();
    }
    if (qStats.exec("SELECT COUNT(*) FROM OperationsCamions "
                    "WHERE EXTRACT(MONTH FROM DateDebut) = EXTRACT(MONTH FROM SYSDATE) "
                    "AND EXTRACT(YEAR FROM DateDebut) = EXTRACT(YEAR FROM SYSDATE)")) {
        if (qStats.next()) opsMonth = qStats.value(0).toInt();
    }

    int visibleOps = 0;
    QMap<QString, int> statusCounts;
    
    // We will do one pass query for everything matching
    QString filter = searchEdit->text().trimmed();
    QString whereClause = "";
    if (!filter.isEmpty()) {
        whereClause = QString("WHERE (o.TypeOperation LIKE '%%1%' OR o.ZoneOperation LIKE '%%1%' "
                              "OR o.Statut LIKE '%%1%' OR o.Description LIKE '%%1%' "
                              "OR c.Immatriculation LIKE '%%1%') ").arg(filter);
    }
    
    QSqlQuery q;
    QString queryStr = QString(
        "SELECT c.Immatriculation, o.TypeOperation, o.ZoneOperation, "
        "TO_CHAR(o.DateDebut, 'DD/MM/YYYY HH24:MI'), TO_CHAR(o.DateFin, 'DD/MM/YYYY HH24:MI'), "
        "o.Statut, o.Priorite, o.Description "
        "FROM OperationsCamions o "
        "LEFT JOIN Camions c ON o.CamionId = c.CamionId "
        "%1"
    ).arg(whereClause);
    
    // First pass to get stats (query result might be reused depending on ODBC cursor, but safe approach is to re-execute or store)
    QList<QStringList> allRows;
    if (q.exec(queryStr)) {
        while (q.next()) {
            visibleOps++;
            statusCounts[q.value(5).toString()]++;
            
            QStringList rowData;
            for (int i=0; i<8; i++) {
                rowData << q.value(i).toString();
            }
            allRows << rowData;
        }
    }

    QFont statsFont("Arial", 10, QFont::DemiBold);
    painter.setFont(statsFont);
    painter.setPen(QColor(0x4B, 0x55, 0x63));
    
    painter.drawText(x, y, QString("FLOTTE TOTALE: %1   |   OPÉRATIONS (MOIS): %2").arg(globalCamions).arg(opsMonth));
    y += dpi / 5;

    QString reportLine = QString("REPORT FILTRÉ: %1 opérations ( ").arg(visibleOps);
    for (auto it = statusCounts.begin(); it != statusCounts.end(); ++it) {
        reportLine += QString("%1: %2, ").arg(it.key().toUpper()).arg(it.value());
    }
    if (reportLine.endsWith(", ")) reportLine.chop(2);
    reportLine += " )";
    painter.drawText(x, y, reportLine);
    y += dpi / 4;
    // ---------------------------

    // Determine visible columns (skip 0,1,2 hidden + last Actions)
    QVector<int> cols;
    for (int c = 3; c < table->columnCount() - 1; ++c) cols << c;

    const int colW = pageW / cols.size();
    const int rowH = dpi / 6;

    // Header row
    QFont hdrFont("Arial", 9, QFont::Bold);
    painter.setFont(hdrFont);
    painter.setBrush(QColor(0x53, 0x90, 0xD9));
    painter.setPen(Qt::NoPen);
    painter.drawRect(x, y, pageW, rowH);
    painter.setPen(Qt::white);
    for (int i = 0; i < cols.size(); ++i) {
        QTableWidgetItem *hItem = table->horizontalHeaderItem(cols[i]);
        painter.drawText(QRect(x + i*colW + 4, y, colW - 8, rowH),
                         Qt::AlignVCenter | Qt::AlignLeft,
                         hItem ? hItem->text() : "");
    }
    y += rowH;

    // Data rows
    QFont dataFont("Arial", 8);
    painter.setFont(dataFont);
    bool alt = false;
    for (int r = 0; r < allRows.size(); ++r) {
        if (y + rowH > pageRect.bottom() - margin) {
            printer.newPage();
            y = pageRect.top() + margin;
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(alt ? QColor(0xf8, 0xfa, 0xff) : Qt::white);
        painter.drawRect(x, y, pageW, rowH);
        painter.setPen(QColor(0x1B, 0x1B, 0x2F));
        
        const QStringList &rowData = allRows[r];
        for (int i = 0; i < cols.size() && i < rowData.size(); ++i) {
            painter.drawText(QRect(x + i*colW + 4, y, colW - 8, rowH),
                             Qt::AlignVCenter | Qt::AlignLeft,
                             rowData[i]);
        }
        y += rowH;
        alt = !alt;
    }

    painter.end();
    QMessageBox::information(this, "Export réussi",
        QString("PDF exporté avec succès :\n%1").arg(path));
}
void OperationsCamionsPage::syncToPredictorCSV()
{
    // Robust search for the project root to find the assets folder
    QString root = QCoreApplication::applicationDirPath();
    QString path;
    bool found = false;

    // Search up to 5 levels up for the gestion_logistique folder
    QDir dir(root);
    for (int i = 0; i < 5; ++i) {
        if (dir.exists("gestion_logistique/assets/data")) {
            path = dir.absoluteFilePath("gestion_logistique/assets/data/operations_camions.csv");
            found = true;
            break;
        }
        if (!dir.cdUp()) break;
    }

    if (!found) {
        // Last chance effort: current working directory
        if (QDir("gestion_logistique/assets/data").exists()) {
            path = "gestion_logistique/assets/data/operations_camions.csv";
            found = true;
        }
    }

    if (!found) {
        qDebug() << "[AI SYNC] Skip: Data directory not found.";
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "[AI SYNC ERROR] Failed to open:" << path;
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Header (Primary Database names for AI compatibility)
    out << "Immatriculation;TypeOperation;ZoneOperation;DateDebut;DateFin;Statut;Priorite;Description\n";

    // Rows (from DB)
    QSqlQuery q;
    q.exec("SELECT c.Immatriculation, o.TypeOperation, o.ZoneOperation, "
           "TO_CHAR(o.DateDebut, 'YYYY-MM-DD HH24:MI:SS'), TO_CHAR(o.DateFin, 'YYYY-MM-DD HH24:MI:SS'), "
           "o.Statut, o.Priorite, o.Description "
           "FROM OperationsCamions o "
           "LEFT JOIN Camions c ON o.CamionId = c.CamionId");
    while (q.next()) {
        QStringList row;
        for (int c = 0; c < 8; ++c) {
            QString val = q.value(c).toString();
            if (val.contains(';')) val = '"' + val + '"';
            row << val;
        }
        out << row.join(";") << "\n";
    }
    file.close();
    qDebug() << "[AI SYNC] Successfully updated predictor data source:" << path;
}
