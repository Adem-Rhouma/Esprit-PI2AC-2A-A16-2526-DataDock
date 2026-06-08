#include "logistiquepage.h"
#include "ui_logistiquepage.h"

#include <QComboBox>
#include <QDebug>
#include <QFont>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

LogistiquePage::LogistiquePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LogistiquePage)
{
    ui->setupUi(this);

    QFont font("Cal Sans", 10);
    setFont(font);
    setObjectName("LogistiquePageCrud");

    addButton = ui->btnAddCamion;
    sortCombo = ui->comboSortField;
    sortOrderCombo = ui->comboSortOrder;
    searchCombo = ui->comboSearchField;
    searchEdit = ui->searchBar;
    exportPdfButton = ui->btnExportPdf;
    exportExcelButton = ui->btnExportExcel;
    exportTxtButton = ui->btnExportTxt;
    tableCard = ui->tableCard;
    table = ui->tableCamions;

    connect(sortCombo, &QComboBox::currentIndexChanged, this, &LogistiquePage::updateSort);
    connect(sortOrderCombo, &QComboBox::currentIndexChanged, this, &LogistiquePage::updateSort);
    connect(searchCombo, &QComboBox::currentIndexChanged, this, &LogistiquePage::applySearchFilter);
    connect(searchEdit, &QLineEdit::textChanged, this, &LogistiquePage::applySearchFilter);
    connect(exportPdfButton, &QPushButton::clicked, this, &LogistiquePage::onExportPdf);
    connect(exportExcelButton, &QPushButton::clicked, this, &LogistiquePage::onExportExcel);
    connect(exportTxtButton, &QPushButton::clicked, this, &LogistiquePage::onExportTxt);

    setupTable();
    applyStyles();
    loadSampleData();
    updateSort();
}

LogistiquePage::~LogistiquePage()
{
    delete ui;
}

void LogistiquePage::setupTable()
{
    table->setColumnCount(10);
    table->setHorizontalHeaderLabels({
        tr("ID"),
        tr("Immatriculation"),
        tr("Type"),
        tr("Capacité"),
        tr("Charge Actuelle"),
        tr("Zone"),
        tr("Statut"),
        tr("Entrée"),
        tr("Sortie"),
        tr("Actions")
    });

    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(true);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    // Stretch "Zone" or "Immatriculation" maybe? Let's stretch Immatriculation (index 1) or Type
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); 
    table->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);
    table->setFrameShape(QFrame::NoFrame);
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(false);
}

void LogistiquePage::applyStyles()
{
    const QString baseButton = QStringLiteral(
        "QPushButton {"
        "  color: white;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #7400B8, stop:0.5 #5390D9, stop:1 #56CFE1);"
        "  border: 1px solid rgba(94, 96, 206, 160);"
        "  border-radius: 10px;"
        "  padding: 7px 16px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #6930C3, stop:0.5 #48BFE3, stop:1 #72EFDD);"
        "  border-color: rgba(72, 191, 227, 200);"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #5E60CE, stop:0.5 #5390D9, stop:1 #4EA8DE);"
        "}"
    );

    const QString inputStyle = QStringLiteral(
        "QComboBox, QLineEdit {"
        "  color: #1B1B1B;"
        "  background-color: #FFFFFF;"
        "  border: 1px solid #5390D9;"
        "  border-radius: 10px;"
        "  padding: 6px 10px;"
        "}"
        "QComboBox:focus, QLineEdit:focus {"
        "  border: 2px solid #48BFE3;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 18px;"
        "  border-left: 1px solid #5E60CE;"
        "}"
        "QComboBox QAbstractItemView {"
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
        "QTableWidget#tableCamions {"
        "  background-color: #FFFFFF;"
        "  alternate-background-color: rgba(78, 168, 222, 10);"
        "  color: #1B1B1B;"
        "  gridline-color: rgba(0, 0, 0, 18);"
        "  border: none;"
        "}"
        "QTableWidget#tableCamions::item {"
        "  padding: 6px;"
        "  color: #1B1B1B;"
        "}"
        "QTableWidget#tableCamions::item:selected {"
        "  background-color: rgba(128, 255, 219, 70);"
        "}"
        "QTableWidget#tableCamions::item:hover {"
        "  background-color: rgba(86, 207, 225, 40);"
        "}"
        "QHeaderView::section {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #7400B8, stop:0.5 #5390D9, stop:1 #56CFE1);"
        "  color: #FFFFFF;"
        "  font-weight: 600;"
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
        "QToolButton#btnView, QToolButton#btnEdit, QToolButton#btnDelete, "
        "QToolButton#btnCancel, QToolButton#btnArchive {"
        "  color: #5E60CE;"
        "  background-color: rgba(116, 0, 184, 12);"
        "  border: 1px solid rgba(116, 0, 184, 120);"
        "  border-radius: 8px;"
        "  min-width: 28px;"
        "  max-width: 28px;"
        "  min-height: 28px;"
        "  max-height: 28px;"
        "}"
        "QToolButton#btnView:hover, QToolButton#btnEdit:hover, QToolButton#btnDelete:hover, "
        "QToolButton#btnCancel:hover, QToolButton#btnArchive:hover {"
        "  color: #FFFFFF;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #6930C3, stop:1 #56CFE1);"
        "  border-color: rgba(72, 191, 227, 200);"
        "}"
    );

    setStyleSheet(
        "QWidget#LogistiquePageCrud { color: #1B1B1B; }"
        + baseButton + inputStyle + tableStyle + actionButton
    );

    table->setAlternatingRowColors(true);
    table->viewport()->setAutoFillBackground(true);
    table->setShowGrid(false);
}

void LogistiquePage::loadSampleData()
{
    table->setSortingEnabled(false);

    addRow("CAM-01", "123-TN-4567", "Réfrigéré", 10.5, 5.2, "Zone A", "Disponible", "08:00", "12:00");
    addRow("CAM-02", "890-TN-1234", "Standard", 15.0, 15.0, "Zone B", "En attente", "09:30", "--");
    addRow("CAM-03", "456-TN-7890", "Réfrigéré", 8.0, 0.0, "Zone C", "En panne", "07:15", "--");
    addRow("CAM-04", "111-TN-2222", "Standard", 20.0, 10.0, "Zone A", "Disponible", "10:00", "14:00");
    addRow("CAM-05", "333-TN-4444", "Standard", 12.0, 2.0, "Zone B", "En attente", "11:00", "--");

    table->setSortingEnabled(true);
}

void LogistiquePage::addRow(const QString &idCamion,
                            const QString &immatriculation,
                            const QString &typeCamion,
                            double capacite,
                            double chargeActuelle,
                            const QString &zone,
                            const QString &statut,
                            const QString &heureEntree,
                            const QString &heureSortie)
{
    const int row = table->rowCount();
    table->insertRow(row);
    table->setRowHeight(row, 50);

    auto *idItem = new QTableWidgetItem(idCamion);
    idItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 0, idItem);

    auto *matItem = new QTableWidgetItem(immatriculation);
    matItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 1, matItem);

    auto *typeItem = new QTableWidgetItem(typeCamion);
    typeItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 2, typeItem);

    auto *capItem = new QTableWidgetItem(QString::number(capacite) + " T");
    capItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 3, capItem);

    auto *chargeItem = new QTableWidgetItem(QString::number(chargeActuelle) + " T");
    chargeItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 4, chargeItem);

    auto *zoneItem = new QTableWidgetItem(zone);
    zoneItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 5, zoneItem);

    auto *statusItem = new QTableWidgetItem(statut);
    statusItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 6, statusItem);
    table->setCellWidget(row, 6, createStatusBadge(statut));

    auto *inItem = new QTableWidgetItem(heureEntree);
    inItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 7, inItem);

    auto *outItem = new QTableWidgetItem(heureSortie);
    outItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 8, outItem);

    table->setCellWidget(row, 9, createActionsWidget(idCamion));
}

QWidget *LogistiquePage::createStatusBadge(const QString &statut) const
{
    QString style;
    if (statut.compare(QStringLiteral(u"Disponible"), Qt::CaseInsensitive) == 0) {
        style = "QLabel { color: #1B1B1B; padding: 4px 12px; border-radius: 12px;"
                "background-color: #72EFDD; }"; // Green/Teal
    } else if (statut.compare(QStringLiteral(u"En attente"), Qt::CaseInsensitive) == 0) {
        style = "QLabel { color: #1B1B1B; padding: 4px 12px; border-radius: 12px;"
                "background-color: #56CFE1; }"; // Blue/Cyan
    } else if (statut.compare(QStringLiteral(u"En panne"), Qt::CaseInsensitive) == 0) {
        style = "QLabel { color: white; padding: 4px 12px; border-radius: 12px;"
                "background-color: #e05666; }"; // Red
    } else {
        style = "QLabel { color: white; padding: 4px 12px; border-radius: 12px;"
                "background-color: #5E60CE; }"; // Purple default
    }

    auto *wrapper = new QWidget();
    auto *layout = new QHBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);

    auto *label = new QLabel(statut, wrapper);
    label->setStyleSheet(style);
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(label);
    return wrapper;
}

QWidget *LogistiquePage::createActionsWidget(const QString &idCamion)
{
    auto *wrapper = new QWidget();
    auto *layout = new QHBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->setAlignment(Qt::AlignCenter);

    auto makeButton = [wrapper, &idCamion](const QString &text, const QString &tooltip, const QString &objectName) {
        auto *button = new QToolButton(wrapper);
        button->setText(text);
        button->setToolTip(tooltip);
        button->setObjectName(objectName);
        button->setProperty("idCamion", idCamion);
        return button;
    };

    QToolButton *viewButton = makeButton(QStringLiteral(u"👁"), tr("Consulter"), "btnView");
    QToolButton *editButton = makeButton(QStringLiteral(u"✏️"), tr("Modifier"), "btnEdit");
    QToolButton *deleteButton = makeButton(QStringLiteral(u"🗑️"), tr("Supprimer"), "btnDelete");

    layout->addWidget(viewButton);
    layout->addWidget(editButton);
    layout->addWidget(deleteButton);

    auto bindAction = [this](QToolButton *button, const QString &label) {
        connect(button, &QToolButton::clicked, this, [this, button, label]() {
            const QString idCamion = button->property("idCamion").toString();
            // const int row = findRowById(idCamion); 
            qDebug() << label << "idCamion" << idCamion << "- TODO";
        });
    };

    bindAction(viewButton, "Consulter");
    bindAction(editButton, "Modifier");
    bindAction(deleteButton, "Supprimer");

    return wrapper;
}

int LogistiquePage::findRowById(const QString &idCamion) const
{
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem *item = table->item(row, 0);
        if (item && item->text() == idCamion) {
            return row;
        }
    }
    return -1;
}

void LogistiquePage::updateSort()
{
    int column = 6; // Default Statut
    switch (sortCombo->currentIndex()) {
    case 0:
        column = 6; // Statut
        break;
    case 1:
        column = 2; // Type
        break;
    case 2:
        column = 5; // Zone
        break;
    case 3:
        column = 3; // Capacité
        break;
    default:
        column = 6;
        break;
    }

    const Qt::SortOrder order = (sortOrderCombo->currentIndex() == 0) ? Qt::AscendingOrder : Qt::DescendingOrder;
    table->sortItems(column, order);
    table->horizontalHeader()->setSortIndicator(column, order);
}

void LogistiquePage::applySearchFilter()
{
    const QString filter = searchEdit->text().trimmed();

    int column = 0;
    switch (searchCombo->currentIndex()) {
    case 0:
        column = 1; // Immatriculation
        break;
    case 1:
        column = 6; // Statut
        break;
    case 2:
        column = 2; // Type
        break;
    default:
        column = 1;
        break;
    }

    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem *item = table->item(row, column);
        const QString text = item ? item->text() : QString();
        const bool match = filter.isEmpty() || text.contains(filter, Qt::CaseInsensitive);
        table->setRowHidden(row, !match);
    }
}

void LogistiquePage::onExportPdf()
{
    qDebug() << "Export PDF - TODO";
}

void LogistiquePage::onExportExcel()
{
    qDebug() << "Export Excel - TODO";
}

void LogistiquePage::onExportTxt()
{
    qDebug() << "Export TXT - TODO";
}
