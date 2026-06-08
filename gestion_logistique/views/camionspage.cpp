#include "camionspage.h"
#include "../../connection.h"

#include <QVBoxLayout>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QToolButton>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QScrollArea>
#include <QFrame>
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
CamionsPage::CamionsPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("CamionsPage");

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

CamionsPage::~CamionsPage() {}

// ─────────────────────────────────────────────────────────────────────────────
// UI Setup
// ─────────────────────────────────────────────────────────────────────────────
void CamionsPage::setupListPage()
{
    listPage = new QWidget();
    auto *layout = new QVBoxLayout(listPage);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(20);

    auto *headerLayout = new QHBoxLayout();
    auto *title = new QLabel("Flotte de Camions");
    title->setStyleSheet("font-family: 'Segoe UI', 'Helvetica Neue', Helvetica, Arial, sans-serif; font-size: 28px; font-weight: 900; color: #1B1B2F; letter-spacing: 1px;");

    refreshBtn = new QPushButton(QIcon(":/icons/refresh.png"), "Rafraîchir");
    deleteSelectedBtn = new QPushButton("Supprimer la sélection");
    deleteSelectedBtn->setObjectName("btnDeleteSelected");
    addBtn = new QPushButton(QIcon(":/icons/add.png"), "Nouveau Camion");
    exportBtn = new QPushButton(QIcon(":/icons/export.png"), "Exporter ▾");

    connect(refreshBtn, &QPushButton::clicked, this, &CamionsPage::loadFromDB);
    connect(deleteSelectedBtn, &QPushButton::clicked, this, &CamionsPage::onDeleteSelectedClicked);
    connect(addBtn, &QPushButton::clicked, this, &CamionsPage::onAddClicked);
    connect(exportBtn, &QPushButton::clicked, this, [this]() {
        QMenu menu(this);
        menu.addAction("Exporter en PDF",          this, &CamionsPage::exportToPdf);
        menu.addAction("Exporter en Excel (CSV)", this, &CamionsPage::exportToExcel);
        menu.exec(exportBtn->mapToGlobal(exportBtn->rect().bottomLeft()));
    });

    headerLayout->addWidget(title);
    headerLayout->addStretch();
    headerLayout->addWidget(refreshBtn);
    headerLayout->addWidget(deleteSelectedBtn);
    headerLayout->addWidget(exportBtn);
    headerLayout->addWidget(addBtn);
    layout->addLayout(headerLayout);

    // ── Top bar (moved from old setupUI) ─────────────────────────────────────
    auto *topBar = new QHBoxLayout();
    topBar->setSpacing(10);

    auto *lblSort = new QLabel("Trier :");
    lblSort->setObjectName("lblBar");

    sortCombo = new QComboBox();
    sortCombo->addItems({"Immatriculation", "Type", "Capacité", "Zone", "Statut"});
    sortCombo->setFixedHeight(36);
    connect(sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CamionsPage::onSortChanged);

    sortOrderCombo = new QComboBox();
    sortOrderCombo->addItems({"↑ Asc", "↓ Desc"});
    sortOrderCombo->setFixedHeight(36);
    connect(sortOrderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CamionsPage::onSortChanged);

    auto *lblSearch = new QLabel("Recherche :");
    lblSearch->setObjectName("lblBar");

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Immatriculation, type, zone...");
    searchEdit->setFixedHeight(36);
    searchEdit->setMinimumWidth(200);
    connect(searchEdit, &QLineEdit::textChanged, this, &CamionsPage::onSearchChanged);

    topBar->addStretch(); // Add stretch to push sort/search to the right
    topBar->addWidget(lblSort);
    topBar->addWidget(sortCombo);
    topBar->addWidget(sortOrderCombo);
    topBar->addSpacing(12);
    topBar->addWidget(lblSearch);
    topBar->addWidget(searchEdit);
    layout->addLayout(topBar);

    // ── Table card ───────────────────────────────────────────────
    auto *card = new QFrame();
    card->setObjectName("tableCard");
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 12, 12, 12);

    table = new QTableWidget();
    table->setObjectName("tableCamions");
    
    cardLayout->addWidget(table);
    setupTable(); // Call setupTable here

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

    connect(prevBtn, &QPushButton::clicked, this, &CamionsPage::onPrevPage);
    connect(nextBtn, &QPushButton::clicked, this, &CamionsPage::onNextPage);
}

void CamionsPage::setupFormPage()
{
    formPage = new QWidget();
    formPage->setStyleSheet(
        "QWidget { background-color: #f8faff; }"
        "QPushButton {"
        "  background: #5390D9; color: white; border: none; border-radius: 8px;"
        "  padding: 8px 16px; font-weight: 600;"
        "}"
        "QPushButton:hover { background: #48BFE3; }"
        "QPushButton[text='Retour'] { background: #e5e7eb; color: #374151; }"
        "QPushButton[text='Retour']:hover { background: #d1d5db; }"
        "QLineEdit, QComboBox, QDoubleSpinBox {"
        "  border: 1px solid #d1d5db; border-radius: 6px; padding: 5px 10px;"
        "  background: white; color: #1B1B2F;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus { border: 2px solid #5390D9; }"
        "QDoubleSpinBox { color: #1B1B2F; }"
        "QLabel { color: #1B1B2F; }"
    );
    auto *layout = new QVBoxLayout(formPage);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(20);

    lblTitle = new QLabel("Titre");
    lblTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #1B1B2F; margin-bottom: 20px;");
    layout->addWidget(lblTitle);

    auto *form = new QFormLayout();
    form->setSpacing(15);

    edImmat = new QLineEdit(); edImmat->setFixedHeight(36);
    edImmat->setPlaceholderText("Ex: 123-TN-4567");
    // Format: up to 3 digits - 2 letters - up to 4 digits
    edImmat->setInputMask("999-AA-9999; ");

    edRFID = new QLineEdit(); edRFID->setFixedHeight(36);
    edRFID->setPlaceholderText("RFID Tag (8 hex chars)");
    edRFID->setMaxLength(8);
    edRFID->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9A-Fa-f]{8}"), this));

    cbType = new QComboBox(); cbType->setFixedHeight(36);
    cbType->addItems({"Standard", "Réfrigéré", "Citerne", "Porte-conteneurs", "Autre"});

    sbCapacite = new QDoubleSpinBox(); sbCapacite->setFixedHeight(36);
    sbCapacite->setRange(0.1, 1000.0); sbCapacite->setDecimals(1); sbCapacite->setSuffix(" T");

    edZone = new QLineEdit(); edZone->setFixedHeight(36);
    edZone->setPlaceholderText("Zone de stationnement");
    edZone->setValidator(new QRegularExpressionValidator(QRegularExpression("[a-zA-Z0-9 ]+"), this));

    cbStatut = new QComboBox(); cbStatut->setFixedHeight(36);
    cbStatut->addItems({"Disponible", "En service", "En maintenance", "En panne"});

    edDesc = new QLineEdit(); edDesc->setFixedHeight(36);
    edDesc->setPlaceholderText("Description du camion");

    sbKilometrage = new QDoubleSpinBox(); sbKilometrage->setFixedHeight(36);
    sbKilometrage->setRange(0.0, 10000000.0); sbKilometrage->setDecimals(1); sbKilometrage->setSuffix(" km");

    sbConsommation = new QDoubleSpinBox(); sbConsommation->setFixedHeight(36);
    sbConsommation->setRange(0.0, 10000000.0); sbConsommation->setDecimals(1); sbConsommation->setSuffix(" L");

    form->addRow("Immatriculation :", edImmat);
    form->addRow("RFID Tag :",        edRFID);
    form->addRow("Type :",            cbType);
    form->addRow("Capacité :",        sbCapacite);
    form->addRow("Kilométrage :",     sbKilometrage);
    form->addRow("Consommation :",    sbConsommation);
    form->addRow("Zone :",            edZone);
    form->addRow("Statut :",          cbStatut);
    form->addRow("Description :",     edDesc);

    lblCamionImg = new QLabel();
    lblCamionImg->setFixedSize(300, 200);
    lblCamionImg->setAlignment(Qt::AlignCenter);
    lblCamionImg->setStyleSheet("border: 1px dashed #5390D9; border-radius: 10px; background: rgba(83,144,217,0.05);");
    form->addRow("Aperçu :", lblCamionImg);

    connect(cbType, &QComboBox::currentTextChanged, this, &CamionsPage::updateTruckImage);

    formMapWidget = new QQuickWidget();
    formMapWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    formMapWidget->setSource(QUrl("qrc:/logistique/MapWidget.qml"));
    formMapWidget->setFixedSize(500, 300);
    form->addRow("Emplacement :", formMapWidget);

    if (formMapWidget->rootObject()) {
        connect(formMapWidget->rootObject(), SIGNAL(mapClicked(double,double)), this, SLOT(onMapClicked(double,double)));
        connect(formMapWidget->rootObject(), SIGNAL(markerMoved(int,double,double)), this, SLOT(onMarkerMoved(int,double,double)));
    }

    layout->addLayout(form);

    // Buttons sit directly below the last input field
    auto *btnLayout = new QHBoxLayout();
    auto *btnSave   = new QPushButton("Enregistrer");
    auto *btnCancel = new QPushButton("Retour");
    btnSave->setFixedHeight(40);
    btnCancel->setFixedHeight(40);

    connect(btnSave,   &QPushButton::clicked, this, &CamionsPage::onSaveClicked);
    connect(btnCancel, &QPushButton::clicked, this, &CamionsPage::onCancelClicked);

    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);
    layout->addLayout(btnLayout);

    layout->addStretch(); // fill remaining space below buttons
}


void CamionsPage::setupTable()
{
    table->setColumnCount(12);
    table->setHorizontalHeaderLabels({
        "ID", "#", "Immatriculation", "RFID Tag", "Type", "Capacité (T)",
        "Kilométrage (km)", "Consommation (L)",
        "Zone Stationnement", "Statut", "Description", "Actions"
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
    h->setSectionResizeMode(10, QHeaderView::Stretch);   // Description stretches
    h->setSectionResizeMode(11, QHeaderView::ResizeToContents);
    table->setColumnHidden(0, true);
}

void CamionsPage::applyStyles()
{
    setStyleSheet(
        // Page background
        "QWidget#CamionsPage { background: transparent; }"

        // Table card
        "QFrame#tableCard {"
        "  background: #ffffff;"
        "  border: 1px solid #e5e7eb;"
        "  border-radius: 14px;"
        "}"

        // Table
        "QTableWidget#tableCamions {"
        "  background: #ffffff;"
        "  alternate-background-color: #ffffff;"
        "  color: #000000;"
        "  border: none;"
        "  gridline-color: #e5e7eb;"
        "}"
        "QTableWidget#tableCamions::item { padding: 8px; color: #000000; }"
        "QTableWidget#tableCamions::item:selected { background: #f3f4f6; color: #000000; }"
        "QTableWidget#tableCamions::item:hover { background: #f9fafb; color: #000000; }"
        "QHeaderView::section {"
        "  background: #ffffff;"
        "  color: #000000;"
        "  font-weight: 700;"
        "  padding: 10px;"
        "  border: 1px solid #d1d5db;"
        "}"
        "QTableCornerButton::section {"
        "  background: #ffffff; border: none;"
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

        // Inputs
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
        // New styles for form page
        "QWidget#formPage { background-color: #f8faff; }"
        "QLabel { color: #1B1B2F; }"
        "QPushButton {"
        "  background: #5390D9; color: white; border: none; border-radius: 8px;"
        "  padding: 8px 16px; font-weight: 600;"
        "}"
        "QPushButton:hover { background: #48BFE3; }"
        "QPushButton[text='Retour'] { background: #e5e7eb; color: #374151; }"
        "QPushButton[text='Retour']:hover { background: #d1d5db; }"
        "QLineEdit, QComboBox, QDoubleSpinBox {"
        "  border: 1px solid #d1d5db; border-radius: 6px; padding: 5px 10px;"
        "  background: white; color: #000000;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus { border: 2px solid #5390D9; }"
        "QDoubleSpinBox { color: #000000; }"

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
void CamionsPage::loadFromDB()
{
    // Save scroll position for seamless experience
    int vScroll = table->verticalScrollBar()->value();
    table->setUpdatesEnabled(false);
    
    table->setSortingEnabled(false);
    table->setRowCount(0);

    // 1. Filter
    QString filter = searchEdit->text().trimmed();
    QString whereCl = "";
    if (!filter.isEmpty()) {
        whereCl = QString("WHERE (Immatriculation LIKE '%%1%' OR TypeCamion LIKE '%%1%' "
                          "OR ZoneStationnement LIKE '%%1%' OR Statut LIKE '%%1%' "
                          "OR Description LIKE '%%1%') ").arg(filter);
    }

    // 2. Count
    QSqlQuery countQuery;
    countQuery.prepare("SELECT COUNT(*) FROM Camions " + whereCl);
    if (countQuery.exec() && countQuery.next()) totalRecords = countQuery.value(0).toInt();
    else totalRecords = 0;

    int totalPages = qMax(1, (totalRecords + pageSize - 1) / pageSize);
    if (currentPage >= totalPages) currentPage = totalPages - 1;
    if (currentPage < 0) currentPage = 0;

    // 3. Sorting
    QString sortCol = "CamionId";
    int sIdx = sortCombo->currentIndex();
    if (sIdx == 0) sortCol = "Immatriculation";
    else if (sIdx == 1) sortCol = "TypeCamion";
    else if (sIdx == 2) sortCol = "CapaciteCharge";
    else if (sIdx == 3) sortCol = "ZoneStationnement";
    else if (sIdx == 4) sortCol = "Statut";

    QString sortOrd = (sortOrderCombo->currentIndex() == 0) ? "ASC" : "DESC";

    // 4. Query with Oracle ROWNUM Pagination (more compatible than 12c OFFSET/FETCH)
    int offset = currentPage * pageSize;
    int maxRow = offset + pageSize;
    
    QString baseSql = QString("SELECT CamionId, Immatriculation, TypeCamion, CapaciteCharge, "
                              "ZoneStationnement, Statut, Description, Kilometrage, Consommation_totale, rfid FROM Camions "
                              "%1 ORDER BY %2 %3")
                          .arg(whereCl, sortCol, sortOrd);
                          
    QString sql = QString("SELECT * FROM ( "
                          "  SELECT a.*, ROWNUM rnum FROM ( %1 ) a "
                          "  WHERE ROWNUM <= %2 "
                          ") WHERE rnum > %3")
                      .arg(baseSql).arg(maxRow).arg(offset);

    QSqlQuery q;
    if (!q.exec(sql)) {
        QMessageBox::warning(this, "Erreur BD", "Impossible de charger les camions :\n" + q.lastError().text());
        return;
    }

    while (q.next()) {
        addTableRow(
            q.value(0).toInt(),
            q.value(1).toString(),
            q.value(2).toString(),
            q.value(3).toDouble(),
            q.value(4).toString(),
            q.value(5).toString(),
            q.value(6).toString(),
            q.value(7).toDouble(),
            q.value(8).toDouble(),
            q.value(9).toString()
        );
    }

    // Update UI
    pageInfoLbl->setText(QString("Page %1 / %2 (%3) records").arg(currentPage + 1).arg(totalPages).arg(totalRecords));
    prevBtn->setEnabled(currentPage > 0);
    nextBtn->setEnabled(currentPage < totalPages - 1);

    // Restore scroll position
    table->verticalScrollBar()->setValue(vScroll);
    table->setUpdatesEnabled(true);
}

bool CamionsPage::insertCamion(const QString &immat, const QString &type,
                                double capacite, const QString &zone,
                                const QString &statut, const QString &desc,
                                double lat, double lon, double kilo, double conso, const QString &rfid)
{
    Camion c(0, immat, type, capacite, zone, statut, desc, lat, lon, kilo, conso, rfid);
    return c.ajouter();
}

bool CamionsPage::updateCamion(int id, const QString &immat, const QString &type,
                                double capacite, const QString &zone,
                                const QString &statut, const QString &desc,
                                double lat, double lon, double kilo, double conso, const QString &rfid)
{
    Camion c(id, immat, type, capacite, zone, statut, desc, lat, lon, kilo, conso, rfid);
    return c.modifier();
}

bool CamionsPage::deleteCamion(int id)
{
    return Camion::supprimer(id);
}

// ─────────────────────────────────────────────────────────────────────────────
// Row helpers
// ─────────────────────────────────────────────────────────────────────────────
void CamionsPage::addTableRow(int id, const QString &immat, const QString &type,
                               double capacite, const QString &zone,
                               const QString &statut, const QString &desc,
                               double kilo, double conso, const QString &rfid)
{
    const int row = table->rowCount();
    table->insertRow(row);
    table->setRowHeight(row, 50);

    auto cell = [&](int col, const QString &val) {
        auto *item = new QTableWidgetItem(val);
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, col, item);
    };

    cell(0, QString::number(id));
    cell(1, QString::number((currentPage * pageSize) + row + 1)); // Global index
    cell(2, immat);
    cell(3, rfid);
    cell(4, type);

    // Capacité: use NumericTableItem so operator< compares doubles, not strings.
    table->setItem(row, 5, new NumericTableItem(
        QString::number(capacite, 'f', 1) + " T", capacite));

    // Kilométrage
    table->setItem(row, 6, new NumericTableItem(
        QString::number(kilo, 'f', 1) + " km", kilo));

    // Consommation
    table->setItem(row, 7, new NumericTableItem(
        QString::number(conso, 'f', 1) + " L", conso));

    cell(8, zone);

    // Statut badge (col 9)
    auto *statusItem = new QTableWidgetItem(statut);
    statusItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 9, statusItem);
    table->setCellWidget(row, 9, createStatusBadge(statut));

    cell(10, desc);

    // Actions (col 11)
    table->setCellWidget(row, 11, createActionsWidget(id));
}

QWidget *CamionsPage::createStatusBadge(const QString &statut) const
{
    QString color, bg;
    if (statut.compare("Disponible", Qt::CaseInsensitive) == 0) {
        bg = "#d1fae5"; color = "#065f46";
    } else if (statut.compare("En service", Qt::CaseInsensitive) == 0) {
        bg = "#dbeafe"; color = "#1e40af";
    } else if (statut.compare("En maintenance", Qt::CaseInsensitive) == 0) {
        bg = "#fef3c7"; color = "#92400e";
    } else if (statut.compare("En panne", Qt::CaseInsensitive) == 0) {
        bg = "#fee2e2"; color = "#991b1b";
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
        "QLabel { background: %1; color: %2; border-radius: 10px;"
        "         padding: 3px 12px; font-weight: 600; font-size: 12px; }"
    ).arg(bg, color));

    layout->addWidget(lbl);
    return wrapper;
}

QWidget *CamionsPage::createActionsWidget(int id)
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
    btnDel->setText("🗑"); btnDel->setToolTip("Supprimer");

    // UI Connections
    connect(btnView, &QToolButton::clicked, this, [=]() { onViewClicked(id); });
    connect(btnEdit, &QToolButton::clicked, this, [=]() { onEditClicked(id); });
    connect(btnDel,  &QToolButton::clicked, this, [=]() { onDeleteClicked(id); });

    layout->addWidget(btnView);
    layout->addWidget(btnEdit);
    layout->addWidget(btnDel);

    return wrapper;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stacked Widget UI Interactions
// ─────────────────────────────────────────────────────────────────────────────

void CamionsPage::populateForm(int id)
{
    edImmat->clear();
    edRFID->clear();
    cbType->setCurrentIndex(0);
    sbCapacite->setValue(0);
    edZone->clear();
    cbStatut->setCurrentIndex(0);
    edDesc->clear();
    sbKilometrage->setValue(0);
    sbConsommation->setValue(0);

    currentLat = 0.0;
    currentLon = 0.0;
    if (formMapWidget && formMapWidget->rootObject()) {
        formMapWidget->rootObject()->setProperty("camionsModel", QVariantList());
    }

    if (id != -1) {
        QSqlQuery q;
        q.prepare("SELECT Immatriculation, TypeCamion, CapaciteCharge, ZoneStationnement, Statut, Description, Latitude, Longitude, Kilometrage, Consommation_totale, rfid FROM Camions WHERE CamionId = :id");
        q.bindValue(":id", id);
        if (q.exec() && q.next()) {
            edImmat->setText(q.value(0).toString());
            cbType->setCurrentText(q.value(1).toString());
            sbCapacite->setValue(q.value(2).toDouble());
            edZone->setText(q.value(3).toString());
            cbStatut->setCurrentText(q.value(4).toString());
            edDesc->setText(q.value(5).toString());
            currentLat = q.value(6).toDouble();
            currentLon = q.value(7).toDouble();
            sbKilometrage->setValue(q.value(8).toDouble());
            sbConsommation->setValue(q.value(9).toDouble());
            edRFID->setText(q.value(10).toString());
            
            if (currentLat != 0.0 || currentLon != 0.0) {
                QVariantList list;
                QVariantMap map;
                map["id"] = id;
                map["immat"] = q.value(0).toString();
                map["statut"] = q.value(4).toString();
                map["latitude"] = currentLat;
                map["longitude"] = currentLon;
                list.append(map);
                if (formMapWidget && formMapWidget->rootObject()) {
                    formMapWidget->rootObject()->setProperty("camionsModel", list);
                }
            }
        }
    }
    updateTruckImage(cbType->currentText());
}

void CamionsPage::updateTruckImage(const QString &type)
{
    // If type is "Autre", we don't display any image
    if (type.compare("Autre", Qt::CaseInsensitive) == 0) {
        lblCamionImg->clear();
        lblCamionImg->setText("Aucun aperçu");
        return;
    }

    QString baseDir = ":/logistique/";
    QString imgPath = "";

    if (type.contains("Citerne", Qt::CaseInsensitive)) {
        imgPath = baseDir + "camion_citerne.jpg";
    } else if (type.contains("conteneur", Qt::CaseInsensitive)) {
        imgPath = baseDir + "camion_conteneur.jpg";
    } else if (type.contains("Réfrigéré", Qt::CaseInsensitive) || type.contains("Refrigere", Qt::CaseInsensitive)) {
        imgPath = baseDir + "camion_refrigere.jpeg";
    } else if (type.contains("Standard", Qt::CaseInsensitive)) {
        imgPath = baseDir + "camion_standard.png";
    }

    if (!imgPath.isEmpty() && QFile::exists(imgPath)) {
        QPixmap pix(imgPath);
        if (!pix.isNull()) {
            lblCamionImg->setPixmap(pix.scaled(lblCamionImg->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            lblCamionImg->setText(""); 
            return;
        }
    }

    // Default case if no specific image found or loading failed
    lblCamionImg->clear();
    lblCamionImg->setText("Aperçu non disponible");
}

void CamionsPage::onAddClicked()
{
    currentEditId = -1;
    isViewMode = false;
    lblTitle->setText("Ajouter un Camion");
    populateForm(-1);

    edImmat->setReadOnly(false); edRFID->setReadOnly(false); cbType->setEnabled(true); sbCapacite->setEnabled(true);
    edZone->setReadOnly(false); cbStatut->setEnabled(true); edDesc->setReadOnly(false);
    sbKilometrage->setReadOnly(false); sbConsommation->setReadOnly(false);

    stackedWidget->setCurrentWidget(formPage);
}

void CamionsPage::onEditClicked(int id)
{
    currentEditId = id;
    isViewMode = false;
    lblTitle->setText("Modifier le Camion");
    populateForm(id);

    edImmat->setReadOnly(false); edRFID->setReadOnly(false); cbType->setEnabled(true); sbCapacite->setEnabled(true);
    edZone->setReadOnly(false); cbStatut->setEnabled(true); edDesc->setReadOnly(false);
    sbKilometrage->setReadOnly(false); sbConsommation->setReadOnly(false);

    stackedWidget->setCurrentWidget(formPage);
}

void CamionsPage::onViewClicked(int id)
{
    currentEditId = id;
    isViewMode = true;
    lblTitle->setText("Détails du Camion");
    populateForm(id);

    edImmat->setReadOnly(true); edRFID->setReadOnly(true); cbType->setEnabled(false); sbCapacite->setEnabled(false);
    edZone->setReadOnly(true); cbStatut->setEnabled(false); edDesc->setReadOnly(true);
    sbKilometrage->setReadOnly(true); sbConsommation->setReadOnly(true);

    stackedWidget->setCurrentWidget(formPage);
}

void CamionsPage::onDeleteClicked(int id)
{
    QSqlQuery q;
    q.prepare("SELECT Immatriculation FROM Camions WHERE CamionId=:id");
    q.bindValue(":id", id);
    QString immat;
    if (q.exec() && q.next()) immat = q.value(0).toString();

    auto reply = QMessageBox::question(
        nullptr, "Confirmation",
        QString("Voulez-vous vraiment supprimer le camion %1 (%2) ?").arg(immat, QString::number(id)),
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        if (deleteCamion(id)) {
            loadFromDB();
            QMessageBox::information(nullptr, "Succès", "Camion supprimé avec succès.");
        } else {
            QMessageBox::critical(nullptr, "Erreur", "La suppression a échoué. BD contrainte (cle etrangere) ?");
        }
    }
}

void CamionsPage::onDeleteSelectedClicked()
{
    QList<QTableWidgetItem*> selectedItems = table->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "Sélection vide", "Veuillez sélectionner au moins un camion à supprimer.");
        return;
    }

    // Get unique row indices
    QSet<int> rows;
    for (auto* item : selectedItems) {
        rows.insert(item->row());
    }

    auto reply = QMessageBox::question(
        this, "Confirmation",
        QString("Voulez-vous vraiment supprimer les %1 camions sélectionnés ?").arg(rows.size()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    int successCount = 0;
    int failCount = 0;
    QString lastError;

    for (int row : rows) {
        int id = table->item(row, 0)->text().toInt();
        if (deleteCamion(id)) {
            successCount++;
        } else {
            failCount++;
            lastError = "Erreur BD ou contrainte d'intégrité.";
        }
    }

    loadFromDB();

    if (failCount == 0) {
        QMessageBox::information(this, "Succès", QString("%1 camion(s) supprimé(s) avec succès.").arg(successCount));
    } else {
        QMessageBox::warning(this, "Suppression partielle",
            QString("%1 camion(s) supprimé(s), %2 échec(s).\n\nNOTE: Certains camions ne peuvent pas être supprimés s'ils sont liés à des opérations (contrainte de clé étrangère).").arg(successCount).arg(failCount));
    }
}

void CamionsPage::onSaveClicked()
{
    if (isViewMode) {
        stackedWidget->setCurrentWidget(listPage);
        return;
    }

    QString immat = edImmat->text().trimmed();
    QString rfid  = edRFID->text().trimmed().toUpper();
    QString zone  = edZone->text().trimmed();
    QString desc  = edDesc->text().trimmed();
    double cap    = sbCapacite->value();
    double kilo   = sbKilometrage->value();
    double conso  = sbConsommation->value();

    // Check generic format: XXX-LL-XXXX where L is a letter
    QRegularExpression re("^(\\d{1,3})-([a-zA-Z]{2})-(\\d{1,4})$");
    QRegularExpressionMatch match = re.match(immat);
    
    if (!match.hasMatch()) {
        QMessageBox::warning(this, "Validation", "Le format d'immatriculation doit être XXX-LL-XXXX (ex: 123-TN-4567).");
        edImmat->setFocus();
        return;
    }

    int p1 = match.captured(1).toInt();
    int p2 = match.captured(3).toInt();
    QString letters = match.captured(2).toUpper();

    if (p1 < 1 || p1 > 999) {
        QMessageBox::warning(this, "Validation", "La première partie (XXX) doit être comprise entre 1 et 999.");
        edImmat->setFocus();
        return;
    }
    if (p2 < 1 || p2 > 9999) {
        QMessageBox::warning(this, "Validation", "La deuxième partie (XXXX) doit être comprise entre 1 et 9999.");
        edImmat->setFocus();
        return;
    }

    // Reconstruct normalized immatriculation (e.g., 123-TN-4567)
    QString finalImmat = QString("%1-%2-%3").arg(p1).arg(letters).arg(p2);

    // Check for uniqueness in the database
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM Camions WHERE Immatriculation = :immat AND CamionId <> :id");
    checkQuery.bindValue(":immat", finalImmat);
    checkQuery.bindValue(":id", currentEditId);
    if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        QMessageBox::warning(this, "Validation", QString("L'immatriculation %1 existe déjà dans la base de données.").arg(finalImmat));
        edImmat->setFocus();
        return;
    }
    if (cap <= 0) {
        QMessageBox::warning(this, "Validation", "La capacité doit être supérieure à 0.");
        sbCapacite->setFocus();
        return;
    }
    if (zone.isEmpty()) {
        QMessageBox::warning(this, "Validation", "La zone est obligatoire.");
        edZone->setFocus();
        return;
    }

    bool success = false;
    if (currentEditId == -1) {
        success = insertCamion(finalImmat, cbType->currentText(),
                               cap, zone,
                               cbStatut->currentText(), desc, currentLat, currentLon, kilo, conso, rfid);
        if (success) {
            QMessageBox::information(this, "Succès", "Camion ajouté avec succès !");
        }
    } else {
        success = updateCamion(currentEditId, finalImmat, cbType->currentText(),
                               cap, zone,
                               cbStatut->currentText(), desc, currentLat, currentLon, kilo, conso, rfid);
        if (success) {
            QMessageBox::information(this, "Succès", "Camion modifié avec succès !");
        }
    }

    if (success) {
        loadFromDB();
        stackedWidget->setCurrentWidget(listPage);
    } else {
        QMessageBox::critical(this, "Erreur", "L'opération en BD a échoué.");
    }
}

void CamionsPage::onCancelClicked()
{
    stackedWidget->setCurrentWidget(listPage);
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────

void CamionsPage::onRefreshClicked()
{
    // Total Reset
    currentPage = 0;
    searchEdit->clear();
    
    QSignalBlocker b1(sortCombo);
    QSignalBlocker b2(sortOrderCombo);
    sortCombo->setCurrentIndex(0);
    sortOrderCombo->setCurrentIndex(0);
    
    // Clear scroll position by forcing it to top in loadFromDB or here
    table->verticalScrollBar()->setValue(0);
    
    loadFromDB();
}

void CamionsPage::onSearchChanged(const QString &/*text*/)
{
    currentPage = 0;
    loadFromDB();
}

void CamionsPage::onSortChanged(int /*index*/)
{
    currentPage = 0;
    loadFromDB();
}

void CamionsPage::onPrevPage()
{
    if (currentPage > 0) {
        currentPage--;
        loadFromDB();
    }
}

void CamionsPage::onNextPage()
{
    int totalPages = (totalRecords + pageSize - 1) / pageSize;
    if (currentPage < totalPages - 1) {
        currentPage++;
        loadFromDB();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Export
// ─────────────────────────────────────────────────────────────────────────────
void CamionsPage::exportToExcel()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Exporter en Excel", "camions.csv",
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
    out << "Immatriculation;rfid;TypeCamion;CapaciteCharge;ZoneStationnement;Statut;Description\n";

    // Data rows from database (Export All matching filter)
    QString filter = searchEdit->text().trimmed();
    QString whereCl = "";
    if (!filter.isEmpty()) {
        whereCl = QString("WHERE (Immatriculation LIKE '%%1%' OR rfid LIKE '%%1%' OR TypeCamion LIKE '%%1%' "
                          "OR ZoneStationnement LIKE '%%1%' OR Statut LIKE '%%1%' "
                          "OR Description LIKE '%%1%') ").arg(filter);
    }
    
    QSqlQuery q;
    q.prepare("SELECT Immatriculation, rfid, TypeCamion, CapaciteCharge, ZoneStationnement, Statut, Description FROM Camions " + whereCl);
    if (q.exec()) {
        while (q.next()) {
            QStringList row;
            for (int i = 0; i < 7; ++i) {
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

void CamionsPage::exportToPdf()
{
    QString path = QFileDialog::getSaveFileName(
        this, "Exporter en PDF", "rapport_camions.pdf",
        "PDF Files (*.pdf);;All Files (*)");
    if (path.isEmpty()) return;

    // ── Gather all statistics from DB ────────────────────────────────────────
    int totalCamions = 0, camionsDisponible = 0, camionsEnService = 0,
        camionsEnPanne = 0, camionsMaintenance = 0, camionsPlaced = 0,
        opsMonth = 0, opsTotal = 0;
    double totalCapacity = 0.0, maxCap = 0.0, minCap = std::numeric_limits<double>::max();
    QMap<QString, int> typeCounts, zoneCounts;

    QSqlQuery q;
    if (q.exec("SELECT COUNT(*) FROM Camions")) {
        if (q.next()) totalCamions = q.value(0).toInt();
    }
    if (q.exec("SELECT Statut, TypeCamion, ZoneStationnement, CapaciteCharge, Latitude, Longitude FROM Camions")) {
        while (q.next()) {
            QString statut = q.value(0).toString();
            if (statut == "Disponible")       camionsDisponible++;
            else if (statut == "En service")  camionsEnService++;
            else if (statut == "En panne")    camionsEnPanne++;
            else                              camionsMaintenance++;

            typeCounts[q.value(1).toString()]++;
            zoneCounts[q.value(2).toString()]++;

            double cap = q.value(3).toDouble();
            totalCapacity += cap;
            if (cap > maxCap) maxCap = cap;
            if (cap < minCap) minCap = cap;

            double lat = q.value(4).toDouble(), lon = q.value(5).toDouble();
            if (lat != 0.0 || lon != 0.0) camionsPlaced++;
        }
    }
    if (minCap == std::numeric_limits<double>::max()) minCap = 0.0;
    double avgCap = totalCamions > 0 ? totalCapacity / totalCamions : 0.0;

    if (q.exec("SELECT COUNT(*) FROM OperationsCamions "
               "WHERE EXTRACT(MONTH FROM DateDebut) = EXTRACT(MONTH FROM SYSDATE) "
               "AND EXTRACT(YEAR FROM DateDebut) = EXTRACT(YEAR FROM SYSDATE)")) {
        if (q.next()) opsMonth = q.value(0).toInt();
    }
    if (q.exec("SELECT COUNT(*) FROM OperationsCamions")) {
        if (q.next()) opsTotal = q.value(0).toInt();
    }

    // ── PDF Setup ─────────────────────────────────────────────────────────────
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageSize(QPageSize::A4);

    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::critical(this, "Erreur", "Impossible d'initialiser le PDF.");
        return;
    }

    const int dpi    = printer.resolution();
    const int margin = dpi / 4;
    QRect pageRect   = printer.pageRect(QPrinter::DevicePixel).toRect();
    int x  = pageRect.left() + margin;
    int y  = pageRect.top()  + margin;
    int pw = pageRect.width() - 2 * margin;

    // ── Header Banner ─────────────────────────────────────────────────────────
    painter.setBrush(QColor(0x1a, 0x3a, 0x6b));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(x, y, pw, dpi * 3 / 4, 12, 12);

    painter.setPen(Qt::white);
    QFont titleFont("Arial", 18, QFont::Bold);
    painter.setFont(titleFont);
    painter.drawText(QRect(x, y, pw, dpi / 2), Qt::AlignCenter, "Rapport Statistique — Flotte de Camions");

    QFont subFont("Arial", 9);
    painter.setFont(subFont);
    painter.drawText(QRect(x, y + dpi / 2, pw, dpi / 4),
                     Qt::AlignCenter,
                     QString("Généré le : %1").arg(QDate::currentDate().toString("dd/MM/yyyy")));
    y += dpi * 3 / 4 + dpi / 6;

    // ── Helper lambdas ────────────────────────────────────────────────────────
    auto drawSectionTitle = [&](const QString &txt) {
        y += dpi / 10;
        painter.setBrush(QColor(0xe8, 0xee, 0xf7));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(x, y, pw, dpi / 5, 6, 6);
        painter.setPen(QColor(0x1a, 0x3a, 0x6b));
        QFont sf("Arial", 11, QFont::Bold);
        painter.setFont(sf);
        painter.drawText(QRect(x + 8, y, pw - 16, dpi / 5), Qt::AlignVCenter, txt);
        y += dpi / 5 + dpi / 20;
    };

    auto drawStatRow = [&](const QString &label, const QString &value, bool highlight = false) {
        painter.setBrush(highlight ? QColor(0xf0, 0xf5, 0xff) : Qt::white);
        painter.setPen(QColor(0xe2, 0xe8, 0xf0));
        painter.drawRect(x, y, pw, dpi / 7);
        painter.setPen(QColor(0x37, 0x41, 0x51));
        QFont lf("Arial", 9); painter.setFont(lf);
        painter.drawText(QRect(x + 10, y, pw / 2, dpi / 7), Qt::AlignVCenter, label);
        QFont vf("Arial", 9, QFont::Bold); painter.setFont(vf);
        painter.setPen(QColor(0x1a, 0x3a, 0x6b));
        painter.drawText(QRect(x + pw / 2, y, pw / 2 - 10, dpi / 7), Qt::AlignVCenter | Qt::AlignRight, value);
        y += dpi / 7;
    };

    // ── Section 1 : Fleet Overview ────────────────────────────────────────────
    drawSectionTitle("1.  Vue d'ensemble de la flotte");
    drawStatRow("Nombre total de camions", QString::number(totalCamions), true);
    drawStatRow("Camions positionnés sur la carte", QString::number(camionsPlaced));
    drawStatRow("Capacité de charge totale (T)", QString::number(totalCapacity, 'f', 1), true);
    drawStatRow("Capacité moyenne (T)", QString::number(avgCap, 'f', 1));
    drawStatRow("Capacité maximale (T)", QString::number(maxCap, 'f', 1), true);
    drawStatRow("Capacité minimale (T)", QString::number(minCap, 'f', 1));

    // ── Section 2 : Status Breakdown ─────────────────────────────────────────
    drawSectionTitle("2.  Répartition par statut");

    auto pct = [&](int n) -> QString {
        return totalCamions > 0
            ? QString("%1  (%2%)").arg(n).arg(int(100.0 * n / totalCamions))
            : "0";
    };
    drawStatRow("Disponible",    pct(camionsDisponible), true);
    drawStatRow("En service",    pct(camionsEnService));
    drawStatRow("En panne",      pct(camionsEnPanne), true);
    drawStatRow("Maintenance",   pct(camionsMaintenance));

    // ── Section 3 : Type Breakdown ────────────────────────────────────────────
    drawSectionTitle("3.  Répartition par type de camion");
    bool alt = false;
    for (auto it = typeCounts.begin(); it != typeCounts.end(); ++it, alt = !alt) {
        drawStatRow(it.key(), pct(it.value()), alt);
    }

    // ── Section 4 : Zone Distribution ────────────────────────────────────────
    drawSectionTitle("4.  Répartition par zone de stationnement");
    alt = false;
    for (auto it = zoneCounts.begin(); it != zoneCounts.end(); ++it, alt = !alt) {
        drawStatRow(it.key().isEmpty() ? "(non définie)" : it.key(), pct(it.value()), alt);
    }

    // ── Section 5 : Operations ────────────────────────────────────────────────
    drawSectionTitle("5.  Opérations");
    drawStatRow("Total des opérations (toutes périodes)", QString::number(opsTotal), true);
    drawStatRow("Opérations ce mois-ci", QString::number(opsMonth));

    // ── Footer ────────────────────────────────────────────────────────────────
    y = pageRect.bottom() - margin / 2;
    painter.setPen(QColor(0x9c, 0xa3, 0xaf));
    QFont footFont("Arial", 7); painter.setFont(footFont);
    painter.drawText(QRect(x, y, pw, dpi / 6), Qt::AlignCenter,
                     "Port de Pêche Intelligent — Système de Gestion Logistique");

    painter.end();
    QMessageBox::information(this, "Export réussi",
        QString("Rapport PDF généré avec succès :\n%1").arg(path));
}

void CamionsPage::onMapClicked(double lat, double lon)
{
    if (isViewMode) return;
    currentLat = lat;
    currentLon = lon;
    
    QVariantList list;
    QVariantMap map;
    map["id"] = currentEditId == -1 ? 0 : currentEditId;
    map["immat"] = edImmat->text().isEmpty() ? "Nouveau" : edImmat->text();
    map["statut"] = cbStatut->currentText();
    map["latitude"] = lat;
    map["longitude"] = lon;
    list.append(map);
    
    if (formMapWidget && formMapWidget->rootObject()) {
        formMapWidget->rootObject()->setProperty("camionsModel", list);
    }
}

void CamionsPage::onMarkerMoved(int id, double lat, double lon)
{
    Q_UNUSED(id);
    onMapClicked(lat, lon);
}

