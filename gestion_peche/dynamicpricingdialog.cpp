#include "dynamicpricingdialog.h"
#include "ui_dynamicpricingdialog.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QPixmap>
#include <QTableWidgetItem>

namespace {
QString resolveAssetPath(const QString &relativePath)
{
    if (QFile::exists(relativePath)) {
        return relativePath;
    }

    const QString fileName = QFileInfo(relativePath).fileName();
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + "/../gestion_peche/assets/icons/" + fileName,
        appDir + "/../../gestion_peche/assets/icons/" + fileName,
        appDir + "/../../../gestion_peche/assets/icons/" + fileName,
        appDir + "/../../../../gestion_peche/assets/icons/" + fileName,
        "gestion_peche/assets/icons/" + fileName,
        "../gestion_peche/assets/icons/" + fileName,
        "../../gestion_peche/assets/icons/" + fileName
    };

    for (const QString &candidate : candidates) {
        const QString cleaned = QDir::cleanPath(candidate);
        if (QFile::exists(cleaned)) {
            return cleaned;
        }
    }

    return relativePath;
}

void setLabelIcon(QLabel *label, const QString &path, int size)
{
    if (!label) {
        return;
    }

    QPixmap pixmap(resolveAssetPath(path));
    if (pixmap.isNull()) {
        label->setText(QStringLiteral("AI"));
        return;
    }

    label->setText(QString());
    label->setPixmap(pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
}

DynamicPricingDialog::DynamicPricingDialog(QWidget *parent)
    : PecheBaseDialog(parent)
    , ui(new Ui::DynamicPricingDialog)
{
    ui->setupUi(this);

    applyStyling();
    setupHeader();
    setupTable();

    if (ui->btnClose) {
        connect(ui->btnClose, &QPushButton::clicked, this, &QDialog::accept);
    }
}

DynamicPricingDialog::~DynamicPricingDialog()
{
    delete ui;
}

void DynamicPricingDialog::applyStyling()
{
    setAttribute(Qt::WA_StyledBackground, true);
}

void DynamicPricingDialog::setupTable()
{
    if (!ui->tableDynamicPricing) {
        return;
    }

    ui->tableDynamicPricing->setRowCount(3);
    ui->tableDynamicPricing->setColumnCount(5);
    ui->tableDynamicPricing->setHorizontalHeaderLabels({
        "TypeProduit", "Prix réel", "Prix recommandé IA", "Écart", "Avis"
    });

    const QList<QStringList> rows = {
        { "Sardine", "6.0 TND/kg", "6.5 - 7.2 TND/kg", "+0.8", "Hausse prudente" },
        { "Calamar", "18.5 TND/kg", "19.0 - 19.8 TND/kg", "+0.9", "Aligner marché" },
        { "Dorade", "24.0 TND/kg", "22.5 - 23.5 TND/kg", "-1.0", "Stabiliser" }
    };

    for (int row = 0; row < rows.size(); ++row) {
        const QStringList &values = rows[row];
        for (int col = 0; col < values.size(); ++col) {
            auto *item = new QTableWidgetItem(values[col]);
            ui->tableDynamicPricing->setItem(row, col, item);
        }
    }

    ui->tableDynamicPricing->horizontalHeader()->setStretchLastSection(true);
    ui->tableDynamicPricing->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableDynamicPricing->verticalHeader()->setVisible(false);
    ui->tableDynamicPricing->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableDynamicPricing->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void DynamicPricingDialog::setupHeader()
{
    setLabelIcon(ui->lblAiIcon, QStringLiteral(":/peche/icons/ic_smartservices_dynamic_pricing.png"), 36);
}


