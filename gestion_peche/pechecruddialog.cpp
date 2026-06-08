#include "pechecruddialog.h"
#include "ui_pechecruddialog.h"

#include "pechefontutils.h"
#include "model/peche.h"
#include "model/pechedao.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCompleter>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QRegularExpressionValidator>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStyledItemDelegate>
#include <QTableWidgetItem>
#include <QTextCursor>
#include <QStringList>

// ---------------------------------------------------------------------------
// ComboBoxDelegate  – shows a pre-populated QComboBox in any table column.
// Each item stores the DB ID as Qt::UserRole and the display label as
// Qt::DisplayRole / Qt::UserRole+1 is not used.
// ---------------------------------------------------------------------------
namespace {

class ComboBoxDelegate final : public QStyledItemDelegate
{
public:
    // items: list of (id, displayLabel)
    explicit ComboBoxDelegate(const QList<QPair<int, QString>> &items,
                              QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_items(items)
    {}

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &,
                          const QModelIndex &) const override
    {
        auto *combo = new QComboBox(parent);
        combo->setFrame(false);
        // Blank first entry so the user can't accidentally leave an "index 0 default"
        combo->addItem(tr("-- Sélectionner --"), QVariant(-1));
        for (const auto &pair : m_items) {
            combo->addItem(pair.second, pair.first);
        }
        return combo;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        auto *combo = qobject_cast<QComboBox *>(editor);
        if (!combo) return;
        // The table item stores the id in Qt::UserRole
        const int id = index.data(Qt::UserRole).toInt();
        const int idx = combo->findData(id);
        if (idx >= 0) {
            combo->setCurrentIndex(idx);
        } else {
            combo->setCurrentIndex(0); // blank sentinel
        }
    }

    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const override
    {
        auto *combo = qobject_cast<QComboBox *>(editor);
        if (!combo) return;
        const int id = combo->currentData().toInt();
        const QString label = combo->currentText();
        // Write label to DisplayRole and id to UserRole
        model->setData(index, label, Qt::DisplayRole);
        model->setData(index, id,    Qt::UserRole);
    }

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &) const override
    {
        editor->setGeometry(option.rect);
    }

private:
    QList<QPair<int, QString>> m_items;
};

// ---------------------------------------------------------------------------
// DoubleItemDelegate  – restricts a cell to a valid double value.
// ---------------------------------------------------------------------------
class DoubleItemDelegate final : public QStyledItemDelegate
{
public:
    explicit DoubleItemDelegate(double minValue,
                                double maxValue,
                                int decimals,
                                QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
        , m_min(minValue)
        , m_max(maxValue)
        , m_decimals(decimals)
    {}

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        QWidget *editor = QStyledItemDelegate::createEditor(parent, option, index);
        auto *line = qobject_cast<QLineEdit *>(editor);
        if (line) {
            auto *validator = new QDoubleValidator(m_min, m_max, m_decimals, line);
            validator->setNotation(QDoubleValidator::StandardNotation);
            validator->setLocale(QLocale::system());
            line->setValidator(validator);
        }
        return editor;
    }

private:
    double m_min = 0.0;
    double m_max = 0.0;
    int m_decimals = 2;
};

} // anonymous namespace


// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------
PecheCrudDialog::PecheCrudDialog(Mode mode, QWidget *parent)
    : PecheBaseDialog(parent)
    , ui(new Ui::PecheCrudDialog)
    , m_mode(mode)
{
    ui->setupUi(this);
    ui->tabWidget->setDocumentMode(true);
    setMinimumSize(600, 520);
    resize(600, 520);

    // Load lookup data FIRST so delegates are ready
    loadEspeces();
    loadEmployes();

    setupTables();
    setupErrorLabels();
    setupUiHints();
    setupValidators();
    setupLiveValidation();
    loadNavires();

    auto *saveButton = ui->buttonBox->button(QDialogButtonBox::Save);
    if (saveButton) {
        saveButton->setProperty("primary", true);
        saveButton->setDefault(true);
    }
    auto *cancelButton = ui->buttonBox->button(QDialogButtonBox::Cancel);
    if (cancelButton) {
        cancelButton->setProperty("secondary", true);
    }
    ui->buttonBox->setCenterButtons(false);
    ui->btnAddEspeceRow->setProperty("lineAction", true);
    ui->btnRemoveEspeceRow->setProperty("lineAction", true);
    ui->btnAddEmployeRow->setProperty("lineAction", true);
    ui->btnRemoveEmployeRow->setProperty("lineAction", true);

    setStyleSheet(QStringLiteral(
        "QDialog {"
        "  background-color: #0F172A;"
        "  color: #E2E8F0;"
        "  font-size: 13px;"
        "}"
        "QLabel#labelTitle {"
        "  font-size: 20px;"
        "  font-weight: 700;"
        "  color: #E2E8F0;"
        "}"
        "QLabel#labelSectionForm, QLabel#labelSectionDetails {"
        "  font-size: 14px;"
        "  font-weight: 700;"
        "  color: #E2E8F0;"
        "}"
        "QLabel {"
        "  color: #A0AEC0;"
        "  font-weight: 700;"
        "}"
        "QFrame#formFrame {"
        "  background-color: #1E293B;"
        "  border-radius: 16px;"
        "}"
        "QFrame#dividerLine {"
        "  color: #334155;"
        "  background-color: #334155;"
        "  min-height: 1px;"
        "  max-height: 1px;"
        "}"
        "QLineEdit, QDateEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {"
        "  background-color: #0F172A;"
        "  border: 1px solid #334155;"
        "  border-radius: 8px;"
        "  padding: 8px;"
        "  color: #E2E8F0;"
        "}"
        "QLineEdit:focus, QDateEdit:focus, QPlainTextEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {"
        "  border: 1px solid #3B82F6;"
        "}"
        "QLineEdit[invalid=\"true\"], QDateEdit[invalid=\"true\"], QPlainTextEdit[invalid=\"true\"], "
        "QComboBox[invalid=\"true\"], QTableWidget[invalid=\"true\"] {"
        "  border: 1px solid #EF4444;"
        "  background-color: #1F1115;"
        "}"
        "QLabel[errorLabel=\"true\"] {"
        "  color: #FCA5A5;"
        "  font-size: 11px;"
        "  padding-left: 4px;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 24px;"
        "}"
        "QComboBox::down-arrow {"
        "  image: none;"
        "  border-left: 5px solid transparent;"
        "  border-right: 5px solid transparent;"
        "  border-top: 6px solid #94A3B8;"
        "}"
        "QTabWidget::pane {"
        "  border: 1px solid #334155;"
        "  border-radius: 12px;"
        "  top: -1px;"
        "  background: #0F172A;"
        "}"
        "QTabBar::tab {"
        "  background: #1E293B;"
        "  color: #CBD5F5;"
        "  padding: 10px 20px;"
        "  margin-right: 6px;"
        "  border-radius: 10px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #3B82F6;"
        "  color: #FFFFFF;"
        "}"
        "QTableWidget {"
        "  background-color: #0F172A;"
        "  border: 1px solid #334155;"
        "  border-radius: 12px;"
        "  gridline-color: transparent;"
        "  color: #E2E8F0;"
        "  alternate-background-color: #0B1220;"
        "}"
        "QTableWidget::item {"
        "  padding: 6px;"
        "}"
        "QTableWidget::item:alternate {"
        "  background-color: #0B1220;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #1E40AF;"
        "  color: #FFFFFF;"
        "}"
        "QHeaderView::section {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #1E3A8A, stop:1 #1D4ED8);"
        "  color: #FFFFFF;"
        "  padding: 8px;"
        "  border: none;"
        "}"
        "QTableCornerButton::section {"
        "  background-color: #1E3A8A;"
        "  border: none;"
        "}"
        "QPushButton {"
        "  border: none;"
        "  border-radius: 10px;"
        "  padding: 10px 20px;"
        "  background: #334155;"
        "  color: #E2E8F0;"
        "}"
        "QPushButton:hover {"
        "  background: #475569;"
        "}"
        "QPushButton[primary=\"true\"] {"
        "  background: #3B82F6;"
        "  color: #FFFFFF;"
        "}"
        "QPushButton[primary=\"true\"]:hover {"
        "  background: #2563EB;"
        "}"
        "QPushButton[secondary=\"true\"] {"
        "  background: #334155;"
        "  color: #E2E8F0;"
        "}"
        "QPushButton[secondary=\"true\"]:hover {"
        "  background: #475569;"
        "}"
        "QPushButton[lineAction=\"true\"] {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1D4ED8, stop:1 #2563EB);"
        "  padding: 8px 16px;"
        "  border-radius: 8px;"
        "}"
        "QPushButton[lineAction=\"true\"]:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2563EB, stop:1 #3B82F6);"
        "}"
    ));

    if (m_mode == AddMode) {
        setWindowTitle(tr("Ajouter un lot de pêche"));
    } else {
        setWindowTitle(tr("Modifier un lot de pêche"));
    }

    connect(ui->btnAddEspeceRow,    &QPushButton::clicked, this, &PecheCrudDialog::onAddEspeceRow);
    connect(ui->btnRemoveEspeceRow, &QPushButton::clicked, this, &PecheCrudDialog::onRemoveEspeceRow);
    connect(ui->btnAddEmployeRow,   &QPushButton::clicked, this, &PecheCrudDialog::onAddEmployeRow);
    connect(ui->btnRemoveEmployeRow,&QPushButton::clicked, this, &PecheCrudDialog::onRemoveEmployeRow);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &PecheCrudDialog::onAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &PecheCrudDialog::reject);

    updateSaveEnabled();
}

PecheCrudDialog::PecheCrudDialog(Mode mode, int idPeche, QWidget *parent)
    : PecheCrudDialog(mode, parent)
{
    m_pecheId = idPeche;
    if (m_mode == EditMode && m_pecheId > 0) {
        loadPeche(m_pecheId);
    }
}

PecheCrudDialog::~PecheCrudDialog()
{
    delete ui;
}

int PecheCrudDialog::pecheId() const
{
    return m_pecheId;
}

// ---------------------------------------------------------------------------
// DB lookup tables
// ---------------------------------------------------------------------------
void PecheCrudDialog::loadEspeces()
{
    m_especesList.clear();
    QSqlQuery q;
    // Use NOMESPECE as the display label (confirmed from existing code in this module)
    q.prepare(QStringLiteral("SELECT IDESPECE, NOMESPECE FROM ESPECES ORDER BY NOMESPECE"));
    if (!q.exec()) {
        qDebug() << "PecheCrudDialog loadEspeces failed:" << q.lastError().text();
        return;
    }
    while (q.next()) {
        const int id     = q.value(0).toInt();
        const QString nm = q.value(1).toString().trimmed();
        m_especesList.append({ id, nm.isEmpty() ? QString::number(id) : nm });
    }
}

void PecheCrudDialog::loadEmployes()
{
    m_employesList.clear();
    QSqlQuery q;
    // EMPLOYES has NOM + PRENOM columns (confirmed from pechedetailsdialog.cpp)
    q.prepare(QStringLiteral(
        "SELECT IDEMPLOYE, TRIM(NVL(NOM,'') || ' ' || NVL(PRENOM,'')) AS FULLNAME "
        "FROM EMPLOYES ORDER BY NOM, PRENOM"));
    if (!q.exec()) {
        qDebug() << "PecheCrudDialog loadEmployes failed:" << q.lastError().text();
        return;
    }
    while (q.next()) {
        const int id     = q.value(0).toInt();
        const QString nm = q.value(1).toString().trimmed();
        m_employesList.append({ id, nm.isEmpty() ? QString::number(id) : nm });
    }
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void PecheCrudDialog::setupTables()
{
    // ---- Espèces table ----
    ui->tableEspeces->setColumnCount(3);
    ui->tableEspeces->setHorizontalHeaderLabels(
        { tr("Espèce"), tr("Quantité (kg)"), tr("Prix unitaire") });
    ui->tableEspeces->horizontalHeader()->setStretchLastSection(true);
    ui->tableEspeces->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableEspeces->setEditTriggers(QAbstractItemView::AllEditTriggers);
    ui->tableEspeces->setAlternatingRowColors(true);
    ui->tableEspeces->setRowCount(0);

    // Column 0 → ComboBoxDelegate for species selection
    ui->tableEspeces->setItemDelegateForColumn(
        0, new ComboBoxDelegate(m_especesList, ui->tableEspeces));
    // Columns 1 & 2 → double entry
    ui->tableEspeces->setItemDelegateForColumn(
        1, new DoubleItemDelegate(0.001, 1000000.0, 3, ui->tableEspeces));
    ui->tableEspeces->setItemDelegateForColumn(
        2, new DoubleItemDelegate(0.0,   1000000.0, 3, ui->tableEspeces));

    // ---- Employés table ----
    ui->tableEmployes->setColumnCount(2);
    ui->tableEmployes->setHorizontalHeaderLabels(
        { tr("Employé"), tr("Description") });
    ui->tableEmployes->horizontalHeader()->setStretchLastSection(true);
    ui->tableEmployes->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableEmployes->setEditTriggers(QAbstractItemView::AllEditTriggers);
    ui->tableEmployes->setAlternatingRowColors(true);
    ui->tableEmployes->setRowCount(0);

    // Column 0 → ComboBoxDelegate for employee selection
    ui->tableEmployes->setItemDelegateForColumn(
        0, new ComboBoxDelegate(m_employesList, ui->tableEmployes));
}

void PecheCrudDialog::setupErrorLabels()
{
    auto *form = ui->formLayout;
    auto makeErrorLabel = [](QWidget *parent, const char *name) {
        auto *label = new QLabel(parent);
        label->setObjectName(QLatin1String(name));
        label->setProperty("errorLabel", true);
        label->setWordWrap(true);
        label->setText(QString());
        label->setVisible(false);
        return label;
    };

    auto insertErrorRow = [&](QWidget *field, QLabel **outLabel, const char *name) {
        int row = 0;
        QFormLayout::ItemRole role = QFormLayout::FieldRole;
        form->getWidgetPosition(field, &row, &role);
        auto *empty = new QLabel(form->parentWidget());
        empty->setText(QString());
        empty->setMinimumWidth(10);
        auto *label = makeErrorLabel(form->parentWidget(), name);
        form->insertRow(row + 1, empty, label);
        if (outLabel) *outLabel = label;
    };

    insertErrorRow(ui->datePecheEdit,   &m_errorDate,        "labelDateError");
    insertErrorRow(ui->comboIdNavire,   &m_errorNavire,      "labelNavireError");
    insertErrorRow(ui->lineZonePeche,   &m_errorZone,        "labelZoneError");
    insertErrorRow(ui->plainDescription,&m_errorDescription, "labelDescriptionError");

    m_errorEspeces = makeErrorLabel(ui->tabEspeces,  "labelEspecesError");
    ui->tabEspecesLayout->insertWidget(1, m_errorEspeces);

    m_errorEmployes = makeErrorLabel(ui->tabEmployes, "labelEmployesError");
    ui->tabEmployesLayout->insertWidget(1, m_errorEmployes);
}

void PecheCrudDialog::setupUiHints()
{
    const QDate today = QDate::currentDate();
    ui->datePecheEdit->setDateRange(today.addYears(-2), today.addDays(1));
    if (!ui->datePecheEdit->date().isValid()) {
        ui->datePecheEdit->setDate(today);
    }
    ui->datePecheEdit->setToolTip(tr("Date de débarquement du lot (obligatoire)."));

    ui->comboIdNavire->setToolTip(tr("Navire associé au lot (obligatoire)."));

    ui->lineZonePeche->setPlaceholderText(tr("Ex : Zone Nord"));
    ui->lineZonePeche->setToolTip(tr("Zone de pêche (2 à 60 caractères)."));
    ui->lineZonePeche->setMaxLength(m_zoneMaxLen);
    const QStringList zoneSuggestions = {
        tr("Zone Nord"), tr("Zone Sud"), tr("Zone Est"),
        tr("Zone Ouest"), tr("Zone Côtière"), tr("Zone Hauturière")
    };
    auto *zoneCompleter = new QCompleter(zoneSuggestions, ui->lineZonePeche);
    zoneCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    zoneCompleter->setFilterMode(Qt::MatchContains);
    ui->lineZonePeche->setCompleter(zoneCompleter);

    ui->plainDescription->setPlaceholderText(tr("Statut ou description courte"));
    ui->plainDescription->setToolTip(
        tr("Description courte (max %1 caractères).").arg(m_descMaxLen));
}

void PecheCrudDialog::setupValidators()
{
    m_zoneRegex = QRegularExpression(
        QStringLiteral("^[\\p{L}\\d \\-'/\\.]{2,60}$"));
    auto *zoneValidator = new QRegularExpressionValidator(m_zoneRegex, ui->lineZonePeche);
    ui->lineZonePeche->setValidator(zoneValidator);
}

void PecheCrudDialog::setupLiveValidation()
{
    connect(ui->datePecheEdit,   &QDateEdit::dateChanged,
            this, &PecheCrudDialog::onFieldEdited);
    connect(ui->comboIdNavire,   &QComboBox::currentTextChanged,
            this, &PecheCrudDialog::onFieldEdited);
    connect(ui->lineZonePeche,   &QLineEdit::textChanged,
            this, &PecheCrudDialog::onFieldEdited);
    connect(ui->plainDescription,&QPlainTextEdit::textChanged,
            this, &PecheCrudDialog::onDescriptionChanged);
    connect(ui->tableEspeces,    &QTableWidget::itemChanged,
            this, &PecheCrudDialog::onFieldEdited);
    connect(ui->tableEmployes,   &QTableWidget::itemChanged,
            this, &PecheCrudDialog::onFieldEdited);
}

// ---------------------------------------------------------------------------
// Helpers: insert a pre-filled combo cell into a table row
// ---------------------------------------------------------------------------
void PecheCrudDialog::insertEspeceComboCell(int row, int col, int selectedId)
{
    auto *item = new QTableWidgetItem();
    if (selectedId > 0) {
        // Find the label for this id
        for (const auto &pair : std::as_const(m_especesList)) {
            if (pair.first == selectedId) {
                item->setText(pair.second);
                item->setData(Qt::UserRole, pair.first);
                break;
            }
        }
        if (item->data(Qt::UserRole).isNull()) {
            // Not found – show the raw ID as a fallback
            item->setText(QString::number(selectedId));
            item->setData(Qt::UserRole, selectedId);
        }
    } else {
        item->setText(QString());
        item->setData(Qt::UserRole, QVariant(-1));
    }
    ui->tableEspeces->setItem(row, col, item);
}

void PecheCrudDialog::insertEmployeComboCell(int row, int col, int selectedId)
{
    auto *item = new QTableWidgetItem();
    if (selectedId > 0) {
        for (const auto &pair : std::as_const(m_employesList)) {
            if (pair.first == selectedId) {
                item->setText(pair.second);
                item->setData(Qt::UserRole, pair.first);
                break;
            }
        }
        if (item->data(Qt::UserRole).isNull()) {
            item->setText(QString::number(selectedId));
            item->setData(Qt::UserRole, selectedId);
        }
    } else {
        item->setText(QString());
        item->setData(Qt::UserRole, QVariant(-1));
    }
    ui->tableEmployes->setItem(row, col, item);
}

// ---------------------------------------------------------------------------
// Navires
// ---------------------------------------------------------------------------
void PecheCrudDialog::loadNavires()
{
    QSqlQuery q;
    // Show navire name, store navire ID as item data (same pattern as espèces/employés)
    q.prepare("SELECT IDNAVIRE, NVL(NOMNAVIRE, CAST(IDNAVIRE AS VARCHAR2(20))) FROM NAVIRES ORDER BY NOMNAVIRE, IDNAVIRE");
    if (!q.exec()) {
        qDebug() << "PecheCrudDialog loadNavires failed:" << q.lastError().text();
        return;
    }
    ui->comboIdNavire->setEditable(false);
    ui->comboIdNavire->clear();
    // Blank sentinel so the widget starts with nothing selected
    ui->comboIdNavire->addItem(tr("-- Sélectionner un navire --"), QVariant(-1));
    while (q.next()) {
        const int id     = q.value(0).toInt();
        const QString nm = q.value(1).toString().trimmed();
        ui->comboIdNavire->addItem(nm.isEmpty() ? QString::number(id) : nm, id);
    }
    ui->comboIdNavire->setCurrentIndex(0); // start on the blank sentinel
}

// ---------------------------------------------------------------------------
// Load existing peche for Edit mode
// ---------------------------------------------------------------------------
void PecheCrudDialog::loadPeche(int idPeche)
{
    m_ignoreSignals = true;
    PecheDAO dao;
    Peche peche;
    QString err;
    if (!dao.loadById(idPeche, &peche, &err)) {
        qDebug() << "PecheCrudDialog loadPeche failed:" << err;
        m_ignoreSignals = false;
        return;
    }

    if (peche.getDatePeche().isValid()) {
        ui->datePecheEdit->setDate(peche.getDatePeche());
    }
    ui->lineZonePeche->setText(peche.getZonePeche());
    ui->plainDescription->setPlainText(peche.getDescription());
    // Select navire by its stored ID (item data)
    const int idxNav = ui->comboIdNavire->findData(peche.getIdNavire());
    ui->comboIdNavire->setCurrentIndex(idxNav >= 0 ? idxNav : 0);

    // Load especes, using ComboBoxDelegate-compatible items
    QSqlQuery qEspeces;
    qEspeces.prepare(
        "SELECT IDESPECE, QUANTITE, PRIXUNITAIRE FROM PECHESESPECES WHERE IDPECHE = :id");
    qEspeces.bindValue(":id", idPeche);
    if (qEspeces.exec()) {
        ui->tableEspeces->blockSignals(true);
        while (qEspeces.next()) {
            const int idEspece   = qEspeces.value(0).toInt();
            const QString qte    = qEspeces.value(1).toString();
            const QString prix   = qEspeces.value(2).toString();
            const int row = ui->tableEspeces->rowCount();
            ui->tableEspeces->insertRow(row);
            // Col 0: combo cell (displays species name, stores id in UserRole)
            insertEspeceComboCell(row, 0, idEspece);
            // Cols 1 & 2: plain text
            ui->tableEspeces->setItem(row, 1, new QTableWidgetItem(qte));
            ui->tableEspeces->setItem(row, 2, new QTableWidgetItem(prix));
        }
        ui->tableEspeces->blockSignals(false);
    } else {
        qDebug() << "PecheCrudDialog load especes failed:" << qEspeces.lastError().text();
    }

    // Load employes
    QSqlQuery qEmployes;
    qEmployes.prepare(
        "SELECT IDEMPLOYE, DESCRIPTION FROM PECHESEMPLOYES WHERE IDPECHE = :id");
    qEmployes.bindValue(":id", idPeche);
    if (qEmployes.exec()) {
        ui->tableEmployes->blockSignals(true);
        while (qEmployes.next()) {
            const int idEmploye  = qEmployes.value(0).toInt();
            const QString desc   = qEmployes.value(1).toString();
            const int row = ui->tableEmployes->rowCount();
            ui->tableEmployes->insertRow(row);
            // Col 0: combo cell (displays employee name, stores id in UserRole)
            insertEmployeComboCell(row, 0, idEmploye);
            ui->tableEmployes->setItem(row, 1, new QTableWidgetItem(desc));
        }
        ui->tableEmployes->blockSignals(false);
    } else {
        qDebug() << "PecheCrudDialog load employes failed:" << qEmployes.lastError().text();
    }

    m_ignoreSignals = false;
    updateSaveEnabled();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------
void PecheCrudDialog::onAddEspeceRow()
{
    const int row = ui->tableEspeces->rowCount();
    ui->tableEspeces->insertRow(row);
    // Pre-create the combo cell so the delegate opens correctly on first click
    insertEspeceComboCell(row, 0, -1);
    updateSaveEnabled();
}

void PecheCrudDialog::onRemoveEspeceRow()
{
    const int row = ui->tableEspeces->currentRow();
    if (row >= 0) {
        ui->tableEspeces->removeRow(row);
    }
    updateSaveEnabled();
}

void PecheCrudDialog::onAddEmployeRow()
{
    const int row = ui->tableEmployes->rowCount();
    ui->tableEmployes->insertRow(row);
    insertEmployeComboCell(row, 0, -1);
    updateSaveEnabled();
}

void PecheCrudDialog::onRemoveEmployeRow()
{
    const int row = ui->tableEmployes->currentRow();
    if (row >= 0) {
        ui->tableEmployes->removeRow(row);
    }
    updateSaveEnabled();
}

void PecheCrudDialog::onFieldEdited()
{
    if (m_ignoreSignals) return;
    updateSaveEnabled();
}

void PecheCrudDialog::onDescriptionChanged()
{
    if (m_ignoreSignals) return;

    const QString text = ui->plainDescription->toPlainText();
    if (text.size() > m_descMaxLen) {
        m_ignoreSignals = true;
        const QString trimmed = text.left(m_descMaxLen);
        ui->plainDescription->setPlainText(trimmed);
        QTextCursor cursor = ui->plainDescription->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->plainDescription->setTextCursor(cursor);
        m_ignoreSignals = false;
    }
    updateSaveEnabled();
}

void PecheCrudDialog::onAccept()
{
    if (save()) {
        accept();
    }
}

// ---------------------------------------------------------------------------
// Error UI helpers
// ---------------------------------------------------------------------------
void PecheCrudDialog::applyErrors(const QMap<QWidget *, QString> &errors)
{
    applyErrorState(ui->datePecheEdit,    m_errorDate,        errors.value(ui->datePecheEdit));
    applyErrorState(ui->comboIdNavire,    m_errorNavire,      errors.value(ui->comboIdNavire));
    applyErrorState(ui->lineZonePeche,    m_errorZone,        errors.value(ui->lineZonePeche));
    applyErrorState(ui->plainDescription, m_errorDescription, errors.value(ui->plainDescription));
    applyErrorState(ui->tableEspeces,     m_errorEspeces,     errors.value(ui->tableEspeces));
    applyErrorState(ui->tableEmployes,    m_errorEmployes,    errors.value(ui->tableEmployes));
}

void PecheCrudDialog::applyErrorState(QWidget *field, QLabel *label, const QString &message)
{
    if (!field || !label) return;
    const bool hasError = !message.isEmpty();
    label->setText(message);
    label->setVisible(hasError);
    field->setProperty("invalid", hasError);
    field->style()->unpolish(field);
    field->style()->polish(field);
    field->update();
}

void PecheCrudDialog::clearErrorState(QWidget *field, QLabel *label)
{
    applyErrorState(field, label, QString());
}

void PecheCrudDialog::focusFirstInvalid(const QMap<QWidget *, QString> &errors)
{
    const QList<QWidget *> order = {
        ui->datePecheEdit,
        ui->comboIdNavire,
        ui->lineZonePeche,
        ui->plainDescription,
        ui->tableEspeces,
        ui->tableEmployes
    };
    for (QWidget *field : order) {
        if (!errors.contains(field)) continue;
        if (field == ui->tableEspeces) {
            ui->tabWidget->setCurrentWidget(ui->tabEspeces);
            if (ui->tableEspeces->rowCount() > 0) ui->tableEspeces->setCurrentCell(0, 0);
        } else if (field == ui->tableEmployes) {
            ui->tabWidget->setCurrentWidget(ui->tabEmployes);
            if (ui->tableEmployes->rowCount() > 0) ui->tableEmployes->setCurrentCell(0, 0);
        }
        field->setFocus(Qt::TabFocusReason);
        break;
    }
}

// ---------------------------------------------------------------------------
// Live save-button state
// ---------------------------------------------------------------------------
void PecheCrudDialog::updateSaveEnabled()
{
    if (m_ignoreSignals) return;
    const QMap<QWidget *, QString> errors = validateAll(false);
    applyErrors(errors);
    auto *saveButton = ui->buttonBox->button(QDialogButtonBox::Save);
    if (saveButton) {
        saveButton->setEnabled(errors.isEmpty());
    }
}

// ---------------------------------------------------------------------------
// Normalization / parsing helpers
// ---------------------------------------------------------------------------
QString PecheCrudDialog::normalizeText(const QString &text) const
{
    QString s = text.trimmed();
    s.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    return s;
}

bool PecheCrudDialog::parseDouble(const QString &text, double *out) const
{
    if (!out) return false;
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return false;
    bool ok = false;
    double value = QLocale::system().toDouble(trimmed, &ok);
    if (!ok) {
        QString alt = trimmed;
        alt.replace(',', '.');
        value = alt.toDouble(&ok);
    }
    if (!ok) return false;
    *out = value;
    return true;
}

bool PecheCrudDialog::parseInt(const QString &text, int *out) const
{
    if (!out) return false;
    const QString trimmed = text.trimmed();
    bool ok = false;
    int value = trimmed.toInt(&ok);
    if (!ok) value = QLocale::system().toInt(trimmed, &ok);
    if (!ok) return false;
    *out = value;
    return true;
}

void PecheCrudDialog::applyNormalizedInputs()
{
    const QString nZone = normalizeText(ui->lineZonePeche->text());
    if (ui->lineZonePeche->text() != nZone) ui->lineZonePeche->setText(nZone);

    const QString nDesc = normalizeText(ui->plainDescription->toPlainText());
    if (ui->plainDescription->toPlainText() != nDesc) {
        ui->plainDescription->setPlainText(nDesc);
        QTextCursor cursor = ui->plainDescription->textCursor();
        cursor.movePosition(QTextCursor::End);
        ui->plainDescription->setTextCursor(cursor);
    }
}

// ---------------------------------------------------------------------------
// Row readers – col 0 is now a combo cell: id is in Qt::UserRole
// ---------------------------------------------------------------------------
bool PecheCrudDialog::readEspeceRow(int row, int &idEspece, double &quantite, double &prix, QString &err) const
{
    const auto *colItem  = ui->tableEspeces->item(row, 0);
    const auto *qteItem  = ui->tableEspeces->item(row, 1);
    const auto *prixItem = ui->tableEspeces->item(row, 2);

    // Read the FK from UserRole (set by the delegate or insertEspeceComboCell)
    const int idFromRole = colItem ? colItem->data(Qt::UserRole).toInt() : -1;
    const QString qteText  = qteItem  ? qteItem->text().trimmed()  : QString();
    const QString prixText = prixItem ? prixItem->text().trimmed() : QString();

    // Completely empty / unselected row → silent skip
    if (idFromRole <= 0 && qteText.isEmpty() && prixText.isEmpty()) {
        return false;
    }

    // Species not selected
    if (idFromRole <= 0) {
        err = tr("Espèce non sélectionnée à la ligne %1.").arg(row + 1);
        return false;
    }
    idEspece = idFromRole;

    if (qteText.isEmpty() || !parseDouble(qteText, &quantite) || quantite <= 0.0) {
        err = tr("Quantité invalide à la ligne %1 (nombre > 0 requis).").arg(row + 1);
        return false;
    }

    if (prixText.isEmpty() || !parseDouble(prixText, &prix) || prix < 0.0) {
        err = tr("Prix unitaire invalide à la ligne %1 (nombre ≥ 0 requis).").arg(row + 1);
        return false;
    }

    return true;
}

bool PecheCrudDialog::readEmployeRow(int row, int &idEmploye, QString &description, QString &err) const
{
    const auto *colItem  = ui->tableEmployes->item(row, 0);
    const auto *descItem = ui->tableEmployes->item(row, 1);

    const int idFromRole  = colItem  ? colItem->data(Qt::UserRole).toInt() : -1;
    const QString descText = descItem ? descItem->text().trimmed() : QString();

    // Completely empty row → silent skip
    if (idFromRole <= 0 && descText.isEmpty()) {
        return false;
    }

    if (idFromRole <= 0) {
        err = tr("Employé non sélectionné à la ligne %1.").arg(row + 1);
        return false;
    }
    idEmploye   = idFromRole;
    description = descText;
    return true;
}

// ---------------------------------------------------------------------------
// Table validators
// ---------------------------------------------------------------------------
bool PecheCrudDialog::validateEspecesTable(QString *err, QWidget **firstInvalid) const
{
    int validRowCount = 0;
    QSet<int> seenIds;

    for (int row = 0; row < ui->tableEspeces->rowCount(); ++row) {
        int idEspece   = 0;
        double quantite = 0.0;
        double prix     = 0.0;
        QString rowErr;

        if (!readEspeceRow(row, idEspece, quantite, prix, rowErr)) {
            if (!rowErr.isEmpty()) {
                if (err)          *err = rowErr;
                if (firstInvalid) *firstInvalid = ui->tableEspeces;
                return false;
            }
            continue; // silent skip
        }

        if (seenIds.contains(idEspece)) {
            if (err)          *err = tr("Espèce dupliquée à la ligne %1.").arg(row + 1);
            if (firstInvalid) *firstInvalid = ui->tableEspeces;
            return false;
        }
        seenIds.insert(idEspece);
        ++validRowCount;
    }

    if (validRowCount == 0) {
        if (err)          *err = tr("Au moins une espèce valide est requise.");
        if (firstInvalid) *firstInvalid = ui->tableEspeces;
        return false;
    }
    return true;
}

bool PecheCrudDialog::validateEmployesTable(QString *err, QWidget **firstInvalid) const
{
    QSet<int> seenIds;

    for (int row = 0; row < ui->tableEmployes->rowCount(); ++row) {
        int idEmploye = 0;
        QString description;
        QString rowErr;

        if (!readEmployeRow(row, idEmploye, description, rowErr)) {
            if (!rowErr.isEmpty()) {
                if (err)          *err = rowErr;
                if (firstInvalid) *firstInvalid = ui->tableEmployes;
                return false;
            }
            continue; // silent skip
        }

        if (seenIds.contains(idEmploye)) {
            if (err)          *err = tr("Employé dupliqué à la ligne %1.").arg(row + 1);
            if (firstInvalid) *firstInvalid = ui->tableEmployes;
            return false;
        }
        seenIds.insert(idEmploye);

        const QString nd = normalizeText(description);
        if (nd.isEmpty()) {
            if (err)          *err = tr("Description employé requise à la ligne %1.").arg(row + 1);
            if (firstInvalid) *firstInvalid = ui->tableEmployes;
            return false;
        }
        if (nd.size() > m_empDescMax) {
            if (err)          *err = tr("Description trop longue à la ligne %1 (max %2 car.).").arg(row + 1).arg(m_empDescMax);
            if (firstInvalid) *firstInvalid = ui->tableEmployes;
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Main validator
// ---------------------------------------------------------------------------
QMap<QWidget *, QString> PecheCrudDialog::validateAll(bool includeDbChecks)
{
    QMap<QWidget *, QString> errors;

    // Date
    const QDate date = ui->datePecheEdit->date();
    if (!date.isValid()) {
        errors.insert(ui->datePecheEdit, tr("Date de débarquement requise."));
    } else if (date < ui->datePecheEdit->minimumDate() || date > ui->datePecheEdit->maximumDate()) {
        errors.insert(ui->datePecheEdit, tr("Date hors des limites autorisées."));
    }

    // Navire – ID comes from the combo's item data; no text-parsing needed
    int navireId = ui->comboIdNavire->currentData().toInt();
    if (navireId <= 0) {
        errors.insert(ui->comboIdNavire, tr("Navire obligatoire."));
    }
    // No DB existence check: the item was loaded from NAVIRES, so it is always valid.

    // Zone
    const QString zoneText = normalizeText(ui->lineZonePeche->text());
    if (zoneText.isEmpty()) {
        errors.insert(ui->lineZonePeche, tr("Zone de pêche obligatoire."));
    } else if (zoneText.size() < 2) {
        errors.insert(ui->lineZonePeche, tr("Zone trop courte (minimum 2 caractères)."));
    } else if (zoneText.size() > m_zoneMaxLen) {
        errors.insert(ui->lineZonePeche, tr("Zone trop longue (max %1 caractères).").arg(m_zoneMaxLen));
    } else if (!m_zoneRegex.match(zoneText).hasMatch()) {
        errors.insert(ui->lineZonePeche, tr("Zone invalide (caractères non autorisés)."));
    }

    // Description
    const QString descText = normalizeText(ui->plainDescription->toPlainText());
    if (descText.isEmpty()) {
        errors.insert(ui->plainDescription, tr("Statut / description obligatoire."));
    } else if (descText.size() > m_descMaxLen) {
        errors.insert(ui->plainDescription, tr("Description trop longue (max %1 caractères).").arg(m_descMaxLen));
    }

    // Espèces
    {
        QString tableErr;
        QWidget *firstInvalid = nullptr;
        if (!validateEspecesTable(&tableErr, &firstInvalid) && firstInvalid) {
            errors.insert(firstInvalid, tableErr);
        }
        // No additional DB check needed: species IDs come from the combo,
        // so they are already guaranteed to be real ESPECES rows.
    }

    // Employés
    {
        QString tableErr;
        QWidget *firstInvalid = nullptr;
        if (!validateEmployesTable(&tableErr, &firstInvalid) && firstInvalid) {
            errors.insert(firstInvalid, tableErr);
        }
        // Same reasoning: IDs come from the combo, they are real EMPLOYES rows.
    }

    // Duplicate lot check
    if (includeDbChecks
        && !errors.contains(ui->datePecheEdit)
        && !errors.contains(ui->lineZonePeche)
        && !errors.contains(ui->comboIdNavire)
        && navireId > 0)
    {
        PecheDAO dao;
        QString dbErr;
        const bool dup = dao.existsDuplicate(date, zoneText, navireId, m_pecheId, &dbErr);
        if (!dbErr.isEmpty()) {
            errors.insert(ui->lineZonePeche, tr("Vérification en base impossible."));
        } else if (dup) {
            errors.insert(ui->lineZonePeche, tr("Lot déjà existant pour cette date/zone/navire."));
        }
    }

    return errors;
}

// ---------------------------------------------------------------------------
// DB: associations
// ---------------------------------------------------------------------------
bool PecheCrudDialog::deleteAssociations(int idPeche, QString *err)
{
    QSqlQuery qDelE;
    qDelE.prepare("DELETE FROM PECHESESPECES WHERE IDPECHE = :id");
    qDelE.bindValue(":id", idPeche);
    if (!qDelE.exec()) {
        const QString msg = qDelE.lastError().text();
        qDebug() << "deleteAssociations especes:" << msg;
        if (err) *err = msg;
        return false;
    }

    QSqlQuery qDelEmp;
    qDelEmp.prepare("DELETE FROM PECHESEMPLOYES WHERE IDPECHE = :id");
    qDelEmp.bindValue(":id", idPeche);
    if (!qDelEmp.exec()) {
        const QString msg = qDelEmp.lastError().text();
        qDebug() << "deleteAssociations employes:" << msg;
        if (err) *err = msg;
        return false;
    }
    return true;
}

bool PecheCrudDialog::insertAssociations(int idPeche, QString *err)
{
    QSqlQuery qEsp;
    qEsp.prepare("INSERT INTO PECHESESPECES (IDPECHE, IDESPECE, QUANTITE, PRIXUNITAIRE) "
                 "VALUES (:idPeche, :idEspece, :qte, :prix)");

    for (int row = 0; row < ui->tableEspeces->rowCount(); ++row) {
        int idEspece = 0; double quantite = 0.0, prix = 0.0; QString rowErr;
        if (!readEspeceRow(row, idEspece, quantite, prix, rowErr)) {
            if (!rowErr.isEmpty()) { if (err) *err = rowErr; return false; }
            continue;
        }
        qEsp.bindValue(":idPeche",  idPeche);
        qEsp.bindValue(":idEspece", idEspece);
        qEsp.bindValue(":qte",      quantite);
        qEsp.bindValue(":prix",     prix);
        if (!qEsp.exec()) {
            const QString msg = qEsp.lastError().text();
            qDebug() << "insertAssociations espece:" << msg;
            if (err) *err = msg;
            return false;
        }
    }

    QSqlQuery qEmp;
    qEmp.prepare("INSERT INTO PECHESEMPLOYES (IDPECHE, IDEMPLOYE, DESCRIPTION) "
                 "VALUES (:idPeche, :idEmploye, :description)");

    for (int row = 0; row < ui->tableEmployes->rowCount(); ++row) {
        int idEmploye = 0; QString description, rowErr;
        if (!readEmployeRow(row, idEmploye, description, rowErr)) {
            if (!rowErr.isEmpty()) { if (err) *err = rowErr; return false; }
            continue;
        }
        qEmp.bindValue(":idPeche",    idPeche);
        qEmp.bindValue(":idEmploye",  idEmploye);
        qEmp.bindValue(":description",normalizeText(description));
        if (!qEmp.exec()) {
            const QString msg = qEmp.lastError().text();
            qDebug() << "insertAssociations employe:" << msg;
            if (err) *err = msg;
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------------
bool PecheCrudDialog::save()
{
    applyNormalizedInputs();

    const QMap<QWidget *, QString> errors = validateAll(true);
    if (!errors.isEmpty()) {
        applyErrors(errors);
        focusFirstInvalid(errors);
        return false;
    }

    // Read navire ID from combo item data (always valid if validateAll passed)
    const int idNavire = ui->comboIdNavire->currentData().toInt();
    if (idNavire <= 0) {
        applyErrorState(ui->comboIdNavire, m_errorNavire, tr("Navire obligatoire."));
        focusFirstInvalid({ { ui->comboIdNavire, tr("Navire obligatoire.") } });
        return false;
    }

    Peche peche;
    peche.setId(m_pecheId);
    peche.setDatePeche(ui->datePecheEdit->date());
    peche.setZonePeche(normalizeText(ui->lineZonePeche->text()));
    peche.setDescription(normalizeText(ui->plainDescription->toPlainText()));
    peche.setIdNavire(idNavire);

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        QMessageBox::warning(this, tr("Erreur"), db.lastError().text());
        return false;
    }

    PecheDAO dao;
    QString dbErr;
    bool okSave = false;
    if (m_mode == AddMode) {
        if (peche.getId() <= 0) {
            int newId = 0;
            if (!dao.nextId(&newId, &dbErr)) {
                db.rollback();
                QMessageBox::warning(this, tr("Erreur"),
                    dbErr.isEmpty() ? tr("Génération d'ID échouée.") : dbErr);
                return false;
            }
            peche.setId(newId);
        }
        okSave = dao.insert(peche, &dbErr);
        if (okSave) m_pecheId = peche.getId();
    } else {
        okSave = dao.update(peche, &dbErr);
    }

    if (!okSave) {
        db.rollback();
        QMessageBox::warning(this, tr("Erreur"),
            dbErr.isEmpty() ? tr("Enregistrement échoué.") : dbErr);
        return false;
    }

    if (m_mode == EditMode) {
        QString assocErr;
        if (!deleteAssociations(m_pecheId, &assocErr)) {
            db.rollback();
            QMessageBox::warning(this, tr("Erreur"),
                assocErr.isEmpty() ? tr("Suppression des associations échouée.") : assocErr);
            return false;
        }
    }

    QString assocErr;
    if (!insertAssociations(m_pecheId, &assocErr)) {
        db.rollback();
        QMessageBox::warning(this, tr("Erreur"),
            assocErr.isEmpty() ? tr("Insertion des associations échouée.") : assocErr);
        return false;
    }

    if (!db.commit()) {
        QMessageBox::warning(this, tr("Erreur"), db.lastError().text());
        return false;
    }

    return true;
}



bool PecheCrudDialog::zoneexiste(QString zone)
{
    QSqlQuery query;

    query.prepare("SELECT COUNT(*) FROM PECHES WHERE UPPER(ZONEPECHE) = UPPER(:zone)");
    query.bindValue(":zone", zone.trimmed());

    if (!query.exec()) {
        qDebug() << "Erreur vérification zone:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

