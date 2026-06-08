#include "chambresfroidespage.h"
#include "ui_chambresfroidespage.h"

#include <QDebug>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QSqlQueryModel>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>

#include <QPainter>
#include <QPdfWriter>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QDate>
#include "chambresfroides.h"
#include "chambresfroidesdialog.h"
#include "chambresfroidesconfigdialog.h"
#include <QMenu>
#include <QAction>
ChambresFroidesPage::ChambresFroidesPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChambresFroidesPage)
{
    ui->setupUi(this);

    setupTable();
    applyStyles();
    refreshTable();

    connect(ui->btnExport, &QPushButton::clicked, this, &ChambresFroidesPage::exportRequested);

    connect(ui->searchBar, &QLineEdit::textChanged, this, &ChambresFroidesPage::on_searchBar_textChanged);
    connect(ui->comboSortField, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ChambresFroidesPage::on_comboSortField_currentIndexChanged);

    // Main Page Tooltips
    ui->searchBar->setToolTip("Rechercher par ID, Tag, Type de poisson ou État.");
    ui->comboSortField->setToolTip("Trier la liste par date, température ou capacité.");
    ui->btnExport->setToolTip("Exporter les données actuelles du parc de stockage.");
    ui->btnAddChambre->setToolTip("Enregistrer une nouvelle chambre froide dans le système.");
}

ChambresFroidesPage::~ChambresFroidesPage()
{
    delete ui;
}

void ChambresFroidesPage::setupTable()
{
    QStringList headers = {
        "ID", "Tag", "Temp (°C)", "Date Entrée", "Date Limite", 
        "Certificat", "Cap. Max (kg)", "Cap. Occupée (kg)", "État", "Pêche Associée", "Actions"
    };

    ui->tableChambres->setColumnCount(headers.size());
    ui->tableChambres->setHorizontalHeaderLabels(headers);
    
    QHeaderView *header = ui->tableChambres->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Stretch);
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents); // ID
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Tag
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Temp
    header->setSectionResizeMode(8, QHeaderView::ResizeToContents); // State
    header->setSectionResizeMode(9, QHeaderView::ResizeToContents); // Pêche Associée
    header->setSectionResizeMode(10, QHeaderView::ResizeToContents); // Actions
    
    ui->tableChambres->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableChambres->verticalHeader()->setVisible(false);
    ui->tableChambres->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableChambres->setAlternatingRowColors(true);
    ui->tableChambres->setShowGrid(false);
    ui->tableChambres->setFocusPolicy(Qt::NoFocus);
    ui->tableChambres->setColumnHidden(0, true);
}

void ChambresFroidesPage::applyStyles()
{
    // Premium "Cold Blue" Gradient Theme
    // Gradient: #0052D4 -> #4364F7 -> #6FB1FC
    
    // 1. Buttons
    const QString buttonStyle = QStringLiteral(
        "QPushButton#btnAddChambre, QPushButton#btnExport {"
        "  color: white;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #0052D4, stop:0.5 #4364F7, stop:1 #6FB1FC);"
        "  border: 1px solid rgba(67, 100, 247, 160);"
        "  border-radius: 10px;"
        "  padding: 8px 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton#btnAddChambre:hover, QPushButton#btnExport:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #0041c2, stop:0.5 #3652d9, stop:1 #5aa0eb);"
        "  border-color: rgba(67, 100, 247, 200);"
        "}"
        "QPushButton#btnAddChambre:pressed, QPushButton#btnExport:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #003399, stop:0.5 #2b41b0, stop:1 #4a8cd0);"
        "}"
    );

    // 2. Inputs (Search, Combo)
    const QString inputStyle = QStringLiteral(
        "QLineEdit, QComboBox {"
        "  background: #ffffff;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 10px;"
        "  padding: 8px 12px;"
        "  color: #1e293b;"
        "  font-size: 13px;"
        "}"
        "QLineEdit:focus, QComboBox:focus {"
        "  border: 2px solid #4364F7;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 20px;"
        "  border-left: none;"
        "}"
    );

    // 3. Table
    const QString tableStyle = QStringLiteral(
        "QTableWidget {"
        "  background-color: #ffffff;"
        "  alternate-background-color: #f8fafc;"
        "  border: 1px solid #e2e8f0;"
        "  border-radius: 12px;"
        "  gridline-color: transparent;"
        "}"
        "QHeaderView::section {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #0052D4, stop:0.5 #4364F7, stop:1 #6FB1FC);"
        "  color: white;"
        "  padding: 12px;"
        "  border: none;"
        "  font-weight: 600;"
        "  font-size: 13px;"
        "  text-transform: uppercase;"
        "}"
        "QTableWidget::item {"
        "  padding: 10px 12px;"
        "  border-bottom: 1px solid #f1f5f9;"
        "  color: #334155;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: rgba(67, 100, 247, 0.1);"
        "  color: #0052D4;"
        "}"
        "QTableCornerButton::section {"
        "  background: #0052D4;"
        "  border: none;"
        "}"
    );

    this->setStyleSheet(buttonStyle + inputStyle + tableStyle);
    
    // Explicitly set for specific widgets if needed, but parenting should handle it
    ui->tableChambres->setStyleSheet(tableStyle);
}

void ChambresFroidesPage::refreshTable()
{
    ui->tableChambres->setRowCount(0); // Clear table
    
    QString errorMsg;
    QSqlQueryModel *model = ChambresFroides::afficher_chf(&errorMsg);
    if (!model) {
        QMessageBox::critical(this, "Erreur", "Erreur lors du chargement des données: " + errorMsg);
        return;
    }

    for (int i = 0; i < model->rowCount(); ++i) {
        QString id = model->record(i).value("CF_ID").toString();
        double temp = model->record(i).value("TEMP").toDouble();
        
        QDate inDate = model->record(i).value("INDATE").toDate();
        QDate limitDate = model->record(i).value("DATELIMITE").toDate();
        QString inDateStr = inDate.isValid() ? inDate.toString("yyyy-MM-dd") : "-";
        QString limitDateStr = limitDate.isValid() ? limitDate.toString("yyyy-MM-dd") : "-";
        
        // Ensure accurate dates formats for Oracle depending on connection fetch
        // If they come as strings, we might need toString() direct from QVariant
        if(!model->record(i).value("INDATE").toDateTime().isNull()){
            inDateStr = model->record(i).value("INDATE").toDateTime().toString("yyyy-MM-dd");
        }
        if(!model->record(i).value("DATELIMITE").toDateTime().isNull()){
            limitDateStr = model->record(i).value("DATELIMITE").toDateTime().toString("yyyy-MM-dd");
        }

        QString certSan = model->record(i).value("CERTIFICATESAN").toString();
        double maxCap = model->record(i).value("MAXCAP").toDouble();
        double occCap = model->record(i).value("OCCCAP").toDouble();
        QString state = model->record(i).value("STATUT").toString();
        QVariant idPeche = model->record(i).value("IDPECHE");
        QString tagNumber = model->record(i).value("TAG_NUMBER").toString();
        double humidity = model->record(i).value("HUMIDITY").toDouble();
        bool betaTest = model->record(i).value("BETA_TEST").toInt() == 1;

        addRow(id, tagNumber, temp, inDateStr, limitDateStr, certSan, maxCap, occCap, state, idPeche, humidity, betaTest);
    }
    delete model;
    emit dataChanged();
}

void ChambresFroidesPage::loadSampleData()
{
    // Removed sample data usage, replaced by refreshTable().
}

void ChambresFroidesPage::addRow(const QString &id, const QString &tagNumber, double temp, 
                                 const QString &inDate, const QString &limitDate, 
                                 const QString &certSan, double maxCap, double occCap, 
                                 const QString &state, const QVariant &idPeche, double humidity, bool betaTest)
{
    int row = ui->tableChambres->rowCount();
    ui->tableChambres->insertRow(row);
    ui->tableChambres->setRowHeight(row, 55);

    auto addItem = [&](int col, const QString &val, Qt::Alignment align = Qt::AlignCenter) {
        auto *item = new QTableWidgetItem(val);
        item->setTextAlignment(align);
        ui->tableChambres->setItem(row, col, item);
    };

    addItem(0, id);
    addItem(1, tagNumber, Qt::AlignLeft | Qt::AlignVCenter);
    addItem(2, QString::number(temp, 'f', 1) + " °C");
    addItem(3, inDate);
    addItem(4, limitDate);
    addItem(5, certSan);
    addItem(6, QString::number(maxCap) + " kg");
    addItem(7, QString::number(occCap) + " kg");
    
    QString pecheOrPoisson = "N/A";
    if (!idPeche.isNull() && idPeche.isValid() && idPeche.toInt() != 0) {
        pecheOrPoisson = "Pêche #" + idPeche.toString();
    }
    addItem(9, pecheOrPoisson);

    ui->tableChambres->setCellWidget(row, 8, createStatusBadge(state));
    ui->tableChambres->setCellWidget(row, 10, createActionsWidget(id, tagNumber, humidity, betaTest));
}

QWidget* ChambresFroidesPage::createStatusBadge(const QString &statut) const
{
    auto *wrapper = new QWidget();
    auto *layout = new QHBoxLayout(wrapper);
    layout->setContentsMargins(0,0,0,0);
    layout->setAlignment(Qt::AlignCenter);
    
    auto *label = new QLabel(statut);
    label->setAlignment(Qt::AlignCenter);
    label->setFixedSize(90, 24);
    
    QString baseStyle = "border-radius: 12px; font-weight: bold; font-size: 11px;";
    
    if (statut == "Active") {
        label->setStyleSheet(baseStyle + "background-color: #e6fffa; color: #047857; border: 1px solid #047857;");
    } else if (statut == "Maintenance" || statut == "Broken") {
        label->setStyleSheet(baseStyle + "background-color: #fff5f5; color: #c53030; border: 1px solid #c53030;");
    } else {
        label->setStyleSheet(baseStyle + "background-color: #edf2f7; color: #4a5568; border: 1px solid #4a5568;");
    }
    
    layout->addWidget(label);
    return wrapper;
}

QWidget* ChambresFroidesPage::createActionsWidget(const QString &id, const QString &tagNumber, double humidity, bool betaTest)
{
    auto *wrapper = new QWidget();
    auto *layout = new QHBoxLayout(wrapper);
    layout->setContentsMargins(4,4,4,4);
    layout->setSpacing(8);
    layout->setAlignment(Qt::AlignCenter);
    
    auto createBtn = [](const QString &text, const QString &color) {
        auto *btn = new QPushButton(text);
        btn->setFixedSize(28, 28);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QString(
            "QPushButton { background-color: transparent; border: 1px solid %1; border-radius: 4px; color: %1; font-weight: bold; }"
            "QPushButton:hover { background-color: %1; color: white; }"
        ).arg(color));
        return btn;
    };

    auto *btnEdit = createBtn("✎", "#3b82f6"); 
    auto *btnConfig = createBtn("⚙", "#8b5cf6"); // Purple for Config
    auto *btnDelete = createBtn("🗑", "#ef4444"); 

    btnEdit->setProperty("chambre_id", id);
    btnConfig->setProperty("chambre_id", id);
    btnConfig->setProperty("chambre_tag", tagNumber);  // TAG lisible pour l'OLED
    btnConfig->setProperty("humidity", humidity);
    btnConfig->setProperty("beta_test", betaTest);
    btnDelete->setProperty("chambre_id", id);

    connect(btnEdit, &QPushButton::clicked, this, &ChambresFroidesPage::onEditButtonClicked);
    connect(btnConfig, &QPushButton::clicked, this, &ChambresFroidesPage::onConfigButtonClicked);
    connect(btnDelete, &QPushButton::clicked, this, &ChambresFroidesPage::onDeleteButtonClicked);

    layout->addWidget(btnEdit);
    layout->addWidget(btnConfig);
    layout->addWidget(btnDelete);
    
    return wrapper;
}

void ChambresFroidesPage::onEditButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString id = btn->property("chambre_id").toString();

    // Find details locally from model or table
    QString errorMsg;
    QSqlQueryModel *model = ChambresFroides::afficher_chf(&errorMsg);
    if (!model) return;
    
    ChambresFroides chambre;
    bool found = false;
    for (int i = 0; i < model->rowCount(); ++i) {
        if (model->record(i).value("CF_ID").toString() == id) {
            chambre.setId(id);
            chambre.setTemp(model->record(i).value("TEMP").toDouble());
            chambre.setInDate(model->record(i).value("INDATE").toDateTime().date());
            chambre.setLimitDate(model->record(i).value("DATELIMITE").toDateTime().date());
            chambre.setCertificateSan(model->record(i).value("CERTIFICATESAN").toString());
            chambre.setMaxCap(model->record(i).value("MAXCAP").toDouble());
            chambre.setOccCap(model->record(i).value("OCCCAP").toDouble());
            chambre.setState(model->record(i).value("STATUT").toString());
            chambre.setIdPeche(model->record(i).value("IDPECHE"));
            chambre.setTagNumber(model->record(i).value("TAG_NUMBER").toString());
            chambre.setBetaTest(model->record(i).value("BETA_TEST").toInt() == 1);
            found = true;
            break;
        }
    }
    delete model;

    if (!found) {
        QMessageBox::warning(this, "Erreur", "Chambre introuvable.");
        return;
    }

    ChambresFroidesDialog dialog(this);
    dialog.setChambre(chambre);
    if (dialog.exec() == QDialog::Accepted) {
        refreshTable();
    }
}

void ChambresFroidesPage::onConfigButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString id = btn->property("chambre_id").toString();
    QString tagNumber = btn->property("chambre_tag").toString();
    if (tagNumber.isEmpty()) tagNumber = id;  // fallback
    double currentHum = btn->property("humidity").toDouble();
    bool betaTest = btn->property("beta_test").toBool();

    // Fetch current temperature and status from DB
    QSqlQuery query;
    query.prepare("SELECT TEMP, HUMIDITY, STATUT, BETA_TEST FROM CHAMBRESFROIDES WHERE CF_ID = :id");
    query.bindValue(":id", id);
    double currentTemp = 0.0;
    QString currentState = "Active";
    if (query.exec() && query.next()) {
        currentTemp = query.value(0).toDouble();
        currentHum = query.value(1).toDouble();
        currentState = query.value(2).toString();
        betaTest = query.value(3).toInt() == 1;
    }

    // Direct opening of the config dialog - Auto-connect handled inside
    ChambresFroidesConfigDialog configDialog(id, tagNumber, currentTemp, currentHum, currentState, betaTest, this);
    if (configDialog.exec() == QDialog::Accepted) {
        double newTemp = configDialog.getTemperature();
        double newHum = configDialog.getHumidity();

        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE CHAMBRESFROIDES SET TEMP = :temp, HUMIDITY = :hum WHERE CF_ID = :id");
        updateQuery.bindValue(":temp", newTemp);
        updateQuery.bindValue(":hum", newHum);
        updateQuery.bindValue(":id", id);

        if (updateQuery.exec()) {
            refreshTable();
            QMessageBox::information(this, "Succès", "Configuration climatique mise à jour et synchronisée.");
        } else {
            QMessageBox::critical(this, "Erreur", "Échec de la synchronisation: " + updateQuery.lastError().text());
        }
    }
}

void ChambresFroidesPage::onDeleteButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString id = btn->property("chambre_id").toString();

    // Find tag number for a more user-friendly message
    QString tag = id;
    for(int i=0; i<ui->tableChambres->rowCount(); ++i) {
        if(ui->tableChambres->item(i, 0)->text() == id) {
            tag = ui->tableChambres->item(i, 1)->text();
            break;
        }
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirmation", "Voulez-vous vraiment supprimer la chambre " + tag + " ?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QString errorMsg;
        if (ChambresFroides::supprimer_chf(id, &errorMsg)) {
            QMessageBox::information(this, "Succès", "Chambre supprimée avec succès.");
            refreshTable();
        } else {
            QMessageBox::critical(this, "Erreur", "Erreur lors de la suppression:\n" + errorMsg);
        }
    }
}

void ChambresFroidesPage::on_btnAddChambre_clicked()
{
    ChambresFroidesDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshTable();
    }
}

// Legacy export methods removed

void ChambresFroidesPage::on_searchBar_textChanged(const QString &text)
{
    for (int i = 0; i < ui->tableChambres->rowCount(); ++i) {
        bool match = false;
        // Search in Tag (1), Certificat (5) or Peche/Poisson (9)
        if (ui->tableChambres->item(i, 1)->text().contains(text, Qt::CaseInsensitive) ||
            ui->tableChambres->item(i, 5)->text().contains(text, Qt::CaseInsensitive) ||
            ui->tableChambres->item(i, 9)->text().contains(text, Qt::CaseInsensitive))
        {
            match = true;
        }
        ui->tableChambres->setRowHidden(i, !match);
    }
}

void ChambresFroidesPage::on_comboSortField_currentIndexChanged(int index)
{
    // 0: State(8), 1: Temp(2), 2: Cap(6)
    int col = 8;
    if (index == 1) col = 2;
    if (index == 2) col = 6;
    
    ui->tableChambres->sortItems(col, Qt::AscendingOrder);
}

