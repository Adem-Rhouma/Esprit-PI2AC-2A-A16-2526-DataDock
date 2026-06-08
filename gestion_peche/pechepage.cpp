#include "pechepage.h"
#include "ui_pechepage.h"
#include "pechecruddialog.h"
#include "pechedetailsdialog.h"
#include "pechearchiveddialog.h"
#include "pechefontutils.h"
#include "model/peche.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QHash>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPrinter>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QSqlError>
#include <QSqlQuery>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QFile>
#include <QFileDialog>
#include <QStringConverter>
#include <QTextStream>

namespace {
QString formatMoney(double value)
{
    return QString::number(value, 'f', 3) + " TND";
}

void applyShadow(QWidget *widget, int blur, const QPoint &offset, const QColor &color, qreal opacity)
{
    if (!widget) {
        return;
    }
    auto *effect = new QGraphicsDropShadowEffect(widget);
    QColor tuned = color;
    tuned.setAlphaF(opacity);
    effect->setBlurRadius(blur);
    effect->setOffset(offset);
    effect->setColor(tuned);
    widget->setGraphicsEffect(effect);
}

QString detectDisplayColumn(const QString &table, const QStringList &candidates)
{
    static QHash<QString, QString> cache;
    const QString key = table.toUpper();
    if (cache.contains(key)) {
        return cache.value(key);
    }

    QSet<QString> columns;
    QSqlQuery q;
    q.prepare("SELECT COLUMN_NAME FROM ALL_TAB_COLUMNS WHERE TABLE_NAME = :table");
    q.bindValue(":table", key);
    if (q.exec()) {
        while (q.next()) {
            columns.insert(q.value(0).toString().toUpper());
        }
    }

    QString chosen;
    for (const QString &candidate : candidates) {
        if (columns.contains(candidate.toUpper())) {
            chosen = candidate.toUpper();
            break;
        }
    }

    cache.insert(key, chosen);
    return chosen;
}

QString resolveDisplayName(const QString &table, const QString &idColumn, int id,
                           const QStringList &candidates)
{
    const QString displayColumn = detectDisplayColumn(table, candidates);
    if (displayColumn.isEmpty()) {
        return QString();
    }

    QSqlQuery q;
    q.prepare(QStringLiteral("SELECT %1 FROM %2 WHERE %3 = :id")
                  .arg(displayColumn, table, idColumn));
    q.bindValue(":id", id);
    if (!q.exec()) {
        return QString();
    }
    if (q.next()) {
        return q.value(0).toString();
    }
    return QString();
}
}

PechePage::PechePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PechePage)
{
    ui->setupUi(this);

    PecheFontUtils::applyModuleFont(this);
    setObjectName("PecheCrudPage");

    addButton = ui->btnAddPeche;
    archivedButton = ui->btnArchivedPeche;
    sortCombo = ui->comboSortField;
    sortOrderCombo = ui->comboSortOrder;
    searchCombo = ui->comboSearchField;
    searchEdit = ui->searchBar;
    exportButton = ui->btnExport;
    tableCard = ui->tableCard;
    table = ui->tablePeche;

    if (ui->controlsWidget) {
        ui->controlsWidget->setAttribute(Qt::WA_StyledBackground, true);
    }
    if (tableCard) {
        tableCard->setAttribute(Qt::WA_StyledBackground, true);
    }

    connect(sortCombo, &QComboBox::currentIndexChanged, this, &PechePage::updateSort);
    connect(sortOrderCombo, &QComboBox::currentIndexChanged, this, &PechePage::updateSort);
    connect(searchCombo, &QComboBox::currentIndexChanged, this, &PechePage::applySearchFilter);
    connect(searchEdit, &QLineEdit::textChanged, this, &PechePage::applySearchFilter);
    if (exportButton) {
        connect(exportButton, &QPushButton::clicked, this, [this]() {
            QMenu menu(this);
            menu.setStyleSheet(QStringLiteral(
                "QMenu {"
                "  background-color: #1B2442;"
                "  border: 1px solid rgba(86, 207, 225, 180);"
                "  border-radius: 10px;"
                "  padding: 6px;"
                "}"
                "QMenu::item {"
                "  color: #F8FAFF;"
                "  background: transparent;"
                "  padding: 6px 16px;"
                "  border-radius: 8px;"
                "}"
                "QMenu::item:selected {"
                "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
                "    stop:0 #5E60CE, stop:0.5 #5390D9, stop:1 #56CFE1);"
                "  color: #FFFFFF;"
                "}"
            ));
            menu.addAction(tr("Exporter en PDF"), this, &PechePage::onExportPdf);
            menu.addAction(tr("Exporter en CSV"), this, &PechePage::onExportCsv);
            menu.exec(exportButton->mapToGlobal(exportButton->rect().bottomLeft()));
        });
    }
    connect(addButton, &QPushButton::clicked, this, [this]() {
        PecheCrudDialog dlg(PecheCrudDialog::AddMode, this);
        if (dlg.exec() == QDialog::Accepted) {
            refreshTable();
        }
    });
    if (archivedButton) {
        connect(archivedButton, &QPushButton::clicked, this, [this]() {
            PecheArchivedDialog dlg(this);
            dlg.exec();
            refreshTable();
        });
    }

    setupTable();
    applyStyles();
    loadFromDb();
    updateSort();
}

PechePage::~PechePage()
{
    delete ui;
}

void PechePage::setupTable()
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
    table->setSortingEnabled(true);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    table->setFrameShape(QFrame::NoFrame);
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(false);
    table->setMouseTracking(true);
    table->viewport()->setMouseTracking(true);
    table->horizontalHeader()->setHighlightSections(false);
}

void PechePage::applyStyles()
{
    const QString baseButton = QStringLiteral(
        "QWidget#PecheCrudPage { color: #1B1B1B; }"
        "QWidget#PecheCrudPage QPushButton {"
        "  color: white;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #7400B8, stop:0.5 #5390D9, stop:1 #56CFE1);"
        "  border: 1px solid rgba(94, 96, 206, 160);"
        "  border-radius: 10px;"
        "  padding: 7px 16px;"
        "}"
        "QWidget#PecheCrudPage QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #6930C3, stop:0.5 #48BFE3, stop:1 #72EFDD);"
        "  border-color: rgba(72, 191, 227, 200);"
        "}"
        "QWidget#PecheCrudPage QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #5E60CE, stop:0.5 #5390D9, stop:1 #4EA8DE);"
        "}"
    );

    const QString inputStyle = QStringLiteral(
        "QWidget#PecheCrudPage QComboBox, "
        "QWidget#PecheCrudPage QLineEdit {"
        "  color: #1B1B1B;"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #5390D9;"
        "  border-radius: 10px;"
        "  padding: 6px 10px;"
        "}"
        "QWidget#PecheCrudPage QComboBox:focus, "
        "QWidget#PecheCrudPage QLineEdit:focus {"
        "  border: 2px solid #48BFE3;"
        "}"
        "QWidget#PecheCrudPage QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 18px;"
        "  border-left: 1px solid #5E60CE;"
        "}"
        "QWidget#PecheCrudPage QComboBox QAbstractItemView {"
        "  background-color: #FFFFFF;"
        "  color: #1B1B1B;"
        "  selection-background-color: rgba(72, 191, 227, 80);"
        "}"
    );

    const QString tableStyle = QStringLiteral(
        "QWidget#tableCard {"
        "  background-color: #FFFFFF;"
        "  border: 1px solid rgba(0, 0, 0, 20);"
        "  border-radius: 16px;"
        "}"
        "QTableWidget#tablePeche {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: rgba(78, 168, 222, 10);"
        "  color: #1B1B1B;"
        "  gridline-color: rgba(0, 0, 0, 18);"
        "  border: none;"
        "}"
        "QTableWidget#tablePeche::item {"
        "  padding: 6px;"
        "  color: #1B1B1B;"
        "}"
        "QTableWidget#tablePeche::item:selected {"
        "  background-color: rgba(128, 255, 219, 70);"
        "}"
        "QTableWidget#tablePeche::item:hover {"
        "  background-color: rgba(86, 207, 225, 40);"
        "}"
        "QHeaderView::section {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #7400B8, stop:0.5 #5390D9, stop:1 #56CFE1);"
        "  color: #FFFFFF;"
        "  font-weight: 700;"
        "  padding: 8px;"
        "  border: none;"
        "}"
        "QTableCornerButton::section {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #7400B8, stop:0.5 #5390D9, stop:1 #56CFE1);"
        "  border: none;"
        "}"
    );

    const QString actionButton = QStringLiteral(
        "QWidget#PecheCrudPage QToolButton#btnView, "
        "QWidget#PecheCrudPage QToolButton#btnEdit, "
        "QWidget#PecheCrudPage QToolButton#btnDelete, "
        "QWidget#PecheCrudPage QToolButton#btnCancel, "
        "QWidget#PecheCrudPage QToolButton#btnArchive {"
        "  color: #5E60CE;"
        "  background-color: rgba(116, 0, 184, 12);"
        "  border: 1px solid rgba(116, 0, 184, 120);"
        "  border-radius: 8px;"
        "  min-width: 28px;"
        "  max-width: 28px;"
        "  min-height: 28px;"
        "  max-height: 28px;"
        "}"
        "QWidget#PecheCrudPage QToolButton#btnView:hover, "
        "QWidget#PecheCrudPage QToolButton#btnEdit:hover, "
        "QWidget#PecheCrudPage QToolButton#btnDelete:hover, "
        "QWidget#PecheCrudPage QToolButton#btnCancel:hover, "
        "QWidget#PecheCrudPage QToolButton#btnArchive:hover {"
        "  color: #FFFFFF;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #6930C3, stop:1 #56CFE1);"
        "  border-color: rgba(72, 191, 227, 200);"
        "}"
    );

    setStyleSheet(baseButton + inputStyle + tableStyle + actionButton);

    table->setAlternatingRowColors(true);
    table->viewport()->setAutoFillBackground(true);
    table->setShowGrid(false);

}

void PechePage::loadFromDb()
{
    table->setSortingEnabled(false);
    table->setRowCount(0);

    QSqlQuery qPeche;
    qPeche.prepare(
        "SELECT IDPECHE, DATEPECHE, ZONEPECHE, DESCRIPTION, IDNAVIRE "
        "FROM PECHES WHERE NVL(IS_ARCHIVED, 0) = 0 ORDER BY IDPECHE");
    if (!qPeche.exec()) {
        qDebug() << "PechePage loadFromDb failed:" << qPeche.lastError().text();
        table->setSortingEnabled(true);
        return;
    }

    while (qPeche.next()) {
        const int idPeche = qPeche.value(0).toInt();
        const QDate datePeche = qPeche.value(1).toDate();
        const QString zonePeche = qPeche.value(2).toString();
        const QString description = qPeche.value(3).toString();
        const int idNavire = qPeche.value(4).toInt();

        QString origine = resolveDisplayName("NAVIRES", "IDNAVIRE", idNavire,
                                             { "NOMNAVIRE", "NOM", "LIBELLE", "DESIGNATION" });
        if (origine.isEmpty()) {
            origine = zonePeche;
        }
        if (origine.isEmpty()) {
            origine = tr("Navire %1").arg(idNavire);
        }

        double totalQuantite = 0.0;
        double totalValeur = 0.0;
        QSet<int> especes;
        int especeId = 0;

        QSqlQuery qAssoc;
        qAssoc.prepare("SELECT IDESPECE, QUANTITE, PRIXUNITAIRE FROM PECHESESPECES WHERE IDPECHE = :id");
        qAssoc.bindValue(":id", idPeche);
        if (qAssoc.exec()) {
            while (qAssoc.next()) {
                const int idEspece = qAssoc.value(0).toInt();
                const double qte = qAssoc.value(1).toDouble();
                const double prix = qAssoc.value(2).toDouble();
                especes.insert(idEspece);
                especeId = idEspece;
                totalQuantite += qte;
                totalValeur += qte * prix;
            }
        } else {
            qDebug() << "PechePage load assoc failed:" << qAssoc.lastError().text();
        }

        QString especeDisplay;
        if (especes.size() > 1) {
            especeDisplay = tr("MULTI");
        } else if (especes.size() == 1) {
            especeDisplay = resolveDisplayName("ESPECES", "IDESPECE", especeId,
                                               { "NOMESPECE", "NOM", "LIBELLE", "DESIGNATION" });
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

    table->setSortingEnabled(true);
}

void PechePage::addRow(const QString &idPeche,
                              const QString &datePeche,
                              const QString &typeOperation,
                              const QString &typeProduit,
                              double quantite,
                              double prixUnitaire,
                              const QString &statutPeche)
{
    const int row = table->rowCount();
    table->insertRow(row);
    table->setRowHeight(row, 50);

    auto *idItem = new QTableWidgetItem(idPeche);
    idItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 0, idItem);

    auto *dateItem = new QTableWidgetItem(datePeche);
    dateItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 1, dateItem);

    auto *opItem = new QTableWidgetItem(typeOperation);
    opItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 2, opItem);

    auto *prodItem = new QTableWidgetItem(typeProduit);
    prodItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 3, prodItem);

    auto *qtyItem = new QTableWidgetItem(QString::number(quantite, 'f', 2));
    qtyItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 4, qtyItem);

    const QString prixText = formatMoney(prixUnitaire);
    auto *prixItem = new QTableWidgetItem(prixText);
    prixItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 5, prixItem);

    const QString totalText = formatMoney(quantite * prixUnitaire);
    auto *totalItem = new QTableWidgetItem(totalText);
    totalItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 6, totalItem);

    auto *statusItem = new QTableWidgetItem(statutPeche);
    statusItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 7, statusItem);
    table->setCellWidget(row, 7, createStatusBadge(statutPeche));

    table->setCellWidget(row, 8, createActionsWidget(idPeche));
}

QWidget *PechePage::createStatusBadge(const QString &statut) const
{
    QString style;
    if (statut.compare(QStringLiteral(u"VENDU_LOCAL"), Qt::CaseInsensitive) == 0
        || statut.compare("EN_STOCK", Qt::CaseInsensitive) == 0) {
        style = "QLabel { color: #20305E; padding: 4px 12px; border-radius: 12px;"
                "background-color: rgba(72, 191, 227, 130); }";
    } else if (statut.compare("EN_ATTENTE", Qt::CaseInsensitive) == 0) {
        style = "QLabel { color: #2C3568; padding: 4px 12px; border-radius: 12px;"
                "background-color: rgba(94, 96, 206, 120); }";
    } else if (statut.compare(QStringLiteral(u"ANNULE"), Qt::CaseInsensitive) == 0) {
        style = "QLabel { color: #FFFFFF; padding: 4px 12px; border-radius: 12px;"
                "background-color: rgba(90, 64, 188, 200); }";
    } else if (statut.compare(QStringLiteral(u"ARCHIVE"), Qt::CaseInsensitive) == 0) {
        style = "QLabel { color: #2E3B6B; padding: 4px 12px; border-radius: 12px;"
                "background-color: rgba(200, 210, 245, 180); }";
    } else {
        style = "QLabel { color: #FFFFFF; padding: 4px 12px; border-radius: 12px;"
                "background-color: rgba(94, 96, 206, 190); }";
    }

    auto *wrapper = new QWidget();
    auto *layout = new QHBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);

    auto *label = new QLabel(statut, wrapper);
    label->setStyleSheet(style);
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumHeight(22);

    layout->addWidget(label);
    return wrapper;
}

QWidget *PechePage::createActionsWidget(const QString &idPeche)
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

    QToolButton *viewButton = makeButton(QString::fromUtf8("\xF0\x9F\x91\x81"), tr("Consulter traçabilité"), "btnView");
    QToolButton *editButton = makeButton(QString::fromUtf8("\xE2\x9C\x8F\xEF\xB8\x8F"), tr("Modifier lot"), "btnEdit");
    QToolButton *deleteButton = makeButton(QString::fromUtf8("\xF0\x9F\x97\x91\xEF\xB8\x8F"), tr("Supprimer lot"), "btnDelete");
    QToolButton *archiveButton = makeButton(QString::fromUtf8("\xF0\x9F\x93\xA6"), tr("Archiver lot"), "btnArchive");

    layout->addWidget(viewButton);
    layout->addWidget(editButton);
    layout->addWidget(deleteButton);
    layout->addWidget(archiveButton);

    auto bindAction = [this](QToolButton *button, const QString &label) {
        connect(button, &QToolButton::clicked, this, [this, button, label]() {
            const QString idPeche = button->property("idPeche").toString();
            const int row = findRowById(idPeche);
            qDebug() << label << "row" << row << "idPeche" << idPeche;

            if (label == "Consulter") {
                bool ok = false;
                const int id = idPeche.toInt(&ok);
                if (!ok) {
                    QMessageBox::warning(this, tr("Erreur"), tr("ID lot invalide."));
                    return;
                }
                PecheDetailsDialog dlg(id, this);
                dlg.exec();
            } else if (label == "Modifier") {
                bool ok = false;
                const int id = idPeche.toInt(&ok);
                if (!ok) {
                    QMessageBox::warning(this, tr("Erreur"), tr("ID lot invalide."));
                    return;
                }
                PecheCrudDialog dlg(PecheCrudDialog::EditMode, id, this);
                if (dlg.exec() == QDialog::Accepted) {
                    refreshTable();
                }
            } else if (label == "Supprimer") {
                const QMessageBox::StandardButton reply = QMessageBox::question(
                    this, tr("Confirmation"),
                    tr("Supprimer le lot %1 ?").arg(idPeche),
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    bool ok = false;
                    const int id = idPeche.toInt(&ok);
                    if (!ok) {
                        QMessageBox::warning(this, tr("Erreur"), tr("ID lot invalide."));
                        return;
                    }
                    if (!Peche::supprimer(id)) {
                        QMessageBox::warning(this, tr("Erreur"), tr("Suppression échouée."));
                        return;
                    }
                    refreshTable();
                }
            } else if (label == "Archiver") {
                const QMessageBox::StandardButton reply = QMessageBox::question(
                    this, tr("Confirmation"),
                    tr("Archiver le lot %1 ?").arg(idPeche),
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    bool ok = false;
                    const int id = idPeche.toInt(&ok);
                    if (!ok) {
                        QMessageBox::warning(this, tr("Erreur"), tr("ID lot invalide."));
                        return;
                    }
                    QSqlQuery q;
                    q.prepare("UPDATE PECHES SET IS_ARCHIVED = 1 WHERE IDPECHE = :id");
                    q.bindValue(":id", id);
                    if (!q.exec()) {
                        QMessageBox::warning(this, tr("Erreur"), q.lastError().text());
                        return;
                    }
                    refreshTable();
                }
            }
        });
    };

    bindAction(viewButton, "Consulter");
    bindAction(editButton, "Modifier");
    bindAction(deleteButton, "Supprimer");
    bindAction(archiveButton, "Archiver");

    return wrapper;
}

int PechePage::findRowById(const QString &idPeche) const
{
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem *item = table->item(row, 0);
        if (item && item->text() == idPeche) {
            return row;
        }
    }
    return -1;
}

void PechePage::updateSort()
{
    int column = 1;
    switch (sortCombo->currentIndex()) {
    case 0:
        column = 1;
        break;
    case 1:
        column = 3;
        break;
    case 2:
        column = 7;
        break;
    case 3:
        column = 2;
        break;
    default:
        column = 1;
        break;
    }

    const Qt::SortOrder order = (sortOrderCombo->currentIndex() == 0) ? Qt::AscendingOrder : Qt::DescendingOrder;
    table->sortItems(column, order);
    table->horizontalHeader()->setSortIndicator(column, order);
}

void PechePage::applySearchFilter()
{
    const QString filter = searchEdit->text().trimmed();

    int column = 0;
    switch (searchCombo->currentIndex()) {
    case 0:
        column = 0;
        break;
    case 1:
        column = 1;
        break;
    case 2:
        column = 3;
        break;
    case 3:
        column = 7;
        break;
    default:
        column = 0;
        break;
    }

    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem *item = table->item(row, column);
        const QString text = item ? item->text() : QString();
        const bool match = filter.isEmpty() || text.contains(filter, Qt::CaseInsensitive);
        table->setRowHidden(row, !match);
    }
}

void PechePage::refreshTable()
{
    loadFromDb();
    updateSort();
}

QVector<int> PechePage::chooseExportColumns(bool *accepted, bool *visibleRowsOnly)
{
    if (accepted) {
        *accepted = false;
    }
    if (visibleRowsOnly) {
        *visibleRowsOnly = true;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Choisir les donnees a exporter"));
    dialog.setModal(true);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    auto *title = new QLabel(tr("Colonnes a exporter"), &dialog);
    title->setFont(PecheFontUtils::moduleFont(11.0, QFont::Bold));
    layout->addWidget(title);

    auto *selectAll = new QCheckBox(tr("Tout selectionner"), &dialog);
    selectAll->setChecked(true);
    layout->addWidget(selectAll);

    QVector<QCheckBox *> columnChecks;
    for (int c = 0; c < table->columnCount() - 1; ++c) {
        QTableWidgetItem *hItem = table->horizontalHeaderItem(c);
        auto *check = new QCheckBox(hItem ? hItem->text() : tr("Colonne %1").arg(c + 1), &dialog);
        check->setProperty("columnIndex", c);
        check->setChecked(true);
        columnChecks << check;
        layout->addWidget(check);
    }

    auto *visibleOnly = new QCheckBox(tr("Exporter uniquement les lignes visibles"), &dialog);
    visibleOnly->setChecked(true);
    layout->addWidget(visibleOnly);

    auto updateSelectAll = [selectAll, columnChecks]() {
        bool allChecked = true;
        for (QCheckBox *check : columnChecks) {
            allChecked = allChecked && check->isChecked();
        }
        QSignalBlocker blocker(selectAll);
        selectAll->setChecked(allChecked);
    };

    connect(selectAll, &QCheckBox::toggled, &dialog, [columnChecks](bool checked) {
        for (QCheckBox *check : columnChecks) {
            check->setChecked(checked);
        }
    });
    for (QCheckBox *check : columnChecks) {
        connect(check, &QCheckBox::toggled, &dialog, updateSelectAll);
    }
    updateSelectAll();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Exporter"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Annuler"));
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&dialog, columnChecks, this]() {
        for (QCheckBox *check : columnChecks) {
            if (check->isChecked()) {
                dialog.accept();
                return;
            }
        }
        QMessageBox::warning(this, tr("Erreur"), tr("Selectionnez au moins une colonne a exporter."));
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return {};
    }

    QVector<int> cols;
    for (QCheckBox *check : columnChecks) {
        if (check->isChecked()) {
            cols << check->property("columnIndex").toInt();
        }
    }

    if (accepted) {
        *accepted = true;
    }
    if (visibleRowsOnly) {
        *visibleRowsOnly = visibleOnly->isChecked();
    }
    return cols;
}

void PechePage::onExportPdf()
{
    bool accepted = false;
    bool visibleRowsOnly = true;
    const QVector<int> cols = chooseExportColumns(&accepted, &visibleRowsOnly);
    if (!accepted) return;
    if (cols.isEmpty()) {
        QMessageBox::warning(this, tr("Erreur"), tr("Aucune colonne exportable."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this, tr("Exporter en PDF"), "peche.pdf",
        tr("PDF Files (*.pdf);;All Files (*)"));
    if (path.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setPageSize(QPageSize::A4);

    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible d'initialiser le PDF."));
        return;
    }

    const int dpi    = printer.resolution();
    const int margin = dpi / 4;
    QRect pageRect   = printer.pageRect(QPrinter::DevicePixel).toRect();
    int x = pageRect.left()  + margin;
    int y = pageRect.top()   + margin;
    int pageW = pageRect.width() - 2 * margin;

    QFont titleFont = PecheFontUtils::moduleFont(16.0, QFont::Bold);
    painter.setFont(titleFont);
    painter.drawText(x, y, tr("Rapport — Gestion Pêche"));
    y += dpi / 4;

    const int colW = pageW / cols.size();
    const int rowH = dpi / 6;

    QFont hdrFont = PecheFontUtils::moduleFont(9.0, QFont::Bold);
    painter.setFont(hdrFont);
    painter.setBrush(QColor(0x53, 0x90, 0xD9));
    painter.setPen(Qt::NoPen);
    painter.drawRect(x, y, pageW, rowH);
    painter.setPen(Qt::white);
    for (int i = 0; i < cols.size(); ++i) {
        QTableWidgetItem *hItem = table->horizontalHeaderItem(cols[i]);
        painter.drawText(QRect(x + i * colW + 4, y, colW - 8, rowH),
                         Qt::AlignVCenter | Qt::AlignLeft,
                         hItem ? hItem->text() : QString());
    }
    y += rowH;

    QFont dataFont = PecheFontUtils::moduleFont(8.0);
    painter.setFont(dataFont);
    bool alt = false;
    for (int r = 0; r < table->rowCount(); ++r) {
        if (visibleRowsOnly && table->isRowHidden(r)) continue;
        if (y + rowH > pageRect.bottom() - margin) {
            printer.newPage();
            y = pageRect.top() + margin;
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(alt ? QColor(0xf8, 0xfa, 0xff) : Qt::white);
        painter.drawRect(x, y, pageW, rowH);
        painter.setPen(QColor(0x1B, 0x1B, 0x2F));
        for (int i = 0; i < cols.size(); ++i) {
            QTableWidgetItem *it = table->item(r, cols[i]);
            painter.drawText(QRect(x + i * colW + 4, y, colW - 8, rowH),
                             Qt::AlignVCenter | Qt::AlignLeft,
                             it ? it->text() : QString());
        }
        y += rowH;
        alt = !alt;
    }

    painter.end();
    QMessageBox::information(this, tr("Export réussi"),
        tr("PDF exporté avec succès :\n%1").arg(path));
}

void PechePage::onExportCsv()
{
    bool accepted = false;
    bool visibleRowsOnly = true;
    const QVector<int> cols = chooseExportColumns(&accepted, &visibleRowsOnly);
    if (!accepted) return;
    if (cols.isEmpty()) {
        QMessageBox::warning(this, tr("Erreur"), tr("Aucune colonne exportable."));
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this, tr("Exporter en CSV"), "peche.csv",
        tr("CSV Files (*.csv);;All Files (*)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible d'écrire dans le fichier."));
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    QStringList headers;
    for (int c : cols) {
        QTableWidgetItem *hItem = table->horizontalHeaderItem(c);
        headers << (hItem ? hItem->text() : QString());
    }
    out << headers.join(";") << "\n";

    for (int r = 0; r < table->rowCount(); ++r) {
        if (visibleRowsOnly && table->isRowHidden(r)) continue;
        QStringList row;
        for (int c : cols) {
            QTableWidgetItem *it = table->item(r, c);
            QString val = it ? it->text() : QString();
            if (val.contains(';') || val.contains('"') || val.contains('\n')) {
                val.replace('"', QStringLiteral("\"\""));
                val = '"' + val + '"';
            }
            row << val;
        }
        out << row.join(";") << "\n";
    }
    file.close();

    QMessageBox::information(this, tr("Export réussi"),
        tr("Fichier exporté avec succès :\n%1").arg(path));
}







