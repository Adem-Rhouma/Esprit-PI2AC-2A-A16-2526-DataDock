#include "chambresfroidesoptimizationpage.h"
#include "ui_chambresfroidesoptimizationpage.h"
#include "chambresfroides.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QScrollArea>
#include <QFrame>
#include <QProcess>
#include <algorithm>

ChambresFroidesOptimizationPage::ChambresFroidesOptimizationPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChambresFroidesOptimizationPage)
{
    ui->setupUi(this);
    aiProcess = new QProcess(this);
    connect(aiProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
            this, &ChambresFroidesOptimizationPage::onAiProcessFinished);
    
    // Setup reference to cards layout
    cardsLayout = ui->resultsContainerLayout;
    
    applyStyles();
    loadPecheIds();
    ui->labelFishNameDisplay->setVisible(false);
}

ChambresFroidesOptimizationPage::~ChambresFroidesOptimizationPage()
{
    delete ui;
}

void ChambresFroidesOptimizationPage::applyStyles()
{
    const QString buttonStyle = "QPushButton#btnOptimize { "
                                "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0052D4, stop:0.5 #4364F7, stop:1 #6FB1FC); "
                                "color: white; border-radius: 8px; font-weight: bold; padding: 10px; }"
                                "QPushButton#btnOptimize:hover { background: #3652d9; }";
    ui->btnOptimize->setStyleSheet(buttonStyle);
}

void ChambresFroidesOptimizationPage::loadPecheIds()
{
    ui->comboPecheId->clear();
    QSqlQuery query("SELECT IDPECHE FROM PECHES");
    while (query.next()) {
        ui->comboPecheId->addItem(query.value(0).toString(), query.value(0));
    }
}

void ChambresFroidesOptimizationPage::on_comboMode_currentIndexChanged(int index)
{
    bool isPecheMode = (index == 1);
    ui->comboFishType->setEnabled(!isPecheMode);
    ui->comboPecheId->setEnabled(isPecheMode);
    ui->labelFishNameDisplay->setVisible(isPecheMode);
    if (isPecheMode) on_comboPecheId_currentIndexChanged(ui->comboPecheId->currentIndex());
}

void ChambresFroidesOptimizationPage::on_btnOptimize_clicked()
{
    // Clear old cards
    QLayoutItem *child;
    while ((child = cardsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }
    cardsLayout->addStretch();

    if (ui->checkEnableAI->isChecked()) {
        if (!checkAiDependencies()) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, "Dépendances IA Manquantes",
                "Les bibliothèques IA (Ollama, Python modules) ne sont pas détectées.\n\n"
                "Souhaitez-vous les installer automatiquement ?\n"
                "(Si non, le mode IA sera désactivé)",
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                installDependencies();
                QMessageBox::information(this, "Installation", "L'installation a été lancée en arrière-plan. "
                    "Le moteur IA sera prêt d'ici quelques minutes.");
                return; // Wait for installation
            } else {
                ui->checkEnableAI->setChecked(false);
                performLegacyOptimization();
                return;
            }
        }
        performAIOptimization();
    } else {
        performLegacyOptimization();
    }
}

void ChambresFroidesOptimizationPage::performLegacyOptimization()
{
    QMessageBox::information(this, "Note", "Le mode Legacy est temporairement désactivé durant la migration vers l'UI Scenario-First AI.");
}

void ChambresFroidesOptimizationPage::performAIOptimization()
{
    ui->btnOptimize->setEnabled(false);
    ui->btnOptimize->setText("Analyse IA en cours... ⏳");

    // 1. Gather Data
    QJsonObject root;
    QJsonArray fishBatches;
    QJsonObject batch;
    batch["id"] = "B01";
    batch["quantity"] = ui->spinQuantity->value();
    if (ui->comboMode->currentIndex() == 0) {
        batch["species"] = ui->comboFishType->currentText();
        batch["target_temp"] = -18.0; // Default
    } else {
        batch["species"] = ui->labelFishNameDisplay->text().replace("Poisson : ", "");
        batch["peche_id"] = ui->comboPecheId->currentText();
        batch["target_temp"] = -20.0; // Preference for peche
    }
    fishBatches.append(batch);
    root["batches"] = fishBatches;

    QJsonArray coldRooms;
    QString err;
    QSqlQueryModel *model = ChambresFroides::afficher_chf(&err);
    if (model) {
        for (int i = 0; i < model->rowCount(); ++i) {
            QSqlRecord rec = model->record(i);
            if (rec.value("STATUT").toString() != "Active") continue;
            QJsonObject room;
            room["id"] = rec.value("TAG_NUMBER").toString();
            room["capacity"] = rec.value("MAXCAP").toDouble();
            room["current_occ"] = rec.value("OCCCAP").toDouble();
            room["current_temp"] = rec.value("TEMP").toDouble();
            coldRooms.append(room);
        }
        delete model;
    }
    root["cold_rooms"] = coldRooms;

    // 2. Write to temp file
    QString inputPath = QDir::tempPath() + "/cf_ai_input.json";
    QString outputPath = QDir::tempPath() + "/cf_ai_output.json";
    
    QFile inputFile(inputPath);
    if (inputFile.open(QIODevice::WriteOnly)) {
        inputFile.write(QJsonDocument(root).toJson());
        inputFile.close();
    }

    // 3. Find Python and script robustly
    QString venvPython;
    QString scriptPath;
    bool found = false;
    QDir search(QDir::currentPath());
    
    for (int i = 0; i < 5; ++i) {
        QString venvTest = search.absoluteFilePath("gestion_chambres_froides/ai_engine/venv/bin/python3");
        QString scriptTest = search.absoluteFilePath("gestion_chambres_froides/ai_engine/standalone_optimizer.py");
        
        if (QFile::exists(scriptTest)) {
            scriptPath = scriptTest;
            if (QFile::exists(venvTest)) venvPython = venvTest;
            found = true;
            break;
        }
        if (!search.cdUp()) break;
    }

    if (!found) {
        QMessageBox::critical(this, "Erreur", "Moteur d'expertise (script) introuvable.");
        ui->btnOptimize->setEnabled(true);
        ui->btnOptimize->setText("Trouver la Meilleure Option ✨");
        return;
    }

    QString pythonExec = venvPython.isEmpty() ? "python3" : venvPython;
    aiProcess->start(pythonExec, QStringList() << scriptPath << inputPath << outputPath);
}

void ChambresFroidesOptimizationPage::onAiProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    ui->btnOptimize->setEnabled(true);
    ui->btnOptimize->setText("Trouver la Meilleure Option ✨");

    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        QString err = QString::fromUtf8(aiProcess->readAllStandardError());
        if (err.isEmpty()) err = QString::fromUtf8(aiProcess->readAllStandardOutput());
        
        // Sanitize: If the error contains markdown headers or weird text, it might be a corrupted buffer
        if (err.contains("# ") || err.contains("[Goal")) {
            err = "Erreur de chargement du moteur IA. Veuillez redémarrer l'application.";
        }
        
        if (err.length() > 500) err = err.left(500) + "... [tronqué]";
        
        QMessageBox::critical(this, "Erreur AI", "Échec (Code " + QString::number(exitCode) + ").\n\n" + 
                                               (err.isEmpty() ? "Vérifiez que le venv est configuré." : err));
        return;
    }

    QString outputPath = QDir::tempPath() + "/cf_ai_output.json";
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::ReadOnly)) {
         QMessageBox::critical(this, "Erreur AI", "Le script a terminé mais le fichier de résultat est inaccessible.");
         return;
    }

    QByteArray response = outputFile.readAll();
    outputFile.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(response);
    if (doc.isNull() || !doc.isArray()) {
        QJsonObject rootObj = doc.object();
        if (rootObj.contains("error")) {
             QMessageBox::warning(this, "Erreur AI", rootObj["error"].toString());
             return;
        }
    }

    currentAiSolutions.clear();
    
    // Clear layout
    QLayoutItem *child;
    while ((child = cardsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    QJsonArray solutions = doc.array();
    for (int i = 0; i < solutions.size(); ++i) {
        QJsonObject obj = solutions[i].toObject();
        AISolution sol;
        sol.id = obj["id"].toString();
        sol.score = obj["score"].toDouble();
        sol.qualitative = obj["qualitative_score"].toString();
        sol.allocations = obj["allocations"].toArray();
        sol.predictions = obj["predictions"].toArray().at(0).toObject()["prediction"].toObject(); // Fallback if structure differs
        
        // Handle nesting if needed (predictor returns curve and risk)
        if (obj.contains("predictions") && obj["predictions"].isObject()) {
             sol.predictions = obj["predictions"].toObject();
        }
        
        sol.explanation = obj["explanation"].toString();
        sol.index = i;
        currentAiSolutions.append(sol);

        // COLOR LOGIC
        QString bannerColor = "#3b82f6"; // Default Blue
        QString textColor = "#1e40af";
        QString bgColor = "#eff6ff";
        
        if (sol.qualitative.contains("Optimal") || sol.qualitative.contains("Excellent")) {
            bannerColor = "#10b981"; // Green
            bgColor = "#ecfdf5";
            textColor = "#065f46";
        } else if (sol.qualitative.contains("Moyen") || sol.qualitative.contains("Fragmenté")) {
            bannerColor = "#f59e0b"; // Orange
            bgColor = "#fffbeb";
            textColor = "#92400e";
        } else if (sol.qualitative.contains("Sous") || sol.qualitative.contains("Critique")) {
            bannerColor = "#ef4444"; // Red
            bgColor = "#fef2f2";
            textColor = "#991b1b";
        }

        // CREATE CARD
        QFrame *card = new QFrame();
        card->setStyleSheet(QString("QFrame { background: white; border-radius: 12px; border: 2px solid #e2e8f0; padding: 15px; } "
                            "QFrame:hover { border-color: %1; }").arg(bannerColor));
        QVBoxLayout *l = new QVBoxLayout(card);

        QHBoxLayout *h = new QHBoxLayout();
        QLabel *title = new QLabel(QString("Option %1: %2").arg(i+1).arg(sol.id));
        title->setStyleSheet("font-weight: bold; font-size: 16px; color: #1e293b; border: none;");
        
        QLabel *badge = new QLabel(sol.qualitative);
        badge->setStyleSheet(QString("background: %1; color: %2; border-radius: 6px; padding: 4px 8px; font-weight: 800; font-size: 10px; text-transform: uppercase;")
                             .arg(bgColor).arg(textColor));
        
        QLabel *score = new QLabel(QString("Score: %1/100").arg(sol.score));
        score->setStyleSheet(sol.score >= 80 ? "color: #10b981; font-weight: bold;" : 
                           (sol.score >= 50 ? "color: #f59e0b; font-weight: bold;" : "color: #ef4444; font-weight: bold;"));
        score->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        
        h->addWidget(title); 
        h->addSpacing(10);
        h->addWidget(badge);
        h->addStretch(); 
        h->addWidget(score);
        l->addLayout(h);

        badge->setToolTip("Évaluation qualitative basée sur le compromis température/espace.");
        score->setToolTip("Indice de performance globale (plus le score est élevé, plus le plan est efficient).");

        QString summary;
        for (const auto &aval : sol.allocations) {
            QJsonObject a = aval.toObject();
            summary += QString("- Chambre %1 : %2 kg (%3)\n").arg(a["cold_room"].toString()).arg(a["quantity"].toDouble()).arg(a["species"].toString());
        }
        QLabel *content = new QLabel(summary);
        content->setStyleSheet("color: #64748b; font-size: 13px; border: none;");
        l->addWidget(content);

        double risk = sol.predictions["spoilage_risk"].toDouble() * 100;
        QLabel *prediction = new QLabel(QString("🛡️ Risque de dégradation : %1%").arg(risk, 0, 'f', 1));
        prediction->setStyleSheet(risk > 15 ? "color: #ef4444; font-weight: bold; border: none;" : "color: #10b981; font-weight: bold; border: none;");
        l->addWidget(prediction);

        QHBoxLayout *btnBox = new QHBoxLayout();
        QPushButton *btnExpl = new QPushButton("📊 Analyser & Expliquer");
        btnExpl->setProperty("solution_index", i);
        btnExpl->setCursor(Qt::PointingHandCursor);
        btnExpl->setStyleSheet("QPushButton { background: #f1f5f9; color: #334155; border-radius: 6px; padding: 8px; font-weight: bold; border: 1px solid #e2e8f0; } "
                               "QPushButton:hover { background: #e2e8f0; }");
        connect(btnExpl, &QPushButton::clicked, this, &ChambresFroidesOptimizationPage::on_explainScenario_clicked);

        QPushButton *btnUse = new QPushButton("✅ Utiliser ce Scénario");
        btnUse->setProperty("solution_index", i);
        btnUse->setCursor(Qt::PointingHandCursor);
        btnUse->setStyleSheet(QString("QPushButton { background: %1; color: white; border-radius: 6px; padding: 8px; font-weight: bold; border: none; } "
                              "QPushButton:hover { opacity: 0.8; }").arg(bannerColor));
        connect(btnUse, &QPushButton::clicked, this, &ChambresFroidesOptimizationPage::on_applyStorage_clicked);

        btnBox->addWidget(btnExpl);
        btnBox->addWidget(btnUse);
        l->addLayout(btnBox);

        cardsLayout->insertWidget(cardsLayout->count() - 1, card);
    }
}

void ChambresFroidesOptimizationPage::on_explainScenario_clicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    int index = btn->property("solution_index").toInt();
    if (index >= 0 && index < currentAiSolutions.size()) {
        const AISolution &sol = currentAiSolutions[index];
        QJsonObject solObj;
        solObj["solution_id"] = sol.id;
        solObj["score"] = sol.score;
        solObj["allocations"] = sol.allocations;
        solObj["predictions"] = sol.predictions;
        solObj["explanation"] = sol.explanation;
        
        ScenarioDetailsDialog dialog(solObj, index, this);
        dialog.exec();
    }
}

void ChambresFroidesOptimizationPage::on_applyStorage_clicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    int index = btn->property("solution_index").toInt();
    if (index < 0 || index >= currentAiSolutions.size()) return;

    const AISolution &sol = currentAiSolutions[index];
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirmation AI", 
        QString("Voulez-vous appliquer le scénario '%1' (%2 chambres concernées) ?").arg(sol.id).arg(sol.allocations.size()),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        bool allSuccess = true;
        for (const QJsonValue &aval : sol.allocations) {
            QJsonObject a = aval.toObject();
            QString id = a["cold_room"].toString();
            double qty = a["quantity"].toDouble();

            QSqlQuery query;
            query.prepare("UPDATE CHAMBRESFROIDES SET OCCCAP = OCCCAP + ? WHERE TAG_NUMBER = ?");
            query.addBindValue(qty);
            query.addBindValue(id);
            if (!query.exec()) {
                allSuccess = false;
                QMessageBox::warning(this, "Avis de mise à jour", "Erreur partielle sur la chambre " + id);
            }
        }
        if (allSuccess) QMessageBox::information(this, "Succès IA", "Le scénario d'expertise a été appliqué avec succès !");
    }
}

void ChambresFroidesOptimizationPage::refreshData()
{
    loadPecheIds();
}

bool ChambresFroidesOptimizationPage::checkAiDependencies()
{
    // For the new script-based AI, we just need python3.
    // Heuristics handle missing libraries like ortools.
    QProcess pythonCheck;
    pythonCheck.start("python3", QStringList() << "--version");
    if (!pythonCheck.waitForFinished(1000) || pythonCheck.exitCode() != 0) {
        return false;
    }

    return true;
}

void ChambresFroidesOptimizationPage::installDependencies()
{
    QString scriptPath = QDir(qApp->applicationDirPath()).absoluteFilePath("../../../gestion_chambres_froides/ai_engine/start_ai_engine.sh");
    // Backup check if running from source dir
    if (!QFile::exists(scriptPath)) {
        scriptPath = QDir::current().absoluteFilePath("gestion_chambres_froides/ai_engine/start_ai_engine.sh");
    }

    if (QFile::exists(scriptPath)) {
        QProcess::startDetached("bash", QStringList() << scriptPath);
    } else {
        QMessageBox::warning(this, "Erreur", "Script d'installation non trouvé : " + scriptPath);
    }
}

void ChambresFroidesOptimizationPage::on_comboPecheId_currentIndexChanged(int index)
{
    if (index == -1 || ui->comboMode->currentIndex() != 1) {
        ui->labelFishNameDisplay->setText("Poisson : -");
        return;
    }
    QVariant pecheId = ui->comboPecheId->itemData(index);
    QSqlQuery query;
    query.prepare("SELECT e.NOMESPECE FROM PECHESESPECES ps LEFT JOIN ESPECES e ON ps.IDESPECE = e.IDESPECE WHERE ps.IDPECHE = ?");
    query.addBindValue(pecheId);
    QStringList fishes;
    if (query.exec()) { while (query.next()) { QString fish = query.value(0).toString(); if (!fish.isEmpty()) fishes << fish; } }
    ui->labelFishNameDisplay->setText(fishes.isEmpty() ? "Poisson : (Aucun)" : "Poisson : " + fishes.join(", "));
}

