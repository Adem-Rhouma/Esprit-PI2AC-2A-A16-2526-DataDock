#include "pechestatspage.h"
#include "ui_pechestatspage.h"
#include "pechefontutils.h"

#include <QCoreApplication>
#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHash>
#include <QHBoxLayout>
#include <QLocale>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QTimer>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QPainter>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QValueAxis>

#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEvent>

namespace {
void ensureChartLayout(QWidget *container)
{
    if (!container || container->layout()) {
        return;
    }
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

void applyDropShadow(QWidget *widget)
{
    if (!widget) {
        return;
    }
    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 100));
    shadow->setOffset(0, 8);
    widget->setGraphicsEffect(shadow);
}

QString resolveIconPath(const QString &resourcePath)
{
    if (QFile::exists(resourcePath)) {
        return resourcePath;
    }

    const QString fileName = QFileInfo(resourcePath).fileName();
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + "/../assets/img/" + fileName,
        appDir + "/../../assets/img/" + fileName,
        appDir + "/../../../assets/img/" + fileName,
        appDir + "/../../../../assets/img/" + fileName,
        "assets/img/" + fileName,
        "../assets/img/" + fileName,
        "../../assets/img/" + fileName
    };

    for (const QString &candidate : candidates) {
        const QString cleaned = QDir::cleanPath(candidate);
        if (QFile::exists(cleaned)) {
            return cleaned;
        }
    }

    return resourcePath;
}

void setLabelIcon(QLabel *label, const QString &path, int size)
{
    if (!label) {
        return;
    }
    QPixmap pixmap(resolveIconPath(path));
    if (!pixmap.isNull()) {
        const QBitmap mask = pixmap.createMaskFromColor(Qt::white);
        pixmap.setMask(mask);
    }
    label->setPixmap(pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    label->setFixedSize(size, size);
}

void setToolButtonIcon(QToolButton *button, const QString &path, int size)
{
    if (!button) {
        return;
    }
    QPixmap pixmap(resolveIconPath(path));
    if (!pixmap.isNull()) {
        const QBitmap mask = pixmap.createMaskFromColor(Qt::white);
        pixmap.setMask(mask);
    }
    button->setIcon(QIcon(pixmap));
    button->setIconSize(QSize(size, size));
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

QString resolveDisplayName(const QString &table,
                           const QString &idColumn,
                           int id,
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

QString statusExprSql(const QString &alias)
{
    return QStringLiteral(
        "TRIM(CASE WHEN INSTR(NVL(%1.DESCRIPTION,''),'|') > 0 "
        "THEN SUBSTR(NVL(%1.DESCRIPTION,''), 1, INSTR(NVL(%1.DESCRIPTION,''),'|')-1) "
        "ELSE NVL(%1.DESCRIPTION,'') END)").arg(alias);
}

QString statusFromDescription(const QString &description)
{
    QString status = description;
    const int idx = status.indexOf('|');
    if (idx >= 0) {
        status = status.left(idx);
    }
    status = status.trimmed().toUpper();
    return status.isEmpty() ? QStringLiteral("INCONNU") : status;
}

QString formatNumber(qint64 value)
{
    return QLocale().toString(value);
}

QString formatNumber(double value, int decimals)
{
    return QLocale().toString(value, 'f', decimals);
}

QString formatBadge(double current, double previous)
{
    if (previous <= 0.000001) {
        if (current <= 0.000001) {
            return QStringLiteral("0%");
        }
        return QStringLiteral("N/A");
    }
    const double change = (current - previous) / previous * 100.0;
    const QString sign = (change >= 0.0) ? QStringLiteral("+") : QString();
    return sign + formatNumber(change, 1) + QStringLiteral("%");
}

double queryDouble(const QString &sql, const QMap<QString, QVariant> &binds = {})
{
    QSqlQuery q;
    q.prepare(sql);
    for (auto it = binds.constBegin(); it != binds.constEnd(); ++it) {
        q.bindValue(it.key(), it.value());
    }
    if (!q.exec()) {
        return 0.0;
    }
    if (!q.next()) {
        return 0.0;
    }
    return q.value(0).toDouble();
}
}

PecheStatsPage::PecheStatsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PecheStatsPage)
{
    ui->setupUi(this);
    PecheFontUtils::applyModuleFont(this);

    applyStyling();
    setupIcons();
    setupCharts();
    setupTable();

    if (ui->btnRefresh) {
        connect(ui->btnRefresh, &QToolButton::clicked, this, &PecheStatsPage::refreshAll);
    }
    if (ui->btnFilter) {
        connect(ui->btnFilter, &QToolButton::clicked, this, &PecheStatsPage::onFilterClicked);
    }
    if (ui->btnCalendar) {
        connect(ui->btnCalendar, &QToolButton::clicked, this, &PecheStatsPage::onCalendarClicked);
    }
    if (ui->btnExport) {
        connect(ui->btnExport, &QToolButton::clicked, this, &PecheStatsPage::onExportClicked);
    }
    if (ui->btnVoirPlus) {
        connect(ui->btnVoirPlus, &QPushButton::clicked, this, &PecheStatsPage::showFullHistoryDialog);
    }

    m_lastRefresh = new QElapsedTimer();
    m_lastRefresh->start();

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(30000);
    connect(m_refreshTimer, &QTimer::timeout, this, &PecheStatsPage::refreshAll);
    m_refreshTimer->start();

    QTimer::singleShot(0, this, &PecheStatsPage::refreshAll);
}

PecheStatsPage::~PecheStatsPage()
{
    delete ui;
}

void PecheStatsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    QTimer::singleShot(0, this, &PecheStatsPage::refreshAll);
}

bool PecheStatsPage::eventFilter(QObject *obj, QEvent *event)
{
    QFrame *card = qobject_cast<QFrame *>(obj);
    if (card && card->property("card").toString() == "kpi") {
        if (event->type() == QEvent::Enter) {
            auto *shadow = qobject_cast<QGraphicsDropShadowEffect *>(card->graphicsEffect());
            if (shadow) {
                auto *anim = new QPropertyAnimation(shadow, "blurRadius", this);
                anim->setDuration(250);
                anim->setEndValue(40);
                anim->start(QAbstractAnimation::DeleteWhenStopped);

                auto *animOffset = new QPropertyAnimation(shadow, "offset", this);
                animOffset->setDuration(250);
                animOffset->setEndValue(QPointF(0, 12));
                animOffset->start(QAbstractAnimation::DeleteWhenStopped);
            }
            return true;
        } else if (event->type() == QEvent::Leave) {
            auto *shadow = qobject_cast<QGraphicsDropShadowEffect *>(card->graphicsEffect());
            if (shadow) {
                auto *anim = new QPropertyAnimation(shadow, "blurRadius", this);
                anim->setDuration(250);
                anim->setEndValue(30);
                anim->start(QAbstractAnimation::DeleteWhenStopped);

                auto *animOffset = new QPropertyAnimation(shadow, "offset", this);
                animOffset->setDuration(250);
                animOffset->setEndValue(QPointF(0, 8));
                animOffset->start(QAbstractAnimation::DeleteWhenStopped);
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void PecheStatsPage::applyStyling()
{
    setAttribute(Qt::WA_StyledBackground, true);

    const QList<QFrame *> kpiCards = {
        ui->cardKpiTotalPeche,
        ui->cardKpiTotalAchats,
        ui->cardKpiTotalVentes,
        ui->cardKpiTotalTonnes
    };
    for (QFrame *card : kpiCards) {
        if (card) {
            card->setProperty("card", "kpi");
            card->setAttribute(Qt::WA_StyledBackground, true);
            card->setMaximumHeight(90);
            applyDropShadow(card);
            card->installEventFilter(this);
        }
    }

    if (ui->cardKpiTotalPeche && ui->cardKpiTotalPeche->parentWidget() && ui->cardKpiTotalPeche->parentWidget()->layout()) {
        auto* kpiLayout = qobject_cast<QHBoxLayout*>(ui->cardKpiTotalPeche->parentWidget()->layout());
        if (kpiLayout) {
            kpiLayout->setSpacing(24);
        }
    }

    const QList<QFrame *> chartCards = {
        ui->cardChartLine,
        ui->cardChartDonut,
        ui->cardChartBar,
        ui->cardBottomBar
    };
    for (QFrame *card : chartCards) {
        if (card) {
            card->setProperty("card", "chart");
            card->setAttribute(Qt::WA_StyledBackground, true);
            applyDropShadow(card);
        }
    }

    if (ui->cardRecentTable) {
        ui->cardRecentTable->setProperty("card", "table");
        ui->cardRecentTable->setAttribute(Qt::WA_StyledBackground, true);
        applyDropShadow(ui->cardRecentTable);
    }

    if (ui->lblTitle) {
        ui->lblTitle->setProperty("role", "pageTitle");
    }
    if (ui->lblSubtitle) {
        ui->lblSubtitle->setProperty("role", "pageSubtitle");
    }

    if (ui->lblKpiTotalPecheTitle) ui->lblKpiTotalPecheTitle->setText(tr("LOTS DE PÊCHE"));
    if (ui->lblKpiTotalAchatsTitle) ui->lblKpiTotalAchatsTitle->setText(tr("LOTS EN ATTENTE"));
    if (ui->lblKpiTotalVentesTitle) ui->lblKpiTotalVentesTitle->setText(tr("VALEUR VENTES (TND)"));
    if (ui->lblKpiTotalTonnesTitle) ui->lblKpiTotalTonnesTitle->setText(tr("TONNAGE TOTAL (kg)"));

    const QList<QLabel *> kpiTitles = {
        ui->lblKpiTotalPecheTitle, ui->lblKpiTotalAchatsTitle, ui->lblKpiTotalVentesTitle, ui->lblKpiTotalTonnesTitle
    };
    for (QLabel *label : kpiTitles) {
        if (label) label->setProperty("role", "kpiTitle");
    }

    const QList<QLabel *> kpiValues = {
        ui->lblKpiTotalPecheValue,
        ui->lblKpiTotalAchatsValue,
        ui->lblKpiTotalVentesValue,
        ui->lblKpiTotalTonnesValue
    };
    for (QLabel *label : kpiValues) {
        if (label) {
            label->setProperty("role", "kpiValue");
        }
    }

    const QList<QLabel *> kpiBadges = {
        ui->lblKpiTotalPecheBadge,
        ui->lblKpiTotalAchatsBadge,
        ui->lblKpiTotalVentesBadge,
        ui->lblKpiTotalTonnesBadge
    };
    for (QLabel *label : kpiBadges) {
        if (label) {
            label->setProperty("role", "kpiBadge");
        }
    }

    const QString style = QStringLiteral(R"(
QWidget#PecheStatsPage {
    background-color: transparent;
}
QLabel {
    color: #e2e8f0;
    font-family: "Inter", "Segoe UI", sans-serif;
}
QLabel[role="pageTitle"] {
    font-size: 26px;
    font-weight: 800;
    color: #ffffff;
    letter-spacing: 0.5px;
}
QLabel[role="pageSubtitle"] {
    font-size: 13px;
    color: #38bdf8;
    letter-spacing: 1px;
    text-transform: uppercase;
}
QFrame[card="kpi"] {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgba(22, 32, 50, 200), stop:1 rgba(15, 23, 42, 255));
    border: 1px solid rgba(255, 255, 255, 0.07);
    border-radius: 20px;
}
QFrame[card="chart"],
QFrame[card="table"] {
    background: rgba(15, 23, 42, 180);
    border: 1px solid rgba(255, 255, 255, 0.05);
    border-radius: 24px;
}
QLabel[role="kpiValue"] {
    font-size: 24px;
    font-weight: 800;
    color: #f8fafc;
}
QLabel[role="kpiTitle"] {
    font-size: 10px;
    font-weight: 700;
    color: #64748b;
    letter-spacing: 1.5px;
    text-transform: uppercase;
}
QLabel[role="kpiBadge"] {
    font-size: 12px;
    font-weight: 600;
}
QToolButton {
    background: transparent;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
    padding: 8px;
    color: #cbd5e1;
}
QToolButton:hover {
    background: rgba(255, 255, 255, 0.1);
}
QTableWidget {
    background-color: transparent;
    border: none;
    color: #cbd5e1;
    font-size: 13px;
    outline: none;
}
QTableWidget::item {
    border-bottom: 1px solid rgba(255, 255, 255, 0.05);
    padding: 12px;
}
QHeaderView::section {
    background-color: transparent;
    color: #64748b;
    font-weight: 600;
    font-size: 12px;
    text-transform: uppercase;
    border: none;
    border-bottom: 2px solid rgba(255, 255, 255, 0.05);
    padding: 12px;
}
QPushButton#btnVoirPlus {
    color: #0ea5e9;
    background: transparent;
    border: none;
    font-size: 12px;
    font-weight: 600;
}
QPushButton#btnVoirPlus:hover {
    text-decoration: underline;
}
QFrame#headerSeparator {
    background: transparent;
}
)" );

    setStyleSheet(style);
}

void PecheStatsPage::setupIcons()
{
    const QString base = QStringLiteral(":/peche/icons/");

    setLabelIcon(ui->lblKpiTotalPecheIcon, base + "ic_kpi_total_peche.png", 48);
    setLabelIcon(ui->lblKpiTotalAchatsIcon, base + "ic_kpi_total_achats.png", 48);
    setLabelIcon(ui->lblKpiTotalVentesIcon, base + "ic_kpi_total_ventes.png", 48);
    setLabelIcon(ui->lblKpiTotalTonnesIcon, base + "ic_kpi_total_tonnes.png", 48);

    setLabelIcon(ui->lblChartLineIcon, base + "ic_chart_line.png", 24);
    setLabelIcon(ui->lblChartDonutIcon, base + "ic_chart_donut.png", 24);
    setLabelIcon(ui->lblChartBarIcon, base + "ic_chart_bar.png", 24);
    setLabelIcon(ui->lblBottomBarIcon, base + "ic_chart_bar.png", 24);

    setLabelIcon(ui->lblRecentIcon, base + "ic_table_recent.png", 24);

    setToolButtonIcon(ui->btnFilter, base + "ic_filter.png", 20);
    setToolButtonIcon(ui->btnCalendar, base + "ic_calendar.png", 20);
    setToolButtonIcon(ui->btnRefresh, base + "ic_refresh.png", 20);
    setToolButtonIcon(ui->btnExport, base + "ic_export.png", 20);
}

void PecheStatsPage::setupCharts()
{
    const QStringList months = {"Jan", "Fev", "Mar", "Avr", "Mai", "Jun", "Jul", "Aou", "Sep", "Oct", "Nov", "Dec"};

    m_lineSeries = new QSplineSeries();
    QPen splinePen(QColor("#0ea5e9"));
    splinePen.setWidth(3);
    m_lineSeries->setPen(splinePen);
    m_lineChart = new QChart();
    m_lineChart->addSeries(m_lineSeries);
    m_lineChart->legend()->setVisible(false);
    m_lineChart->setBackgroundVisible(false);
    m_lineChart->setPlotAreaBackgroundVisible(false);
    m_lineChart->setMargins(QMargins(0, 0, 0, 0));
    m_lineChart->setTheme(QChart::ChartThemeDark);

    m_lineAxisX = new QCategoryAxis();
    for (int i = 0; i < months.size(); ++i) {
        m_lineAxisX->append(months[i], i);
    }
    m_lineAxisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

    m_lineAxisY = new QValueAxis();
    m_lineAxisY->setRange(0, 1);

    m_lineChart->addAxis(m_lineAxisX, Qt::AlignBottom);
    m_lineChart->addAxis(m_lineAxisY, Qt::AlignLeft);
    m_lineSeries->attachAxis(m_lineAxisX);
    m_lineSeries->attachAxis(m_lineAxisY);

    ensureChartLayout(ui->chartLineContainer);
    if (ui->chartLineContainer && ui->chartLineContainer->layout()) {
        auto *view = new QChartView(m_lineChart, ui->chartLineContainer);
        view->setRenderHint(QPainter::Antialiasing);
        ui->chartLineContainer->layout()->addWidget(view);
    }

    m_donutSeries = new QPieSeries();
    m_donutSeries->setHoleSize(0.55);

    m_donutChart = new QChart();
    m_donutChart->addSeries(m_donutSeries);
    m_donutChart->legend()->setAlignment(Qt::AlignBottom);
    m_donutChart->setBackgroundVisible(false);
    m_donutChart->setPlotAreaBackgroundVisible(false);
    m_donutChart->setMargins(QMargins(0, 0, 0, 0));
    m_donutChart->setTheme(QChart::ChartThemeDark);

    ensureChartLayout(ui->chartDonutContainer);
    if (ui->chartDonutContainer && ui->chartDonutContainer->layout()) {
        auto *view = new QChartView(m_donutChart, ui->chartDonutContainer);
        view->setRenderHint(QPainter::Antialiasing);
        ui->chartDonutContainer->layout()->addWidget(view);
    }

    m_barSeries = new QBarSeries();
    m_barChart = new QChart();
    m_barChart->addSeries(m_barSeries);
    m_barChart->legend()->setVisible(false);
    m_barChart->setBackgroundVisible(false);
    m_barChart->setPlotAreaBackgroundVisible(false);
    m_barChart->setMargins(QMargins(0, 0, 0, 0));
    m_barChart->setTheme(QChart::ChartThemeDark);

    m_barAxisX = new QBarCategoryAxis();
    m_barAxisY = new QValueAxis();
    m_barAxisY->setRange(0, 1);

    m_barChart->addAxis(m_barAxisX, Qt::AlignBottom);
    m_barChart->addAxis(m_barAxisY, Qt::AlignLeft);
    m_barSeries->attachAxis(m_barAxisX);
    m_barSeries->attachAxis(m_barAxisY);

    ensureChartLayout(ui->chartBarContainer);
    if (ui->chartBarContainer && ui->chartBarContainer->layout()) {
        auto *view = new QChartView(m_barChart, ui->chartBarContainer);
        view->setRenderHint(QPainter::Antialiasing);
        ui->chartBarContainer->layout()->addWidget(view);
    }

    m_bottomSeries = new QBarSeries();
    m_bottomChart = new QChart();
    m_bottomChart->addSeries(m_bottomSeries);
    m_bottomChart->legend()->setAlignment(Qt::AlignBottom);
    m_bottomChart->setBackgroundVisible(false);
    m_bottomChart->setPlotAreaBackgroundVisible(false);
    m_bottomChart->setMargins(QMargins(0, 0, 0, 0));
    m_bottomChart->setTheme(QChart::ChartThemeDark);

    m_bottomAxisX = new QBarCategoryAxis();
    m_bottomAxisY = new QValueAxis();
    m_bottomAxisY->setRange(0, 1);

    m_bottomChart->addAxis(m_bottomAxisX, Qt::AlignBottom);
    m_bottomChart->addAxis(m_bottomAxisY, Qt::AlignLeft);
    m_bottomSeries->attachAxis(m_bottomAxisX);
    m_bottomSeries->attachAxis(m_bottomAxisY);

    ensureChartLayout(ui->chartBottomBarContainer);
    if (ui->chartBottomBarContainer && ui->chartBottomBarContainer->layout()) {
        auto *view = new QChartView(m_bottomChart, ui->chartBottomBarContainer);
        view->setRenderHint(QPainter::Antialiasing);
        ui->chartBottomBarContainer->layout()->addWidget(view);
    }
}

void PecheStatsPage::setupTable()
{
    if (!ui->tableRecent) {
        return;
    }

    ui->tableRecent->setColumnCount(6);
    ui->tableRecent->setHorizontalHeaderLabels({
        tr("N° Lot"), tr("Date"), tr("Origine"), tr("Espèce(s)"), tr("Tonnage (kg)"), tr("Statut")
    });
    ui->tableRecent->verticalHeader()->setVisible(false);
    ui->tableRecent->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableRecent->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableRecent->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableRecent->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableRecent->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableRecent->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableRecent->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->tableRecent->setShowGrid(false);
    ui->tableRecent->setFocusPolicy(Qt::NoFocus);
    ui->tableRecent->verticalHeader()->setDefaultSectionSize(46);
}

void PecheStatsPage::populateRecentTable(QTableWidget *table, int rowLimit)
{
    if (!table) {
        return;
    }

    table->setRowCount(0);
    QSqlQuery q;
    QString sql =
        "SELECT P.IDPECHE, P.DATEPECHE, P.ZONEPECHE, P.DESCRIPTION, P.IDNAVIRE "
        "FROM PECHES P WHERE NVL(P.IS_ARCHIVED, 0) = 0 "
        "ORDER BY P.DATEPECHE DESC, P.IDPECHE DESC";
    if (rowLimit > 0) {
        sql = QStringLiteral("SELECT * FROM (%1) WHERE ROWNUM <= %2").arg(sql).arg(rowLimit);
    }
    q.prepare(sql);
    if (!q.exec()) {
        return;
    }

    int row = 0;
    while (q.next()) {
        const int idPeche = q.value(0).toInt();
        const QDate datePeche = q.value(1).toDate();
        const QString zonePeche = q.value(2).toString();
        const QString description = q.value(3).toString();
        const int idNavire = q.value(4).toInt();

        QString origine = resolveDisplayName("NAVIRES", "IDNAVIRE", idNavire,
                                             { "NOMNAVIRE", "NOM", "LIBELLE", "DESIGNATION" });
        if (origine.isEmpty()) {
            origine = zonePeche;
        }
        if (origine.isEmpty()) {
            origine = tr("Navire %1").arg(idNavire);
        }

        QString especeDisplay = QStringLiteral("-");
        QSqlQuery qCount;
        qCount.prepare("SELECT COUNT(DISTINCT IDESPECE), MIN(IDESPECE) "
                       "FROM PECHESESPECES WHERE IDPECHE = :id");
        qCount.bindValue(":id", idPeche);
        if (qCount.exec() && qCount.next()) {
            const int count = qCount.value(0).toInt();
            const int especeId = qCount.value(1).toInt();
            if (count > 1) {
                especeDisplay = tr("MULTI");
            } else if (count == 1) {
                especeDisplay = resolveDisplayName("ESPECES", "IDESPECE", especeId,
                                                   { "NOMESPECE", "NOM", "LIBELLE", "DESIGNATION" });
                if (especeDisplay.isEmpty()) {
                    especeDisplay = QString::number(especeId);
                }
            }
        }

        double totalQuantite = 0.0;
        QSqlQuery qSum;
        qSum.prepare("SELECT NVL(SUM(QUANTITE), 0) FROM PECHESESPECES WHERE IDPECHE = :id");
        qSum.bindValue(":id", idPeche);
        if (qSum.exec() && qSum.next()) {
            totalQuantite = qSum.value(0).toDouble();
        }

        const QString statut = statusFromDescription(description);

        table->insertRow(row);
        const QStringList values = {
            QString::number(idPeche),
            datePeche.isValid() ? datePeche.toString("dd/MM/yyyy") : QString(),
            origine,
            especeDisplay,
            formatNumber(totalQuantite, 0),
            statut
        };

        for (int col = 0; col < values.size(); ++col) {
            auto *item = new QTableWidgetItem(values[col]);
            if (col == 4) {
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            if (col == 5) {
                item->setTextAlignment(Qt::AlignCenter);
                if (statut == "EN_STOCK") {
                    item->setBackground(QBrush(QColor("#EEEDFE")));
                    item->setForeground(QBrush(QColor("#3C3489")));
                } else if (statut == "VENDU_LOCAL") {
                    item->setBackground(QBrush(QColor("#EAF3DE")));
                    item->setForeground(QBrush(QColor("#3B6D11")));
                } else if (statut == "REJETE") {
                    item->setBackground(QBrush(QColor("#FCEBEB")));
                    item->setForeground(QBrush(QColor("#A32D2D")));
                } else {
                    item->setForeground(QBrush(QColor("#888")));
                }
                QFont f = item->font();
                f.setBold(true);
                f.setPointSize(9);
                item->setFont(f);
            }
            table->setItem(row, col, item);
        }
        ++row;
    }
}

void PecheStatsPage::refreshAll()
{
    if (m_refreshing) {
        return;
    }
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        return;
    }
    if (m_lastRefresh && m_lastRefresh->isValid() && m_lastRefresh->elapsed() < 1500) {
        return;
    }
    m_refreshing = true;
    if (m_lastRefresh) {
        m_lastRefresh->restart();
    }
    refreshKpis();
    refreshRecentTable();

    QTimer::singleShot(0, this, [this]() {
        refreshLineChart();
        QTimer::singleShot(0, this, [this]() {
            refreshDonutChart();
            QTimer::singleShot(0, this, [this]() {
                refreshBarChart();
                QTimer::singleShot(0, this, [this]() {
                    refreshBottomChart();
                    m_refreshing = false;
                });
            });
        });
    });
}

void PecheStatsPage::refreshKpis()
{
    const QString statusExpr = statusExprSql("P");
    const QDate today = QDate::currentDate();
    const QDate monthStart(today.year(), today.month(), 1);
    const QDate periodStart = monthStart.addMonths(-(m_monthsFilter - 1));
    const QDate periodEnd = monthStart.addMonths(1);
    const QDate prevStart = periodStart.addMonths(-m_monthsFilter);
    const QDate prevEnd = periodStart;

    const double totalLots = queryDouble(
        "SELECT COUNT(*) FROM PECHES P WHERE NVL(P.IS_ARCHIVED, 0) = 0");
    const double totalKg = queryDouble(
        "SELECT NVL(SUM(E.QUANTITE), 0) "
        "FROM PECHES P LEFT JOIN PECHESESPECES E ON E.IDPECHE = P.IDPECHE "
        "WHERE NVL(P.IS_ARCHIVED, 0) = 0");
    const double attenteLots = queryDouble(
        QStringLiteral(
            "SELECT COUNT(*) FROM PECHES P "
            "WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND UPPER(%1) IN ('EN_ATTENTE', 'EN_STOCK')")
            .arg(statusExpr));
    const double ventesTnd = queryDouble(
        QStringLiteral(
            "SELECT NVL(SUM(E.QUANTITE * E.PRIXUNITAIRE), 0) "
            "FROM PECHES P LEFT JOIN PECHESESPECES E ON E.IDPECHE = P.IDPECHE "
            "WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND UPPER(%1) = 'VENDU_LOCAL'")
            .arg(statusExpr));
    const double totalTonnes = totalKg / 1000.0;

    if (ui->lblKpiTotalPecheValue) {
        ui->lblKpiTotalPecheValue->setText(formatNumber(static_cast<qint64>(totalLots)));
    }
    if (ui->lblKpiTotalAchatsValue) {
        ui->lblKpiTotalAchatsValue->setText(formatNumber(static_cast<qint64>(attenteLots)));
    }
    if (ui->lblKpiTotalVentesValue) {
        ui->lblKpiTotalVentesValue->setText(formatNumber(ventesTnd, 0));
    }
    if (ui->lblKpiTotalTonnesValue) {
        ui->lblKpiTotalTonnesValue->setText(formatNumber(totalTonnes, 1));
    }

    const double monthLots = queryDouble(
        "SELECT COUNT(*) FROM PECHES P "
        "WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND P.DATEPECHE >= :start AND P.DATEPECHE < :end",
        { { ":start", periodStart }, { ":end", periodEnd } });
    const double prevLots = queryDouble(
        "SELECT COUNT(*) FROM PECHES P "
        "WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND P.DATEPECHE >= :start AND P.DATEPECHE < :end",
        { { ":start", prevStart }, { ":end", prevEnd } });

    const double monthAttente = queryDouble(
        QStringLiteral(
            "SELECT COUNT(*) FROM PECHES P "
            "WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND P.DATEPECHE >= :start AND P.DATEPECHE < :end "
            "AND UPPER(%1) IN ('EN_ATTENTE', 'EN_STOCK')")
            .arg(statusExpr),
        { { ":start", periodStart }, { ":end", periodEnd } });
    const double prevAttente = queryDouble(
        QStringLiteral(
            "SELECT COUNT(*) FROM PECHES P "
            "WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND P.DATEPECHE >= :start AND P.DATEPECHE < :end "
            "AND UPPER(%1) IN ('EN_ATTENTE', 'EN_STOCK')")
            .arg(statusExpr),
        { { ":start", prevStart }, { ":end", prevEnd } });

    const double monthVentes = queryDouble(
        QStringLiteral(
            "SELECT NVL(SUM(E.QUANTITE * E.PRIXUNITAIRE), 0) "
            "FROM PECHES P LEFT JOIN PECHESESPECES E ON E.IDPECHE = P.IDPECHE "
            "WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND P.DATEPECHE >= :start AND P.DATEPECHE < :end "
            "AND UPPER(%1) = 'VENDU_LOCAL'")
            .arg(statusExpr),
        { { ":start", periodStart }, { ":end", periodEnd } });
    const double prevVentes = queryDouble(
        QStringLiteral(
            "SELECT NVL(SUM(E.QUANTITE * E.PRIXUNITAIRE), 0) "
            "FROM PECHES P LEFT JOIN PECHESESPECES E ON E.IDPECHE = P.IDPECHE "
            "WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND P.DATEPECHE >= :start AND P.DATEPECHE < :end "
            "AND UPPER(%1) = 'VENDU_LOCAL'")
            .arg(statusExpr),
        { { ":start", prevStart }, { ":end", prevEnd } });

    const double monthTonnes = queryDouble(
        "SELECT NVL(SUM(E.QUANTITE), 0) "
        "FROM PECHES P LEFT JOIN PECHESESPECES E ON E.IDPECHE = P.IDPECHE "
        "WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND P.DATEPECHE >= :start AND P.DATEPECHE < :end",
        { { ":start", periodStart }, { ":end", periodEnd } }) / 1000.0;
    const double prevTonnes = queryDouble(
        "SELECT NVL(SUM(E.QUANTITE), 0) "
        "FROM PECHES P LEFT JOIN PECHESESPECES E ON E.IDPECHE = P.IDPECHE "
        "WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND P.DATEPECHE >= :start AND P.DATEPECHE < :end",
        { { ":start", prevStart }, { ":end", prevEnd } }) / 1000.0;

    auto styleBadge = [](QLabel* lbl, double current, double previous) {
        if (!lbl) return;
        lbl->setText(formatBadge(current, previous));
        if (current >= previous) {
            lbl->setStyleSheet("color: #10b981; background: rgba(16, 185, 129, 0.15); border-radius: 4px; padding: 2px 4px;");
        } else {
            lbl->setStyleSheet("color: #ef4444; background: rgba(239, 68, 68, 0.15); border-radius: 4px; padding: 2px 4px;");
        }
    };

    styleBadge(ui->lblKpiTotalPecheBadge, monthLots, prevLots);
    if (ui->lblKpiTotalPecheIcon) ui->lblKpiTotalPecheIcon->setStyleSheet("background: rgba(79, 70, 229, 0.2); border-radius: 12px;");

    styleBadge(ui->lblKpiTotalAchatsBadge, monthAttente, prevAttente);
    if (ui->lblKpiTotalAchatsIcon) ui->lblKpiTotalAchatsIcon->setStyleSheet("background: rgba(14, 165, 233, 0.2); border-radius: 12px;");

    styleBadge(ui->lblKpiTotalVentesBadge, monthVentes, prevVentes);
    if (ui->lblKpiTotalVentesIcon) ui->lblKpiTotalVentesIcon->setStyleSheet("background: rgba(16, 185, 129, 0.2); border-radius: 12px;");

    styleBadge(ui->lblKpiTotalTonnesBadge, monthTonnes, prevTonnes);
    if (ui->lblKpiTotalTonnesIcon) ui->lblKpiTotalTonnesIcon->setStyleSheet("background: rgba(245, 158, 11, 0.2); border-radius: 12px;");
}

void PecheStatsPage::refreshLineChart()
{
    if (!m_lineSeries || !m_lineAxisY) {
        return;
    }

    const int year = QDate::currentDate().year();
    QMap<int, double> monthly;
    QSqlQuery q;
    q.prepare(
        "SELECT EXTRACT(MONTH FROM P.DATEPECHE) AS M, NVL(SUM(E.QUANTITE), 0) AS Q "
        "FROM PECHES P LEFT JOIN PECHESESPECES E ON E.IDPECHE = P.IDPECHE "
        "WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND EXTRACT(YEAR FROM P.DATEPECHE) = :year "
        "GROUP BY EXTRACT(MONTH FROM P.DATEPECHE) "
        "ORDER BY M");
    q.bindValue(":year", year);
    if (q.exec()) {
        while (q.next()) {
            monthly.insert(q.value(0).toInt(), q.value(1).toDouble());
        }
    }

    m_lineSeries->clear();
    double maxValue = 0.0;
    for (int month = 1; month <= 12; ++month) {
        const double value = monthly.value(month, 0.0);
        m_lineSeries->append(month - 1, value);
        maxValue = qMax(maxValue, value);
    }
    if (maxValue <= 0.0) {
        maxValue = 1.0;
    }
    m_lineAxisY->setRange(0.0, maxValue * 1.2);
}

void PecheStatsPage::refreshDonutChart()
{
    if (!m_donutSeries) {
        return;
    }

    m_donutSeries->clear();

    const QString statusExpr = statusExprSql("P");
    QSqlQuery q;
    
    // Filter donut by date
    const QDate today = QDate::currentDate();
    const QDate monthStart(today.year(), today.month(), 1);
    const QDate periodStart = monthStart.addMonths(-(m_monthsFilter - 1));
    const QDate periodEnd = monthStart.addMonths(1);

    q.prepare(QStringLiteral(
        "SELECT UPPER(%1) AS STATUT, COUNT(*) AS CNT "
        "FROM PECHES P WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND P.DATEPECHE >= :start AND P.DATEPECHE < :end "
        "GROUP BY UPPER(%1)").arg(statusExpr));
    q.bindValue(":start", periodStart);
    q.bindValue(":end", periodEnd);

    if (q.exec()) {
        while (q.next()) {
            QString status = q.value(0).toString().trimmed().toUpper();
            if (status.isEmpty()) {
                status = QStringLiteral("INCONNU");
            }
            const double count = q.value(1).toDouble();
            if (count > 0.0) {
                auto *slice = new QPieSlice(status + " (" + QString::number(count) + ")", count);
                if (status == "EN_STOCK") slice->setBrush(QColor("#7c6fff"));
                else if (status == "VENDU_LOCAL") slice->setBrush(QColor("#1D9E75"));
                else if (status == "REJETE") slice->setBrush(QColor("#E24B4A"));
                else slice->setBrush(QColor("#aaa"));
                m_donutSeries->append(slice);
            }
        }
    }
}

void PecheStatsPage::refreshBarChart()
{
    if (!m_barSeries || !m_barAxisX || !m_barAxisY) {
        return;
    }

    const QString nameColumn = detectDisplayColumn(
        "ESPECES", { "NOMESPECE", "NOM", "LIBELLE", "DESIGNATION" });
    QString baseSql;
    if (nameColumn.isEmpty()) {
        baseSql = "SELECT IDESPECE, SUM(QUANTITE) AS Q "
                  "FROM PECHESESPECES GROUP BY IDESPECE ORDER BY Q DESC";
    } else {
        baseSql = QStringLiteral(
            "SELECT E.IDESPECE, S.%1, SUM(E.QUANTITE) AS Q "
            "FROM PECHESESPECES E JOIN ESPECES S ON S.IDESPECE = E.IDESPECE "
            "GROUP BY E.IDESPECE, S.%1 ORDER BY Q DESC").arg(nameColumn);
    }

    QSqlQuery q;
    q.prepare(QStringLiteral("SELECT * FROM (%1) WHERE ROWNUM <= 5").arg(baseSql));
    QStringList categories;
    QList<qreal> values;
    if (q.exec()) {
        while (q.next()) {
            const int id = q.value(0).toInt();
            QString label;
            if (nameColumn.isEmpty()) {
                label = QString::number(id);
                values.append(q.value(1).toDouble());
            } else {
                label = q.value(1).toString().trimmed();
                if (label.isEmpty()) {
                    label = QString::number(id);
                }
                values.append(q.value(2).toDouble());
            }
            categories.append(label);
        }
    }

    const auto sets = m_barSeries->barSets();
    for (QBarSet *set : sets) {
        m_barSeries->remove(set);
    }

    auto *barSet = new QBarSet(tr("Quantité (kg)"));
    barSet->setBrush(QBrush(QColor("#7c6fff")));
    for (qreal value : values) {
        barSet->append(value);
    }
    m_barSeries->append(barSet);

    m_barAxisX->clear();
    m_barAxisX->append(categories);

    double maxValue = 0.0;
    for (qreal value : values) {
        maxValue = qMax(maxValue, static_cast<double>(value));
    }
    if (maxValue <= 0.0) {
        maxValue = 1.0;
    }
    m_barAxisY->setRange(0.0, maxValue * 1.2);
}

void PecheStatsPage::refreshBottomChart()
{
    if (!m_bottomSeries || !m_bottomAxisX || !m_bottomAxisY) {
        return;
    }

    const QDate today = QDate::currentDate();
    QDate startMonth(today.year(), today.month(), 1);
    startMonth = startMonth.addMonths(-5);
    const QDate endMonth = QDate(today.year(), today.month(), 1).addMonths(1);

    const QString statusExpr = statusExprSql("P");
    QSqlQuery q;
    q.prepare(QStringLiteral(
        "SELECT EXTRACT(YEAR FROM P.DATEPECHE) AS Y, "
        "EXTRACT(MONTH FROM P.DATEPECHE) AS M, "
        "UPPER(%1) AS STATUT, NVL(SUM(E.QUANTITE), 0) AS Q "
        "FROM PECHES P LEFT JOIN PECHESESPECES E ON E.IDPECHE = P.IDPECHE "
        "WHERE NVL(P.IS_ARCHIVED, 0) = 0 AND P.DATEPECHE >= :start AND P.DATEPECHE < :end "
        "GROUP BY EXTRACT(YEAR FROM P.DATEPECHE), EXTRACT(MONTH FROM P.DATEPECHE), UPPER(%1)")
              .arg(statusExpr));
    q.bindValue(":start", startMonth);
    q.bindValue(":end", endMonth);

    QMap<int, double> stockByMonth;
    QMap<int, double> venduByMonth;
    if (q.exec()) {
        while (q.next()) {
            const int year = q.value(0).toInt();
            const int month = q.value(1).toInt();
            const int key = year * 100 + month;
            QString status = q.value(2).toString().trimmed().toUpper();
            const double value = q.value(3).toDouble();
            if (status == QStringLiteral("EN_STOCK")) {
                stockByMonth[key] += value;
            } else if (status == QStringLiteral("VENDU_LOCAL")) {
                venduByMonth[key] += value;
            }
        }
    }

    QStringList categories;
    QList<qreal> stockValues;
    QList<qreal> venduValues;
    QLocale locale;
    for (int i = 0; i < 6; ++i) {
        const QDate month = startMonth.addMonths(i);
        const int key = month.year() * 100 + month.month();
        const QString label = locale.monthName(month.month(), QLocale::ShortFormat).left(3);
        categories.append(label);
        stockValues.append(stockByMonth.value(key, 0.0));
        venduValues.append(venduByMonth.value(key, 0.0));
    }

    const auto sets = m_bottomSeries->barSets();
    for (QBarSet *set : sets) {
        m_bottomSeries->remove(set);
    }

    auto *venteSet = new QBarSet(tr("Vente locale"));
    venteSet->setBrush(QBrush(QColor("#7c6fff")));
    auto *stockSet = new QBarSet(tr("Stockage"));
    stockSet->setBrush(QBrush(QColor("#1D9E75")));
    for (qreal value : venduValues) {
        venteSet->append(value);
    }
    for (qreal value : stockValues) {
        stockSet->append(value);
    }
    m_bottomSeries->append(venteSet);
    m_bottomSeries->append(stockSet);

    m_bottomAxisX->clear();
    m_bottomAxisX->append(categories);

    double maxValue = 0.0;
    for (qreal value : venduValues) {
        maxValue = qMax(maxValue, static_cast<double>(value));
    }
    for (qreal value : stockValues) {
        maxValue = qMax(maxValue, static_cast<double>(value));
    }
    if (maxValue <= 0.0) {
        maxValue = 1.0;
    }
    m_bottomAxisY->setRange(0.0, maxValue * 1.2);
}

void PecheStatsPage::refreshRecentTable()
{
    if (!ui->tableRecent) {
        return;
    }
    populateRecentTable(ui->tableRecent, 5);
}

void PecheStatsPage::showFullHistoryDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Historique complet des lots"));
    dialog.resize(980, 620);
    dialog.setStyleSheet(QStringLiteral(
        "QDialog { border-image: url(\"C:/Users/moonm/OneDrive/Desktop/2a16-smart-fishing-port-management/assets/img/winbg.png\") 0 0 0 0 stretch stretch; }"
        "QLabel#dialogTitle { font-size:24px; font-weight:700; color:#FFFFFF; }"
        "QLabel#dialogSubtitle { font-size:13px; color:#E0E5FF; }"
        "QPushButton { background:#0FA5A5; color:#FFFFFF; border:none; border-radius:10px; padding:10px 18px; font-weight: 700; }"
        "QPushButton:hover { background:#0D8E8E; }"
        "QPushButton:pressed { background:#096565; }"
        "QTableWidget { background:#FFFFFF; border:1px solid #D8E1EA; border-radius:14px; }"
        "QHeaderView::section { background:#0B1A2E; color:#FFFFFF; padding:10px; border:none; font-weight: 700; }"
        "QTableWidget::item { padding:8px; border-bottom:1px solid #EEF2F6; }"
        "QTableWidget::item:selected { background:#E6F7F7; color:#0B1A2E; }"));

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(14);

    auto *title = new QLabel(tr("Historique complet des lots"), &dialog);
    title->setObjectName(QStringLiteral("dialogTitle"));
    layout->addWidget(title);

    auto *subtitle = new QLabel(
        tr("Affichage integral des debarquements, avec statut, origine, espece et tonnage."), &dialog);
    subtitle->setObjectName(QStringLiteral("dialogSubtitle"));
    subtitle->setWordWrap(true);
    layout->addWidget(subtitle);

    auto *table = new QTableWidget(&dialog);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({
        tr("N° Lot"), tr("Date"), tr("Origine"), tr("Espèce(s)"), tr("Tonnage (kg)"), tr("Statut")
    });
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    table->setShowGrid(false);
    table->setFocusPolicy(Qt::NoFocus);
    table->verticalHeader()->setDefaultSectionSize(46);
    populateRecentTable(table, -1);
    layout->addWidget(table, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    if (auto *closeButton = buttons->button(QDialogButtonBox::Close)) {
        closeButton->setText(tr("Fermer"));
    }
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    dialog.exec();
}

void PecheStatsPage::onFilterClicked()
{
    // Toggle active filter (1 month, 3 months, 6 months)
    QMenu menu(this);
    
    QAction *a1 = menu.addAction(tr("Ce mois"));
    a1->setCheckable(true);
    a1->setChecked(m_monthsFilter == 1);
    
    QAction *a3 = menu.addAction(tr("3 derniers mois"));
    a3->setCheckable(true);
    a3->setChecked(m_monthsFilter == 3);

    QAction *a6 = menu.addAction(tr("6 derniers mois"));
    a6->setCheckable(true);
    a6->setChecked(m_monthsFilter == 6);

    QAction *ay = menu.addAction(tr("Cette année"));
    ay->setCheckable(true);
    ay->setChecked(m_monthsFilter == 12);

    QAction *selected = menu.exec(ui->btnFilter->mapToGlobal(QPoint(0, ui->btnFilter->height())));
    if (selected) {
        if (selected == a1) m_monthsFilter = 1;
        else if (selected == a3) m_monthsFilter = 3;
        else if (selected == a6) m_monthsFilter = 6;
        else if (selected == ay) m_monthsFilter = 12;

        refreshAll();
    }
}

void PecheStatsPage::onCalendarClicked()
{
    // Similar to Filter but presented as a quick jump option
    onFilterClicked();
}

void PecheStatsPage::onExportClicked()
{
    if (!ui->tableRecent || ui->tableRecent->rowCount() == 0) {
        QMessageBox::information(this, tr("Export"), tr("Aucune donnée à exporter."));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Exporter les données"), 
                                                    "export_activite_peche.csv", 
                                                    tr("Fichier CSV (*.csv)"));
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Erreur"), tr("Impossible de créer le fichier d'export."));
        return;
    }

    QTextStream out(&file);
    // Write headers
    QStringList headers;
    for (int i = 0; i < ui->tableRecent->columnCount(); ++i) {
        headers << ui->tableRecent->horizontalHeaderItem(i)->text();
    }
    out << headers.join(";") << "\n";

    // Write data
    for (int r = 0; r < ui->tableRecent->rowCount(); ++r) {
        QStringList rowData;
        for (int c = 0; c < ui->tableRecent->columnCount(); ++c) {
            rowData << ui->tableRecent->item(r, c)->text();
        }
        out << rowData.join(";") << "\n";
    }

    file.close();
    QMessageBox::information(this, tr("Succès"), tr("L'exportation a réussi."));
}

void PecheStatsPage::onVoirPlusClicked()
{
    QMessageBox::information(this, tr("Activité Récente"), 
                             tr("Affichage complet de l'historique non disponible dans la vue Dashboard.\nVeuillez consulter le journal principal pour voir tous les lots."));
}









