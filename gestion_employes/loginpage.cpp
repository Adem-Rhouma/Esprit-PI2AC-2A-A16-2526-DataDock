#include "loginpage.h"
#include <QMessageBox>
#include <QAction>
#include <QFont>
#include <QFontDatabase>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QDir>
#include <QStandardPaths>
#include"gestion_employes/employe.h"
#include"email.h"
#include <QSettings>
LoginPage::LoginPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("LoginPage"));
    setupUi();
    applyStyles();

    connect(loginButton, &QPushButton::clicked, this, &LoginPage::onLoginClicked);
    connect(faceIdButton, &QPushButton::clicked, this, &LoginPage::onFaceIdClicked);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginPage::onLoginClicked);
    connect(forgotButton, &QPushButton::clicked,
            this, &LoginPage::onForgotPasswordClicked);

    resetServer = new PasswordResetServer(this);
    resetServer->startServer(1705);
    // QTimer::singleShot(500, this, [this]() {
    //     resetServer->startServer(1705);
    // });
}

void LoginPage::focusUsername()
{
    if (usernameEdit) {
        usernameEdit->setFocus();
    }
}

void LoginPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(60, 40, 60, 40);
    rootLayout->setSpacing(0);

    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("loginCard"));
    card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    card->setMinimumWidth(420);
    card->setMaximumWidth(520);

    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(24.0);
    shadow->setOffset(0.0, 10.0);
    shadow->setColor(QColor(0, 0, 0, 60));
    card->setGraphicsEffect(shadow);

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(40, 36, 40, 36);
    cardLayout->setSpacing(18);

    logoLabel = new QLabel(card);
    logoLabel->setObjectName(QStringLiteral("loginLogo"));
    logoLabel->setPixmap(QPixmap(QStringLiteral(":/assets/logo.png")));
    logoLabel->setFixedSize(110, 110);
    logoLabel->setScaledContents(true);
    logoLabel->setAlignment(Qt::AlignCenter);

    titleLabel = new QLabel(QStringLiteral("Connexion à DataDock"), card);
    titleLabel->setObjectName(QStringLiteral("loginTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);

    subtitleLabel = new QLabel(QStringLiteral("Veuillez saisir vos identifiants pour continuer."), card);
    subtitleLabel->setObjectName(QStringLiteral("loginSubtitle"));
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setWordWrap(true);
    subtitleLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    subtitleLabel->setMinimumHeight(subtitleLabel->fontMetrics().lineSpacing() * 2);

    usernameEdit = new QLineEdit(card);
    usernameEdit->setObjectName(QStringLiteral("usernameEdit"));
    usernameEdit->setPlaceholderText(QStringLiteral("Nom d'utilisateur"));
    usernameEdit->setClearButtonEnabled(true);

    const QIcon userIcon(QStringLiteral(":/assets/user.svg"));
    QAction *userAction = usernameEdit->addAction(userIcon, QLineEdit::LeadingPosition);
    userAction->setEnabled(false);

    passwordEdit = new QLineEdit(card);
    passwordEdit->setObjectName(QStringLiteral("passwordEdit"));
    passwordEdit->setPlaceholderText(QStringLiteral("Mot de passe"));
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setClearButtonEnabled(false);

    const QIcon lockIcon(QStringLiteral(":/assets/lock.svg"));
    QAction *lockAction = passwordEdit->addAction(lockIcon, QLineEdit::LeadingPosition);
    lockAction->setEnabled(false);

    togglePasswordAction = passwordEdit->addAction(QIcon(), QLineEdit::TrailingPosition);
    togglePasswordAction->setCheckable(true);
    connect(togglePasswordAction, &QAction::triggered, this, &LoginPage::onTogglePasswordVisibility);
    updatePasswordToggleIcon();

    forgotButton = new QPushButton(QStringLiteral("Mot de passe oublié ?"), card);
    forgotButton->setObjectName(QStringLiteral("forgotLink"));
    forgotButton->setCursor(Qt::PointingHandCursor);
    forgotButton->setFlat(true);
    forgotButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    auto *forgotLayout = new QHBoxLayout();
    forgotLayout->setContentsMargins(0, 0, 0, 0);
    forgotLayout->addStretch();
    forgotLayout->addWidget(forgotButton);

    loginButton = new QPushButton(QStringLiteral("Connexion"), card);
    loginButton->setObjectName(QStringLiteral("btnConnexion"));
    loginButton->setCursor(Qt::PointingHandCursor);

    faceIdButton = new QPushButton(QStringLiteral("Se connecter avec Face ID"), card);
    faceIdButton->setObjectName(QStringLiteral("btnFaceId"));
    faceIdButton->setCursor(Qt::PointingHandCursor);

    auto *headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(12);
    headerLayout->addWidget(logoLabel, 0, Qt::AlignHCenter);
    headerLayout->addWidget(titleLabel, 0, Qt::AlignHCenter);
    headerLayout->addWidget(subtitleLabel, 0, Qt::AlignHCenter);

    cardLayout->addLayout(headerLayout);
    cardLayout->addSpacing(6);
    cardLayout->addWidget(usernameEdit);
    cardLayout->addWidget(passwordEdit);
    cardLayout->addLayout(forgotLayout);
    cardLayout->addSpacing(6);
    cardLayout->addWidget(loginButton);
    cardLayout->addWidget(faceIdButton);

    rootLayout->addStretch(1);
    rootLayout->addWidget(card, 0, Qt::AlignHCenter);
    rootLayout->addStretch(1);
}

void LoginPage::updatePasswordToggleIcon()
{
    if (!togglePasswordAction) {
        return;
    }

    const QString iconPath = passwordVisible
                                 ? QStringLiteral(":/assets/eye_off.svg")
                                 : QStringLiteral(":/assets/eye.svg");
    togglePasswordAction->setIcon(QIcon(iconPath));
    togglePasswordAction->setToolTip(passwordVisible
                                         ? QStringLiteral("Masquer le mot de passe")
                                         : QStringLiteral("Afficher le mot de passe"));
}

void LoginPage::onTogglePasswordVisibility()
{
    passwordVisible = !passwordVisible;
    if (passwordEdit) {
        passwordEdit->setEchoMode(passwordVisible ? QLineEdit::Normal : QLineEdit::Password);
    }
    if (togglePasswordAction) {
        togglePasswordAction->setChecked(passwordVisible);
    }
    updatePasswordToggleIcon();
}

void LoginPage::applyStyles()
{
    const int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/assets/CalSans-Regular.ttf"));
    if (fontId >= 0) {
        const QString family = QFontDatabase::applicationFontFamilies(fontId).value(0);
        if (!family.isEmpty()) {
            QFont font(family);
            font.setPointSize(10);
            setFont(font);
        }
    }

    setStyleSheet(QStringLiteral(R"(
        QWidget#LoginPage {
            background: transparent;
        }

        QFrame#loginCard {
            background: rgba(255, 255, 255, 238);
            border-radius: 22px;
        }

        QLabel#loginTitle {
            font-size: 26px;
            font-weight: 700;
            color: #1f2a44;
        }

        QLabel#loginSubtitle {
            font-size: 14px;
            color: rgba(31, 42, 68, 170);
        }

        QLineEdit#usernameEdit,
        QLineEdit#passwordEdit {
            background: #ffffff;
            border: 1px solid rgba(31, 42, 68, 40);
            border-radius: 12px;
            padding: 10px 14px 10px 40px;
            font-size: 14px;
            color: #1f2a44;
            min-height: 44px;
        }

        QLineEdit#usernameEdit:focus,
        QLineEdit#passwordEdit:focus {
            border: 1px solid rgba(47, 107, 255, 160);
        }

QPushButton#forgotLink {
    background: transparent;
    border: none;
    padding: 0;
    font-size: 13px;
    color: #2f6bff;
    text-decoration: underline;
    text-align: right;
}

QPushButton#forgotLink:hover {
    color: #2658d6;
}

QPushButton#forgotLink:pressed {
    color: #1f49b0;
}

        QPushButton#btnConnexion {
            background: #2f6bff;
            color: white;
            border: none;
            border-radius: 12px;
            font-size: 16px;
            font-weight: 600;
            padding: 12px 16px;
            min-height: 48px;
        }

        QPushButton#btnConnexion:hover {
            background: #2658d6;
        }

        QPushButton#btnConnexion:pressed {
            background: #1f49b0;
        }

        QPushButton#btnFaceId {
            background: #ffffff;
            color: #2f6bff;
            border: 1px solid #2f6bff;
            border-radius: 12px;
            font-size: 15px;
            font-weight: 600;
            padding: 12px 16px;
            min-height: 48px;
        }

        QPushButton#btnFaceId:hover {
            background: #eef4ff;
        }

        QPushButton#btnFaceId:pressed {
            background: #dfeaff;
        }
    )"));
}
void LoginPage::onLoginClicked()
{
    Employe e;
    e.set_email(usernameEdit->text());
    e.set_mdp(passwordEdit->text());
    if(e.login())
    {
        QSettings settings("MonApp", "GestionEmployes");
        settings.setValue("login/nom", e.get_nom());
        settings.setValue("login/prenom", e.get_prenom());
        loginRequested();
    }
    else
    {
        QMessageBox::warning(this, "Échec de connexion", "Email ou mot de passe incorrect.");
    }
}


void LoginPage::onFaceIdClicked()
{
    if (camera) {
        return;
    }

    const auto cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty()) {
        QMessageBox::warning(this, "Caméra", "Aucune caméra détectée.");
        return;
    }

    camera = new QCamera(cameras.first(), this);
    captureSession = new QMediaCaptureSession(this);
    imageCapture = new QImageCapture(this);

    captureSession->setCamera(camera);
    captureSession->setImageCapture(imageCapture);

    connect(imageCapture, &QImageCapture::imageCaptured,
            this, &LoginPage::onImageCaptured);

    connect(imageCapture, &QImageCapture::imageSaved,
            this, &LoginPage::onImageSaved);

    connect(imageCapture, &QImageCapture::errorOccurred,
            this, &LoginPage::onImageCaptureError);

    camera->start();

    const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    const QString usersDirPath = desktopPath + "/DataDock/2a16-smart-fishing-port-management/assets/users";
    QDir().mkpath(usersDirPath);
    const QString path = usersDirPath + "/face.jpg";

    QTimer::singleShot(1500, this, [this, path]() {
        if (!imageCapture) {
            QMessageBox::warning(this, "Capture", "Le module de capture n'est pas disponible.");
            return;
        }

        if (!imageCapture->isReadyForCapture()) {
            QMessageBox::warning(this, "Capture", "La caméra n'est pas encore prête pour la capture.");
            return;
        }

        imageCapture->captureToFile(path);
    });
}

void LoginPage::onImageCaptured(int id, const QImage &preview)
{
    Q_UNUSED(id);

    capturedFaceImage = preview;

    QMessageBox::information(this, "Capture", "Image capturée avec succès.");

    emit faceImageCaptured(capturedFaceImage);
    Employe e;
    if(e.faceid(preview))
    {
        QString infos =
            "ID : " + e.get_id() + "\n" +
            "Nom : " + e.get_nom() + "\n" +
            "Prénom : " + e.get_prenom() + "\n" +
            "Rôle : " + e.get_role() + "\n" +
            "Email : " + e.get_email();

        QMessageBox::information(this, "Utilisateur reconnu", infos);
        QSettings settings("MonApp", "GestionEmployes");
        settings.setValue("login/nom", e.get_nom());
        settings.setValue("login/prenom", e.get_prenom());
         loginRequested();
    }
    else
    {
        QMessageBox::warning(this, "Échec de connexion", "non enregistrer.");

    }
}
void LoginPage::onImageCaptureError(int id, QImageCapture::Error error, const QString &errorString)
{
    Q_UNUSED(id);
    Q_UNUSED(error);

    QMessageBox::warning(this, "Erreur capture", errorString);
}
LoginPage::~LoginPage()
{
    if (camera) {
        camera->stop();
    }
}

void LoginPage::onImageSaved(int id, const QString &fileName)
{
    Q_UNUSED(id);
    QMessageBox::information(this, "Capture", "Image enregistrée : " + fileName);
}
void LoginPage::onForgotPasswordClicked()
{
    QString email = usernameEdit->text().trimmed();

    if (email.isEmpty()) {
        QMessageBox::warning(this, "Mot de passe oublié",
                             "Veuillez saisir votre email dans le champ username.");
        usernameEdit->setFocus();
        return;
    }

    Employe e;
    e.set_email(email);

    if (e.employeExiste()) {
        mailer m;
        QString name = e.get_prenom() + " " + e.get_nom();
        
        // Generate Token and Link
        QString token = resetServer->createToken(email);
        QString link = "http://localhost:1705/reset?token=" + token;
        
        m.sendEmail(email, "Password Reset Request", link, name);
        QMessageBox::information(this, "Mot de passe oublié",
                                 "Un email avec un lien de réinitialisation a été envoyé.");
    } else {
        QMessageBox::warning(this, "Mot de passe oublié",
                             "Aucun employé n'existe avec cet email.");
    }
}
