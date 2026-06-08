#include "mainwindow.h"
#include "ui_mainwindow.h"
#include"gestion_peche/pechecruddialog.h"
#include <cctype>
#include "gestion_chambres_froides/chambresfroidesmodulewidget.h"
#include "gestion_equipements/equipementsmodulewidget.h"
#include "gestion_employes/employesmodulewidget.h"
#include "gestion_logistique/logistiquemodulewidget.h"
#include "gestion_logistique/views/camionactivitypage.h"
#include "gestion_navires/naviresmodulewidget.h"
#include "gestion_peche/pechemodulewidget.h"
#include "gestion_employes/loginpage.h"
#include <QRegularExpression>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QDebug>
#include <QEvent>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QMovie>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QResizeEvent>
#include <QColor>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QStackedWidget>
#include <QStyle>
#include <QDateTime>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>
#include"gestion_employes/employe.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->layoutRacine->setStretch(0, 0);
    ui->layoutRacine->setStretch(1, 1);
    ui->layoutCorps->setStretch(0, 0);
    ui->layoutCorps->setStretch(1, 1);

    // Paint bg.png via paintEvent (see below). Make every widget layer
    // between MainWindow and the page content transparent so the image
    // shows through.  WA_NoSystemBackground + setAutoFillBackground(false)
    // is the correct pair for non-top-level widgets.
    auto makeTransparent = [](QWidget *w) {
        if (!w) return;
        w->setAttribute(Qt::WA_NoSystemBackground, true);
        w->setAutoFillBackground(false);
    };
    makeTransparent(ui->centralwidget);

    if (ui->scrollAreaZoneContenu) {
        makeTransparent(ui->scrollAreaZoneContenu);
        makeTransparent(ui->scrollAreaZoneContenu->viewport());
        makeTransparent(ui->scrollAreaZoneContenu->widget());
    }
    if (ui->scrollAreaSidebar) {
        ui->scrollAreaSidebar->setFrameShape(QFrame::NoFrame);
        makeTransparent(ui->scrollAreaSidebar->viewport());
    }

    statusBar()->hide();

    setupDashboardVisuals();
    createPages();
    setupNavigation();

    if (ui->lblSidebarLogo) {
        ui->lblSidebarLogo->setCursor(Qt::PointingHandCursor);
        ui->lblSidebarLogo->installEventFilter(this);
    }
    int ret=A.connect_arduino(); // lancer la connexion à arduino
    switch(ret){
    case(0):qDebug()<< "arduino is available and connected to : "<< A.getarduino_port_name();
        break;
    case(1):qDebug() << "arduino is available but not connected to :" <<A.getarduino_port_name();
        break;
    case(-1):qDebug() << "arduino is not available";
    }
    QObject::connect(A.getserial(),SIGNAL(readyRead()),this,SLOT(update_label())); // permet de lancer
    //le slot update_label suite à la reception du signal readyRead (reception des données).

    safeDockMonitorTimer = new QTimer(this);
    safeDockMonitorTimer->setInterval(60 * 1000);
    connect(safeDockMonitorTimer, &QTimer::timeout, this, &MainWindow::monitorActiveDepartures);
    safeDockMonitorTimer->start();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
    QPainter painter(this);
    
    bool isDashboard = false;
    if (ui && ui->stackedPages && ui->pageDashboard) {
        isDashboard = (ui->stackedPages->currentWidget() == ui->pageDashboard);
    }

    if (isDashboard && dashBgMovie && dashBgMovie->isValid()) {
        QPixmap currentFrame = dashBgMovie->currentPixmap();
        if (!currentFrame.isNull()) {
            // Draw the default background first so sidebar/topbar areas are covered
            static const QPixmap bg(QStringLiteral(":/assets/bg.png"));
            if (!bg.isNull()) {
                painter.drawPixmap(rect(), bg);
            }
            
            // Expand to cover the entire right pane including margins
            QRect targetRect(ui->scrollAreaZoneContenu->mapTo(this, QPoint(0, 0)), ui->scrollAreaZoneContenu->size());
            targetRect.adjust(-40, -30, 40, 30);
            
            // Use KeepAspectRatioByExpanding to fill the entire space WITH cropping (rognage) and NO stretching
            QPixmap scaledBg = currentFrame.scaled(targetRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            
            // Center the pixmap within the targetRect to crop equally from all sides
            int xOffset = targetRect.x() + (targetRect.width() - scaledBg.width()) / 2;
            int yOffset = targetRect.y() + (targetRect.height() - scaledBg.height()) / 2;
            
            painter.setClipRect(targetRect);
            painter.drawPixmap(xOffset, yOffset, scaledBg);
            painter.setClipping(false);
        }
    } else {
        static const QPixmap bg(QStringLiteral(":/assets/bg.png"));
        if (!bg.isNull()) {
            painter.drawPixmap(rect(), bg);
        }
    }
}
void MainWindow::update_label()
{
    data = A.read_from_arduino();
    logArduinoState(QStringLiteral("received data"));
    // ── RFID TRUCK SCENARIO ──────────────────────────────────────────────────
    // Arduino sends the full tag on one line (e.g. "A1B23C4D\n").
    // We detect this by checking for an 8-char hex string (after trimming).
    static QByteArray rfidBuffer;
    rfidBuffer.append(data);

    if (rfidBuffer.contains('\n') || rfidBuffer.contains('\r')) {
        QString candidate = QString::fromLatin1(rfidBuffer).trimmed().toUpper();
        rfidBuffer.clear();

        static QRegularExpression hexTag("^[0-9A-F]{8}$");
        if (!candidate.isEmpty() && hexTag.match(candidate).hasMatch()) {
            // It's an RFID tag for the truck barrier
            CamionActivityPage *activityPage = LogistiqueModule
                ? LogistiqueModule->findChild<CamionActivityPage*>()
                : nullptr;

            auto log = [&](const QString &m) {
                qDebug() << "[RFID]" << m;
                if (activityPage) activityPage->logMessage(m);
            };

            log("Tag reçu: " + candidate);

            QSqlQuery q;
            q.prepare("SELECT Immatriculation FROM Camions WHERE UPPER(rfid) = :rfid");
            q.bindValue(":rfid", candidate);

            if (q.exec() && q.next()) {
                QString immat = q.value(0).toString();
                log("ACCEPTÉ – Camion: " + immat);
                A.write_to_arduino("a\n");   // access granted
                log("Envoyé 'a' (access) à l'Arduino");

                QSqlQuery getId;
int nextId = 1;

if (getId.exec("SELECT NVL(MAX(id_activity), 0) + 1 FROM camion_activity") && getId.next()) {
    nextId = getId.value(0).toInt();
} else {
    log("Erreur génération ID: " + getId.lastError().text());
}

QSqlQuery ins;
ins.prepare("INSERT INTO camion_activity "
            "(id_activity, matricule, type_mouvement, date_heure) "
            "VALUES (:id, :m, 'ENTREE', SYSTIMESTAMP)");

ins.bindValue(":id", nextId);
ins.bindValue(":m", immat);

if (ins.exec()) {
    log("Activité 'ENTREE' enregistrée pour " + immat);
    if (activityPage) activityPage->loadFromDB();
} else {
    log("Erreur BD: " + ins.lastError().text());
}
            } else {
                log("REFUSÉ – Tag inconnu: " + candidate);
                A.write_to_arduino("d\n");   // access denied
                log("Envoyé 'd' (denied) à l'Arduino");
            }
            return; // do NOT fall through to employee logic
        }
        // Not a full RFID tag — put it back for the character-by-character employee logic
        // (the employee scenario reads byte-by-byte so we don't buffer there)
        data = rfidBuffer;
        rfidBuffer.clear();
    }
    // ── END RFID TRUCK SCENARIO ──────────────────────────────────────────────

    if (data == "e")
    {
        if (etat2== 0) etat2 = 1;
        else etat2 = 0;
        etat = 0;
        idEmployer = "";
        zone = "";
        heure = "";
        logArduinoState(QStringLiteral("mode toggle"));
    }

    else if(data != "#")
    {
        if(etat == 0)
        {
            idEmployer = idEmployer + data;
        }
        else if(etat==1)
        {
            zone = zone + data ;
        }
        else if(etat==2)
        {
            heure = heure+ data ;
        }

    }
    else
    {
        etat ++;
        if(etat == 1)
        {

            Employe e;
            if (idEmployer.contains(QRegularExpression("^[0-9]+$")))
            {
                e.set_id(idEmployer);
                if(e.employeExisteA())
                {
                    if (etat2 == 1)
                    {
                        if(e.enregistrerHeureRetour(idEmployer))
                        {
                            sendArduinoCode("5");
                            resetArduinoInputState();
                        }
                        else
                        {
                            sendArduinoCode("4");
                            resetArduinoInputState();
                        }
                    }
                    else
                    {
                        sendArduinoCode("2");
                    }
                }
                else
                {
                    sendArduinoCode("1");
                    resetArduinoInputState();
                }
            }
            else
            {
                sendArduinoCode("1");
                resetArduinoInputState();
            }
        }
        else if (etat == 2)
        {
            if(zoneexiste(zone))
            {
                sendArduinoCode("2");
            }
            else
            {
                sendArduinoCode("1");
                etat = 1;
                zone = "";
                heure = "";
            }
        }
        else
        {
            if (heure.contains(QRegularExpression("^[0-9]+$")))
            {
                Employe e;
                if (e.enregitrement(idEmployer , zone , heure.toInt()))
                {
                    sendArduinoCode("3");
                    resetArduinoInputState();

                }
                else
                {
                    sendArduinoCode("4");
                    resetArduinoInputState();
                }
            }
            else
            {
                sendArduinoCode("1");
                etat = 2;
                heure ="";
            }
        }
    }
    logArduinoState(QStringLiteral("after processing"));
}

void MainWindow::resetArduinoInputState()
{
    etat = 0;
    etat2 = 0;
    idEmployer = "";
    zone = "";
    heure = "";
}

void MainWindow::sendArduinoCode(const QByteArray &code)
{
    A.write_to_arduino(code);
    qDebug() << "[SafeDock] sent Arduino code:" << code;
}

void MainWindow::logArduinoState(const QString &step) const
{
    const QString inputStep = etat == 0 ? QStringLiteral("employee_id")
                            : etat == 1 ? QStringLiteral("zone")
                                        : QStringLiteral("estimated_time");
    qDebug() << "[SafeDock]" << step
             << "data:" << data
             << "etat:" << etat
             << "step:" << inputStep
             << "id:" << idEmployer
             << "zone:" << zone
             << "heure:" << heure
             << "mode:" << (etat2 == 1 ? "return" : "departure");
}

int MainWindow::parseEstimatedTimeToMinutes(const QString &value) const
{
    const QString trimmed = value.trimmed();
    if (!trimmed.contains(QRegularExpression(QStringLiteral("^[0-9]+$")))) {
        return -1;
    }

    if (trimmed.length() <= 2) {
        const int hour = trimmed.toInt();
        return (hour >= 0 && hour <= 23) ? hour * 60 : -1;
    }

    if (trimmed.length() == 3 || trimmed.length() == 4) {
        const QString normalized = trimmed.rightJustified(4, QLatin1Char('0'));
        const int hour = normalized.left(2).toInt();
        const int minute = normalized.right(2).toInt();
        if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59) {
            return hour * 60 + minute;
        }
    }

    return -1;
}

int MainWindow::alertLevelForDelay(int estimatedMinutes, const QTime &now) const
{
    if (estimatedMinutes < 0) {
        return 0;
    }

    const int nowMinutes = now.hour() * 60 + now.minute();
    int delayMinutes = nowMinutes - estimatedMinutes;
    if (delayMinutes < -12 * 60) {
        delayMinutes += 24 * 60;
    }

    if (delayMinutes <= 0) {
        return 0;
    }
    if (delayMinutes < 60) {
        return 1;
    }
    return 2;
}

void MainWindow::monitorActiveDepartures()
{
    // TODO: fetch weather every 5 minutes.
    // TODO: combine wind / storm / visibility with zone danger.
    // TODO: adjust delay threshold with fisherman history and zone danger.
    QSqlQuery query;
    query.prepare("SELECT IDEMPLOYE, HEURE FROM EMPLOYES "
                  "WHERE HEURE IS NOT NULL AND HEURE_R IS NULL");

    if (!query.exec()) {
        qDebug() << "[SafeDock] active departure monitor query failed:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        const QString employeeId = query.value("IDEMPLOYE").toString();
        const QString estimatedValue = query.value("HEURE").toString();
        const int estimatedMinutes = parseEstimatedTimeToMinutes(estimatedValue);
        const int alertLevel = alertLevelForDelay(estimatedMinutes, QTime::currentTime());

        qDebug() << "[SafeDock] monitor"
                 << "id:" << employeeId
                 << "estimated:" << estimatedValue
                 << "alertLevel:" << alertLevel;

        if (alertLevel == 2 && !criticalDelayAlertsSent.contains(employeeId)) {
            sendArduinoCode("6");
            criticalDelayAlertsSent.insert(employeeId);
        }
        else if (alertLevel < 2) {
            criticalDelayAlertsSent.remove(employeeId);
        }
    }
}

namespace {
QPixmap tintPixmap(const QPixmap &source, const QColor &color)
{
    if (source.isNull()) {
        return source;
    }
    QPixmap tinted(source.size());
    tinted.fill(Qt::transparent);
    QPainter painter(&tinted);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawPixmap(0, 0, source);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(tinted.rect(), color);
    painter.end();
    return tinted;
}
} // namespace

static QWidget *createEmptyPage(const QString &objectName, QWidget *parent)
{
    auto *page = new QWidget(parent);
    page->setObjectName(objectName);

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addStretch(1);

    return page;
}

void MainWindow::createPages()
{
    if (!ui->stackedPages) {
        qWarning() << "stackedPages not found in UI.";
        return;
    }

    if (ui->stackedPages->count() == 0) {
        qWarning() << "Dashboard page missing from UI.";
    }

    if (!loginPage) {
        loginPage = new LoginPage(ui->stackedPages);
        loginPage->setObjectName(QStringLiteral("pageLogin"));
        loginIndex = ui->stackedPages->insertWidget(0, loginPage);
        connect(loginPage, &LoginPage::loginRequested, this, &MainWindow::showDashboard);
    } else {
        loginIndex = ui->stackedPages->indexOf(loginPage);
    }

    dashboardIndex = ui->stackedPages->indexOf(ui->pageDashboard);

    if (employesIndex < 0) {
        employesIndex = ui->stackedPages->addWidget(new EmployesModuleWidget(ui->stackedPages));
    }
    if (naviresIndex < 0) {
        naviresIndex = ui->stackedPages->addWidget(new NaviresModuleWidget(ui->stackedPages));
    }
    if (chambresIndex < 0) {
        chambresIndex = ui->stackedPages->addWidget(new ChambresFroidesModuleWidget(ui->stackedPages));
    }
    if (!LogistiqueModule) {
        LogistiqueModule = new LogistiqueModuleWidget(ui->stackedPages);
        logistiqueIndex = ui->stackedPages->addWidget(LogistiqueModule);
    } else {
        logistiqueIndex = ui->stackedPages->indexOf(LogistiqueModule);
    }
    if (equipementsIndex < 0) {
        equipementsIndex = ui->stackedPages->addWidget(new EquipementsModuleWidget(ui->stackedPages));
    }
    if (!PecheModule) {
        PecheModule = new PecheModuleWidget(ui->stackedPages);
        PecheIndex = ui->stackedPages->addWidget(PecheModule);
    } else {
        PecheIndex = ui->stackedPages->indexOf(PecheModule);
    }

    const int expectedCount = 8;
    if (ui->stackedPages->count() < expectedCount) {
        qWarning() << "stackedPages has unexpected page count:" << ui->stackedPages->count();
        while (ui->stackedPages->count() < expectedCount) {
            ui->stackedPages->addWidget(createEmptyPage(QStringLiteral("pageMissing"), ui->stackedPages));
        }
    }
}

void MainWindow::setupNavigation()
{
    if (!ui->stackedPages) {
        qWarning() << "stackedPages not found in UI.";
        return;
    }

    sidebarGroup = new QButtonGroup(this);
    sidebarGroup->setExclusive(true);

    struct SidebarPage {
        QPushButton *button;
        int index;
        QString title;
    };

    const SidebarPage pages[] = {
        { ui->btnSidebarEmployes, employesIndex, QStringLiteral("G. Employés") },
        { ui->btnSidebarNavires, naviresIndex, QStringLiteral("G. Navires") },
        { ui->btnSidebarChambres, chambresIndex, QStringLiteral("G. Chambres Froides") },
        { ui->btnSidebarEquipements, equipementsIndex, QStringLiteral("G. Equipements") },
    };

    for (const SidebarPage &page : pages) {
        if (!page.button) {
            qWarning() << "Missing sidebar button for index:" << page.index;
            continue;
        }
        if (page.index < 0) {
            qWarning() << "Sidebar page index not ready for button:" << page.button->objectName();
            continue;
        }

        page.button->setCheckable(true);
        page.button->setCursor(Qt::PointingHandCursor);
        sidebarGroup->addButton(page.button, page.index);

        connect(page.button, &QPushButton::clicked, this, [this, page]() {
            setPage(page.title, page.index);
        });
    }

    LogistiqueSubmenuVisible = false;

    if (ui->btnSidebarLogistique) {
        ui->btnSidebarLogistique->setCheckable(true);
        ui->btnSidebarLogistique->setCursor(Qt::PointingHandCursor);
        if (logistiqueIndex >= 0)
            sidebarGroup->addButton(ui->btnSidebarLogistique, logistiqueIndex);

        connect(ui->btnSidebarLogistique, &QPushButton::clicked, this, [this]() {
            toggleLogistiqueSubmenu();
            setLogistiquePage(LogistiqueModuleWidget::CamionsIdx, QStringLiteral("Gestion de la Logistique"));
        });
    }

    if (ui->btnSidebarLogistiqueCamions) {
        ui->btnSidebarLogistiqueCamions->setCheckable(true);
        ui->btnSidebarLogistiqueCamions->setCursor(Qt::PointingHandCursor);
        connect(ui->btnSidebarLogistiqueCamions, &QPushButton::clicked, this, [this]() {
            setLogistiquePage(LogistiqueModuleWidget::CamionsIdx, QStringLiteral("Gestion de la Logistique - Camions"));
        });
    }

    if (ui->btnSidebarLogistiqueOperations) {
        ui->btnSidebarLogistiqueOperations->setCheckable(true);
        ui->btnSidebarLogistiqueOperations->setCursor(Qt::PointingHandCursor);
        connect(ui->btnSidebarLogistiqueOperations, &QPushButton::clicked, this, [this]() {
            setLogistiquePage(LogistiqueModuleWidget::OperationsIdx, QStringLiteral("Gestion de la Logistique - Opérations"));
        });
    }

    if (ui->btnSidebarLogistiqueStats) {
        ui->btnSidebarLogistiqueStats->setCheckable(true);
        ui->btnSidebarLogistiqueStats->setCursor(Qt::PointingHandCursor);
        connect(ui->btnSidebarLogistiqueStats, &QPushButton::clicked, this, [this]() {
            setLogistiquePage(LogistiqueModuleWidget::StatsIdx, QStringLiteral("Gestion de la Logistique - Statistiques"));
        });
    }

    if (ui->btnSidebarLogistiqueMap) {
        ui->btnSidebarLogistiqueMap->setCheckable(true);
        ui->btnSidebarLogistiqueMap->setCursor(Qt::PointingHandCursor);
        connect(ui->btnSidebarLogistiqueMap, &QPushButton::clicked, this, [this]() {
            setLogistiquePage(LogistiqueModuleWidget::MapIdx, QStringLiteral("Gestion de la Logistique - Carte"));
        });
    }

    // Dynamically created buttons for Efficiency and Activity
    // Note: These are now skipped as the layouts don't exist as widgets
    // Buttons will be created dynamically if needed in the future

    PecheSubmenuVisible = false;

    if (ui->btnSidebarPeche) {
        ui->btnSidebarPeche->setCheckable(true);
        ui->btnSidebarPeche->setCursor(Qt::PointingHandCursor);
        if (PecheIndex < 0) {
            qWarning() << "Peche page index not ready.";
        } else {
            sidebarGroup->addButton(ui->btnSidebarPeche, PecheIndex);
        }

        connect(ui->btnSidebarPeche, &QPushButton::clicked, this, [this]() {
            togglePecheSubmenu();
            setPechePage(PecheModuleWidget::CrudPage, QStringLiteral("Gestion de la Pêche"));
        });
    }

    if (ui->btnSidebarPecheCrud) {
        ui->btnSidebarPecheCrud->setCursor(Qt::PointingHandCursor);
        connect(ui->btnSidebarPecheCrud, &QPushButton::clicked, this, [this]() {
            setPechePage(PecheModuleWidget::CrudPage, QStringLiteral("Gestion de la Pêche"));
        });
    }

    if (ui->btnSidebarPecheStats) {
        ui->btnSidebarPecheStats->setCursor(Qt::PointingHandCursor);
        connect(ui->btnSidebarPecheStats, &QPushButton::clicked, this, [this]() {
            setPechePage(PecheModuleWidget::StatsPage,
                                QStringLiteral("Gestion de la Pêche - Statistiques"));
        });
    }

    if (ui->btnSidebarPecheSmartServices) {
        ui->btnSidebarPecheSmartServices->setCursor(Qt::PointingHandCursor);
        connect(ui->btnSidebarPecheSmartServices, &QPushButton::clicked, this, [this]() {
            setPechePage(PecheModuleWidget::SmartServicesPage,
                                QStringLiteral("Gestion de la Pêche - Services intelligents"));
        });
    }

    if (ui->btnSidebarLogout) {
        connect(ui->btnSidebarLogout, &QPushButton::clicked, this, [this]() {
            showLogin();
        });
    }

    // Defer showLogin() until after the window has been fully shown.
    // Calling it here (inside the constructor, before w.show()) hides the
    // sidebar and topBar while the window has zero geometry, which causes
    // Qt's layout engine to collapse the entire window to nothing — only
    // the taskbar icon remains visible.  A singleShot(0) delay runs the
    // slot on the very next event-loop iteration, after the first paint.
    QTimer::singleShot(0, this, &MainWindow::showLogin);
}

void MainWindow::applyCardStyle(QWidget *card)
{
    if (!card) {
        return;
    }
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setProperty("hoverBorder", QStringLiteral("#C7D2FE"));
    card->setProperty("baseBorder", QStringLiteral("#E6EAF2"));

    int radius = 24;
    const QString name = card->objectName();
    if (name.startsWith(QStringLiteral("kpiCard"))) {
        radius = 20;
    } else if (name.startsWith(QStringLiteral("navCard")) || name.startsWith(QStringLiteral("quickCard"))) {
        radius = 20;
    }

    const bool hovered = card->property("hovered").toBool();
    const QString border = hovered ? card->property("hoverBorder").toString()
                                   : card->property("baseBorder").toString();
    QString accentStyle;
    const QVariant accentVar = card->property("accentBarColor");
    if (accentVar.canConvert<QColor>()) {
        const QColor accent = accentVar.value<QColor>();
        if (accent.isValid()) {
            accentStyle = QStringLiteral("border-left:4px solid %1;").arg(accent.name());
        }
    }

    card->setStyleSheet(QStringLiteral("background-color:#FFFFFF; border:1px solid %1; %2 border-radius:%3px;")
                            .arg(border, accentStyle)
                            .arg(radius));
}

void MainWindow::applyShadow(QWidget *card)
{
    if (!card) {
        return;
    }
    int blur = card->property("shadowBlur").toInt();
    int xOffset = card->property("shadowOffsetX").toInt();
    int yOffset = card->property("shadowOffsetY").toInt();
    QColor color = card->property("shadowColor").value<QColor>();
    if (blur <= 0) {
        blur = 30;
    }
    if (yOffset == 0) {
        yOffset = 10;
    }
    if (!color.isValid()) {
        color = QColor(15, 23, 42, 40);
    }

    auto *effect = new QGraphicsDropShadowEffect(card);
    effect->setBlurRadius(blur);
    effect->setOffset(xOffset, yOffset);
    effect->setColor(color);
    card->setGraphicsEffect(effect);

    card->setProperty("shadowBlur", blur);
    card->setProperty("shadowOffsetX", xOffset);
    card->setProperty("shadowOffsetY", yOffset);
    card->setProperty("shadowColor", color);
}

void MainWindow::setScaledIcon(QLabel *label, const QString &path, const QSize &size)
{
    if (!label) {
        return;
    }
    label->setFixedSize(size);
    label->setScaledContents(false);
    label->setAlignment(Qt::AlignCenter);

    QPixmap pixmap(path);
    const QPixmap currentPixmap = label->pixmap();
    if (pixmap.isNull() && !currentPixmap.isNull()) {
        pixmap = currentPixmap;
    }
    const QVariant accentVariant = label->property("accentColor");
    if (accentVariant.canConvert<QColor>()) {
        const QColor accent = accentVariant.value<QColor>();
        if (accent.isValid()) {
            pixmap = tintPixmap(pixmap, accent);
        }
    }

    int inset = label->property("iconInset").toInt();
    if (inset <= 0) {
        inset = 6;
    }
    const int targetW = qMax(8, size.width() - inset);
    const int targetH = qMax(8, size.height() - inset);
    if (!pixmap.isNull()) {
        label->setPixmap(pixmap.scaled(targetW, targetH, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void MainWindow::applyFonts()
{
    auto applyLabelFont = [](QLabel *label, int size, QFont::Weight weight, const QColor &color) {
        if (!label) {
            return;
        }
        QFont font = label->font();
        font.setPointSize(size);
        font.setWeight(weight);
        label->setFont(font);
        label->setStyleSheet(QStringLiteral("color:%1;").arg(color.name()));
    };

    const QColor primary("#111827");
    const QColor secondary("#475569");
    const QColor muted("#64748B");

    const char *sectionTitles[] = { "lblVueGeneraleTitle", "lblAlertsTitle", "lblRecentSectionTitle",
                                    "lblRecentActivityCardTitle" };
    for (const char *name : sectionTitles) {
        applyLabelFont(ui->pageDashboard->findChild<QLabel *>(name), 18, QFont::Bold, primary);
    }

    const char *kpiLabels[] = { "lblKpiNaviresTitle", "lblKpiChambresTitle", "lblKpiEmployesTitle",
                                "lblKpiLotsTitle", "lblKpiAlertesTitle" };
    for (const char *name : kpiLabels) {
        applyLabelFont(ui->pageDashboard->findChild<QLabel *>(name), 15, QFont::DemiBold, secondary);
    }
    const char *kpiValues[] = { "lblKpiNaviresValue", "lblKpiChambresValue", "lblKpiEmployesValue",
                                "lblKpiLotsValue", "lblKpiAlertesValue" };
    for (const char *name : kpiValues) {
        auto *label = ui->pageDashboard->findChild<QLabel *>(name);
        applyLabelFont(label, 24, QFont::Bold, primary);
        if (label) {
            label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        }
    }
    applyLabelFont(ui->pageDashboard->findChild<QLabel *>("lblKpiLotsTrend"), 12, QFont::Medium, secondary);

    const char *navTitles[] = { "lblModuleEmployesTitle", "lblModuleNaviresTitle", "lblModuleChambresTitle",
                                "lblModuleLogistiqueTitle", "lblQuickEmployesTitle", "lblQuickNaviresTitle",
                                "lblQuickEquipementsTitle", "lblQuickPecheTitle" };
    for (const char *name : navTitles) {
        applyLabelFont(ui->pageDashboard->findChild<QLabel *>(name), 14, QFont::DemiBold, primary);
    }
    const char *navSubs[] = { "lblModuleEmployesSub", "lblModuleNaviresSub", "lblModuleChambresSub",
                              "lblModuleLogistiqueSub", "lblQuickEmployesSub", "lblQuickNaviresSub",
                              "lblQuickEquipementsSub", "lblQuickPecheSub" };
    for (const char *name : navSubs) {
        applyLabelFont(ui->pageDashboard->findChild<QLabel *>(name), 12, QFont::Medium, muted);
    }

    const char *alertTimes[] = { "alertTime1", "alertTime2", "alertTime3", "alertTime4",
                                 "recentTime1", "recentTime2", "recentTime3", "recentTime4" };
    for (const char *name : alertTimes) {
        applyLabelFont(ui->pageDashboard->findChild<QLabel *>(name), 12, QFont::Medium, muted);
    }
    const char *alertTexts[] = { "alertText1", "alertText2", "alertText3", "alertText4",
                                 "recentText1", "recentText2", "recentText3", "recentText4" };
    for (const char *name : alertTexts) {
        applyLabelFont(ui->pageDashboard->findChild<QLabel *>(name), 13, QFont::Normal, primary);
    }
}

void MainWindow::setPage(const QString &title, int pageIndex)
{
    if (!ui->stackedPages) {
        qWarning() << "stackedPages not found in UI.";
        return;
    }

    if (pageIndex < 0 || pageIndex >= ui->stackedPages->count()) {
        qWarning() << "Invalid page index:" << pageIndex;
        return;
    }

    setLoginMode(false);
    ui->stackedPages->setCurrentIndex(pageIndex);
    if (ui->lblTitreDash) {
        ui->lblTitreDash->setText(title);
    }

    // Note: PecheSubmenu widget doesn't exist in UI (it's a layout)
    PecheSubmenuVisible = false;
}

void MainWindow::showLogin()
{
    if (!ui->stackedPages) {
        qWarning() << "stackedPages not found in UI.";
        return;
    }

    if (loginIndex < 0) {
        qWarning() << "Login page missing from UI.";
        return;
    }

    setLoginMode(true);
    ui->stackedPages->setCurrentIndex(loginIndex);
    if (ui->lblTitreDash) {
        ui->lblTitreDash->setText(QStringLiteral("Connexion"));
    }
    if (loginPage) {
        loginPage->focusUsername();
    }
    if (sidebarGroup) {
        sidebarGroup->setExclusive(false);
        const QList<QAbstractButton *> buttons = sidebarGroup->buttons();
        for (QAbstractButton *button : buttons) {
            if (button) {
                button->setChecked(false);
            }
        }
        sidebarGroup->setExclusive(true);
    }
}

void MainWindow::setupDashboardVisuals()
{
    if (!ui || !ui->pageDashboard) {
        return;
    }

    if (!dashBgMovie) {
        // Read the GIF from the filesystem
        QString gifPath = QCoreApplication::applicationDirPath() + "/assets/img/loop.webp";
        if (!QFileInfo::exists(gifPath)) {
            gifPath = QDir::currentPath() + "/assets/img/loop.webp";
        }
#ifdef PROJECT_SOURCE_DIR
        // If we are running from a shadow build in Qt Creator, find the assets folder in the source tree
        if (!QFileInfo::exists(gifPath)) {
            gifPath = QStringLiteral(PROJECT_SOURCE_DIR) + "/assets/img/loop.webp";
        }
#endif
        dashBgMovie = new QMovie(gifPath, QByteArray(), this);
        
        if (dashBgMovie->isValid()) {
            // When the movie frame changes, we need to repaint the window
            connect(dashBgMovie, &QMovie::frameChanged, this, [this](int){
                if (ui->stackedPages->currentWidget() == ui->pageDashboard) {
                    this->update();
                }
            });
            
            // Hide the old dashboard Qt widgets since the GIF handles the full dashboard UI
            for (QWidget *child : ui->pageDashboard->findChildren<QWidget*>()) {
                child->hide();
            }
            
            // Hide scrollbar on dashboard, show on other pages
            connect(ui->stackedPages, &QStackedWidget::currentChanged, this, [this](int index){
                if (ui->scrollAreaZoneContenu) {
                    if (index == dashboardIndex) {
                        ui->scrollAreaZoneContenu->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    } else {
                        ui->scrollAreaZoneContenu->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                    }
                }
            });
            
            // Apply it immediately for the current state
            if (ui->stackedPages->currentIndex() == dashboardIndex) {
                ui->scrollAreaZoneContenu->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            }
            
            // Start the movie
            dashBgMovie->start();
            
            // Also stop/start movie when page changes to save resources
            connect(ui->stackedPages, &QStackedWidget::currentChanged, this, [this](int index){
                if (dashBgMovie) {
                    if (ui->stackedPages->widget(index) == ui->pageDashboard) {
                        dashBgMovie->setPaused(false);
                    } else {
                        dashBgMovie->setPaused(true);
                    }
                    this->update();
                }
            });
        } else {
            qWarning() << "Failed to load dashboard background GIF:" << gifPath;
        }
    }
}

void MainWindow::showDashboard()
{
    if (!ui->stackedPages) {
        qWarning() << "stackedPages not found in UI.";
        return;
    }

    if (dashboardIndex < 0) {
        qWarning() << "No dashboard page available.";
        return;
    }

    setLoginMode(false);
    ui->stackedPages->setCurrentIndex(dashboardIndex);
    if (ui->lblTitreDash) {
        ui->lblTitreDash->setText(QStringLiteral("Dashboard"));
    }
    if (ui->lblUtilisateurTop) {
        QSettings settings("MonApp", "GestionEmployes");
        const QString nom = settings.value("login/nom").toString().trimmed();
        const QString prenom = settings.value("login/prenom").toString().trimmed();

        const QString fullName = QStringLiteral("%1 %2").arg(nom, prenom).simplified();
        ui->lblUtilisateurTop->setText(fullName.isEmpty() ? QStringLiteral("Admin ONP") : fullName);
    }

    // Note: PecheSubmenu widget doesn't exist in UI (it's a layout)
    PecheSubmenuVisible = false;
}

void MainWindow::setLoginMode(bool enabled)
{
    // Hide/show top bar and sidebar in login mode
    if (ui->lblTitreDash) {
        ui->lblTitreDash->setVisible(!enabled);
    }
    if (ui->lblUtilisateurTop) {
        ui->lblUtilisateurTop->setVisible(!enabled);
    }
    if (ui->btnParametres) {
        ui->btnParametres->setVisible(!enabled);
    }
    if (ui->scrollAreaSidebar) {
        ui->scrollAreaSidebar->setVisible(!enabled);
    }

    // Remove / restore the content-area margins so the login page fills
    // the full window width/height without the 40 / 30 px insets.
    if (ui->layoutZoneContenu) {
        if (enabled) {
            ui->layoutZoneContenu->setContentsMargins(0, 0, 0, 0);
            ui->layoutZoneContenu->setSpacing(0);
        } else {
            ui->layoutZoneContenu->setContentsMargins(40, 30, 40, 30);
            ui->layoutZoneContenu->setSpacing(16);
        }
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->lblSidebarLogo && event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            showDashboard();
            return true;
        }
    }

    auto *widget = qobject_cast<QWidget *>(watched);
    if (widget && dashboardHoverWidgets.contains(widget)) {
        if (event->type() == QEvent::Enter) {
            widget->setProperty("hovered", true);
            if (widget->property("useHoverShadow").toBool()) {
                if (!widget->graphicsEffect()) {
                    const int blur = widget->property("hoverShadowBlur").toInt();
                    const int offsetY = widget->property("hoverShadowOffsetY").toInt();
                    const QColor color = widget->property("hoverShadowColor").value<QColor>();
                    widget->setProperty("shadowBlur", blur);
                    widget->setProperty("shadowOffsetX", 0);
                    widget->setProperty("shadowOffsetY", offsetY);
                    widget->setProperty("shadowColor", color.isValid() ? color : QColor(15, 23, 42, 40));
                    applyShadow(widget);
                }
            }
            applyCardStyle(widget);
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
            widget->update();
        } else if (event->type() == QEvent::Leave) {
            widget->setProperty("hovered", false);
            if (widget->property("useHoverShadow").toBool()) {
                widget->setGraphicsEffect(nullptr);
            }
            applyCardStyle(widget);
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
            widget->update();
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setPechePage(int subIndex, const QString &title)
{
    if (!ui->stackedPages || !PecheModule) {
        qWarning() << "Peche module not available.";
        return;
    }

    const int moduleIndex = ui->stackedPages->indexOf(PecheModule);
    if (moduleIndex < 0) {
        qWarning() << "Peche module missing from stackedPages.";
        return;
    }

    setLoginMode(false);
    ui->stackedPages->setCurrentIndex(moduleIndex);
    if (ui->lblTitreDash) {
        ui->lblTitreDash->setText(title);
    }

    if (ui->btnSidebarPeche) {
        ui->btnSidebarPeche->setChecked(true);
    }

    switch (subIndex) {
    case PecheModuleWidget::CrudPage:
        PecheModule->showCrud();
        break;
    case PecheModuleWidget::StatsPage:
        PecheModule->showStats();
        break;
    case PecheModuleWidget::SmartServicesPage:
        PecheModule->showSmartServices();
        break;
    default:
        PecheModule->showCrud();
        break;
    }
}

void MainWindow::togglePecheSubmenu()
{
    // Note: PecheSubmenu widget doesn't exist in UI (it's a layout)
    PecheSubmenuVisible = !PecheSubmenuVisible;
}
bool MainWindow::zoneexiste(QString zone)
{
    QSqlQuery query;

    query.prepare("SELECT COUNT(*) FROM PECHES WHERE UPPER(ZONEPECHE) = UPPER(:zone)");
    query.bindValue(":zone", zone.trimmed());

    if (!query.exec()) {
        return false;
    }

    if (query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

void MainWindow::setLogistiquePage(int subIndex, const QString &title)
{
    if (!ui->stackedPages || !LogistiqueModule) {
        qWarning() << "Logistique module not available.";
        return;
    }

    const int moduleIndex = ui->stackedPages->indexOf(LogistiqueModule);
    if (moduleIndex < 0) {
        qWarning() << "Logistique module missing from stackedPages.";
        return;
    }

    setLoginMode(false);
    ui->stackedPages->setCurrentIndex(moduleIndex);
    if (ui->lblTitreDash) {
        ui->lblTitreDash->setText(title);
    }

    if (ui->btnSidebarLogistique) {
        ui->btnSidebarLogistique->setChecked(true);
    }

    if (ui->btnSidebarLogistiqueCamions)
        ui->btnSidebarLogistiqueCamions->setChecked(subIndex == LogistiqueModuleWidget::CamionsIdx);
    if (ui->btnSidebarLogistiqueOperations)
        ui->btnSidebarLogistiqueOperations->setChecked(subIndex == LogistiqueModuleWidget::OperationsIdx);
    if (ui->btnSidebarLogistiqueStats)
        ui->btnSidebarLogistiqueStats->setChecked(subIndex == LogistiqueModuleWidget::StatsIdx);
    if (ui->btnSidebarLogistiqueMap)
        ui->btnSidebarLogistiqueMap->setChecked(subIndex == LogistiqueModuleWidget::MapIdx);

    auto *btnEff = this->findChild<QPushButton*>("btnSidebarLogistiqueEfficiency");
    if (btnEff) btnEff->setChecked(subIndex == LogistiqueModuleWidget::EfficiencyIdx);

    auto *btnAct = this->findChild<QPushButton*>("btnSidebarLogistiqueActivity");
    if (btnAct) btnAct->setChecked(subIndex == LogistiqueModuleWidget::ActivityIdx);

    LogistiqueModule->setPage(subIndex);
}

void MainWindow::toggleLogistiqueSubmenu()
{
    LogistiqueSubmenuVisible = !LogistiqueSubmenuVisible;
}


