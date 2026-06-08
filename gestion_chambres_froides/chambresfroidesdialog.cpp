#include "chambresfroidesdialog.h"
#include "ui_chambresfroidesdialog.h"
#include "chambresfroides.h"

#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QIntValidator>

ChambresFroidesDialog::ChambresFroidesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChambresFroidesDialog),
    m_isEditMode(false)
{
    ui->setupUi(this);
    
    // Control de saisie: ID numerical only
    ui->txtId->setValidator(new QIntValidator(0, 999999, this));
    ui->txtId->setPlaceholderText("Ex: 101");
    ui->txtId->setToolTip("L'ID doit être uniquement numérique (Ex: 101)");
    
    // Sanity Certificate: URL placeholder
    ui->txtCertSan->setPlaceholderText("https://example.com/certificat.jpg");
    ui->txtCertSan->setToolTip("Entrez le lien (URL) vers l'image du certificat");
    
    // Capacity units: Add " kg" suffix
    ui->spinMaxCap->setSuffix(" kg");
    ui->spinOccCap->setSuffix(" kg");
    ui->spinMaxCap->setToolTip("Capacité maximale en kilogrammes");
    ui->spinOccCap->setToolTip("Capacité occupée en kilogrammes");
    
    // Hide warnings by default
    ui->lblIdWarning->hide();
    ui->lblUrlWarning->hide();
    ui->lblCapWarning->hide();
    
    // Connect capacity signals for real-time validation
    connect(ui->spinMaxCap, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ChambresFroidesDialog::on_capacity_changed);
    connect(ui->spinOccCap, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ChambresFroidesDialog::on_capacity_changed);
    
    // Connect ALL other signals for strict enforcement
    connect(ui->txtId, &QLineEdit::textChanged, this, &ChambresFroidesDialog::updateSaveButtonState);
    connect(ui->txtCertSan, &QLineEdit::textChanged, this, &ChambresFroidesDialog::updateSaveButtonState);
    connect(ui->dateIn, &QDateEdit::dateChanged, this, &ChambresFroidesDialog::updateSaveButtonState);
    connect(ui->dateLimit, &QDateEdit::dateChanged, this, &ChambresFroidesDialog::updateSaveButtonState);
    connect(ui->spinTemp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ChambresFroidesDialog::updateSaveButtonState);
    connect(ui->comboIdPeche, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ChambresFroidesDialog::updateSaveButtonState);

    // Hide ID field (Automated)
    ui->labelId->hide();
    ui->txtId->hide(); 
    ui->lblIdWarning->hide();
    
    // Hide Beta Test field (Automated)
    ui->labelBetaTest->hide();
    ui->checkBetaTest->hide();
    
    // Initial Tag state
    ui->txtTag->setText("Généré à l'enregistrement");
    
    loadPecheIds();
    
    // Set current dates as default
    ui->dateIn->setDate(QDate::currentDate());
    ui->dateLimit->setDate(QDate::currentDate().addMonths(1));

    // Connect combo changes to update button state
    connect(ui->comboIdPeche, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ChambresFroidesDialog::updateSaveButtonState);

    updateSaveButtonState(); // Initial check
}

ChambresFroidesDialog::~ChambresFroidesDialog()
{
    delete ui;
}

void ChambresFroidesDialog::loadPecheIds()
{
    ui->comboIdPeche->clear();
    ui->comboIdPeche->addItem("--- Aucun ---", QVariant(QMetaType::fromType<int>()));
    
    QSqlQuery query("SELECT IDPECHE FROM PECHES");
    while (query.next()) {
        int idPeche = query.value(0).toInt();
        ui->comboIdPeche->addItem("Pêche #" + QString::number(idPeche), idPeche);
    }
}

void ChambresFroidesDialog::setChambre(const ChambresFroides &chambre)
{
    m_isEditMode = true;
    m_oldId = chambre.getId();

    ui->txtId->setText(chambre.getId());
    ui->txtId->setReadOnly(true); // Disable ID editing
    
    ui->spinTemp->setValue(chambre.getTemp());
    ui->spinTemp->setEnabled(false); // Temperature is now a configuration parameter
    ui->spinTemp->setToolTip("Utilisez le bouton de configuration (⚙) pour ajuster les paramètres climatiques.");
    ui->dateIn->setDate(chambre.getInDate());
    ui->dateLimit->setDate(chambre.getLimitDate());
    ui->txtCertSan->setText(chambre.getCertificateSan());
    ui->spinMaxCap->setValue(chambre.getMaxCap());
    ui->spinOccCap->setValue(chambre.getOccCap());
    ui->comboState->setCurrentText(chambre.getState());
    ui->txtTag->setText(chambre.getTagNumber());

    // Find and set IDPECHE in combo if it exists, otherwise keep at "Aucun"
    QVariant pecheId = chambre.getIdPeche();
    if (!pecheId.isNull() && pecheId.isValid() && pecheId.toInt() != 0) {
        int index = ui->comboIdPeche->findData(pecheId.toInt());
        if (index != -1) {
            ui->comboIdPeche->setCurrentIndex(index);
        }
    } else {
        ui->comboIdPeche->setCurrentIndex(0);
    }
    
    ui->checkBetaTest->setChecked(chambre.isBetaTest());
}

void ChambresFroidesDialog::on_btnSave_clicked()
{
    QString id = ui->txtId->text().trimmed();

    QString validationErr;
    if (!validateInputs(&validationErr)) {
        QMessageBox::critical(this, "Erreur de Validation", validationErr);
        return;
    }

    ChambresFroides chambre;
    chambre.setId(id);
    chambre.setTemp(ui->spinTemp->value());
    chambre.setInDate(ui->dateIn->date());
    chambre.setLimitDate(ui->dateLimit->date());
    chambre.setCertificateSan(ui->txtCertSan->text().trimmed());
    chambre.setMaxCap(ui->spinMaxCap->value());
    chambre.setOccCap(ui->spinOccCap->value());
    chambre.setState(ui->comboState->currentText());
    chambre.setTagNumber(ui->txtTag->text());

    // Handle IDPECHE QVariant
    QVariant idPeche = ui->comboIdPeche->currentData();
    chambre.setIdPeche(idPeche);
    chambre.setBetaTest(true); // Always enable Arduino capability

    QString errorMsg;
    bool success;

    if (m_isEditMode) {
        success = chambre.modifier_chf(m_oldId, &errorMsg);
    } else {
        success = chambre.ajouter_chf(&errorMsg);
    }

    if (success) {
        QMessageBox::information(this, "Succès", m_isEditMode ? "Chambre modifiée avec succès!" : "Chambre ajoutée avec succès!");
        accept();
    } else {
        QMessageBox::critical(this, "Erreur", "Une erreur est survenue:\n" + errorMsg);
    }
}

void ChambresFroidesDialog::on_btnCancel_clicked()
{
    reject();
}

void ChambresFroidesDialog::on_txtId_textChanged(const QString &)
{
    // ID is now automated, but we keep the slot for safety if referenced in UI
    updateSaveButtonState();
}

void ChambresFroidesDialog::on_txtCertSan_textChanged(const QString &arg1)
{
    // Basic URL validation
    if (!arg1.isEmpty() && !arg1.startsWith("http://") && !arg1.startsWith("https://")) {
        ui->lblUrlWarning->show();
    } else {
        ui->lblUrlWarning->hide();
    }
    updateSaveButtonState();
}

void ChambresFroidesDialog::on_capacity_changed()
{
    updateSaveButtonState();
}

bool ChambresFroidesDialog::validateInputs(QString *firstError) const
{
    // 1. Capacity Check
    if (ui->spinMaxCap->value() <= 0) {
        if (firstError) *firstError = "La capacité maximale doit être supérieure à 0.";
        return false;
    }
    if (ui->spinOccCap->value() <= 0) {
        if (firstError) *firstError = "L'espace occupé est obligatoire (doit être > 0).";
        return false;
    }
    if (ui->spinOccCap->value() > ui->spinMaxCap->value()) {
        if (firstError) *firstError = "La capacité occupée ne peut pas dépasser le maximum.";
        return false;
    }

    // 3. URL Check
    QString url = ui->txtCertSan->text().trimmed();
    if (url.isEmpty()) {
        if (firstError) *firstError = "Le certificat de salubrité (URL) est obligatoire.";
        return false;
    }
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        if (firstError) *firstError = "L'URL du certificat doit commencer par http:// ou https://.";
        return false;
    }

    // 4. Date Check
    if (ui->dateIn->date() > ui->dateLimit->date()) {
        if (firstError) *firstError = "La date d'entrée ne peut pas être postérieure à la date limite.";
        return false;
    }

    bool hasPeche = (ui->comboIdPeche->currentIndex() > 0);
    if (!hasPeche) {
        if (firstError) *firstError = "Veuillez choisir une Pêche associée.";
        return false;
    }

    return true;
}

void ChambresFroidesDialog::updateSaveButtonState()
{
    QString error;
    bool isValid = validateInputs(&error);
    
    ui->btnSave->setEnabled(isValid);
    
    if (isValid) {
        ui->btnSave->setToolTip("Enregistrer les données");
        ui->lblCapWarning->hide();
        ui->lblUrlWarning->hide();
    } else {
        ui->btnSave->setToolTip(error);
        
        // Show specific visual cues based on error type for better UX
        if (error.contains("capacité", Qt::CaseInsensitive)) ui->lblCapWarning->show();
        else ui->lblCapWarning->hide();
        
        if (error.contains("URL") || error.contains("certificat")) ui->lblUrlWarning->show();
        else ui->lblUrlWarning->hide();
    }
}

