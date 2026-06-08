#include "pechedetailsdialog.h"
#include "ui_pechedetailsdialog.h"

#include <QDate>
#include <QHeaderView>
#include <QSqlError>
#include <QSqlQuery>
#include <QPushButton>

static void setupCloseButton(Ui::PecheDetailsDialog *ui, QDialog *dialog)
{
    if (ui && ui->btnClose && dialog) {
        QObject::connect(ui->btnClose, &QPushButton::clicked, dialog, &QDialog::accept);
    }
}

PecheDetailsDialog::PecheDetailsDialog(int idPeche, QWidget *parent)
    : PecheBaseDialog(parent)
    , ui(new Ui::PecheDetailsDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Détails du lot de pêche"));
    setModal(true);

    ui->tableEspeces->setColumnCount(4);
    ui->tableEspeces->setHorizontalHeaderLabels({
        tr("Espèce"),
        tr("Quantité"),
        tr("Prix unitaire"),
        tr("Total")
    });
    ui->tableEspeces->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableEspeces->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableEspeces->horizontalHeader()->setStretchLastSection(true);

    ui->tableEmployes->setColumnCount(2);
    ui->tableEmployes->setHorizontalHeaderLabels({
        tr("Employé"),
        tr("Description")
    });
    ui->tableEmployes->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableEmployes->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableEmployes->horizontalHeader()->setStretchLastSection(true);

    setupCloseButton(ui, this);

    loadDetails(idPeche);
    loadEspeces(idPeche);
    loadEmployes(idPeche);
}

PecheDetailsDialog::~PecheDetailsDialog()
{
    delete ui;
}

void PecheDetailsDialog::loadDetails(int idPeche)
{
    QSqlQuery q;
    q.prepare(
        "SELECT p.IDPECHE, p.DATEPECHE, p.ZONEPECHE, p.DESCRIPTION, "
        "p.IDNAVIRE, n.NOMNAVIRE "
        "FROM PECHES p "
        "LEFT JOIN NAVIRES n ON p.IDNAVIRE = n.IDNAVIRE "
        "WHERE p.IDPECHE = :id");
    q.bindValue(":id", idPeche);
    if (!q.exec() || !q.next()) {
        return;
    }

    const QString idText = q.value(0).toString();
    const QDate date = q.value(1).toDate();
    const QString zone = q.value(2).toString();
    const QString description = q.value(3).toString();
    const QString idNavire = q.value(4).toString();
    const QString nomNavire = q.value(5).toString();

    ui->valueIdPeche->setText(idText);
    ui->valueDatePeche->setText(date.isValid() ? date.toString("dd/MM/yyyy") : QString());
    ui->valueZonePeche->setText(zone);
    ui->valueNavire->setText(nomNavire.isEmpty() ? idNavire : nomNavire);
    ui->valueStatut->setText(description);
}

void PecheDetailsDialog::loadEspeces(int idPeche)
{
    ui->tableEspeces->setRowCount(0);

    QSqlQuery q;
    q.prepare(
        "SELECT e.NOMESPECE, ps.QUANTITE, ps.PRIXUNITAIRE, "
        "(NVL(ps.QUANTITE,0) * NVL(ps.PRIXUNITAIRE,0)) AS TOTAL "
        "FROM PECHESESPECES ps "
        "LEFT JOIN ESPECES e ON ps.IDESPECE = e.IDESPECE "
        "WHERE ps.IDPECHE = :id");
    q.bindValue(":id", idPeche);
    if (!q.exec()) {
        return;
    }

    while (q.next()) {
        const int row = ui->tableEspeces->rowCount();
        ui->tableEspeces->insertRow(row);
        ui->tableEspeces->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
        ui->tableEspeces->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
        ui->tableEspeces->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
        ui->tableEspeces->setItem(row, 3, new QTableWidgetItem(q.value(3).toString()));
    }
}

void PecheDetailsDialog::loadEmployes(int idPeche)
{
    ui->tableEmployes->setRowCount(0);

    QSqlQuery q;
    q.prepare(
        "SELECT (NVL(em.NOM,'') || ' ' || NVL(em.PRENOM,'')) AS NOMEMPLOYE, "
        "pe.DESCRIPTION "
        "FROM PECHESEMPLOYES pe "
        "LEFT JOIN EMPLOYES em ON pe.IDEMPLOYE = em.IDEMPLOYE "
        "WHERE pe.IDPECHE = :id");
    q.bindValue(":id", idPeche);
    if (!q.exec()) {
        return;
    }

    while (q.next()) {
        const int row = ui->tableEmployes->rowCount();
        ui->tableEmployes->insertRow(row);
        ui->tableEmployes->setItem(row, 0, new QTableWidgetItem(q.value(0).toString().trimmed()));
        ui->tableEmployes->setItem(row, 1, new QTableWidgetItem(q.value(1).toString()));
    }
}
