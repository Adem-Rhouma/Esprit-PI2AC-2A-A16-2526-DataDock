#include "equipementssmsalertspage.h"

#include <QDateTime>
#include <QEventLoop>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
QString normalizeText(const QString &value)
{
    return value.trimmed().toLower();
}
}

EquipementsSmsAlertsPage::EquipementsSmsAlertsPage(QWidget *parent)
    : QWidget(parent)
    , tableAlerts(nullptr)
    , lblCountValue(nullptr)
    , lblSelectedIdValue(nullptr)
    , lblSelectedLabelValue(nullptr)
    , lblSelectedEtatValue(nullptr)
    , inputPhoneNumber(nullptr)
    , messagePreview(nullptr)
    , sentLog(nullptr)
    , btnRefresh(nullptr)
    , btnPrepare(nullptr)
    , btnSend(nullptr)
    , m_selectedRow(-1)
{
    buildUi();
    applyStyling();
    loadFaultyEquipments();
}

void EquipementsSmsAlertsPage::refreshData()
{
    loadFaultyEquipments();
}

void EquipementsSmsAlertsPage::buildUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 20, 20, 20);
    rootLayout->setSpacing(14);

    auto *header = new QWidget(this);
    auto *headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    auto *title = new QLabel(tr("Alertes SMS"), header);
    title->setObjectName(QStringLiteral("smsTitle"));
    auto *subtitle = new QLabel(
        tr("Envoi réel via WhatsApp Sandbox Vonage (curl): alerte au réparateur pour un équipement en panne"),
        header);
    subtitle->setObjectName(QStringLiteral("smsSubtitle"));
    headerLayout->addWidget(title);
    headerLayout->addWidget(subtitle);
    rootLayout->addWidget(header);

    auto *kpiCard = new QFrame(this);
    kpiCard->setObjectName(QStringLiteral("smsKpiCard"));
    auto *kpiLayout = new QHBoxLayout(kpiCard);
    kpiLayout->setContentsMargins(16, 14, 16, 14);
    kpiLayout->setSpacing(12);

    auto *lblCountTitle = new QLabel(tr("Équipements en panne"), kpiCard);
    lblCountTitle->setObjectName(QStringLiteral("smsKpiTitle"));
    lblCountValue = new QLabel(QStringLiteral("0"), kpiCard);
    lblCountValue->setObjectName(QStringLiteral("smsKpiValue"));

    kpiLayout->addWidget(lblCountTitle);
    kpiLayout->addStretch(1);
    kpiLayout->addWidget(lblCountValue);
    rootLayout->addWidget(kpiCard);

    auto *mainRow = new QHBoxLayout();
    mainRow->setSpacing(14);

    auto *tableCard = new QFrame(this);
    tableCard->setObjectName(QStringLiteral("smsTableCard"));
    auto *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(16, 14, 16, 14);
    tableLayout->setSpacing(10);

    auto *tableTitle = new QLabel(tr("Liste des équipements concernés"), tableCard);
    tableTitle->setObjectName(QStringLiteral("smsSectionTitle"));
    btnRefresh = new QPushButton(tr("Rafraîchir"), tableCard);

    auto *tableHeader = new QHBoxLayout();
    tableHeader->addWidget(tableTitle);
    tableHeader->addStretch(1);
    tableHeader->addWidget(btnRefresh);

    tableAlerts = new QTableWidget(tableCard);
    tableAlerts->setColumnCount(5);
    tableAlerts->setHorizontalHeaderLabels({
        tr("ID"), tr("Libellé"), tr("Type"), tr("État"), tr("Action")
    });
    tableAlerts->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableAlerts->setSelectionMode(QAbstractItemView::SingleSelection);
    tableAlerts->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableAlerts->setAlternatingRowColors(true);
    tableAlerts->setSortingEnabled(true);
    tableAlerts->horizontalHeader()->setStretchLastSection(false);
    tableAlerts->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tableAlerts->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    tableAlerts->verticalHeader()->setVisible(false);

    tableLayout->addLayout(tableHeader);
    tableLayout->addWidget(tableAlerts, 1);

    auto *detailsCard = new QFrame(this);
    detailsCard->setObjectName(QStringLiteral("smsDetailsCard"));
    auto *detailsLayout = new QVBoxLayout(detailsCard);
    detailsLayout->setContentsMargins(16, 14, 16, 14);
    detailsLayout->setSpacing(10);

    auto *detailsTitle = new QLabel(tr("Préparation SMS"), detailsCard);
    detailsTitle->setObjectName(QStringLiteral("smsSectionTitle"));
    detailsLayout->addWidget(detailsTitle);

    auto *selectedGrid = new QGridLayout();
    selectedGrid->setHorizontalSpacing(10);
    selectedGrid->setVerticalSpacing(8);

    selectedGrid->addWidget(new QLabel(tr("ID sélectionné"), detailsCard), 0, 0);
    lblSelectedIdValue = new QLabel(QStringLiteral("-"), detailsCard);
    selectedGrid->addWidget(lblSelectedIdValue, 0, 1);

    selectedGrid->addWidget(new QLabel(tr("Libellé"), detailsCard), 1, 0);
    lblSelectedLabelValue = new QLabel(QStringLiteral("-"), detailsCard);
    lblSelectedLabelValue->setWordWrap(true);
    selectedGrid->addWidget(lblSelectedLabelValue, 1, 1);

    selectedGrid->addWidget(new QLabel(tr("État"), detailsCard), 2, 0);
    lblSelectedEtatValue = new QLabel(QStringLiteral("-"), detailsCard);
    selectedGrid->addWidget(lblSelectedEtatValue, 2, 1);

    selectedGrid->addWidget(new QLabel(tr("Téléphone réparateur"), detailsCard), 3, 0);
    inputPhoneNumber = new QLineEdit(detailsCard);
    inputPhoneNumber->setPlaceholderText(tr("Ex: +21698123456"));
    selectedGrid->addWidget(inputPhoneNumber, 3, 1);

    detailsLayout->addLayout(selectedGrid);

    auto *messageLabel = new QLabel(tr("Message SMS"), detailsCard);
    messageLabel->setObjectName(QStringLiteral("smsFieldLabel"));
    detailsLayout->addWidget(messageLabel);

    messagePreview = new QPlainTextEdit(detailsCard);
    messagePreview->setPlaceholderText(tr("Sélectionnez un équipement pour générer le message."));
    detailsLayout->addWidget(messagePreview, 1);

    auto *buttonsLayout = new QHBoxLayout();
    btnPrepare = new QPushButton(tr("Préparer SMS"), detailsCard);
    btnSend = new QPushButton(tr("Envoyer SMS"), detailsCard);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(btnPrepare);
    buttonsLayout->addWidget(btnSend);
    detailsLayout->addLayout(buttonsLayout);

    auto *configHint = new QLabel(
        tr("Config requise (variables d'environnement): VONAGE_API_KEY, VONAGE_API_SECRET, VONAGE_WHATSAPP_FROM"),
        detailsCard);
    configHint->setWordWrap(true);
    configHint->setObjectName(QStringLiteral("smsConfigHint"));
    detailsLayout->addWidget(configHint);

    auto *logLabel = new QLabel(tr("Journal d'envoi"), detailsCard);
    logLabel->setObjectName(QStringLiteral("smsFieldLabel"));
    detailsLayout->addWidget(logLabel);

    sentLog = new QPlainTextEdit(detailsCard);
    sentLog->setReadOnly(true);
    sentLog->setPlaceholderText(tr("Aucun SMS envoyé pour le moment."));
    detailsLayout->addWidget(sentLog, 1);

    mainRow->addWidget(tableCard, 3);
    mainRow->addWidget(detailsCard, 2);
    rootLayout->addLayout(mainRow, 1);

    connect(btnRefresh, &QPushButton::clicked, this, &EquipementsSmsAlertsPage::refreshData);
    connect(btnPrepare, &QPushButton::clicked, this, &EquipementsSmsAlertsPage::prepareSms);
    connect(btnSend, &QPushButton::clicked, this, &EquipementsSmsAlertsPage::sendSms);
    connect(tableAlerts, &QTableWidget::cellClicked, this, &EquipementsSmsAlertsPage::selectFaultyEquipment);
}

void EquipementsSmsAlertsPage::applyStyling()
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(R"(
QWidget {
    color: #1c232b;
}
QLabel#smsTitle {
    font-size: 26px;
    font-weight: 700;
}
QLabel#smsSubtitle {
    color: #5f6b7a;
    font-size: 14px;
}
QLabel#smsConfigHint {
    color: #5f6b7a;
    font-size: 12px;
}
QFrame#smsKpiCard,
QFrame#smsTableCard,
QFrame#smsDetailsCard {
    background: #ffffff;
    border: 1px solid #e3e8ee;
    border-radius: 14px;
}
QLabel#smsKpiTitle,
QLabel#smsSectionTitle,
QLabel#smsFieldLabel {
    font-weight: 700;
}
QLabel#smsKpiValue {
    font-size: 24px;
    font-weight: 800;
    color: #d64545;
}
QTableWidget {
    background: #ffffff;
    border: 1px solid #edf1f6;
    gridline-color: #edf1f6;
}
QHeaderView::section {
    background: #f7f9fc;
    border: none;
    padding: 6px 8px;
    font-weight: 600;
    color: #3b4654;
}
QLineEdit, QPlainTextEdit {
    background: #ffffff;
    border: 1px solid #d7dee8;
    border-radius: 10px;
    padding: 8px 10px;
}
QPlainTextEdit {
    min-height: 100px;
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

bool EquipementsSmsAlertsPage::isFaultyEtat(const QString &etat)
{
    const QString value = normalizeText(etat);
    return value.contains("panne") || value.contains("hs") || value.contains("ko");
}

void EquipementsSmsAlertsPage::loadFaultyEquipments()
{
    m_faultyEquipments.clear();

    const QList<materiel> allEquipments = materiel::afficherTous();
    for (const materiel &value : allEquipments) {
        if (isFaultyEtat(value.etat())) {
            m_faultyEquipments.append(value);
        }
    }

    if (lblCountValue) {
        lblCountValue->setText(QString::number(m_faultyEquipments.size()));
    }

    if (!tableAlerts) {
        return;
    }

    tableAlerts->setSortingEnabled(false);
    tableAlerts->clearContents();
    tableAlerts->setRowCount(m_faultyEquipments.size());
    m_selectedRow = -1;

    for (int row = 0; row < m_faultyEquipments.size(); ++row) {
        const materiel &value = m_faultyEquipments[row];

        auto *idItem = new QTableWidgetItem(QString::number(value.idMateriel()));
        idItem->setTextAlignment(Qt::AlignCenter);
        tableAlerts->setItem(row, 0, idItem);

        auto *libelleItem = new QTableWidgetItem(value.libelle());
        tableAlerts->setItem(row, 1, libelleItem);

        auto *typeItem = new QTableWidgetItem(value.type());
        typeItem->setTextAlignment(Qt::AlignCenter);
        tableAlerts->setItem(row, 2, typeItem);

        auto *etatItem = new QTableWidgetItem(value.etat());
        etatItem->setTextAlignment(Qt::AlignCenter);
        tableAlerts->setItem(row, 3, etatItem);

        auto *actionWidget = new QWidget(tableAlerts);
        auto *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(0, 0, 0, 0);
        actionLayout->setSpacing(6);
        auto *btnSelect = new QPushButton(tr("Sélectionner"), actionWidget);
        actionLayout->addWidget(btnSelect);

        connect(btnSelect, &QPushButton::clicked, this, [this, row]() {
            selectFaultyEquipment(row, 0);
        });

        tableAlerts->setCellWidget(row, 4, actionWidget);
    }

    tableAlerts->resizeColumnsToContents();
    tableAlerts->setSortingEnabled(true);

    if (!m_faultyEquipments.isEmpty()) {
        selectFaultyEquipment(0, 0);
        tableAlerts->selectRow(0);
    } else {
        if (lblSelectedIdValue) lblSelectedIdValue->setText(QStringLiteral("-"));
        if (lblSelectedLabelValue) lblSelectedLabelValue->setText(QStringLiteral("-"));
        if (lblSelectedEtatValue) lblSelectedEtatValue->setText(QStringLiteral("-"));
        if (messagePreview) messagePreview->setPlainText(QString());
    }
}

void EquipementsSmsAlertsPage::selectFaultyEquipment(int row, int)
{
    if (row < 0 || row >= m_faultyEquipments.size()) {
        return;
    }

    m_selectedRow = row;
    const materiel &value = m_faultyEquipments[row];

    if (lblSelectedIdValue) lblSelectedIdValue->setText(QString::number(value.idMateriel()));
    if (lblSelectedLabelValue) lblSelectedLabelValue->setText(value.libelle());
    if (lblSelectedEtatValue) lblSelectedEtatValue->setText(value.etat());

    if (messagePreview) {
        messagePreview->setPlainText(buildSmsMessage(value));
    }
}

QString EquipementsSmsAlertsPage::buildSmsMessage(const materiel &value) const
{
    return QStringLiteral("Alerte maintenance: l'equipement #%1 (%2) est en %3. Merci de prendre en charge la reparation.")
        .arg(value.idMateriel())
        .arg(value.libelle().trimmed().isEmpty() ? tr("Sans libelle") : value.libelle().trimmed())
        .arg(value.etat().trimmed().isEmpty() ? tr("panne") : value.etat().trimmed());
}

QString EquipementsSmsAlertsPage::normalizePhoneNumber(const QString &phone)
{
    QString p = phone.trimmed();
    p.remove(' ');
    p.remove('-');
    p.remove('(');
    p.remove(')');
    if (p.startsWith('+')) {
        p.remove(0, 1);
    }
    if (p.startsWith("00")) {
        p = p.mid(2);
    }
    return p;
}

bool EquipementsSmsAlertsPage::isValidE164Phone(const QString &phone)
{
    static const QRegularExpression re(QStringLiteral("^\\+?[1-9]\\d{6,14}$"));
    return re.match(phone).hasMatch();
}

void EquipementsSmsAlertsPage::prepareSms()
{
    if (m_selectedRow < 0 || m_selectedRow >= m_faultyEquipments.size()) {
        QMessageBox::information(this, tr("SMS"), tr("Sélectionnez un équipement en panne."));
        return;
    }

    if (messagePreview) {
        messagePreview->setPlainText(buildSmsMessage(m_faultyEquipments[m_selectedRow]));
    }

    QMessageBox::information(this, tr("SMS"), tr("Message prêt. Cliquez sur Envoyer SMS pour l'envoi réel via WhatsApp Sandbox Vonage."));
}

void EquipementsSmsAlertsPage::sendSms()
{
    if (m_selectedRow < 0 || m_selectedRow >= m_faultyEquipments.size()) {
        QMessageBox::information(this, tr("SMS"), tr("Sélectionnez un équipement en panne."));
        return;
    }

    const QString toPhone = normalizePhoneNumber(inputPhoneNumber ? inputPhoneNumber->text() : QString());
    if (!isValidE164Phone(toPhone)) {
        QMessageBox::warning(this,
                             tr("SMS"),
                             tr("Numéro réparateur invalide. Format attendu: 216XXXXXXXX (sans + ni 00)."));
        return;
    }

    const QString message = messagePreview ? messagePreview->toPlainText().trimmed()
                                           : buildSmsMessage(m_faultyEquipments[m_selectedRow]);
    if (message.isEmpty()) {
        QMessageBox::warning(this, tr("SMS"), tr("Le message SMS est vide."));
        return;
    }

    const QString apiKey = qEnvironmentVariable("VONAGE_API_KEY").trimmed();
    const QString apiSecret = qEnvironmentVariable("VONAGE_API_SECRET").trimmed();
    const QString fromSender = qEnvironmentVariable("VONAGE_WHATSAPP_FROM", "14157386102").trimmed();

    if (apiKey.isEmpty() || apiSecret.isEmpty() || fromSender.isEmpty()) {
        QMessageBox::warning(this,
                             tr("SMS"),
                             tr("Configuration WhatsApp Vonage manquante. Définissez VONAGE_API_KEY, VONAGE_API_SECRET et VONAGE_WHATSAPP_FROM."));
        return;
    }

    QJsonObject payloadObject;
    payloadObject.insert(QStringLiteral("to"), toPhone);
    payloadObject.insert(QStringLiteral("from"), fromSender);
    payloadObject.insert(QStringLiteral("channel"), QStringLiteral("whatsapp"));
    payloadObject.insert(QStringLiteral("message_type"), QStringLiteral("text"));
    payloadObject.insert(QStringLiteral("text"), message);
    const QString payload = QString::fromUtf8(QJsonDocument(payloadObject).toJson(QJsonDocument::Compact));

    QProcess process;
    QStringList args;
    args << QStringLiteral("-s")
         << QStringLiteral("-i")
         << QStringLiteral("-X")
         << QStringLiteral("POST")
         << QStringLiteral("https://messages-sandbox.nexmo.com/v1/messages")
         << QStringLiteral("--user")
         << (apiKey + QStringLiteral(":") + apiSecret)
         << QStringLiteral("-H")
         << QStringLiteral("Content-Type: application/json")
         << QStringLiteral("-H")
         << QStringLiteral("Accept: application/json")
         << QStringLiteral("-d")
         << payload;

    process.start(QStringLiteral("curl.exe"), args);
    if (!process.waitForStarted(10000)) {
        QMessageBox::warning(this, tr("SMS"), tr("Impossible de démarrer curl.exe."));
        return;
    }
    process.waitForFinished(60000);

    const QString stdOut = QString::fromUtf8(process.readAllStandardOutput());
    const QString stdErr = QString::fromUtf8(process.readAllStandardError());
    const QString fullResponse = stdOut + (stdErr.isEmpty() ? QString() : (QStringLiteral("\n") + stdErr));

    const QRegularExpression httpRe(QStringLiteral("HTTP/\\d\\.\\d\\s+(\\d{3})"));
    const QRegularExpressionMatch httpMatch = httpRe.match(fullResponse);
    const int httpCode = httpMatch.hasMatch() ? httpMatch.captured(1).toInt() : 0;

    const int splitIndex = fullResponse.lastIndexOf(QStringLiteral("\r\n\r\n"));
    const QString responseBody = (splitIndex >= 0) ? fullResponse.mid(splitIndex + 4).trimmed()
                                                   : fullResponse.trimmed();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 || httpCode < 200 || httpCode >= 300) {
        QString details = responseBody.left(500);
        const QJsonDocument errorDoc = QJsonDocument::fromJson(responseBody.toUtf8());
        if (errorDoc.isObject()) {
            const QJsonObject errorObject = errorDoc.object();
            const QString title = errorObject.value(QStringLiteral("title")).toString();
            const QString detail = errorObject.value(QStringLiteral("detail")).toString();
            const QString instance = errorObject.value(QStringLiteral("instance")).toString();
            const QString combined = QStringList({title, detail, instance}).join(QStringLiteral(" | ")).trimmed();
            if (!combined.isEmpty()) {
                details = combined;
            }
        }
        QMessageBox::warning(this,
                             tr("SMS"),
                             tr("Échec d'envoi WhatsApp Vonage (HTTP %1): %2").arg(httpCode).arg(details));
        return;
    }

    if (sentLog) {
        sentLog->appendPlainText(
            QStringLiteral("[%1] WhatsApp envoye a %2 | Equipement #%3 | Vonage/curl HTTP %4 | Body: %5")
                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                .arg(toPhone)
                .arg(m_faultyEquipments[m_selectedRow].idMateriel())
                .arg(httpCode)
                .arg(responseBody.left(300)));
    }

    QMessageBox::information(this,
                             tr("SMS"),
                             tr("Message WhatsApp envoyé avec succès via Vonage Sandbox.\nHTTP: %1\nRéponse curl: %2")
                                 .arg(httpCode)
                                 .arg(responseBody.left(600)));
}
