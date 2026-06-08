#include "naviredetailpage.h"
#include "ui_naviredetailpage.h"
#include "connection.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QPlainTextEdit>
#include <QHeaderView>
#include <QLabel>



NavireDetailPage::NavireDetailPage(int shipId, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NavireDetailPage),
    m_shipId(shipId)
{
    ui->setupUi(this);
    loadShipData();
    loadVoyageLogs();


    ui->tableLogs->setStyleSheet(
        "QTableWidget { background-color: #ffffff; color: #1e293b; gridline-color: #f1f5f9; }"
        "QHeaderView::section { background-color: #f8fafc; color: #475569; font-weight: bold; border: none; padding: 8px; }"
    );
}

NavireDetailPage::~NavireDetailPage()
{
    delete ui;
}

void NavireDetailPage::loadShipData() {
    if (m_navire.charger(m_shipId)) {
        ui->lblNom->setText(m_navire.getNomNavire());
        ui->lblMatricule->setText(m_navire.getMatricule());
        ui->lblType->setText(m_navire.getTypeNavire());
        ui->lblPuissance->setText(QString::number(m_navire.getPuissanceMoteur()) + " CV");
        ui->lblLongueur->setText(QString::number(m_navire.getLongueur()) + " m");
        ui->lblProprietaire->setText(m_navire.getProprietaire());
        ui->lblStatut->setText(m_navire.getStatutNavire());
        ui->lblCargoCap->setText(QString::number(m_navire.getMaxCargoCapacity()) + " T");
    }
}

void NavireDetailPage::loadVoyageLogs() {
    QSqlQueryModel *model = new QSqlQueryModel(this);
    QSqlQuery q(Connection::createInstance().db);
    q.prepare("SELECT DEPARTURE_TIME, RETURN_TIME, DISTANCE_NM, FUEL_CONSUMED, CARGO_LOAD_TONS, DESCRIPTION "
              "FROM VESSEL_LOGS WHERE IDNAVIRE = :id ORDER BY RETURN_TIME DESC");
    q.bindValue(":id", m_shipId);
    q.exec();
    model->setQuery(std::move(q));
    
    model->setHeaderData(0, Qt::Horizontal, "Départ");
    model->setHeaderData(1, Qt::Horizontal, "Retour");
    model->setHeaderData(2, Qt::Horizontal, "Distance (nm)");
    model->setHeaderData(3, Qt::Horizontal, "Fuel (L)");
    model->setHeaderData(4, Qt::Horizontal, "Cargo (T)");
    model->setHeaderData(5, Qt::Horizontal, "Observations");
    
    ui->tableLogs->setModel(model);
    ui->tableLogs->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void NavireDetailPage::on_btnAddVoyage_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Ajouter un Voyage");
    QFormLayout form(&dialog);

    QDateTimeEdit *depEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-1), &dialog);
    QDateTimeEdit *retEdit = new QDateTimeEdit(QDateTime::currentDateTime(), &dialog);
    QDoubleSpinBox *distEdit = new QDoubleSpinBox(&dialog);
    distEdit->setRange(0, 10000);
    QDoubleSpinBox *fuelEdit = new QDoubleSpinBox(&dialog);
    fuelEdit->setRange(0, 100000);
    QDoubleSpinBox *cargoEdit = new QDoubleSpinBox(&dialog);
    cargoEdit->setRange(0, 100000);
    QPlainTextEdit *descEdit = new QPlainTextEdit(&dialog);
    descEdit->setPlaceholderText("Décrivez le voyage, problèmes éventuels, sons suspects...");

    form.addRow("Date Départ:", depEdit);
    form.addRow("Date Retour:", retEdit);
    form.addRow("Distance (nm):", distEdit);
    form.addRow("Fuel Consommé (L):", fuelEdit);
    form.addRow("Cargo Transporté (T):", cargoEdit);
    form.addRow("Description/Remarques:", descEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        double dist = distEdit->value();
        double fuel = fuelEdit->value();
        double cargo = cargoEdit->value();
        
        // Deterministic Strain Calculation (Option A)
        // Ratio of fuel consumption per nm scaled by cargo load vs capacity
        double capacity = m_navire.getMaxCargoCapacity();
        double fuelPerNm = (dist > 0) ? (fuel / dist) : 0;
        double loadFactor = (capacity > 0) ? (cargo / capacity) : 0;
        double computedStrain = qBound(0.0, fuelPerNm * (1.0 + loadFactor) * 10.0, 100.0);

        QSqlQuery q(Connection::createInstance().db);
        q.prepare("INSERT INTO VESSEL_LOGS (IDNAVIRE, DEPARTURE_TIME, RETURN_TIME, DISTANCE_NM, "
                  "FUEL_CONSUMED, CARGO_LOAD_TONS, AVG_STRAIN_INDEX, WAVE_EXPOSURE_HOURS, DESCRIPTION) "
                  "VALUES (:id, :dep, :ret, :dist, :fuel, :cargo, :strain, :wave, :desc)");
        q.bindValue(":id", m_shipId);
        q.bindValue(":dep", depEdit->dateTime());
        q.bindValue(":ret", retEdit->dateTime());
        q.bindValue(":dist", dist);
        q.bindValue(":fuel", fuel);
        q.bindValue(":cargo", cargo);
        q.bindValue(":strain", computedStrain);
        q.bindValue(":wave", 0.0); // Safe default as per requirements
        q.bindValue(":desc", descEdit->toPlainText());
        
        if (q.exec()) {
            loadVoyageLogs();
        } else {
            QMessageBox::critical(this, "Erreur", "Impossible d'ajouter le voyage: " + q.lastError().text());
        }
    }
}


