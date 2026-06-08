#include "employespage.h"
#include "ui_employespage.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QPrinter>
#include <QPainter>
#include <QFileDialog>
#include <QTextStream>
#include <QDate>
#include <QDateTime>
#include <QTextDocument>
#include <QPageSize>
#include <QMessageBox>
#include <QLocale>
#include <QCamera>
#include <QCameraDevice>
#include <QMediaDevices>
#include <QMediaCaptureSession>
#include <QImageCapture>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDebug>
#include<QTimer>
#include <QDir>
#include <QStandardPaths>
#include <QSettings>
EmployesPage::EmployesPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EmployesPage)
{
    ui->setupUi(this);

    // 1) Instanciation correcte du modèle (avec parent)
    model = new QStandardItemModel(this);

    // Si tu as plusieurs pages, assure-toi d'afficher la bonne
    ui->employee->show();
    ui->GPAJOUT->hide();
    ui->gpmod->hide();
    ui->gpst->hide();
    ui->gpinf->hide();  //  Cacher gpinf au démarrage
    ui->tableView->hideColumn(6);

    // 2) Headers + colonnes
    QStringList headers = {
        "ID_Employe","Nom","Prenom","Role","Email",
        "Mot_De_Passe","Photo_Identite", "Sexe",
        "Action"
    };

    model->setColumnCount(headers.size());
    model->setHorizontalHeaderLabels(headers);

    // 4) Brancher le model
    ui->tableView->setModel(model);

    // 5) Options d'affichage
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);

    // Cacher la colonne Mot de passe (index 5)
    ui->tableView->hideColumn(5);
    ui->tableView->hideColumn(7);

    // Initial load from DB
    refresh();

    ui->tableView->horizontalHeader()->setStyleSheet(R"(
QHeaderView::section {
    background-color: #38ACEC;
    color: white;
    padding: 8px;
    border: 1px solid #95B9C6;
    font-weight: bold;
    font-size: 13px;
})");

    ui->tableView->verticalHeader()->setVisible(false);
    ui->tableView->verticalHeader()->setDefaultSectionSize(70);

    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableView->setColumnWidth(0, 100); // ID_Employe
    ui->tableView->setColumnWidth(1, 70);  // Nom
    ui->tableView->setColumnWidth(2, 100); // Prenom
    ui->tableView->setColumnWidth(3, 100); // Role
    ui->tableView->setColumnWidth(4, 200); // Email
    ui->tableView->setColumnWidth(5, 150); // Mot_De_Passe
    ui->tableView->setColumnWidth(6, 120); // Photo_Identite
    ui->tableView->setColumnWidth(8, 200); // Action

    ui->tableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //creation des validator
    QRegularExpression rx("^[A-Za-z]+$");
    QValidator *validator2 = new QRegularExpressionValidator(rx, this);

    QRegularExpression rx2("^[0-9]+$");
    QValidator *validator1 = new QRegularExpressionValidator(rx2, this);

    QRegularExpression rxEmail("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
    QValidator *validatorEmail = new QRegularExpressionValidator(rxEmail, this);
    //AJOUT EMPLOYE
    ui->lineEdit_10->setValidator(validator2) ; //nom
    ui->lineEdit_8->setValidator(validator2) ; //prenom
    ui->lineEdit_9->setValidator(validator1); //id
    ui->lineEdit_12->setValidator(validatorEmail); //EMAIL

    //MODIFICATION EMPLOYE
    ui->lineEdit_16->setValidator(validator1); // id numerique
    ui->lineEdit_16->setMaxLength(8);          // 8 chiffres maximum

    QSettings settings("MonApp", "GestionEmployes");

    QString nom = settings.value("login/nom", "").toString();

    Q_UNUSED(nom);

    auto *attestationButton = new QPushButton(ui->employee);
    attestationButton->setObjectName("btnAttestationPdf");
    attestationButton->setFixedSize(42, 42);
    const QRect exportRect = ui->affichage_3->geometry();
    const int btnX = exportRect.right() - attestationButton->width() - 6;
    const int btnY = exportRect.top() - attestationButton->height() - 10;
    attestationButton->move(btnX, btnY);
    attestationButton->setToolTip("Exporter une attestation de travail (PDF)");
    attestationButton->setCursor(Qt::PointingHandCursor);
    attestationButton->setIcon(QIcon(":/assets/ic_export.png"));
    attestationButton->setIconSize(QSize(24, 24));
    attestationButton->setStyleSheet(
        "QPushButton {"
        " background-color: #ffffff;"
        " border: 2px solid #4DA3FF;"
        " border-radius: 12px;"
        "}"
        "QPushButton:hover {"
        " background-color: #EAF4FF;"
        "}"
        "QPushButton:pressed {"
        " background-color: #D8EAFF;"
        "}"
        );

    connect(attestationButton, &QPushButton::clicked, this, &EmployesPage::onAttestationClicked);



}


EmployesPage::~EmployesPage()
{
    delete ui;
}

void EmployesPage::on_ajouter_clicked()
{
    ui->GPAJOUT->show();
    ui->employee->hide();
    ui->gpmod->hide();
    ui->gpst->hide();
    ui->gpinf->hide();
}

void EmployesPage::on_statistique_clicked()
{
    ui->gpst->show();
    ui->gpmod->hide();
    ui->GPAJOUT->hide();
    ui->employee->hide();
    ui->gpinf->hide();
}

// BOUTONS RETOUR - Tous cachent gpinf et affichent employee
void EmployesPage::on_rt1_2_clicked()
{
    ui->gpst->hide();
    ui->gpmod->hide();
    ui->GPAJOUT->hide();
    ui->gpinf->hide();
    ui->employee->show();
}

void EmployesPage::on_rt1_clicked()
{
    ui->gpst->hide();
    ui->gpmod->hide();
    ui->GPAJOUT->hide();
    ui->gpinf->hide();
    ui->employee->show();
}

void EmployesPage::on_rt1_3_clicked()
{
    ui->gpst->hide();
    ui->gpmod->hide();
    ui->GPAJOUT->hide();
    ui->gpinf->hide();
    ui->employee->show();
}

void EmployesPage::on_rt1_5_clicked()
{
    ui->gpst->hide();
    ui->gpmod->hide();
    ui->GPAJOUT->hide();
    ui->gpinf->hide();
    ui->employee->show();
}

// BOUTONS DE CONFIRMATION CRUD
void EmployesPage::envoyerImageFace(const QString &imagePath, std::function<void(QJsonArray)> callback)
{
    QUrl url("http://127.0.0.1:5000/face/register");
    QNetworkRequest request(url);

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant("form-data; name=\"image\"; filename=\"" + QFileInfo(imagePath).fileName() + "\""));
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));

    QFile *file = new QFile(imagePath);
    if (!file->open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Erreur", "Impossible d'ouvrir l'image.");
        delete file;
        delete multiPart;
        callback(QJsonArray());
        return;
    }

    imagePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(imagePart);

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        QByteArray responseData = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Erreur API :" << reply->errorString();
            callback(QJsonArray());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (!doc.isObject()) {
            callback(QJsonArray());
            reply->deleteLater();
            return;
        }

        QJsonObject obj = doc.object();

        bool success = obj["success"].toBool();
        if (!success) {
            callback(QJsonArray());
            reply->deleteLater();
            return;
        }

        QJsonArray embeddingArray = obj["embedding"].toArray();
        callback(embeddingArray);

        reply->deleteLater();
    });
}
void EmployesPage::on_ajouterr_clicked() // Bouton Ajouter dans GPAJOUT
{
    QString nom = ui->lineEdit_10->text().trimmed();
    QString prenom = ui->lineEdit_8->text().trimmed();
    QString id = ui->lineEdit_9->text().trimmed();
    QString email = ui->lineEdit_12->text().trimmed();
    QString mdp = ui->lineEdit_11->text();
    QString role = ui->comboBox_3->currentText();

    QString photo = "C:/photos/" + nom.toLower() + ".png";

    Employe E(id, nom, prenom, role, email, mdp, photo);

    QRegularExpression rxNomPrenom("^[A-Za-z\\s]+$");
    QRegularExpression rxId("^[0-9]+$");
    QRegularExpression rxEmail("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");
    QRegularExpression rxMdp("^(?=.*[a-z])(?=.*[A-Z])(?=.*[0-9])(?=.*[!@#$%^&*]).{8,}$");

    if (id.isEmpty() || !rxId.match(id).hasMatch())
    {
        QMessageBox::critical(this, QObject::tr("Erreur de saisie"),
                              QObject::tr("L'ID est invalide.\nVeuillez saisir un ID numérique."),
                              QMessageBox::Ok);
        return;
    }

    if (nom.isEmpty() || !rxNomPrenom.match(nom).hasMatch())
    {
        QMessageBox::critical(this, QObject::tr("Erreur de saisie"),
                              QObject::tr("Le nom est invalide.\nVeuillez saisir un nom valide (lettres uniquement)."),
                              QMessageBox::Ok);
        return;
    }

    if (prenom.isEmpty() || !rxNomPrenom.match(prenom).hasMatch())
    {
        QMessageBox::critical(this, QObject::tr("Erreur de saisie"),
                              QObject::tr("Le prénom est invalide.\nVeuillez saisir un prénom valide (lettres uniquement)."),
                              QMessageBox::Ok);
        return;
    }

    if (email.isEmpty() || !rxEmail.match(email).hasMatch())
    {
        QMessageBox::critical(this, QObject::tr("Erreur de saisie"),
                              QObject::tr("L'email est invalide.\nVeuillez saisir une adresse email valide (exemple@domaine.com)."),
                              QMessageBox::Ok);
        return;
    }

    if (!rxMdp.match(mdp).hasMatch())
    {
        QMessageBox::critical(this, QObject::tr("Erreur de saisie"),
                              QObject::tr("Le mot de passe est invalide.\nIl doit contenir au moins 8 caractères, une majuscule, une minuscule, un chiffre et un caractère spécial (!@#$%^&*)."),
                              QMessageBox::Ok);
        return;
    }

    if (!E.ajouter()) {
        QMessageBox::critical(this, QObject::tr("Ajout échoué"),
                              QObject::tr("L'employé existe déjà ou une erreur est survenue.\nVérifiez l'ID."),
                              QMessageBox::Ok);
        return;
    }

    qDebug() << "Ajout OK";

    refresh();
    ui->GPAJOUT->hide();
    ui->employee->show();

    ui->lineEdit_10->clear();
    ui->lineEdit_8->clear();
    ui->lineEdit_9->clear();
    ui->lineEdit_12->clear();
    ui->lineEdit_11->clear();

    auto cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty()) {
        QMessageBox::warning(this, "Caméra", "Aucune caméra détectée.");
        return;
    }

    QCamera *camera = new QCamera(cameras.first(), this);
    QMediaCaptureSession *session = new QMediaCaptureSession(this);
    QImageCapture *imageCapture = new QImageCapture(this);

    session->setCamera(camera);
    session->setImageCapture(imageCapture);

    const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    const QString usersDirPath = desktopPath + "/DataDock/2a16-smart-fishing-port-management/assets/users";
    QDir().mkpath(usersDirPath);
    const QString path = usersDirPath + "/capture.jpg";

    int *tentatives = new int(0);
    const int maxTentatives = 5;

    std::function<void()> lancerCapture;

    lancerCapture = [=]() mutable {
        if (!camera->isActive()) {
            camera->start();
        }

        if (!imageCapture->isReadyForCapture()) {
            QTimer::singleShot(800, this, [=]() mutable {
                lancerCapture();
            });
            return;
        }

        qDebug() << "Tentative capture :" << (*tentatives + 1);
        imageCapture->capture();
    };

    connect(imageCapture, &QImageCapture::imageCaptured,
            this,
            [=](int captureId, const QImage &preview) mutable {
                Q_UNUSED(captureId);

                if (!preview.save(path)) {
                    QMessageBox::warning(this, "Erreur", "Impossible d'enregistrer l'image.");
                    camera->stop();
                    return;
                }

                envoyerImageFace(path, [=](QJsonArray embeddingArray) mutable {
                    if (!embeddingArray.isEmpty()) {
                        QString embeddingJson = QString::fromUtf8(
                            QJsonDocument(embeddingArray).toJson(QJsonDocument::Compact)
                            );

                        qDebug() << "Embedding reçu :" << embeddingJson;

                        camera->stop();


                        // ici tu peux le stocker dans Oracle si tu veux
                        // exemple :
                        // E.set_face_embedding(embeddingJson);
                        // E.updateEmbedding();
                        E.embedding = embeddingJson ;
                        if(E.ajoutFaceID())
                        {
                            qDebug()<< E.get_id();
                            QMessageBox::information(this,
                                                     "Succès API",
                                                     "Embedding reçu avec succès.");
                        }
                        else
                        {
                            qDebug()<< E.get_id();
                            QMessageBox::information(this,
                                                     "echouer API",
                                                     "Embedding reçu avec succès.");
                        }
                    } else {
                        (*tentatives)++;

                        if (*tentatives >= maxTentatives) {
                            camera->stop();
                            QMessageBox::warning(this,
                                                 "Échec API",
                                                 "Impossible d'obtenir un embedding valide après plusieurs tentatives.");
                            return;
                        }

                        qDebug() << "Embedding vide, nouvelle tentative...";
                        QTimer::singleShot(1000, this, [=]() mutable {
                            lancerCapture();
                        });
                    }
                });
            });

    connect(imageCapture, &QImageCapture::errorOccurred,
            this,
            [=](int captureId, QImageCapture::Error error, const QString &errorString) {
                Q_UNUSED(captureId);
                Q_UNUSED(error);

                camera->stop();
                QMessageBox::warning(this, "Erreur capture", errorString);
            });

    lancerCapture();

    QMessageBox::information(this,
                             QObject::tr("Ajout réussi"),
                             QObject::tr("L'employé a été ajouté avec succès.\nLa capture faciale va commencer."),
                             QMessageBox::Ok);
}

// Modification
void EmployesPage::on_ajouterr_2_clicked() // Bouton Modifier dans GPMOD
{
    // Mapping RE-VERIFIE sur GPMOD (xml lines 806+)
    // lineEdit_14 (y 120) -> Label "NOM" (y 130) => NOM
    // lineEdit_15 (y 180) -> Label 3 "PRENOM" (y 190) => PRENOM
    // lineEdit_16 (y 240) -> Label 4 "ID" (y 250) => ID
    // lineEdit_17 (y 300) -> Label 6 "EMAIL" (y 310) => EMAIL
    // lineEdit_18 (y 360) -> Label 11 "MOT DE PASSE" (y 370) => MDP
    // comboBox_4 (y 420) -> Label 5 "ROLE" (y 430) => ROLE

    QString nom = ui->lineEdit_14->text();
    QString prenom = ui->lineEdit_15->text();
    QString id = ui->lineEdit_16->text();
    QString email = ui->lineEdit_17->text();
    QString mdp = ui->lineEdit_18->text();
    QString role = ui->comboBox_4->currentText();

    // Récupérer l'ancienne photo stockée
    QString photo = ui->lineEdit_16->property("original_photo").toString();

    Employe E(id, nom, prenom, role, email, mdp, photo);

    // Regex
    QRegularExpression rxNomPrenom("^[A-Za-z\\s]+$");
    QRegularExpression rxId("^[0-9]+$");
    QRegularExpression rxEmail("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$");

    // Validations
    if (id.isEmpty() || !rxId.match(id).hasMatch() || id.length() > 8)
    {
        QMessageBox::critical(nullptr, QObject::tr("Erreur de saisie"),
                              QObject::tr("L'ID est invalide.\n"
                                          "Il doit contenir uniquement des chiffres (8 max).\n"
                                          "Veuillez sélectionner un employé valide."),
                              QMessageBox::Ok);
        return;
    }

    if (nom.isEmpty() || !rxNomPrenom.match(nom).hasMatch())
    {
        QMessageBox::critical(nullptr, QObject::tr("Erreur de saisie"),
                              QObject::tr("Le nom est invalide.\n"
                                          "Veuillez saisir un nom valide (lettres uniquement)."),
                              QMessageBox::Ok);
        return;
    }

    if (prenom.isEmpty() || !rxNomPrenom.match(prenom).hasMatch())
    {
        QMessageBox::critical(nullptr, QObject::tr("Erreur de saisie"),
                              QObject::tr("Le prénom est invalide.\n"
                                          "Veuillez saisir un prénom valide (lettres uniquement)."),
                              QMessageBox::Ok);
        return;
    }

    if (email.isEmpty() || !rxEmail.match(email).hasMatch())
    {
        QMessageBox::critical(nullptr, QObject::tr("Erreur de saisie"),
                              QObject::tr("L'email est invalide.\n"
                                          "Veuillez saisir une adresse email valide."),
                              QMessageBox::Ok);
        return;
    }

    // Password validation regex
    // At least one uppercase, one lowercase, one digit, one special char, min 8 chars
    QRegularExpression rxMdp("^(?=.*[a-z])(?=.*[A-Z])(?=.*[0-9])(?=.*[!@#$%^&*]).{8,}$");

    if (!rxMdp.match(mdp).hasMatch())
    {
        QMessageBox::critical(nullptr, QObject::tr("Erreur de saisie"),
                              QObject::tr("Le mot de passe est invalide.\n"
                                          "Il doit contenir au moins 8 caractères, une majuscule, une minuscule, un chiffre et un caractère spécial (!@#$%^&*)."),
                              QMessageBox::Ok);
        return;
    }

    if(E.modifier()) {
        qDebug() << "Modif OK";
        refresh();
        ui->gpmod->hide();
        ui->employee->show();
        QMessageBox::information(nullptr, QObject::tr("Modification réussie"),
                                 QObject::tr("Les informations de l'employé ont été modifiées avec succès."),
                                 QMessageBox::Ok);
    } else {
        QMessageBox::critical(nullptr, QObject::tr("Modification échouée"),
                              QObject::tr("Erreur lors de la modification.\n"
                                          "Vérifiez que l'employé existe."),
                              QMessageBox::Ok);
    }
}

void EmployesPage::on_rt1_4_clicked()
{
    ui->gpst->hide();
    ui->gpmod->show();
    ui->GPAJOUT->hide();
    ui->employee->hide();
    ui->gpinf->hide();
}

void EmployesPage::populateTable(QSqlQueryModel *qModel)
{
    model->removeRows(0, model->rowCount()); // Clear existing rows

    if(qModel)
    {
        int rows = qModel->rowCount();
        int cols = qModel->columnCount();

        for(int row = 0; row < rows; row++)
        {
            QList<QStandardItem*> rowItems;
            for(int col = 0; col < cols; col++)
            {
                QString cell = qModel->record(row).value(col).toString();
                auto *item = new QStandardItem(cell);
                item->setEditable(false);
                item->setTextAlignment(Qt::AlignCenter);
                rowItems << item;
            }

            // Add placeholder for Action column
            rowItems << new QStandardItem("");

            model->appendRow(rowItems);

            // Create buttons widget (Same logic as constructor)
            QWidget *actionWidget = new QWidget();
            QHBoxLayout *layout = new QHBoxLayout(actionWidget);
            layout->setContentsMargins(8, 8, 8, 8);
            layout->setSpacing(15);
            layout->addStretch();

            QPushButton *btnView = new QPushButton("👁");
            QPushButton *btnEdit = new QPushButton("✏️");
            QPushButton *btnDelete = new QPushButton("🗑");

            btnView->setFixedSize(35, 35);
            btnEdit->setFixedSize(35, 35);
            btnDelete->setFixedSize(35, 35);

            btnView->setStyleSheet(
                "QPushButton { background:#1565C0; border-radius:10px; color:white; font-size:20px; padding:8px; font-weight:bold; }"
                "QPushButton:hover { background:#1976D2; }"
                );
            btnEdit->setStyleSheet(
                "QPushButton { background:#4DA3FF; border-radius:10px; color:white; font-size:20px; padding:8px; font-weight:bold; }"
                "QPushButton:hover { background:#64B5FF; }"
                );
            btnDelete->setStyleSheet(
                "QPushButton { background:#72EFDD; border-radius:10px; color:white; font-size:20px; padding:8px; font-weight:bold; }"
                "QPushButton:hover { background:#FF8787; }"
                );

            // Fetch ID for safety
            // Note: capturing 'row' is brittle if subsequent rows are deleted, but standard for this pattern
            connect(btnView, &QPushButton::clicked, this, [this, row]() {
                QString currentId = model->item(row, 0)->text();
                QString nom = model->item(row, 1)->text();
                QString prenom = model->item(row, 2)->text();
                QString role = model->item(row, 3)->text();
                QString email = model->item(row, 4)->text();
                QString motDePasse = model->item(row, 5)->text();

                ui->lineEdit_24->setText(currentId);
                ui->lineEdit_20->setText(nom);
                ui->lineEdit_21->setText(prenom);
                ui->lineEdit_22->setText(email);
                ui->lineEdit_23->setText(role);
                ui->lineEdit_25->setText(motDePasse);

                ui->lineEdit_24->setReadOnly(true);
                ui->lineEdit_20->setReadOnly(true);
                ui->lineEdit_21->setReadOnly(true);
                ui->lineEdit_22->setReadOnly(true);
                ui->lineEdit_23->setReadOnly(true);
                ui->lineEdit_25->setReadOnly(true);

                ui->gpinf->show();
                ui->GPAJOUT->hide();
                ui->employee->hide();
                ui->gpmod->hide();
                ui->gpst->hide();
            });

            connect(btnEdit, &QPushButton::clicked, this, [this, row]() {
                QString id = model->item(row, 0)->text();
                QString nom = model->item(row, 1)->text();
                QString prenom = model->item(row, 2)->text();
                QString role = model->item(row, 3)->text();
                QString email = model->item(row, 4)->text();
                QString motDePasse = model->item(row, 5)->text();
                QString photo = model->item(row, 6)->text(); // Récupérer la photo

                // CORRECT MAPPING FOR GPMOD:
                ui->lineEdit_16->setText(id);
                // Interdire la modification de l'ID en mode modification
                ui->lineEdit_16->setReadOnly(true);
                // Stocker l'ancien ID pour la requête UPDATE
                ui->lineEdit_16->setProperty("original_id", id);
                // Stocker l'ancienne photo pour ne pas la perdre
                ui->lineEdit_16->setProperty("original_photo", photo);

                ui->lineEdit_14->setText(nom);
                ui->lineEdit_15->setText(prenom);
                ui->lineEdit_17->setText(email);
                ui->lineEdit_18->setText(motDePasse);
                ui->comboBox_4->setCurrentText(role);

                ui->gpmod->show();
                ui->GPAJOUT->hide();
                ui->employee->hide();
                ui->gpst->hide();
                ui->gpinf->hide();
            });

            connect(btnDelete, &QPushButton::clicked, this, [this, row]() {
                QString id = model->item(row, 0)->text();

                QMessageBox::StandardButton reply;
                reply = QMessageBox::question(this, tr("Confirmation de suppression"),
                                              tr("Êtes-vous sûr de vouloir supprimer cet employé ?\n\n"
                                                 "Cette action est irréversible."),
                                              QMessageBox::Yes | QMessageBox::No);

                if (reply == QMessageBox::Yes) {
                    Employe E;
                    if(E.supprimer(id)) {
                        refresh(); // Refresh table after delete
                        QMessageBox::information(this, tr("Suppression réussie"),
                                                 tr("L'employé a été supprimé avec succès."));
                    } else {
                        QMessageBox::critical(this, tr("Suppression échouée"),
                                              tr("Erreur lors de la suppression de l'employé."));
                    }
                }
            });

            layout->addWidget(btnView);
            layout->addWidget(btnEdit);
            layout->addWidget(btnDelete);
            layout->addStretch();

            ui->tableView->setIndexWidget(model->index(row, 8), actionWidget); // Action is col 7 (0-6 are data)
        }
        delete qModel;
    }
}

void EmployesPage::refresh()
{
    Employe E;
    populateTable(E.afficher());
}

void EmployesPage::on_searchBar_2_textChanged(const QString &arg1)
{
    Employe E;
    if(arg1.isEmpty()) refresh();
    else populateTable(E.rechercher(arg1));
}

void EmployesPage::on_comboTrier_currentIndexChanged(int index)
{
    Employe E;
    // index 0 -> "Nom"
    // index 1 -> "Rôle"
    // index 2 -> "PRENOM"
    // index 3 -> "IDEMPLOYE"

    QString critere = "NOM"; // Par défaut

    switch(index) {
    case 0: critere = "NOM"; break;
    case 1: critere = "ROLE"; break;
    case 2: critere = "PRENOM"; break;
    case 3: critere = "IDEMPLOYE"; break;
    default: critere = "NOM"; break;
    }

    qDebug() << "Sort Index:" << index << " -> Critere SQL:" << critere;

    populateTable(E.trier(critere, "ASC"));
}

void EmployesPage::on_affichage_3_clicked()
{
    QStringList availableRoles = {
        "ADMIN",
        "AGENT",
        "RH",
        "CHAUFFEUR CAMION",
        "PECHEUR"
    };

    for (int row = 0; row < model->rowCount(); ++row) {
        const QStandardItem *roleItem = model->item(row, 3);
        const QString role = roleItem ? roleItem->text().trimmed().toUpper() : QString();
        if (!role.isEmpty() && !availableRoles.contains(role)) {
            availableRoles << role;
        }
    }

    if (availableRoles.isEmpty()) {
        QMessageBox::warning(this, "Exportation", "Aucun role disponible pour l'exportation.");
        return;
    }

    bool okRole = false;
    QString selectedRole = QInputDialog::getItem(
        this,
        "Exporter les employes",
        "Selectionnez le role a exporter :",
        availableRoles,
        0,
        false,
        &okRole
        ).trimmed().toUpper();

    if (!okRole || selectedRole.isEmpty()) {
        return;
    }

    QVector<int> filteredRows;
    for (int row = 0; row < model->rowCount(); ++row) {
        const QStandardItem *roleItem = model->item(row, 3);
        const QString role = roleItem ? roleItem->text().trimmed().toUpper() : QString();
        if (role == selectedRole) {
            filteredRows.append(row);
        }
    }

    if (filteredRows.isEmpty()) {
        QMessageBox::information(this,
                                 "Exportation",
                                 QString("Aucun employe trouve pour le role %1.").arg(selectedRole));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Exporter en PDF et TXT", QString(), "PDF Files (*.pdf);;Text Files (*.txt)");
    if (fileName.isEmpty())
        return;

    // Ensure we have base name without extension
    if (fileName.endsWith(".pdf", Qt::CaseInsensitive))
        fileName.chop(4);
    else if (fileName.endsWith(".txt", Qt::CaseInsensitive))
        fileName.chop(4);

    QString pdfFileName = fileName + ".pdf";
    QString txtFileName = fileName + ".txt";

    // --- PDF Export ---
    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize::A4);
    printer.setOutputFileName(pdfFileName);

    QTextDocument doc;
    QString html = "<html><head><style>"
                   "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; color:#0f172a; }"
                   ".header { text-align: center; margin-bottom: 18px; padding: 16px; background:#eaf3ff; border:1px solid #c7defe; border-radius:10px; }"
                   "h1 { color: #0b3b91; font-size: 30px; margin: 0 0 6px 0; text-align: center; }"
                   "h4 { color: #36507a; font-size: 15px; margin:0; font-weight: normal; text-align: center; }"
                   ".info { color: #1e293b; font-size: 13px; margin: 14px 0; text-align: center; background:#f8fbff; border:1px solid #d7e7ff; border-radius:8px; padding:8px; }"
                   ".summary { width:100%; border-collapse:separate; border-spacing:8px; margin-top: 10px; }"
                   ".summary td { width:25%; padding:10px 12px; border-radius:8px; font-size:12px; font-weight:600; }"
                   ".sum-total { background:#eaf2ff; color:#1e3a8a; border:1px solid #bfd5ff; }"
                   ".sum-admin { background:#eef2ff; color:#4338ca; border:1px solid #d7d6ff; }"
                   ".sum-agent { background:#ecfdf3; color:#166534; border:1px solid #b7f0cf; }"
                   ".sum-rh { background:#f5f3ff; color:#6d28d9; border:1px solid #e4dcff; }"
                   ".emp-table { border-collapse: collapse; width: 100%; margin-top: 16px; border:1px solid #dbe6f7; }"
                   ".emp-table th { background-color: #1d4ed8; color: white; text-align: left; padding: 12px 9px; font-size: 13px; border-bottom:2px solid #1e40af; }"
                   ".emp-table td { padding: 10px 9px; border-bottom: 1px solid #e7edf7; font-size: 12px; color:#1f2937; }"
                   ".emp-table tr:nth-child(even) { background-color: #f8fbff; }"
                   ".emp-table tr:nth-child(odd) { background-color: #ffffff; }"
                   ".badge { padding: 4px 9px; border-radius: 10px; color: white; font-weight: bold; font-size: 11px; }"
                   ".badge-admin { background-color: #2563EB; }"
                   ".badge-agent { background-color: #10B981; }"
                   ".badge-rh { background-color: #7C3AED; }"
                   "</style></head><body>";

    html += "<div class='header'>"
            "<h1>Liste des Employés</h1>"
            "<h4>Smart Fishing Port Management — DataDock</h4>"
            "</div>"
                "<div class='info'>Date d'export : <b>" + QDate::currentDate().toString("dd/MM/yyyy") + "</b> | "
            "Role exporte : <b>" + selectedRole + "</b> | "
            "Nombre d'employés : <b>" + QString::number(filteredRows.size()) + "</b></div>";

    html += "<table class='summary'><tr>"
            "<td class='sum-total'>Employes " + selectedRole + "<br><span style='font-size:18px;'>" + QString::number(filteredRows.size()) + "</span></td>"
            "</tr></table>";

    html += "<table class='emp-table'><thead>"
            "<tr><th>ID</th><th>Nom</th><th>Prénom</th><th>Rôle</th><th>Email</th></tr>"
            "</thead><tbody>";

    // --- TXT Export ---
    QFile txtFile(txtFileName);
    if (!txtFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Impossible d'écrire le fichier texte";
        return;
    }
    QTextStream txtOut(&txtFile);
    txtOut << "LISTE DES EMPLOYES (" << selectedRole << ") - " << QDate::currentDate().toString("dd/MM/yyyy") << "\n";
    txtOut << "------------------------------------------------------------\n";
    txtOut << QString("%1\t%2\t%3\t%4\t%5\n").arg("ID", -10).arg("NOM", -15).arg("PRENOM", -15).arg("ROLE", -10).arg("EMAIL");
    txtOut << "------------------------------------------------------------\n";

    auto itemText = [this](int row, int col) -> QString {
        QStandardItem *item = model->item(row, col);
        return item ? item->text() : QString();
    };

    for (int row : filteredRows) {
        QString id = itemText(row, 0);
        QString nom = itemText(row, 1);
        QString prenom = itemText(row, 2);
        QString role = itemText(row, 3);
        QString email = itemText(row, 4);

        // PDF Row
        QString roleBadgeClass = "badge";
        if (role.toUpper() == "ADMIN") roleBadgeClass = "badge badge-admin";
        else if (role.toUpper() == "AGENT") roleBadgeClass = "badge badge-agent";
        else if (role.toUpper() == "RH") roleBadgeClass = "badge badge-rh";

        html += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td><span class='%4'>%5</span></td><td>%6</td></tr>")
                    .arg(id, nom, prenom, roleBadgeClass, role.toUpper(), email);

        // TXT Row
        txtOut << QString("%1\t%2\t%3\t%4\t%5\n").arg(id, -10).arg(nom, -15).arg(prenom, -15).arg(role, -10).arg(email);
    }

    html += "</table></body></html>";
    doc.setHtml(html);
    doc.print(&printer);

    txtFile.close();

    QMessageBox::information(this,
                             "Exportation réussie",
                             QString("Les employes du role %1 ont ete exportes avec succes :\n%2\n%3")
                                 .arg(selectedRole, pdfFileName, txtFileName));
}

void EmployesPage::onAttestationClicked()
{
    bool ok = false;
    QString idEmploye = QInputDialog::getText(
        this,
        tr("Attestation de travail"),
        tr("Saisir l'ID de l'employé :"),
        QLineEdit::Normal,
        QString(),
        &ok
        ).trimmed();

    if (!ok || idEmploye.isEmpty()) {
        return;
    }

    if (!exporterAttestationParId(idEmploye)) {
        QMessageBox::warning(this,
                             tr("Attestation"),
                             tr("Aucun employé trouvé avec l'ID %1.").arg(idEmploye));
    }
}

bool EmployesPage::exporterAttestationParId(const QString &idEmploye)
{
    QSqlQuery query;
    query.prepare("SELECT IDEMPLOYE, NOM, PRENOM, ROLE, EMAIL FROM EMPLOYES WHERE IDEMPLOYE = :id");
    query.bindValue(":id", idEmploye);

    if (!query.exec()) {
        QMessageBox::critical(this,
                              tr("Erreur base de données"),
                              tr("Impossible de récupérer l'employé : %1").arg(query.lastError().text()));
        return false;
    }

    if (!query.next()) {
        return false;
    }

    const QString id = query.value(0).toString();
    const QString nom = query.value(1).toString();
    const QString prenom = query.value(2).toString();
    const QString role = query.value(3).toString();
    const QString email = query.value(4).toString();

    QString suggestedName = QString("Attestation_%1_%2_%3.pdf").arg(nom, prenom, id);
    suggestedName.replace(" ", "_");

    const QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Enregistrer l'attestation PDF"),
        suggestedName,
        tr("PDF Files (*.pdf)")
        );

    if (fileName.isEmpty()) {
        return true;
    }

    QString outputFile = fileName;
    if (!outputFile.endsWith(".pdf", Qt::CaseInsensitive)) {
        outputFile += ".pdf";
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setOutputFileName(outputFile);

    const QDate currentDate = QDate::currentDate();
    const QTime currentTime = QTime::currentTime();
    const QLocale fr(QLocale::French, QLocale::France);
    const QString dateStr = currentDate.toString("dd/MM/yyyy");
    const QString dateLongStr = fr.toString(currentDate, "dd MMMM yyyy");
    const QString reference = QString("AT-%1-%2")
                                  .arg(currentDate.toString("yyyyMMdd"), id.rightJustified(4, '0', true));
    const QString roleUpper = role.toUpper();
    const QString roleColor = (roleUpper == "ADMIN") ? "#0f766e" : (roleUpper == "RH") ? "#6d28d9" : "#1d4ed8";
    const QString roleBg = (roleUpper == "ADMIN") ? "#ccfbf1" : (roleUpper == "RH") ? "#ede9fe" : "#dbeafe";

    QString html;
    html += "<html><head><meta charset='UTF-8'></head>";
    html += "<body style='font-family:Times New Roman, serif; color:#0f172a; margin:16px; background-color:#f8fbff;'>";

    html += "<table width='100%' cellspacing='0' cellpadding='0' style='border:2px solid #1d4ed8; background-color:white;'>";
    html += "<tr>"
            "<td width='24%' style='background-color:#0ea5e9; padding:10px;'>&nbsp;</td>"
            "<td width='26%' style='background-color:#1d4ed8; padding:10px;'>&nbsp;</td>"
            "<td width='24%' style='background-color:#14b8a6; padding:10px;'>&nbsp;</td>"
            "<td width='26%' style='background-color:#7c3aed; padding:10px;'>&nbsp;</td>"
            "</tr>";

    html += "<tr><td colspan='4' style='background-color:#1e40af; color:white; padding:11px 14px; font-size:12px;'>"
            "<b>DATADOCK - SMART FISHING PORT MANAGEMENT</b> | Service Ressources Humaines"
            "</td></tr>";

    html += "<tr><td colspan='4' style='padding:14px;'>";

    html += "<table width='100%' cellspacing='0' cellpadding='0'>"
            "<tr>"
            "<td style='font-size:11px; color:#334155;'>"
            "Adresse : Port de Tunis<br/>"
            "Email : rh@datadock.tn"
            "</td>"
            "<td align='right' style='font-size:11px; color:#334155;'>"
            "Réf : <b>" + reference + "</b><br/>"
            "Date : <b>" + dateStr + "</b><br/>"
            "Heure : <b>" + currentTime.toString("hh:mm") + "</b>"
            "</td>"
            "</tr>"
            "</table>";

    html += "<div style='margin-top:12px; padding:11px 0; text-align:center; background-color:#eef4ff; border:1px solid #bfdbfe;'>"
            "<span style='font-size:30px; font-weight:bold; color:#0f2f78;'>ATTESTATION DE TRAVAIL</span><br/>"
            "<span style='font-size:12px; color:#475569;'>Document administratif officiel</span>"
            "</div>";

    html += "<table width='100%' cellspacing='0' cellpadding='0' style='margin-top:10px;'>"
            "<tr>"
            "<td width='72%' style='background-color:#e0f2fe; border:1px solid #7dd3fc; padding:9px; font-size:13px;'>"
            "<b>Objet :</b> Attestation de présence et d'emploi au sein de l'établissement"
            "</td>"
            "<td width='3%'>&nbsp;</td>"
            "<td width='25%' align='center' style='background-color:" + roleBg + "; border:1px solid " + roleColor + "; color:" + roleColor + "; font-size:12px; padding:9px;'>"
            "Fonction : <b>" + roleUpper + "</b>"
            "</td>"
            "</tr>"
            "</table>";

    html += "<table width='100%' cellspacing='0' cellpadding='7' style='margin-top:12px; border:1px solid #d1d5db; font-size:13px;'>";
    html += QString("<tr><td width='30%%' style='background-color:#f0f9ff; color:#0c4a6e;'><b>ID Employé</b></td><td style='background-color:#ffffff;'><b>%1</b></td></tr>").arg(id);
    html += QString("<tr><td style='background-color:#ecfeff; color:#115e59;'><b>Nom</b></td><td style='background-color:#ffffff;'><b>%1</b></td></tr>").arg(nom.toUpper());
    html += QString("<tr><td style='background-color:#eef2ff; color:#3730a3;'><b>Prénom</b></td><td style='background-color:#ffffff;'><b>%1</b></td></tr>").arg(prenom);
    html += QString("<tr><td style='background-color:#f5f3ff; color:#6d28d9;'><b>Fonction</b></td><td style='background-color:#ffffff;'><b>%1</b></td></tr>").arg(roleUpper);
    html += QString("<tr><td style='background-color:#fff7ed; color:#9a3412;'><b>Email professionnel</b></td><td style='background-color:#ffffff;'>%1</td></tr>").arg(email);
    html += "</table>";

    html += QString("<p style='margin-top:14px; font-size:14px; line-height:1.65; color:#1e293b;'>"
                    "Je soussigné(e), responsable du service Ressources Humaines de <b style='color:#1d4ed8;'>DataDock - Smart Fishing Port Management</b>, "
                    "atteste que <b style='color:#0f766e;'>%1 %2</b>, titulaire de l'identifiant <b>%3</b>, occupe actuellement le poste de <b>%4</b> "
                    "au sein de notre établissement.</p>")
                .arg(prenom, nom, id, roleUpper);

    html += QString("<p style='font-size:14px; line-height:1.65; color:#1e293b;'>"
                    "La présente attestation est délivrée à la demande de l'intéressé(e) pour servir et valoir ce que de droit. "
                    "Elle est établie et signée le <b style='color:#7c3aed;'>%1</b>.</p>")
                .arg(dateLongStr);

    html += "<div style='margin-top:10px; padding:10px; border-left:5px solid #f59e0b; background-color:#fffbeb; font-size:12px; color:#92400e;'>"
            "Mention légale : ce document est strictement personnel. Toute falsification ou usage frauduleux est passible de poursuites."
            "</div>";

    html += "<table width='100%' cellspacing='0' cellpadding='0' style='margin-top:34px;'>"
            "<tr>"
            "<td width='50%'></td>"
            "<td align='center' style='font-size:13px; color:#1e293b;'>"
            "Fait à Tunis, le <b>" + dateLongStr + "</b><br/><br/>"
            "Pour le Service RH<br/><br/><br/><br/>"
            "<span style='border-top:1px solid #334155; padding-top:7px;'>Signature et cachet de l'entreprise</span>"
            "</td>"
            "</tr>"
            "</table>";

    html += "<div style='margin-top:16px; border-top:1px dashed #cbd5e1; padding-top:6px; text-align:center; color:#64748b; font-size:10px;'>"
            "DataDock - Smart Fishing Port Management | Attestation générée automatiquement"
            "</div>";

    html += "</td></tr></table>";
    html += "</body></html>";

    QTextDocument document;
    document.setHtml(html);
    document.setPageSize(printer.pageRect(QPrinter::Point).size());
    document.print(&printer);

    QMessageBox::information(this,
                             tr("Attestation générée"),
                             tr("L'attestation PDF a été créée avec succès :\n%1").arg(outputFile));
    return true;
}

void EmployesPage::on_reglementBtn_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Enregistrer le Règlement du Port", "Reglement_Port_DataDock_Complet.pdf", "*.pdf");
    if (fileName.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setOutputFileName(fileName);

    QString html = 
        "<html><head><meta charset='UTF-8'></head>"
        "<body style='font-family: Arial, sans-serif; color: #1e293b; margin: 0; padding: 0;'>"
        
        // PAGE 1: COVER & TABLE OF CONTENTS
        "<table width='100%' cellspacing='0' cellpadding='0' style='background-color: #f8fbff;'>"
        "<tr><td style='background: #1e40af; padding: 50px; text-align: center;'>"
        "<h1 style='color: white; margin: 0; font-size: 36px; letter-spacing: 3px;'>DATADOCK SMART PORT</h1>"
        "<p style='color: #93c5fd; font-size: 18px; margin-top: 10px;'>Système de Gestion Maritime Intégré</p>"
        "</td></tr>"
        "<tr><td style='padding: 60px 40px; text-align: center; border-bottom: 5px solid #2f6bff;'>"
        "<h1 style='color: #1e3a8a; margin: 0; font-size: 42px;'>RÈGLEMENT INTÉRIEUR</h1>"
        "<h2 style='color: #2f6bff; margin: 15px 0; font-size: 24px;'>ÉDITION OFFICIELLE 2024 - RÉVISION v2.1</h2>"
        "<p style='color: #64748b; font-size: 16px; margin-top: 30px;'>Ce document contient 10 Articles fondamentaux régissant l'exploitation <br/>et la sécurité du complexe portuaire DataDock.</p>"
        "</td></tr>"
        "</table>"

        "<div style='padding: 50px;'>"
        "<h3 style='color: #1e40af; border-bottom: 2px solid #e2e8f0; padding-bottom: 10px;'>SOMMAIRE EXÉCUTIF</h3>"
        "<ul style='list-style: none; padding: 0; line-height: 2.2; color: #475569; font-weight: bold;'>"
        "<li>Art 1 : Protocole d'Identité et d'Accès .................................................. 1</li>"
        "<li>Art 2 : Gestion de la Chaîne Logistique .................................................. 1</li>"
        "<li>Art 3 : Sécurité des Travailleurs et EPI .................................................... 1</li>"
        "<li>Art 4 : Protection du Bassin et Écologie ................................................. 2</li>"
        "<li>Art 5 : Utilisation des Ressources (Énergie) ............................................. 2</li>"
        "<li>Art 6 : Maintenance du Réseau et Matériel ............................................. 2</li>"
        "<li>Art 7 : Code de Navigation et Amarrage ................................................. 3</li>"
        "<li>Art 8 : Intervention d'Urgence et Incendie ............................................. 3</li>"
        "<li>Art 9 : Confidentialité et Données IoT ..................................................... 4</li>"
        "<li>Art 10 : Sanctions et Cadre Légal ............................................................ 4</li>"
        "</ul>"
        "</div>"

        "<div style='page-break-after: always;'></div>" // Try to suggest page break (some Qt versions honor this)

        "<div style='padding: 50px;'>"
        
        // ARTICLE 1
        "<h2 style='color: #1e40af; background: #eff6ff; padding: 15px; border-left: 8px solid #2f6bff; font-size: 20px;'>ARTICLE 1 : PROTOCOLE D'IDENTITÉ ET D'ACCÈS</h2>"
        "<p>L’accès au domaine portuaire est un privilège et non un droit. Chaque individu doit être muni d'une identité numérique enregistrée dans le système central DataDock.</p>"
        "<p>• <b>Double Authentification :</b> L’accès aux zones sensibles (PC Sécurité, Locaux Serveurs) nécessite une validation biométrique et un code PIN dynamique.<br/>"
        "• <b>Zone Temporaire :</b> Les visiteurs doivent porter un badge orange et être accompagnés en permanence par un agent agréé.<br/>"
        "• <b>Véhicules :</b> Seuls les véhicules électriques de service sont autorisés sur les quais de chargement direct.</p>"

        // ARTICLE 2
        "<h2 style='color: #065f46; background: #f0fdf4; padding: 15px; border-left: 8px solid #10b981; font-size: 20px; margin-top: 40px;'>ARTICLE 2 : GESTION DE LA CHAÎNE LOGISTIQUE</h2>"
        "<p>La synchronisation entre l'arrivée des navires et la disponibilité des camions est gérée par l'algorithme d'optimisation DataDock.</p>"
        "<p>• <b>Priorité :</b> La priorité de déchargement est accordée aux produits périssables signalés par les capteurs de température.<br/>"
        "• <b>Stockage :</b> Aucune marchandise ne peut stationner plus de 12 heures sur le quai public sans autorisation spéciale.<br/>"
        "• <b>Traçabilité :</b> Chaque lot doit posséder une étiquette QR-Code générée par le système pour permettre son suivi en temps réel.</p>"

        // ARTICLE 3
        "<h2 style='color: #92400e; background: #fffbeb; padding: 15px; border-left: 8px solid #f59e0b; font-size: 20px; margin-top: 40px;'>ARTICLE 3 : SÉCURITÉ DES TRAVAILLEURS ET EPI</h2>"
        "<p>La sécurité au travail est la priorité absolue du complexe portuaire de Tunis.</p>"
        "<p>• <b>Équipement Obligatoire :</b> Le casque, les gants et les chaussures de sécurité sont requis dès l'entrée en zone de manœuvre.<br/>"
        "• <b>Certification :</b> La manipulation des grues automatisées et des compacteurs nécessite une certification annuelle à jour.<br/>"
        "• <b>Zone de Silence :</b> Le repos des équipages est sacré. Les travaux bruyants sont interdits entre 22h et 06h dans les zones 'Dormitory'.</p>"

        // ARTICLE 4
        "<h2 style='color: #1e40af; background: #f8fbff; padding: 15px; border-left: 8px solid #0ea5e9; font-size: 20px; margin-top: 40px;'>ARTICLE 4 : PROTECTION DU BASSIN ET ÉCOLOGIE</h2>"
        "<p>En tant que port intelligent, DataDock s'engage pour une économie bleue durable.</p>"
        "<p>• <b>Eaux de Lest :</b> Le délestage dans le bassin est formellement interdit. Les navires doivent utiliser les stations de traitement terrestres.<br/>"
        "• <b>Faune Marine :</b> Toute interaction ou pollution impactant l'écosystème local entraînera une amende immédiate et une suspension de licence.<br/>"
        "• <b>Énergie Solaire :</b> Les navires amarrés au quai 'Green' sont encouragés à utiliser le branchement électrique terrestre alimenté par nos panneaux.</p>"

        // ARTICLE 5
        "<h2 style='color: #c026d3; background: #fdf4ff; padding: 15px; border-left: 8px solid #d946ef; font-size: 20px; margin-top: 40px;'>ARTICLE 5 : MAINTENANCE ET PATRIMOINE TECHNOLOGIQUE</h2>"
        "<p>Les installations technologiques (capteurs, bornes wifi, stations météo) sont le cœur du port.</p>"
        "<p>• Toute interaction avec un boîtier électronique doit être consignée numériquement.<br/>"
        "• Tout employé témoin d'une dégradation de capteur doit le signaler via l'application mobile sous peine de complicité passive.</p>"

        // ... Adding more detail for other articles ...
        "<h2 style='color: #4338ca; background: #eef2ff; padding: 15px; border-left: 8px solid #6366f1; font-size: 20px; margin-top: 40px;'>ARTICLE 6 : INTERVENTIONS D'URGENCE</h2>"
        "<p>En cas d'alerte rouge, chaque employé doit se diriger vers son point de rassemblement désigné dans son profil personnel.</p>"

        "<h2 style='color: #b91c1c; background: #fef2f2; padding: 15px; border-left: 8px solid #ef4444; font-size: 20px; margin-top: 40px;'>ARTICLE 10 : CADRE JURIDIQUE ET SANCTIONS</h2>"
        "<p>Le non-respect d'un seul de ces articles expose l'individu à des sanctions allant de l'avertissement simple à l'exclusion définitive du port et des poursuites pénales.</p>"

        // Signatures area on its own block (might go to next page)
        "<table width='100%' style='margin-top: 100px; padding: 30px; border-top: 2px solid #2f6bff;'>"
        "<tr>"
        "<td width='50%' style='text-align: left;'>"
        "<p style='color: #64748b;'>Document certifié par le service informatique et RH</p>"
        "<p style='font-weight: bold;'>Direction DataDock Tunis</p>"
        "<br/><br/>"
        "<div style='width: 150px; height: 100px; border: 3px double #1d4ed8; color: #1d4ed8; font-weight: bold; text-align: center; padding-top: 30px;'>"
        "SCEAU DE <br/> L'ADMINISTRATION"
        "</div>"
        "</td>"
        "<td width='50%' style='text-align: right;'>"
        "<p style='color: #64748b;'>Fait le : " + QDate::currentDate().toString("dd MMMM yyyy") + "</p>"
        "<p style='font-weight: bold;'>Signature de l'agent : ___________________</p>"
        "</td>"
        "</tr>"
        "</table>"

        "</div>"

        "<div style='background-color: #0f172a; color: white; padding: 25px; text-align: center; font-size: 11px;'>"
        "DataDock Smart Port Management System - République Tunisienne - Ministère du Transport et du Commerce"
        "</div>"

        "</body></html>";

    QTextDocument document;
    document.setHtml(html);
    document.print(&printer);

    QMessageBox::information(this, "Succès", "Le règlement complet (multi-pages) a été généré avec succès.");
}
