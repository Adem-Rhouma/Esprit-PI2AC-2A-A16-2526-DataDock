#include "navirecruddialog.h"
#include "ui_navirecruddialog.h"
#include "navire.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

NavireCrudDialog::NavireCrudDialog(Mode mode, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::NavireCrudDialog)
    , m_mode(mode)
{
    ui->setupUi(this);
    applyStyle();
    setupValidators();
    connectRealTimeValidation();

    auto *saveButton = ui->buttonBox->button(QDialogButtonBox::Save);
    if (saveButton) { saveButton->setProperty("primary", true); saveButton->setDefault(true); }
    auto *cancelButton = ui->buttonBox->button(QDialogButtonBox::Cancel);
    if (cancelButton) cancelButton->setProperty("secondary", true);

    if (m_mode == AddMode) {
        setWindowTitle(tr("Ajouter un navire"));
        ui->labelTitle->setText(tr("Ajouter un navire"));
    } else {
        setWindowTitle(tr("Modifier un navire"));
        ui->labelTitle->setText(tr("Modifier un navire"));
    }

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &NavireCrudDialog::onAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &NavireCrudDialog::reject);
}

NavireCrudDialog::NavireCrudDialog(Mode mode, int idNavire, QWidget *parent)
    : NavireCrudDialog(mode, parent)
{
    m_navireId = idNavire;
    if (m_mode == EditMode && m_navireId > 0) {
        loadNavire(m_navireId);
    }
}

NavireCrudDialog::~NavireCrudDialog() { delete ui; }

int NavireCrudDialog::navireId() const { return m_navireId; }

// ── Helpers ────────────────────────────────────────────────────
void NavireCrudDialog::setFieldError(QLineEdit *field, QLabel *errLabel, const QString &msg) {
    if (field)    field->setStyleSheet("border: 1px solid #f87171;");
    if (errLabel) errLabel->setText(msg);
}

void NavireCrudDialog::clearFieldError(QLineEdit *field, QLabel *errLabel) {
    if (field)    field->setStyleSheet(QString());
    if (errLabel) errLabel->clear();
}

void NavireCrudDialog::validateField(QLineEdit *field, QLabel *errLabel, const QString &errorMsg, bool condition) {
    if (condition) clearFieldError(field, errLabel);
    else setFieldError(field, errLabel, errorMsg);
}

// ── Validators ──────────────────────────────────────────────────
void NavireCrudDialog::setupValidators() {
    ui->inputNom->setMaxLength(80);
    ui->inputMatricule->setValidator(
        new QRegularExpressionValidator(QRegularExpression(R"([A-Za-z0-9\-]{0,20})"), this));
    ui->inputMatricule->setMaxLength(20);
    ui->inputType->setMaxLength(50);
    ui->inputProprietaire->setMaxLength(100);
}

// ── Real-time feedback ──────────────────────────────────────────
void NavireCrudDialog::connectRealTimeValidation() {
    connect(ui->inputNom, &QLineEdit::textChanged, this, [this](const QString &t) {
        validateField(ui->inputNom, ui->errNom,
                      tr("⚠ Nom requis (min 2 caractères)."), t.trimmed().length() >= 2);
    });
    connect(ui->inputMatricule, &QLineEdit::textChanged, this, [this](const QString &t) {
        const QRegularExpression rx(R"([A-Za-z0-9\-]{3,20})");
        validateField(ui->inputMatricule, ui->errMatricule,
                      tr("⚠ Matricule invalide (ex: TN-2024-001)."),
                      rx.match(t.trimmed()).hasMatch());
    });
    connect(ui->inputType, &QLineEdit::textChanged, this, [this](const QString &t) {
        validateField(ui->inputType, ui->errType,
                      tr("⚠ Type de navire requis."), !t.trimmed().isEmpty());
    });
    connect(ui->inputProprietaire, &QLineEdit::textChanged, this, [this](const QString &t) {
        validateField(ui->inputProprietaire, ui->errProprietaire,
                      tr("⚠ Nom propriétaire trop court."),
                      t.trimmed().isEmpty() || t.trimmed().length() >= 3);
    });
}

// ── Full validation on Save ─────────────────────────────────────
bool NavireCrudDialog::validateAll() {
    bool valid = true;

    if (ui->inputNom->text().trimmed().length() < 2) {
        setFieldError(ui->inputNom, ui->errNom, tr("⚠ Nom requis (min 2 caractères)."));
        valid = false;
    } else clearFieldError(ui->inputNom, ui->errNom);

    const QRegularExpression rx(R"([A-Za-z0-9\-]{3,20})");
    if (!rx.match(ui->inputMatricule->text().trimmed()).hasMatch()) {
        setFieldError(ui->inputMatricule, ui->errMatricule,
                      tr("⚠ Matricule invalide (lettres, chiffres, tirets, 3-20 car.)."));
        valid = false;
    } else clearFieldError(ui->inputMatricule, ui->errMatricule);

    if (ui->inputType->text().trimmed().isEmpty()) {
        setFieldError(ui->inputType, ui->errType, tr("⚠ Type de navire requis."));
        valid = false;
    } else clearFieldError(ui->inputType, ui->errType);

    if (ui->spinLongueur->value() <= 0.0) {
        ui->errLongueur->setText(tr("⚠ Longueur doit être > 0 m."));
        valid = false;
    } else ui->errLongueur->clear();

    if (ui->spinPuissance->value() <= 0.0) {
        ui->errPuissance->setText(tr("⚠ Puissance doit être > 0 CV."));
        valid = false;
    } else ui->errPuissance->clear();

    return valid;
}

// ── Load for Edit ───────────────────────────────────────────────
bool NavireCrudDialog::loadNavire(int idNavire) {
    Navire n;
    if (!n.charger(idNavire)) {
        QMessageBox::warning(this, tr("Erreur"),
                             n.lastError().isEmpty() ? tr("Chargement impossible.") : n.lastError());
        return false;
    }
    ui->inputNom->setText(n.getNomNavire());
    ui->inputMatricule->setText(n.getMatricule());
    ui->inputType->setText(n.getTypeNavire());
    ui->spinLongueur->setValue(n.getLongueur());
    ui->spinPuissance->setValue(n.getPuissanceMoteur());
    ui->inputProprietaire->setText(n.getProprietaire());
    int idx = ui->inputStatut->findText(n.getStatutNavire(), Qt::MatchFixedString);
    if (idx >= 0) ui->inputStatut->setCurrentIndex(idx);
    return true;
}

// ── Accept ──────────────────────────────────────────────────────
void NavireCrudDialog::onAccept() {
    if (save()) accept();
}

bool NavireCrudDialog::save() {
    if (!validateAll()) {
        QMessageBox::warning(this, tr("Formulaire incomplet"),
                             tr("Veuillez corriger les champs en rouge avant de sauvegarder."));
        return false;
    }

    const QString nom          = ui->inputNom->text().trimmed();
    const QString matricule    = ui->inputMatricule->text().trimmed().toUpper();
    const QString type         = ui->inputType->text().trimmed();
    const double  longueur     = ui->spinLongueur->value();
    const double  puissance    = ui->spinPuissance->value();
    const QString proprietaire = ui->inputProprietaire->text().trimmed();
    const QString statut       = ui->inputStatut->currentText();

    bool okSave = false;

    if (m_mode == AddMode) {
        // ID = 0; ajouter() will use NAVIRE_SEQ.NEXTVAL and store the generated ID back
        Navire n(0, nom, matricule, type, longueur, puissance, proprietaire, statut);
        okSave = n.ajouter();
        if (okSave) m_navireId = n.getIdNavire(); // pick up the sequence-generated ID
    } else {
        Navire n(m_navireId, nom, matricule, type, longueur, puissance, proprietaire, statut);
        okSave = n.modifier();
    }

    if (!okSave) {
        QMessageBox::warning(this, tr("Erreur SQL"), tr("Enregistrement échoué."));
        return false;
    }
    return true;
}

// ── Style ────────────────────────────────────────────────────────
void NavireCrudDialog::applyStyle() {
    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #0F172A; color: #E2E8F0; font-family: 'Segoe UI','Inter','Arial'; font-size: 13px; }"
        "QLabel#labelTitle { font-size: 20px; font-weight: 700; color: #E2E8F0; }"
        "QLabel#labelSectionForm { font-size: 14px; font-weight: 600; color: #E2E8F0; }"
        "QLabel { color: #A0AEC0; font-weight: 500; }"
        "QFrame#formFrame { background-color: #1E293B; border-radius: 16px; }"
        "QLineEdit, QDoubleSpinBox, QComboBox { background-color: #0F172A; border: 1px solid #334155; border-radius: 8px; padding: 8px; color: #E2E8F0; }"
        "QLineEdit:focus, QDoubleSpinBox:focus, QComboBox:focus { border: 1px solid #3B82F6; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox QAbstractItemView { background: #1E293B; color: #E2E8F0; selection-background-color: #3B82F6; }"
        "QPushButton { border: none; border-radius: 10px; padding: 10px 20px; background: #334155; color: #E2E8F0; }"
        "QPushButton:hover { background: #475569; }"
        "QPushButton[primary=\"true\"] { background: #3B82F6; color: #FFFFFF; }"
        "QPushButton[primary=\"true\"]:hover { background: #2563EB; }"
    ));
}
