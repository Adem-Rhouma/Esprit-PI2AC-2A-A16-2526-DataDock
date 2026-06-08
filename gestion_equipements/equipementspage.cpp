#include "equipementspage.h"
#include "ui_equipementspage.h"

#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIntValidator>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>

#include <algorithm>

EquipementsPage::EquipementsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EquipementsPage)
    , model(nullptr)
    , m_selectedMaterielId(-1)
    , m_isEditMode(false)
    , m_pendingDeleteId(-1)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(R"(
QWidget#controlsWidget,
QWidget#formCard,
QWidget#viewCard,
QWidget#deleteCard,
QWidget#tableCard {
    background: #ffffff;
    color: #111111;
    border: 1px solid #e3e8ee;
    border-radius: 12px;
}
QWidget#controlsWidget QLabel,
QWidget#controlsWidget QLineEdit,
QWidget#controlsWidget QComboBox,
QWidget#controlsWidget QPushButton,
QWidget#formCard QLabel,
QWidget#formCard QLineEdit,
QWidget#formCard QComboBox,
QWidget#formCard QPushButton,
QWidget#viewCard QLabel,
QWidget#viewCard QPushButton,
QWidget#deleteCard QLabel,
QWidget#deleteCard QPushButton {
    color: #111111;
}
QWidget#formCard QLineEdit,
QWidget#formCard QComboBox {
    border: 1px solid #d9e1ea;
    border-radius: 8px;
    padding: 6px 8px;
    background: #ffffff;
}
QWidget#formCard QLineEdit:focus,
QWidget#formCard QComboBox:focus {
    border: 1px solid #57a4ff;
}
QPushButton#btnFormSave {
    background: #2e86de;
    color: #ffffff;
    border: none;
    border-radius: 8px;
    padding: 6px 12px;
}
QPushButton#btnFormCancel {
    background: #f1f4f8;
    color: #1c232b;
    border: 1px solid #dfe6ee;
    border-radius: 8px;
    padding: 6px 12px;
}
QPushButton#btnViewClose,
QPushButton#btnDeleteCancel {
    background: #f1f4f8;
    color: #1c232b;
    border: 1px solid #dfe6ee;
    border-radius: 8px;
    padding: 6px 12px;
}
QPushButton#btnDeleteConfirm {
    background: #dc3545;
    color: #ffffff;
    border: none;
    border-radius: 8px;
    padding: 6px 12px;
}
QTableWidget#tableMateriaux {
    background: #ffffff;
    color: #111111;
    border: none;
    gridline-color: #edf1f6;
    selection-color: #111111;
}
QHeaderView::section {
    background: #f7f9fc;
    color: #111111;
    border: none;
    padding: 6px 8px;
    font-weight: 600;
}
)"));

    normalizeFormTexts();
    setupReparableCombos();
    setupInputValidation();
    setupEquipementsTable();

    hidePanels();

    connect(ui->btnAddMateriel, &QPushButton::clicked, this, &EquipementsPage::on_ajouter_clicked);
    if (ui->btnFormSave) {
        connect(ui->btnFormSave, &QPushButton::clicked, this, [this]() {
            const bool uiFormValid = validateCurrentForm(true);
            if (!uiFormValid) {
                return;
            }

            materiel value;
            const bool validForm = m_isEditMode ? readMaterielFromEditForm(value)
                                                : readMaterielFromAddForm(value);

            if (!validForm || !value.isValid()) {
                showFormError("Vérifiez les champs saisis.");
                return;
            }

            if (!m_isEditMode && materiel::existe(value.idMateriel())) {
                showFormError("ID matériel déjà existant.");
                if (ui->inputIdMateriel) {
                    setFieldValidity(ui->inputIdMateriel, false);
                    ui->inputIdMateriel->setFocus();
                }
                return;
            }

            showFormError(QString());

            const bool ok = m_isEditMode ? value.modifier() : value.ajouter();
            if (!ok) {
                QMessageBox::warning(this,
                                     "Erreur SQL",
                                     m_isEditMode ? "Mise à jour impossible." : "Insertion impossible.");
                return;
            }

            clearEditForm();
            refreshTable();
        });
    }
    if (ui->btnFormCancel) {
        connect(ui->btnFormCancel, &QPushButton::clicked, this, &EquipementsPage::clearEditForm);
    }
    if (ui->btnViewClose) {
        connect(ui->btnViewClose, &QPushButton::clicked, this, &EquipementsPage::hidePanels);
    }
    if (ui->btnDeleteCancel) {
        connect(ui->btnDeleteCancel, &QPushButton::clicked, this, &EquipementsPage::hidePanels);
    }
    if (ui->btnDeleteConfirm) {
        connect(ui->btnDeleteConfirm, &QPushButton::clicked, this, [this]() {
            if (m_pendingDeleteId < 0) {
                hidePanels();
                return;
            }

            if (!materiel::supprimer(m_pendingDeleteId)) {
                QMessageBox::warning(this, "Erreur SQL", "Suppression impossible.");
                return;
            }

            m_pendingDeleteId = -1;
            m_selectedMaterielId = -1;
            hidePanels();
            refreshTable();
        });
    }
    connect(ui->searchBar, &QLineEdit::textChanged, this, [this]() {
        refreshTable();
    });
    connect(ui->comboSortField, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        refreshTable();
    });
    connect(ui->btnExport, &QPushButton::clicked, this, [this]() {
        exportTableToExcel();
    });
}

EquipementsPage::~EquipementsPage()
{
    delete ui;
}

void EquipementsPage::normalizeFormTexts()
{
    if (ui->labelSort) {
        ui->labelSort->setText("Trier:");
    }
    if (ui->labelSearch) {
        ui->labelSearch->setText("Recherche:");
    }
    if (ui->searchBar) {
        ui->searchBar->setPlaceholderText("ID, État, Type, Libellé, Réparable...");
    }
    if (ui->comboSortField) {
        const QString selectedText = ui->comboSortField->currentText();
        ui->comboSortField->clear();
        ui->comboSortField->addItems({"ID Matériel", "État", "Type", "ID Employé"});

        const int selectedIndex = ui->comboSortField->findText(selectedText);
        ui->comboSortField->setCurrentIndex(selectedIndex >= 0 ? selectedIndex : 0);
    }
}

void EquipementsPage::setupEquipementsTable()
{
    if (!ui->tableMateriaux) {
        return;
    }

    ui->tableMateriaux->setColumnCount(7);
    ui->tableMateriaux->setHorizontalHeaderLabels({
        "ID Matériel", "Libellé", "Type", "État", "Réparable", "ID Employé", "Action"
    });

    ui->tableMateriaux->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableMateriaux->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableMateriaux->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableMateriaux->verticalHeader()->setVisible(false);
    ui->tableMateriaux->horizontalHeader()->setStretchLastSection(true);
    ui->tableMateriaux->setSortingEnabled(true);

    refreshTable();
}

void EquipementsPage::setupReparableCombos()
{
    if (ui->comboReparableForm && ui->comboReparableForm->count() == 0) {
        ui->comboReparableForm->addItems({"Oui", "Non"});
    }
}

void EquipementsPage::refreshTable()
{
    if (!ui->tableMateriaux) {
        return;
    }

    QList<materiel> items = materiel::afficherTous();
    const QString searchText = ui->searchBar ? ui->searchBar->text().trimmed().toLower() : QString();

    if (!searchText.isEmpty()) {
        QList<materiel> filtered;
        filtered.reserve(items.size());

        for (const materiel &value : items) {
            const QString haystack = (
                QString::number(value.idMateriel()) + " " +
                value.libelle() + " " +
                value.type() + " " +
                value.etat() + " " +
                (value.reparable() ? QStringLiteral("oui") : QStringLiteral("non")) + " " +
                QString::number(value.idEmploye())
            ).toLower();

            if (haystack.contains(searchText)) {
                filtered.append(value);
            }
        }

        items = filtered;
    }

    if (ui->comboSortField) {
        const QString sortText = ui->comboSortField->currentText();
        if (sortText == "ID Matériel") {
            std::sort(items.begin(), items.end(), [](const materiel &a, const materiel &b) {
                return a.idMateriel() < b.idMateriel();
            });
        } else if (sortText == "État") {
            std::sort(items.begin(), items.end(), [](const materiel &a, const materiel &b) {
                return a.etat().toLower() < b.etat().toLower();
            });
        } else if (sortText == "Type") {
            std::sort(items.begin(), items.end(), [](const materiel &a, const materiel &b) {
                return a.type().toLower() < b.type().toLower();
            });
        } else {
            std::sort(items.begin(), items.end(), [](const materiel &a, const materiel &b) {
                if (a.idEmploye() == b.idEmploye()) {
                    return a.idMateriel() < b.idMateriel();
                }
                return a.idEmploye() < b.idEmploye();
            });
        }
    }

    ui->tableMateriaux->setSortingEnabled(false);
    ui->tableMateriaux->setRowCount(items.size());

    for (int row = 0; row < items.size(); ++row) {
        const materiel &value = items[row];

        const QStringList values = {
            QString::number(value.idMateriel()),
            value.libelle(),
            value.type(),
            value.etat(),
            value.reparable() ? "Oui" : "Non",
            QString::number(value.idEmploye())
        };

        for (int col = 0; col < values.size(); ++col) {
            auto *item = new QTableWidgetItem(values[col]);
            item->setTextAlignment(Qt::AlignCenter);
            ui->tableMateriaux->setItem(row, col, item);
        }

        QWidget *actionWidget = new QWidget(ui->tableMateriaux);
        auto *layout = new QHBoxLayout(actionWidget);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(6);

        auto *btnView = new QPushButton("👁", actionWidget);
        auto *btnEdit = new QPushButton("✏️", actionWidget);
        auto *btnDelete = new QPushButton("🗑", actionWidget);

        btnView->setFixedSize(30, 30);
        btnEdit->setFixedSize(30, 30);
        btnDelete->setFixedSize(30, 30);

        layout->addWidget(btnView);
        layout->addWidget(btnEdit);
        layout->addWidget(btnDelete);

        connect(btnView, &QPushButton::clicked, this, [this, value]() {
            showMaterielDetails(value);
        });

        connect(btnEdit, &QPushButton::clicked, this, [this, value]() {
            openEditForm(value);
        });

        connect(btnDelete, &QPushButton::clicked, this, [this, value]() {
            showDeleteConfirm(value);
        });

        ui->tableMateriaux->setCellWidget(row, 6, actionWidget);
    }

    ui->tableMateriaux->resizeColumnsToContents();
    ui->tableMateriaux->setColumnWidth(6, 130);
    ui->tableMateriaux->setSortingEnabled(true);
}

void EquipementsPage::openAddDialog()
{
    hidePanels();
    m_isEditMode = false;
    m_selectedMaterielId = -1;
    clearAddForm();

    if (ui->lblFormTitle) {
        ui->lblFormTitle->setText("Ajouter un Matériel");
    }
    if (ui->inputIdMateriel) {
        ui->inputIdMateriel->setReadOnly(false);
        ui->inputIdMateriel->setStyleSheet(QString());
    }
    if (ui->btnFormSave) {
        ui->btnFormSave->setText("Ajouter");
    }
    clearFormValidationState();
    updateSaveButtonState();
    if (ui->formCard) {
        ui->formCard->setVisible(true);
    }
}

void EquipementsPage::showMaterielDetails(const materiel &value)
{
    hidePanels();
    m_selectedMaterielId = value.idMateriel();

    if (ui->lblViewIdValue) {
        ui->lblViewIdValue->setText(QString::number(value.idMateriel()));
    }
    if (ui->lblViewLibelleValue) {
        ui->lblViewLibelleValue->setText(value.libelle());
    }
    if (ui->lblViewTypeValue) {
        ui->lblViewTypeValue->setText(value.type());
    }
    if (ui->lblViewEtatValue) {
        ui->lblViewEtatValue->setText(value.etat());
    }
    if (ui->lblViewReparableValue) {
        ui->lblViewReparableValue->setText(value.reparable() ? "Oui" : "Non");
    }
    if (ui->lblViewEmployeValue) {
        ui->lblViewEmployeValue->setText(QString::number(value.idEmploye()));
    }
    if (ui->viewCard) {
        ui->viewCard->setVisible(true);
    }
}

void EquipementsPage::showDeleteConfirm(const materiel &value)
{
    hidePanels();
    m_pendingDeleteId = value.idMateriel();

    if (ui->lblDeleteMessage) {
        ui->lblDeleteMessage->setText(
            QString("Supprimer le matériel #%1 (%2) ?").arg(value.idMateriel()).arg(value.libelle()));
    }
    if (ui->deleteCard) {
        ui->deleteCard->setVisible(true);
    }
}

void EquipementsPage::openEditForm(const materiel &value)
{
    hidePanels();
    m_isEditMode = true;
    m_selectedMaterielId = value.idMateriel();

    if (ui->lblFormTitle) {
        ui->lblFormTitle->setText("Modifier un Matériel");
    }
    if (ui->inputIdMateriel) {
        ui->inputIdMateriel->setText(QString::number(value.idMateriel()));
        ui->inputIdMateriel->setReadOnly(true);
        ui->inputIdMateriel->setStyleSheet("background:#f4f6f9;");
    }
    if (ui->inputLibelle) {
        ui->inputLibelle->setText(value.libelle());
    }
    if (ui->inputType) {
        ui->inputType->setText(value.type());
    }
    if (ui->inputEtat) {
        ui->inputEtat->setText(value.etat());
    }
    if (ui->comboReparableForm) {
        ui->comboReparableForm->setCurrentText(value.reparable() ? "Oui" : "Non");
    }
    if (ui->inputIdEmploye) {
        ui->inputIdEmploye->setText(QString::number(value.idEmploye()));
    }
    if (ui->btnFormSave) {
        ui->btnFormSave->setText("Enregistrer");
    }
    clearFormValidationState();
    updateSaveButtonState();
    if (ui->formCard) {
        ui->formCard->setVisible(true);
    }
}

void EquipementsPage::showListGroup()
{
}

void EquipementsPage::clearAddForm()
{
    if (ui->inputIdMateriel) {
        ui->inputIdMateriel->clear();
        ui->inputIdMateriel->setReadOnly(false);
        ui->inputIdMateriel->setStyleSheet(QString());
    }
    if (ui->inputLibelle) {
        ui->inputLibelle->clear();
    }
    if (ui->inputType) {
        ui->inputType->clear();
    }
    if (ui->inputEtat) {
        ui->inputEtat->clear();
    }
    if (ui->comboReparableForm) {
        ui->comboReparableForm->setCurrentText("Oui");
    }
    if (ui->inputIdEmploye) {
        ui->inputIdEmploye->clear();
    }
}

void EquipementsPage::clearEditForm()
{
    clearAddForm();
    clearFormValidationState();
    m_isEditMode = false;
    m_selectedMaterielId = -1;
    m_pendingDeleteId = -1;
    if (ui->lblFormTitle) {
        ui->lblFormTitle->setText("Formulaire Matériel");
    }
    hidePanels();
}

void EquipementsPage::hidePanels()
{
    if (ui->formCard) {
        ui->formCard->setVisible(false);
    }
    if (ui->viewCard) {
        ui->viewCard->setVisible(false);
    }
    if (ui->deleteCard) {
        ui->deleteCard->setVisible(false);
    }
}

bool EquipementsPage::readMaterielFromAddForm(materiel &value) const
{
    bool idOk = false;
    bool employeOk = false;

    const int idMateriel = ui->inputIdMateriel ? ui->inputIdMateriel->text().trimmed().toInt(&idOk) : 0;
    const int idEmploye = ui->inputIdEmploye ? ui->inputIdEmploye->text().trimmed().toInt(&employeOk) : 0;

    value = materiel(
        idMateriel,
        ui->inputLibelle ? ui->inputLibelle->text().trimmed() : QString(),
        ui->inputType ? ui->inputType->text().trimmed() : QString(),
        ui->inputEtat ? ui->inputEtat->text().trimmed() : QString(),
        ui->comboReparableForm ? (ui->comboReparableForm->currentText() == "Oui") : false,
        idEmploye
    );

    return idOk && employeOk;
}

bool EquipementsPage::readMaterielFromEditForm(materiel &value) const
{
    bool employeOk = false;
    const int idEmploye = ui->inputIdEmploye ? ui->inputIdEmploye->text().trimmed().toInt(&employeOk) : 0;

    value = materiel(
        m_selectedMaterielId,
        ui->inputLibelle ? ui->inputLibelle->text().trimmed() : QString(),
        ui->inputType ? ui->inputType->text().trimmed() : QString(),
        ui->inputEtat ? ui->inputEtat->text().trimmed() : QString(),
        ui->comboReparableForm ? (ui->comboReparableForm->currentText() == "Oui") : false,
        idEmploye
    );

    return m_selectedMaterielId >= 0 && employeOk;
}

void EquipementsPage::setupInputValidation()
{
    if (ui->inputIdMateriel) {
        ui->inputIdMateriel->setValidator(new QIntValidator(1, 999999999, this));
    }
    if (ui->inputIdEmploye) {
        ui->inputIdEmploye->setValidator(new QIntValidator(1, 999999999, this));
    }

    auto connectField = [this](QLineEdit *field) {
        if (!field) {
            return;
        }
        connect(field, &QLineEdit::textChanged, this, [this]() {
            updateSaveButtonState();
        });
    };

    connectField(ui->inputIdMateriel);
    connectField(ui->inputLibelle);
    connectField(ui->inputType);
    connectField(ui->inputEtat);
    connectField(ui->inputIdEmploye);

    if (ui->comboReparableForm) {
        connect(ui->comboReparableForm, &QComboBox::currentTextChanged, this, [this](const QString &) {
            updateSaveButtonState();
        });
    }

    clearFormValidationState();
    updateSaveButtonState();
}

void EquipementsPage::clearFormValidationState()
{
    if (!m_isEditMode) {
        setFieldValidity(ui->inputIdMateriel, true);
    }
    setFieldValidity(ui->inputLibelle, true);
    setFieldValidity(ui->inputType, true);
    setFieldValidity(ui->inputEtat, true);
    setFieldValidity(ui->inputIdEmploye, true);
    showFormError(QString());
}

void EquipementsPage::setFieldValidity(QLineEdit *field, bool isValid) const
{
    if (!field) {
        return;
    }

    if (isValid) {
        if (field == ui->inputIdMateriel && m_isEditMode) {
            field->setStyleSheet("background:#f4f6f9;");
        } else {
            field->setStyleSheet(QString());
        }
    } else {
        field->setStyleSheet("border: 1px solid #dc3545;");
    }
}

bool EquipementsPage::validateCurrentForm(bool showErrors)
{
    bool allValid = true;

    auto validateText = [this, showErrors, &allValid](QLineEdit *field) {
        const bool valid = field && !field->text().trimmed().isEmpty();
        if (showErrors) {
            setFieldValidity(field, valid);
        }
        if (!valid) {
            allValid = false;
        }
        return valid;
    };

    bool idMatValid = true;
    if (!m_isEditMode) {
        bool idOk = false;
        const int idMat = ui->inputIdMateriel ? ui->inputIdMateriel->text().trimmed().toInt(&idOk) : 0;
        idMatValid = idOk && idMat > 0;
        if (showErrors) {
            setFieldValidity(ui->inputIdMateriel, idMatValid);
        }
        if (!idMatValid) {
            allValid = false;
        }
    }

    validateText(ui->inputLibelle);
    validateText(ui->inputType);
    validateText(ui->inputEtat);

    bool empOk = false;
    const int idEmp = ui->inputIdEmploye ? ui->inputIdEmploye->text().trimmed().toInt(&empOk) : 0;
    const bool idEmployeValid = empOk && idEmp > 0;
    if (showErrors) {
        setFieldValidity(ui->inputIdEmploye, idEmployeValid);
    }
    if (!idEmployeValid) {
        allValid = false;
    }

    if (showErrors) {
        if (!allValid) {
            QString message = "Veuillez corriger les champs en rouge.";
            if (!m_isEditMode && !idMatValid) {
                message = "ID Matériel doit être un nombre positif.";
            } else if (!idEmployeValid) {
                message = "ID Employé doit être un nombre positif.";
            }
            showFormError(message);
        } else {
            showFormError(QString());
        }
    }

    return allValid;
}

void EquipementsPage::updateSaveButtonState()
{
    if (!ui->btnFormSave) {
        return;
    }

    const bool hasAnyInput =
        (ui->inputIdMateriel && !ui->inputIdMateriel->text().trimmed().isEmpty()) ||
        (ui->inputLibelle && !ui->inputLibelle->text().trimmed().isEmpty()) ||
        (ui->inputType && !ui->inputType->text().trimmed().isEmpty()) ||
        (ui->inputEtat && !ui->inputEtat->text().trimmed().isEmpty()) ||
        (ui->inputIdEmploye && !ui->inputIdEmploye->text().trimmed().isEmpty());

    if (!hasAnyInput) {
        clearFormValidationState();
        ui->btnFormSave->setEnabled(true);
        return;
    }

    validateCurrentForm(true);
    ui->btnFormSave->setEnabled(true);
}

void EquipementsPage::showFormError(const QString &message)
{
    QLabel *lblFormError = findChild<QLabel *>(QStringLiteral("lblFormError"));
    if (!lblFormError) {
        return;
    }

    lblFormError->setText(message);
    lblFormError->setVisible(!message.isEmpty());
}

void EquipementsPage::exportTableToExcel()
{
    if (!ui->tableMateriaux) {
        return;
    }

    if (ui->tableMateriaux->rowCount() <= 0) {
        QMessageBox::information(this, "Export", "Aucune donnée à exporter.");
        return;
    }

    const QString defaultFileName = QString("equipements_%1.csv")
                                        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        "Exporter vers Excel",
        defaultFileName,
        "Fichier Excel (*.csv)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export", "Impossible de créer le fichier d'export.");
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << QChar(0xFEFF);

    auto escapeCsv = [](const QString &value) {
        QString escaped = value;
        escaped.replace('"', "\"\"");
        if (escaped.contains(';') || escaped.contains('"') || escaped.contains('\n') || escaped.contains('\r')) {
            escaped = '"' + escaped + '"';
        }
        return escaped;
    };

    QStringList headers;
    for (int col = 0; col < 6; ++col) {
        auto *headerItem = ui->tableMateriaux->horizontalHeaderItem(col);
        headers << escapeCsv(headerItem ? headerItem->text() : QString());
    }
    stream << headers.join(';') << '\n';

    for (int row = 0; row < ui->tableMateriaux->rowCount(); ++row) {
        QStringList cells;
        for (int col = 0; col < 6; ++col) {
            auto *item = ui->tableMateriaux->item(row, col);
            cells << escapeCsv(item ? item->text() : QString());
        }
        stream << cells.join(';') << '\n';
    }

    file.close();
    QMessageBox::information(this, "Export", "Export Excel terminé avec succès.");
}

void EquipementsPage::on_ajouter_clicked()
{
    openAddDialog();
}

void EquipementsPage::on_statistique_clicked()
{
    QMessageBox::information(this, "Statistiques", "Ouvrez l'onglet Statistiques pour consulter les graphiques.");
}

void EquipementsPage::on_rt1_2_clicked()
{
    showListGroup();
}

void EquipementsPage::on_rt1_clicked()
{
    showListGroup();
}

void EquipementsPage::on_rt1_3_clicked()
{
    showListGroup();
}

void EquipementsPage::on_rt1_5_clicked()
{
    showListGroup();
}

void EquipementsPage::on_rt1_4_clicked()
{
    if (m_selectedMaterielId < 0) {
        return;
    }

    materiel value;
    if (materiel::recupererParId(m_selectedMaterielId, value)) {
        openEditForm(value);
    }
}

void EquipementsPage::on_ajouterr_clicked()
{
    openAddDialog();
}

void EquipementsPage::on_ajouterr_2_clicked()
{
    on_rt1_4_clicked();
}

void EquipementsPage::on_annuler_clicked()
{
    clearAddForm();
}

void EquipementsPage::on_ajouterr_3_clicked()
{
    clearEditForm();
}
