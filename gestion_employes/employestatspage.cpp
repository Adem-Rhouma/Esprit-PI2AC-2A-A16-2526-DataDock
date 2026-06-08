#include "employestatspage.h"
#include "ui_employestatspage.h"
#include "employe.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QHash>
#include <QLocale>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QPainter>
#include <QPdfWriter>
#include <QPen>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSet>
#include <QShowEvent>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QCategoryAxis>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>

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
}

EmployesStatsPage::EmployesStatsPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EmployesStatsPage)
{
    ui->setupUi(this);

    applyStyling();
    setupIcons();
    setupCharts();

    if (ui->btnRefresh) {
        connect(ui->btnRefresh, &QToolButton::clicked, this, &EmployesStatsPage::refreshData);
    }
    if (ui->btnExport) {
        connect(ui->btnExport, &QToolButton::clicked, this, &EmployesStatsPage::exportStatisticsPdf);
    }

    refreshData();
}

EmployesStatsPage::~EmployesStatsPage()
{
    delete ui;
}

void EmployesStatsPage::applyStyling()
{
    setAttribute(Qt::WA_StyledBackground, true);

    const QList<QFrame *> kpiCards = {
        ui->cardKpiTotalEmployes,
        ui->cardKpiTauxPresence,
        ui->cardKpiConges,
        ui->cardKpiMasseSalariale
    };
    for (QFrame *card : kpiCards) {
        if (card) {
            card->setProperty("card", "kpi");
            card->setAttribute(Qt::WA_StyledBackground, true);
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
        }
    }

    if (ui->cardRecentTable) {
        ui->cardRecentTable->setProperty("card", "table");
        ui->cardRecentTable->setAttribute(Qt::WA_StyledBackground, true);
    }

    if (ui->lblTitle) {
        ui->lblTitle->setProperty("role", "pageTitle");
    }
    if (ui->lblSubtitle) {
        ui->lblSubtitle->setProperty("role", "pageSubtitle");
    }

    const QList<QLabel *> kpiValues = {
        ui->lblKpiTotalEmployesValue,
        ui->lblKpiTauxPresenceValue,
        ui->lblKpiCongesValue,
        ui->lblKpiMasseSalarialeValue
    };
    for (QLabel *label : kpiValues) {
        if (label) {
            label->setProperty("role", "kpiValue");
        }
    }

    const QList<QLabel *> kpiBadges = {
        ui->lblKpiTotalEmployesBadge,
        ui->lblKpiTauxPresenceBadge,
        ui->lblKpiCongesBadge,
        ui->lblKpiMasseSalarialeBadge
    };
    for (QLabel *label : kpiBadges) {
        if (label) {
            label->setProperty("role", "kpiBadge");
        }
    }

    const QString style = QStringLiteral(R"(
QLabel {
    color: #1c232b;
}
QLabel[role="pageTitle"] {
    font-size: 26px;
    font-weight: 700;
}
QLabel[role="pageSubtitle"] {
    color: #5f6b7a;
    font-size: 14px;
}
QFrame[card="kpi"],
QFrame[card="chart"],
QFrame[card="table"] {
    background: #ffffff;
    border: 1px solid #e3e8ee;
    border-radius: 14px;
}
QFrame#cardKpiTotalEmployes {
    border: 3px solid #22c55e;
}
QFrame#cardKpiTauxPresence {
    border: 3px solid #3b82f6;
}
QFrame#cardKpiConges {
    border: 3px solid #06b6d4;
}
QFrame#cardKpiMasseSalariale {
    border: 3px solid #8b5cf6;
}
QFrame#cardChartLine {
    border: 3px solid #22c55e;
}
QFrame#cardChartDonut {
    border: 3px solid #3b82f6;
}
QFrame#cardChartBar {
    border: 3px solid #0d9488;
}
QFrame#cardBottomBar {
    border: 3px solid #0ea5e9;
}
QFrame#cardRecentTable {
    border: 3px solid #2563eb;
}
QLabel[role="kpiValue"] {
    font-size: 20px;
    font-weight: 700;
}
QLabel[role="kpiBadge"] {
    background: #e2f6ec;
    color: #1f7a4d;
    border-radius: 10px;
    padding: 2px 8px;
    font-weight: 600;
    max-width: 80px;
}
QToolButton {
    background: #f1f4f8;
    border: 1px solid #e1e6ed;
    border-radius: 8px;
    padding: 4px;
}
QToolButton:hover {
    background: #e6ecf3;
}
QToolButton#btnExport {
    border: 2px solid #2563eb;
    background: #eff6ff;
}
QToolButton#btnExport:hover {
    background: #dbeafe;
}
QTableWidget {
    background: #ffffff;
    border: 1px solid #e3e8ee;
    border-radius: 10px;
    gridline-color: #edf1f6;
}
QHeaderView::section {
    background: #f7f9fc;
    padding: 6px 8px;
    border: none;
    font-weight: 600;
    color: #3b4654;
}
QPushButton#btnVoirPlus {
    background: #1f2c38;
    color: #ffffff;
    border-radius: 10px;
    padding: 6px 14px;
}
QPushButton#btnVoirPlus:hover {
    background: #243342;
}
)" );

    setStyleSheet(style);
}

void EmployesStatsPage::setupIcons()
{
    const QString base = QStringLiteral(":/peche/icons/"); // Reusing Peche icons

    setLabelIcon(ui->lblKpiTotalEmployesIcon, base + "emp.png", 48);
    setLabelIcon(ui->lblKpiTauxPresenceIcon, base + "ic_chart_bar.png", 48);
    setLabelIcon(ui->lblKpiCongesIcon, base + "ic_calendar.png", 48);
    setLabelIcon(ui->lblKpiMasseSalarialeIcon, base + "ic_kpi_total_ventes.png", 48);

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

void EmployesStatsPage::exportStatisticsPdf()
{
    const QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString defaultName = QStringLiteral("statistiques_employes_%1.pdf")
                                    .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    const QString selectedPath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Exporter les statistiques"),
        QDir(defaultDir).filePath(defaultName),
        QStringLiteral("PDF (*.pdf)"));

    if (selectedPath.isEmpty()) {
        return;
    }

    QString pdfPath = selectedPath;
    if (!pdfPath.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) {
        pdfPath.append(QStringLiteral(".pdf"));
    }

    QPdfWriter writer(pdfPath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);
    writer.setTitle(QStringLiteral("Statistiques des employés"));

    QPainter painter(&writer);
    if (!painter.isActive()) {
        QMessageBox::warning(this, QStringLiteral("Export PDF"),
                             QStringLiteral("Impossible de créer le fichier PDF."));
        return;
    }

    Employe e;
    const int total = e.getTotalCount();
    const int admins = e.getCountByRole("ADMIN");
    const int rhs = e.getCountByRole("RH");
    const int agents = e.getCountByRole("AGENT");
    const int hommes = e.getCountBySexe("Homme") + e.getCountBySexe("HOMME");
    const int femmes = e.getCountBySexe("Femme") + e.getCountBySexe("FEMME");
    const int alphaAF = e.getCountByNameRange('A', 'F');
    const int alphaGM = e.getCountByNameRange('G', 'M');
    const int alphaNZ = e.getCountByNameRange('N', 'Z');

    const int knownGenderTotal = qMax(0, hommes + femmes);
    const int alphaTotal = alphaAF + alphaGM + alphaNZ;

    auto pct = [](int count, int totalCount) {
        if (totalCount <= 0) {
            return QStringLiteral("0%");
        }
        return QString::number(qRound((count * 100.0) / totalCount)) + QStringLiteral("%");
    };

    const int pageW = writer.width();
    const int pageH = writer.height();
    const int margin = 170;
    const int contentW = pageW - (2 * margin);
    int y = margin;

    auto setFont = [&painter](int pt, bool bold) {
        QFont f = painter.font();
        f.setPointSize(pt);
        f.setBold(bold);
        painter.setFont(f);
    };

    auto drawCard = [&painter](const QRect &r, const QColor &bg, const QColor &border) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(border, 2));
        painter.setBrush(bg);
        painter.drawRoundedRect(r, 18, 18);
        painter.restore();
    };

    auto drawProgressRow = [&painter, &setFont](int x, int yRow, int width, const QString &label, int value,
                                                int totalValue, const QColor &color) {
        const QString labelText = QStringLiteral("%1: %2 (%3)")
                                      .arg(label)
                                      .arg(value)
                                      .arg(totalValue > 0 ? QString::number(qRound((value * 100.0) / totalValue)) + "%"
                                                          : QStringLiteral("0%"));

        setFont(9, true);
        painter.setPen(QColor("#1F2937"));
        painter.drawText(x, yRow, width, 46, Qt::AlignLeft | Qt::AlignVCenter, labelText);

        const QRect trackRect(x, yRow + 44, width, 18);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#E5E7EB"));
        painter.drawRoundedRect(trackRect, 8, 8);

        if (totalValue > 0 && value > 0) {
            const int fillW = qMax(2, qRound(trackRect.width() * (value / static_cast<double>(totalValue))));
            const QRect fillRect(trackRect.x(), trackRect.y(), fillW, trackRect.height());
            painter.setBrush(color);
            painter.drawRoundedRect(fillRect, 8, 8);
        }
    };

    QRect headerRect(margin, y, contentW, 280);
    drawCard(headerRect, QColor("#EEF6FF"), QColor("#BFDBFE"));

    const int headerLeft = headerRect.x() + 44;
    int headerY = headerRect.y() + 64;

    setFont(19, true);
    painter.setPen(QColor("#0F172A"));
    painter.drawText(headerLeft, headerY, QStringLiteral("Rapport Statistiques Employés"));

    setFont(12, false);
    painter.setPen(QColor("#334155"));
    headerY += 56;
    painter.drawText(headerLeft, headerY,
                     QStringLiteral("Date d'export: %1")
                         .arg(QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy HH:mm"))));
    headerY += 42;
    painter.drawText(headerLeft, headerY, QStringLiteral("Source: table EMPLOYES"));

    const int cardsY = headerRect.bottom() + 34;
    const int gap = 26;
    const int cardW = (contentW - (2 * gap)) / 3;
    const int cardH = 140;

    QRect c1(margin, cardsY, cardW, cardH);
    QRect c2(margin + cardW + gap, cardsY, cardW, cardH);
    QRect c3(margin + 2 * (cardW + gap), cardsY, cardW, cardH);

    drawCard(c1, QColor("#EFF6FF"), QColor("#BFDBFE"));
    drawCard(c2, QColor("#F5F3FF"), QColor("#DDD6FE"));
    drawCard(c3, QColor("#ECFDF5"), QColor("#BBF7D0"));

    setFont(12, true);
    painter.setPen(QColor("#1E3A8A"));
    painter.drawText(c1.x() + 24, c1.y() + 44, QStringLiteral("Total Employés"));
    setFont(18, true);
    painter.drawText(c1.x() + 24, c1.y() + 102, QString::number(total));

    setFont(12, true);
    painter.setPen(QColor("#5B21B6"));
    painter.drawText(c2.x() + 24, c2.y() + 44, QStringLiteral("Répartition Genre"));
    setFont(14, true);
    painter.drawText(c2.x() + 24, c2.y() + 102,
                     QStringLiteral("H: %1 | F: %2").arg(pct(hommes, knownGenderTotal), pct(femmes, knownGenderTotal)));

    setFont(12, true);
    painter.setPen(QColor("#166534"));
    painter.drawText(c3.x() + 24, c3.y() + 44, QStringLiteral("Noms A-M / N-Z"));
    setFont(14, true);
    painter.drawText(c3.x() + 24, c3.y() + 102,
                     QStringLiteral("A-M: %1 | N-Z: %2")
                         .arg(pct(alphaAF + alphaGM, alphaTotal), pct(alphaNZ, alphaTotal)));

    y = cardsY + cardH + 40;
    const int sectionH = 350;
    QRect roleSection(margin, y, contentW, sectionH);
    drawCard(roleSection, QColor("#FFFFFF"), QColor("#E2E8F0"));
    setFont(12, true);
    painter.setPen(QColor("#0F172A"));
    painter.drawText(roleSection.x() + 24, roleSection.y() + 40, QStringLiteral("Répartition par rôle"));

    const int rowX = roleSection.x() + 24;
    const int rowW = roleSection.width() - 48;
    int rowY = roleSection.y() + 86;
    drawProgressRow(rowX, rowY, rowW, QStringLiteral("Admin"), admins, total, QColor("#2563EB"));
    rowY += 102;
    drawProgressRow(rowX, rowY, rowW, QStringLiteral("RH"), rhs, total, QColor("#7C3AED"));
    rowY += 102;
    drawProgressRow(rowX, rowY, rowW, QStringLiteral("Agent"), agents, total, QColor("#16A34A"));

    y = roleSection.bottom() + 30;
    const int halfGap = 28;
    const int halfW = (contentW - halfGap) / 2;
    const int halfH = 300;

    QRect genderSection(margin, y, halfW, halfH);
    QRect alphaSection(margin + halfW + halfGap, y, halfW, halfH);
    drawCard(genderSection, QColor("#FFF7ED"), QColor("#FED7AA"));
    drawCard(alphaSection, QColor("#F0FDF4"), QColor("#BBF7D0"));

    setFont(11, true);
    painter.setPen(QColor("#9A3412"));
    painter.drawText(genderSection.x() + 20, genderSection.y() + 38, QStringLiteral("Répartition par genre"));
    setFont(9, false);
    painter.drawText(genderSection.x() + 20, genderSection.y() + 96,
                     QStringLiteral("Homme: %1 (%2)").arg(hommes).arg(pct(hommes, knownGenderTotal)));
    painter.drawText(genderSection.x() + 20, genderSection.y() + 136,
                     QStringLiteral("Femme: %1 (%2)").arg(femmes).arg(pct(femmes, knownGenderTotal)));

    painter.setPen(QPen(QColor("#F97316"), 6));
    painter.drawLine(genderSection.x() + 20, genderSection.bottom() - 38,
                     genderSection.x() + 20 + qRound((genderSection.width() - 40) *
                                                     (knownGenderTotal > 0 ? hommes / static_cast<double>(knownGenderTotal) : 0.0)),
                     genderSection.bottom() - 38);

    setFont(11, true);
    painter.setPen(QColor("#166534"));
    painter.drawText(alphaSection.x() + 20, alphaSection.y() + 38, QStringLiteral("Répartition alphabétique"));
    setFont(9, false);
    painter.drawText(alphaSection.x() + 20, alphaSection.y() + 96,
                     QStringLiteral("A-F: %1 (%2)").arg(alphaAF).arg(pct(alphaAF, alphaTotal)));
    painter.drawText(alphaSection.x() + 20, alphaSection.y() + 136,
                     QStringLiteral("G-M: %1 (%2)").arg(alphaGM).arg(pct(alphaGM, alphaTotal)));
    painter.drawText(alphaSection.x() + 20, alphaSection.y() + 176,
                     QStringLiteral("N-Z: %1 (%2)").arg(alphaNZ).arg(pct(alphaNZ, alphaTotal)));

    if (y + halfH + 80 < pageH) {
        setFont(8, false);
        painter.setPen(QColor("#64748B"));
        painter.drawText(margin, pageH - 60, contentW, 30, Qt::AlignCenter,
                         QStringLiteral("DataDock - Rapport généré automatiquement"));
    }

    painter.end();

    QMessageBox::information(this, QStringLiteral("Export PDF"),
                             QStringLiteral("Statistiques exportées avec succès.\n%1").arg(pdfPath));
}

void EmployesStatsPage::clearChartContainer(QWidget *container)
{
    if (!container || !container->layout()) {
        return;
    }

    QLayout *layout = container->layout();
    QLayoutItem *item = nullptr;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

void EmployesStatsPage::rebuildRoleLineChart()
{
    if (!ui->chartLineContainer) {
        return;
    }

    clearChartContainer(ui->chartLineContainer);
    ensureChartLayout(ui->chartLineContainer);

    const int monthCount = 6;
    const QDate thisMonth(QDate::currentDate().year(), QDate::currentDate().month(), 1);

    QVector<QDate> months;
    months.reserve(monthCount);
    for (int i = monthCount - 1; i >= 0; --i) {
        months.append(thisMonth.addMonths(-i));
    }

    QMap<QString, int> monthIndexByKey;
    for (int i = 0; i < months.size(); ++i) {
        const QString key = QString::number(months[i].year() * 100 + months[i].month());
        monthIndexByKey.insert(key, i);
    }

    QVector<int> adminMonthly(monthCount, 0);
    QVector<int> rhMonthly(monthCount, 0);
    QVector<int> agentMonthly(monthCount, 0);

    QString dateColumn;
    QSet<QString> columns;
    QSqlQuery columnsQuery("SELECT COLUMN_NAME FROM USER_TAB_COLUMNS WHERE TABLE_NAME = 'EMPLOYES'");
    while (columnsQuery.next()) {
        columns.insert(columnsQuery.value(0).toString().trimmed().toUpper());
    }

    const QStringList dateCandidates = {
        QStringLiteral("DATE_EMBAUCHE"),
        QStringLiteral("DATE_HIRE"),
        QStringLiteral("DATE_AJOUT"),
        QStringLiteral("DATE_CREATION"),
        QStringLiteral("CREATED_AT"),
        QStringLiteral("DATE_INSCRIPTION")
    };
    for (const QString &candidate : dateCandidates) {
        if (columns.contains(candidate)) {
            dateColumn = candidate;
            break;
        }
    }

    if (!dateColumn.isEmpty()) {
        QSqlQuery q;
        const QString sql = QStringLiteral("SELECT ROLE, TRUNC(%1, 'MM') AS MONTH_VALUE FROM EMPLOYES WHERE %1 IS NOT NULL")
                                .arg(dateColumn);
        if (q.exec(sql)) {
            while (q.next()) {
                const QString role = q.value(0).toString().trimmed().toUpper();

                QDate monthDate = q.value(1).toDate();
                if (!monthDate.isValid()) {
                    monthDate = q.value(1).toDateTime().date();
                }
                if (!monthDate.isValid()) {
                    continue;
                }

                const QString key = QString::number(monthDate.year() * 100 + monthDate.month());
                if (!monthIndexByKey.contains(key)) {
                    continue;
                }

                const int idx = monthIndexByKey.value(key);
                if (role == QStringLiteral("ADMIN")) {
                    adminMonthly[idx] += 1;
                } else if (role == QStringLiteral("RH")) {
                    rhMonthly[idx] += 1;
                } else if (role == QStringLiteral("AGENT")) {
                    agentMonthly[idx] += 1;
                }
            }
        }
    } else {
        Employe e;
        const int adminTotal = e.getCountByRole("ADMIN");
        const int rhTotal = e.getCountByRole("RH");
        const int agentTotal = e.getCountByRole("AGENT");

        for (int i = 0; i < monthCount; ++i) {
            adminMonthly[i] = qRound((adminTotal * (i + 1)) / static_cast<double>(monthCount));
            rhMonthly[i] = qRound((rhTotal * (i + 1)) / static_cast<double>(monthCount));
            agentMonthly[i] = qRound((agentTotal * (i + 1)) / static_cast<double>(monthCount));
        }
    }

    for (int i = 1; i < monthCount; ++i) {
        adminMonthly[i] += adminMonthly[i - 1];
        rhMonthly[i] += rhMonthly[i - 1];
        agentMonthly[i] += agentMonthly[i - 1];
    }

    auto *adminSeries = new QSplineSeries();
    auto *rhSeries = new QSplineSeries();
    auto *agentSeries = new QSplineSeries();
    adminSeries->setName(QStringLiteral("Admin"));
    rhSeries->setName(QStringLiteral("RH"));
    agentSeries->setName(QStringLiteral("Agent"));

    QPen adminPen(QColor("#2196F3"));
    adminPen.setWidth(3);
    adminPen.setCapStyle(Qt::RoundCap);
    adminPen.setJoinStyle(Qt::RoundJoin);
    adminSeries->setPen(adminPen);

    QPen rhPen(QColor("#7E57C2"));
    rhPen.setWidth(3);
    rhPen.setCapStyle(Qt::RoundCap);
    rhPen.setJoinStyle(Qt::RoundJoin);
    rhSeries->setPen(rhPen);

    QPen agentPen(QColor("#4CAF50"));
    agentPen.setWidth(3);
    agentPen.setCapStyle(Qt::RoundCap);
    agentPen.setJoinStyle(Qt::RoundJoin);
    agentSeries->setPen(agentPen);

    int yMax = 0;
    for (int i = 0; i < monthCount; ++i) {
        adminSeries->append(i, adminMonthly[i]);
        rhSeries->append(i, rhMonthly[i]);
        agentSeries->append(i, agentMonthly[i]);
        yMax = qMax(yMax, qMax(adminMonthly[i], qMax(rhMonthly[i], agentMonthly[i])));
    }

    auto *lineChart = new QChart();
    lineChart->addSeries(adminSeries);
    lineChart->addSeries(rhSeries);
    lineChart->addSeries(agentSeries);
    lineChart->legend()->setVisible(true);
    lineChart->legend()->setAlignment(Qt::AlignBottom);
    lineChart->setBackgroundVisible(false);
    lineChart->setMargins(QMargins(0, 0, 0, 0));

    auto *lineAxisX = new QCategoryAxis();
    const QLocale fr(QLocale::French, QLocale::France);
    for (int i = 0; i < months.size(); ++i) {
        QString monthLabel = fr.toString(months[i], QStringLiteral("MMM"));
        if (!monthLabel.isEmpty()) {
            monthLabel[0] = monthLabel.at(0).toUpper();
        }
        lineAxisX->append(monthLabel, i);
    }
    lineAxisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);

    auto *lineAxisY = new QValueAxis();
    lineAxisY->setRange(0, qMax(5, yMax + 1));
    lineAxisY->setLabelFormat("%d");

    lineChart->addAxis(lineAxisX, Qt::AlignBottom);
    lineChart->addAxis(lineAxisY, Qt::AlignLeft);
    adminSeries->attachAxis(lineAxisX);
    adminSeries->attachAxis(lineAxisY);
    rhSeries->attachAxis(lineAxisX);
    rhSeries->attachAxis(lineAxisY);
    agentSeries->attachAxis(lineAxisX);
    agentSeries->attachAxis(lineAxisY);

    auto *view = new QChartView(lineChart, ui->chartLineContainer);
    view->setRenderHint(QPainter::Antialiasing);
    ui->chartLineContainer->layout()->addWidget(view);
}

void EmployesStatsPage::setupCharts()
{
    rebuildRoleLineChart();

    // Donut Chart: Roles
    auto *donutSeries = new QPieSeries();
    Employe e_stats;
    donutSeries->append("Agents", e_stats.getCountByRole("AGENT"));
    donutSeries->append("Admin", e_stats.getCountByRole("ADMIN"));
    donutSeries->append("RH", e_stats.getCountByRole("RH"));
    donutSeries->setHoleSize(0.55);

    auto *donutChart = new QChart();
    donutChart->addSeries(donutSeries);
    donutChart->legend()->setAlignment(Qt::AlignBottom);
    donutChart->setBackgroundVisible(false);
    donutChart->setMargins(QMargins(0, 0, 0, 0));

    ensureChartLayout(ui->chartDonutContainer);
    if (ui->chartDonutContainer && ui->chartDonutContainer->layout()) {
        auto *view = new QChartView(donutChart, ui->chartDonutContainer);
        view->setRenderHint(QPainter::Antialiasing);
        ui->chartDonutContainer->layout()->addWidget(view);
    }

    // Bar Chart: Répartition E-mails
    QMap<QString, int> domainCounts;
    QSqlQuery query("SELECT EMAIL FROM EMPLOYES");
    while (query.next()) {
        QString email = query.value(0).toString().trimmed();
        int atPos = email.indexOf('@');
        if (atPos != -1) {
            QString domain = email.mid(atPos).toLower();
            domainCounts[domain]++;
        }
    }

    auto *barSet = new QBarSet("Employés");
    QStringList categories;
    int maxCount = 0;
    
    // Ensure the required domains are shown even if 0
    if (!domainCounts.contains("@gmail.com")) domainCounts["@gmail.com"] = 0;
    if (!domainCounts.contains("@datadock.com")) domainCounts["@datadock.com"] = 0;

    for (auto it = domainCounts.begin(); it != domainCounts.end(); ++it) {
        QString domain = it.key();
        if (domain == "@datadock.com") domain = "@DataDock.com";
        categories << domain;
        *barSet << it.value();
        if (it.value() > maxCount) {
            maxCount = it.value();
        }
    }

    auto *barSeries = new QBarSeries();
    barSeries->append(barSet);
    barSeries->setBarWidth(0.5); // Rend les barres plus fines selon votre préférence

    auto *barChart = new QChart();
    barChart->addSeries(barSeries);
    barChart->legend()->setVisible(false);
    barChart->setBackgroundVisible(false);
    barChart->setMargins(QMargins(0, 0, 0, 0));

    auto *barAxisX = new QBarCategoryAxis();
    barAxisX->append(categories);
    auto *barAxisY = new QValueAxis();
    barAxisY->setRange(0, maxCount > 0 ? maxCount + 2 : 10);
    barAxisY->setLabelFormat("%d");

    barChart->addAxis(barAxisX, Qt::AlignBottom);
    barChart->addAxis(barAxisY, Qt::AlignLeft);
    barSeries->attachAxis(barAxisX);
    barSeries->attachAxis(barAxisY);

    ensureChartLayout(ui->chartBarContainer);
    if (ui->chartBarContainer && ui->chartBarContainer->layout()) {
        auto *view = new QChartView(barChart, ui->chartBarContainer);
        view->setRenderHint(QPainter::Antialiasing);
        ui->chartBarContainer->layout()->addWidget(view);
    }

    // Bottom Chart: Répartition alphabétique des noms
    if (ui->lblBottomBarTitle) {
        ui->lblBottomBarTitle->setText(QStringLiteral("Répartition alphabétique des noms"));
    }

    Employe e_alpha;
    const int countAF = e_alpha.getCountByNameRange('A', 'F');
    const int countGM = e_alpha.getCountByNameRange('G', 'M');
    const int countNZ = e_alpha.getCountByNameRange('N', 'Z');
    const int totalAlpha = countAF + countGM + countNZ;

    auto percentText = [totalAlpha](int count) {
        if (totalAlpha <= 0) {
            return QStringLiteral("0%");
        }
        const int pct = qRound((count * 100.0) / totalAlpha);
        return QString::number(pct) + QStringLiteral("%");
    };

    auto *alphaSeries = new QPieSeries();
    auto *sliceAF = alphaSeries->append(QStringLiteral("A-F %1").arg(percentText(countAF)), countAF);
    auto *sliceGM = alphaSeries->append(QStringLiteral("G-M %1").arg(percentText(countGM)), countGM);
    auto *sliceNZ = alphaSeries->append(QStringLiteral("N-Z %1").arg(percentText(countNZ)), countNZ);
    alphaSeries->setHoleSize(0.62);

    sliceAF->setColor(QColor("#7E57C2"));
    sliceGM->setColor(QColor("#2196F3"));
    sliceNZ->setColor(QColor("#4CAF50"));

    const QList<QPieSlice *> slices = { sliceAF, sliceGM, sliceNZ };
    for (QPieSlice *slice : slices) {
        if (!slice) {
            continue;
        }
        slice->setLabelVisible(true);
        slice->setLabelArmLengthFactor(0.25);
        slice->setBorderColor(QColor("#f5f6f8"));
        slice->setBorderWidth(1.0);
    }

    auto *bottomChart = new QChart();
    bottomChart->addSeries(alphaSeries);
    bottomChart->legend()->setAlignment(Qt::AlignBottom);
    bottomChart->setBackgroundVisible(false);
    bottomChart->setMargins(QMargins(0, 0, 0, 0));

    ensureChartLayout(ui->chartBottomBarContainer);
    if (ui->chartBottomBarContainer && ui->chartBottomBarContainer->layout()) {
        auto *view = new QChartView(bottomChart, ui->chartBottomBarContainer);
        view->setRenderHint(QPainter::Antialiasing);
        ui->chartBottomBarContainer->layout()->addWidget(view);
    }
}

void EmployesStatsPage::setupTable()
{
    if (!ui->chartAlphabeticalContainer) {
        return;
    }

    if (ui->lblRecentTitle) {
        ui->lblRecentTitle->setText(QStringLiteral("Répartition par Genre"));
    }
    if (ui->btnVoirPlus) {
        ui->btnVoirPlus->setVisible(false);
    }

    clearChartContainer(ui->chartAlphabeticalContainer);
    ensureChartLayout(ui->chartAlphabeticalContainer);

    int countHomme = 0;
    int countFemme = 0;
    QSqlQuery q("SELECT UPPER(TRIM(SEXE)) AS SEXE_NORM, COUNT(*) FROM EMPLOYES WHERE SEXE IS NOT NULL GROUP BY UPPER(TRIM(SEXE))");
    while (q.next()) {
        const QString sexe = q.value(0).toString();
        const int count = q.value(1).toInt();

        if (sexe.startsWith(QStringLiteral("H"))) {
            countHomme += count;
        } else if (sexe.startsWith(QStringLiteral("F"))) {
            countFemme += count;
        }
    }

    const int total = countHomme + countFemme;
    auto percentText = [total](int count) {
        if (total <= 0) {
            return QStringLiteral("0%");
        }
        return QString::number(qRound((count * 100.0) / total)) + QStringLiteral("%");
    };

    auto *series = new QPieSeries();
    QPieSlice *sliceHomme = nullptr;
    QPieSlice *sliceFemme = nullptr;

    if (total > 0) {
        sliceHomme = series->append(QStringLiteral("Homme %1").arg(percentText(countHomme)), countHomme);
        sliceFemme = series->append(QStringLiteral("Femme %1").arg(percentText(countFemme)), countFemme);
    } else {
        auto *emptySlice = series->append(QStringLiteral("Aucune donnée"), 1);
        emptySlice->setColor(QColor("#CBD5E1"));
        emptySlice->setLabelVisible(true);
    }

    series->setHoleSize(0.62);

    if (sliceHomme) {
        sliceHomme->setColor(QColor("#2196F3"));
        sliceHomme->setLabelVisible(true);
        sliceHomme->setLabelArmLengthFactor(0.25);
    }
    if (sliceFemme) {
        sliceFemme->setColor(QColor("#EC4899"));
        sliceFemme->setLabelVisible(true);
        sliceFemme->setLabelArmLengthFactor(0.25);
    }

    auto *chart = new QChart();
    chart->addSeries(series);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setBackgroundVisible(false);
    chart->setMargins(QMargins(0, 0, 0, 0));

    auto *view = new QChartView(chart, ui->chartAlphabeticalContainer);
    view->setRenderHint(QPainter::Antialiasing);
    ui->chartAlphabeticalContainer->layout()->addWidget(view);
}

void EmployesStatsPage::updateKpis()
{
    Employe e;
    int total = e.getTotalCount();
    int admins = e.getCountByRole("ADMIN");
    int rh = e.getCountByRole("RH");
    int agents = e.getCountByRole("AGENT");

    auto percentageText = [total](int count) {
        if (total <= 0) {
            return QStringLiteral("0%");
        }
        const int pct = qRound((count * 100.0) / total);
        return QString::number(pct) + QStringLiteral("%");
    };

    auto setBadgeStyle = [](QLabel *label, const QString &bg, const QString &fg) {
        if (!label) {
            return;
        }
        label->setStyleSheet(QStringLiteral("background:%1; color:%2; border-radius:10px; padding:2px 8px; font-weight:600; max-width:80px;")
                                 .arg(bg, fg));
    };

    if (ui->lblKpiTotalEmployesValue) {
        ui->lblKpiTotalEmployesValue->setText(QString::number(total));
    }
    if (ui->lblKpiTotalEmployesBadge) {
        ui->lblKpiTotalEmployesBadge->setText(QStringLiteral("100%"));
        setBadgeStyle(ui->lblKpiTotalEmployesBadge, QStringLiteral("#E2E8F0"), QStringLiteral("#334155"));
    }
    if (ui->lblKpiTauxPresenceValue) {
        ui->lblKpiTauxPresenceValue->setText(QString::number(admins));
    }
    if (ui->lblKpiTauxPresenceBadge) {
        ui->lblKpiTauxPresenceBadge->setText(percentageText(admins));
        setBadgeStyle(ui->lblKpiTauxPresenceBadge, QStringLiteral("#DBEAFE"), QStringLiteral("#1D4ED8"));
    }
    if (ui->lblKpiCongesValue) {
        ui->lblKpiCongesValue->setText(QString::number(rh));
    }
    if (ui->lblKpiCongesBadge) {
        ui->lblKpiCongesBadge->setText(percentageText(rh));
        setBadgeStyle(ui->lblKpiCongesBadge, QStringLiteral("#EDE9FE"), QStringLiteral("#6D28D9"));
    }
    if (ui->lblKpiMasseSalarialeValue) {
        ui->lblKpiMasseSalarialeValue->setText(QString::number(agents));
    }
    if (ui->lblKpiMasseSalarialeBadge) {
        ui->lblKpiMasseSalarialeBadge->setText(percentageText(agents));
        setBadgeStyle(ui->lblKpiMasseSalarialeBadge, QStringLiteral("#DCFCE7"), QStringLiteral("#15803D"));
    }
}

void EmployesStatsPage::refreshData()
{
    rebuildRoleLineChart();
    setupTable();
    updateKpis();
}

void EmployesStatsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshData();
}


