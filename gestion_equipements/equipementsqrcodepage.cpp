#include "equipementsqrcodepage.h"

#include "qrcodegen.hpp"

#include <QComboBox>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>

using qrcodegen::QrCode;

namespace {
constexpr int kQuietZone = 4;
constexpr int kModuleScale = 8;

QImage renderQr(const QrCode &qr)
{
    const int size = qr.getSize();
    const int imageSize = (size + kQuietZone * 2) * kModuleScale;
    QImage image(imageSize, imageSize, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (qr.getModule(x, y)) {
                painter.drawRect((x + kQuietZone) * kModuleScale,
                                 (y + kQuietZone) * kModuleScale,
                                 kModuleScale,
                                 kModuleScale);
            }
        }
    }

    return image;
}

QPixmap qrPixmapForLabel(const QImage &image, QLabel *label)
{
    const QSize targetSize = label ? label->sizeHint().expandedTo(label->minimumSize()) : QSize(320, 320);
    return QPixmap::fromImage(image).scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}
} // namespace

EquipementsQrCodePage::EquipementsQrCodePage(QWidget *parent)
    : QWidget(parent)
    , comboMateriels(nullptr)
    , lblPreview(nullptr)
    , lblInfo(nullptr)
    , btnRefresh(nullptr)
    , btnExport(nullptr)
{
    buildUi();
    applyStyling();
    loadMateriels();

    if (comboMateriels) {
        connect(comboMateriels, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
            updatePreview();
        });
    }
    if (btnRefresh) {
        connect(btnRefresh, &QPushButton::clicked, this, &EquipementsQrCodePage::refreshData);
    }
    if (btnExport) {
        connect(btnExport, &QPushButton::clicked, this, [this]() {
            exportCurrentQr();
        });
    }
}

void EquipementsQrCodePage::refreshData()
{
    loadMateriels();
}

void EquipementsQrCodePage::buildUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
    rootLayout->setSpacing(14);

    auto *header = new QWidget(this);
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    auto *title = new QLabel(tr("QR Code"), header);
    title->setObjectName(QStringLiteral("qrTitle"));
    auto *subtitle = new QLabel(tr("Génération par ID matériel et export PNG"), header);
    subtitle->setObjectName(QStringLiteral("qrSubtitle"));
    headerLayout->addWidget(title);
    headerLayout->addWidget(subtitle);
    rootLayout->addWidget(header);

    auto *controls = new QFrame(this);
    controls->setObjectName(QStringLiteral("controlsCard"));
    auto *controlsLayout = new QHBoxLayout(controls);
    controlsLayout->setContentsMargins(16, 14, 16, 14);
    controlsLayout->setSpacing(10);

    auto *lblSelect = new QLabel(tr("Équipement"), controls);
    comboMateriels = new QComboBox(controls);
    comboMateriels->setMinimumWidth(260);
    btnRefresh = new QPushButton(tr("Rafraîchir"), controls);
    btnExport = new QPushButton(tr("Exporter PNG"), controls);

    controlsLayout->addWidget(lblSelect);
    controlsLayout->addWidget(comboMateriels, 1);
    controlsLayout->addWidget(btnRefresh);
    controlsLayout->addWidget(btnExport);
    rootLayout->addWidget(controls);

    auto *previewCard = new QFrame(this);
    previewCard->setObjectName(QStringLiteral("previewCard"));
    auto *previewLayout = new QVBoxLayout(previewCard);
    previewLayout->setContentsMargins(18, 18, 18, 18);
    previewLayout->setSpacing(12);

    auto *previewTitle = new QLabel(tr("Aperçu du QR Code"), previewCard);
    previewTitle->setObjectName(QStringLiteral("previewTitle"));
    lblPreview = new QLabel(previewCard);
    lblPreview->setObjectName(QStringLiteral("qrPreview"));
    lblPreview->setAlignment(Qt::AlignCenter);
    lblPreview->setMinimumSize(320, 320);
    lblPreview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    lblInfo = new QLabel(previewCard);
    lblInfo->setWordWrap(true);
    lblInfo->setObjectName(QStringLiteral("qrInfo"));
    lblInfo->setAlignment(Qt::AlignCenter);

    previewLayout->addWidget(previewTitle);
    previewLayout->addWidget(lblPreview, 1);
    previewLayout->addWidget(lblInfo);
    rootLayout->addWidget(previewCard, 1);
}

void EquipementsQrCodePage::applyStyling()
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(R"(
QWidget {
    color: #1c232b;
}
QLabel#qrTitle {
    font-size: 26px;
    font-weight: 700;
}
QLabel#qrSubtitle {
    color: #5f6b7a;
    font-size: 14px;
}
QFrame#controlsCard,
QFrame#previewCard {
    background: #ffffff;
    border: 1px solid #e3e8ee;
    border-radius: 14px;
}
QLabel#previewTitle {
    font-size: 16px;
    font-weight: 700;
}
QLabel#qrPreview {
    background: #f7f9fc;
    border: 1px solid #e3e8ee;
    border-radius: 14px;
}
QLabel#qrInfo {
    color: #5f6b7a;
}
QComboBox {
    background: #ffffff;
    border: 1px solid #d7dee8;
    border-radius: 10px;
    padding: 6px 10px;
}
QPushButton {
    background: #1f2c38;
    color: #ffffff;
    border: none;
    border-radius: 10px;
    padding: 7px 16px;
    font-weight: 600;
}
QPushButton:hover {
    background: #243342;
}
)"));
}

void EquipementsQrCodePage::loadMateriels()
{
    if (!comboMateriels) {
        return;
    }

    const int previousId = comboMateriels->currentData().toInt();
    comboMateriels->blockSignals(true);
    comboMateriels->clear();

    const QList<materiel> materiels = materiel::afficherTous();
    for (const materiel &value : materiels) {
        comboMateriels->addItem(
            QStringLiteral("#%1 - %2")
                .arg(value.idMateriel())
                .arg(value.libelle().trimmed().isEmpty() ? tr("Matériel sans libellé") : value.libelle().trimmed()),
            value.idMateriel());
    }

    if (comboMateriels->count() > 0) {
        int index = 0;
        if (previousId > 0) {
            index = comboMateriels->findData(previousId);
            if (index < 0) {
                index = 0;
            }
        }
        comboMateriels->setCurrentIndex(index);
    }

    comboMateriels->blockSignals(false);
    updatePreview();
}

void EquipementsQrCodePage::updatePreview()
{
    if (!lblPreview || !comboMateriels) {
        return;
    }

    const int idMateriel = comboMateriels->currentData().toInt();
    if (idMateriel <= 0) {
        lblPreview->setPixmap(QPixmap());
        if (lblInfo) {
            lblInfo->setText(tr("Aucun équipement disponible."));
        }
        return;
    }

    try {
        const QrCode qr = QrCode::encodeText(QString::number(idMateriel).toUtf8().constData(), QrCode::Ecc::LOW);
        const QImage image = renderQr(qr);
        lblPreview->setPixmap(qrPixmapForLabel(image, lblPreview));
        if (lblInfo) {
            lblInfo->setText(tr("Le QR code encode uniquement l'ID matériel #%1.").arg(idMateriel));
        }
    } catch (const std::exception &) {
        lblPreview->setPixmap(QPixmap());
        if (lblInfo) {
            lblInfo->setText(tr("Impossible de générer le QR code pour cet équipement."));
        }
    }
}

void EquipementsQrCodePage::exportCurrentQr()
{
    if (!comboMateriels || comboMateriels->currentData().toInt() <= 0) {
        QMessageBox::information(this, tr("Export QR"), tr("Aucun équipement sélectionné."));
        return;
    }

    const int idMateriel = comboMateriels->currentData().toInt();
    const QString defaultName = QStringLiteral("qr_materiel_%1.png").arg(idMateriel);
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Exporter le QR Code"),
        defaultName,
        tr("Image PNG (*.png)"));

    if (filePath.isEmpty()) {
        return;
    }

    try {
        const QrCode qr = QrCode::encodeText(QString::number(idMateriel).toUtf8().constData(), QrCode::Ecc::LOW);
        const QImage image = renderQr(qr);
        if (!image.save(filePath, "PNG")) {
            QMessageBox::warning(this, tr("Export QR"), tr("Impossible d'enregistrer l'image."));
            return;
        }
    } catch (const std::exception &) {
        QMessageBox::warning(this, tr("Export QR"), tr("Impossible de générer le QR code."));
        return;
    }

    QMessageBox::information(this, tr("Export QR"), tr("QR code exporté avec succès."));
}
