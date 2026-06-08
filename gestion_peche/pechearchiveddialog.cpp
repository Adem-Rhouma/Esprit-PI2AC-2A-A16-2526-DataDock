#include "pechearchiveddialog.h"
#include "ui_pechearchiveddialog.h"

#include "model/peche.h"

#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlError>
#include <QSqlQuery>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

static QString resolveNavireName(int idNavire)
{
    QSqlQuery q;
    q.prepare("SELECT NOMNAVIRE FROM NAVIRES WHERE IDNAVIRE = :id");
    q.bindValue(":id", idNavire);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return QString();
}

static QString resolveEspeceName(int idEspece)
{
    QSqlQuery q;
    q.prepare("SELECT NOMESPECE FROM ESPECES WHERE IDESPECE = :id");
    q.bindValue(":id", idEspece);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return QString();
}

PecheArchivedDialog::PecheArchivedDialog(QWidget *parent)
    : PecheBaseDialog(parent)
    , ui(new Ui::PecheArchivedDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Lots archivés"));
    setModal(true);

    table = ui->tableArchived;
    setupTable();
    loadArchived();

    connect(ui->btnClose, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &PecheArchivedDialog::loadArchived);
}

PecheArchivedDialog::~PecheArchivedDialog()
{
    delete ui;
}

void PecheArchivedDialog::setupTable()
{
    table->setColumnCount(9);
    table->setHorizontalHeaderLabels({
        tr("Code lot"),
        tr("Date débarquement"),
        tr("Origine"),
        tr("Espèce"),
        tr("Quantité (kg)"),
        tr("Prix/kg (TND)"),
        tr("Montant vente (TND)"),
        tr("Statut lot"),
        tr("Actions")
    });

    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    table->setFrameShape(QFrame::NoFrame);
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(false);
}

void PecheArchivedDialog::loadArchived()
{
    table->setRowCount(0);

    QSqlQuery qPeche;
    qPeche.prepare(
        "SELECT IDPECHE, DATEPECHE, ZONEPECHE, DESCRIPTION, IDNAVIRE "
        "FROM PECHES WHERE NVL(IS_ARCHIVED, 0) = 1 ORDER BY IDPECHE");
    if (!qPeche.exec()) {
        QMessageBox::warning(this, tr("Erreur"), qPeche.lastError().text());
        return;
    }

    while (qPeche.next()) {
        const int idPeche = qPeche.value(0).toInt();
        const QDate datePeche = qPeche.value(1).toDate();
        const QString zonePeche = qPeche.value(2).toString();
        const QString description = qPeche.value(3).toString();
        const int idNavire = qPeche.value(4).toInt();

        QString origine = resolveNavireName(idNavire);
        if (origine.isEmpty()) {
            origine = zonePeche;
        }
        if (origine.isEmpty()) {
            origine = tr("Navire %1").arg(idNavire);
        }

        double totalQuantite = 0.0;
        double totalValeur = 0.0;
        int especeId = 0;
        int especeCount = 0;

        QSqlQuery qAssoc;
        qAssoc.prepare("SELECT IDESPECE, QUANTITE, PRIXUNITAIRE FROM PECHESESPECES WHERE IDPECHE = :id");
        qAssoc.bindValue(":id", idPeche);
        if (qAssoc.exec()) {
            while (qAssoc.next()) {
                especeId = qAssoc.value(0).toInt();
                const double qte = qAssoc.value(1).toDouble();
                const double prix = qAssoc.value(2).toDouble();
                totalQuantite += qte;
                totalValeur += qte * prix;
                especeCount++;
            }
        }

        QString especeDisplay;
        if (especeCount > 1) {
            especeDisplay = tr("MULTI");
        } else if (especeCount == 1) {
            especeDisplay = resolveEspeceName(especeId);
            if (especeDisplay.isEmpty()) {
                especeDisplay = QString::number(especeId);
            }
        } else {
            especeDisplay = tr("-");
        }

        const double prixMoyen = totalQuantite > 0.0 ? (totalValeur / totalQuantite) : 0.0;

        const QString statut = description.section('|', 0, 0).trimmed().isEmpty()
                                   ? description
                                   : description.section('|', 0, 0).trimmed();

        addRow(QString::number(idPeche),
               datePeche.isValid() ? datePeche.toString("dd/MM/yyyy") : QString(),
               origine,
               especeDisplay,
               totalQuantite,
               prixMoyen,
               statut);
    }
}

void PecheArchivedDialog::addRow(const QString &idPeche,
                                 const QString &datePeche,
                                 const QString &origine,
                                 const QString &espece,
                                 double quantite,
                                 double prixUnitaire,
                                 const QString &statut)
{
    const int row = table->rowCount();
    table->insertRow(row);
    table->setRowHeight(row, 48);

    auto *idItem = new QTableWidgetItem(idPeche);
    idItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 0, idItem);

    auto *dateItem = new QTableWidgetItem(datePeche);
    dateItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 1, dateItem);

    auto *origineItem = new QTableWidgetItem(origine);
    origineItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 2, origineItem);

    auto *especeItem = new QTableWidgetItem(espece);
    especeItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 3, especeItem);

    auto *qtyItem = new QTableWidgetItem(QString::number(quantite, 'f', 2));
    qtyItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 4, qtyItem);

    auto *prixItem = new QTableWidgetItem(QString::number(prixUnitaire, 'f', 3));
    prixItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 5, prixItem);

    auto *totalItem = new QTableWidgetItem(QString::number(quantite * prixUnitaire, 'f', 3));
    totalItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 6, totalItem);

    auto *statutItem = new QTableWidgetItem(statut);
    statutItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 7, statutItem);

    table->setCellWidget(row, 8, createActionsWidget(idPeche));
}

QWidget *PecheArchivedDialog::createActionsWidget(const QString &idPeche)
{
    auto *wrapper = new QWidget();
    auto *layout = new QHBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignCenter);

    auto makeButton = [wrapper, &idPeche](const QString &text, const QString &tooltip, const QString &objectName) {
        auto *button = new QToolButton(wrapper);
        button->setText(text);
        button->setToolTip(tooltip);
        button->setObjectName(objectName);
        button->setProperty("idPeche", idPeche);
        return button;
    };
    QToolButton *restoreButton = makeButton(QString::fromUtf8("\xE2\x86\xA9\xEF\xB8\x8F"), tr("Restaurer"), "btnRestore");
    QToolButton *deleteButton = makeButton(QString::fromUtf8("\xF0\x9F\x97\x91\xEF\xB8\x8F"), tr("Supprimer"), "btnDelete");

    layout->addWidget(restoreButton);
    layout->addWidget(deleteButton);

    auto bindAction = [this](QToolButton *button, const QString &label) {
        connect(button, &QToolButton::clicked, this, [this, button, label]() {
            const QString idPeche = button->property("idPeche").toString();
            bool ok = false;
            const int id = idPeche.toInt(&ok);
            if (!ok) {
                QMessageBox::warning(this, tr("Erreur"), tr("ID lot invalide."));
                return;
            }

            if (label == "Restaurer") {
                QSqlQuery q;
                q.prepare("UPDATE PECHES SET IS_ARCHIVED = 0 WHERE IDPECHE = :id");
                q.bindValue(":id", id);
                if (!q.exec()) {
                    QMessageBox::warning(this, tr("Erreur"), q.lastError().text());
                    return;
                }
                loadArchived();
            } else if (label == "Supprimer") {
                const QMessageBox::StandardButton reply = QMessageBox::question(
                    this, tr("Confirmation"),
                    tr("Supprimer définitivement le lot %1 ?").arg(idPeche),
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    if (!Peche::supprimer(id)) {
                        QMessageBox::warning(this, tr("Erreur"), tr("Suppression échouée."));
                        return;
                    }
                    loadArchived();
                }
            }
        });
    };

    bindAction(restoreButton, "Restaurer");
    bindAction(deleteButton, "Supprimer");

    return wrapper;
}
