#include "chambresfroidesalertinbox.h"
#include "chambresfroides.h"
#include <QScrollArea>
#include <QHBoxLayout>
#include <QFrame>
#include <QSettings>
#include <QMessageBox>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

ChambresFroidesAlertInbox::ChambresFroidesAlertInbox(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    applyStyles();
    refreshAlerts();
}

void ChambresFroidesAlertInbox::setupUi()
{
    setWindowTitle("Centre d'Alertes Intelligentes");
    setFixedSize(550, 650);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- PREMIUM HEADER ---
    auto *headerFrame = new QFrame(this);
    headerFrame->setObjectName("headerFrame");
    headerFrame->setFixedHeight(80);
    headerFrame->setStyleSheet("background-color: #0f172a; border-bottom: 2px solid #1e293b;");
    auto *headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(25, 0, 25, 0);
    
    auto *titleLayout = new QVBoxLayout();
    auto *titleLabel = new QLabel("CENTRE D'ALERTES", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: 800; color: #38bdf8; letter-spacing: 2px;");
    titleLayout->addWidget(titleLabel);
    
    auto *subTitle = new QLabel("Surveillance des anomalies de stockage", this);
    subTitle->setStyleSheet("font-size: 11px; color: #64748b; font-weight: 600; text-transform: uppercase;");
    titleLayout->addWidget(subTitle);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    
    m_btnRefresh = new QPushButton("Actualiser", this);
    m_btnRefresh->setFixedSize(120, 38);
    m_btnRefresh->setCursor(Qt::PointingHandCursor);
    m_btnRefresh->setObjectName("btnRefresh");
    connect(m_btnRefresh, &QPushButton::clicked, this, &ChambresFroidesAlertInbox::refreshAlerts);
    headerLayout->addWidget(m_btnRefresh);
    
    mainLayout->addWidget(headerFrame);

    // --- SCROLL AREA ---
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background-color: #020617; border: none;");
    
    m_scrollContent = new QWidget();
    m_scrollContent->setStyleSheet("background-color: transparent;");
    m_alertsLayout = new QVBoxLayout(m_scrollContent);
    m_alertsLayout->setAlignment(Qt::AlignTop);
    m_alertsLayout->setContentsMargins(20, 20, 20, 20);
    m_alertsLayout->setSpacing(15);
    
    scrollArea->setWidget(m_scrollContent);
    mainLayout->addWidget(scrollArea);
}

void ChambresFroidesAlertInbox::applyStyles()
{
    setStyleSheet(R"(
        QDialog { background-color: #020617; }
        
        #btnRefresh {
            background-color: #1e293b;
            color: #38bdf8;
            border: 1px solid #334155;
            border-radius: 8px;
            font-weight: 700;
            font-size: 12px;
        }
        #btnRefresh:hover {
            background-color: #334155;
            border: 1px solid #38bdf8;
        }
        #btnRefresh:pressed {
            background-color: #020617;
        }

        QScrollBar:vertical {
            border: none;
            background: #020617;
            width: 8px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #1e293b;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { border: none; background: none; }
    )");
}

void ChambresFroidesAlertInbox::refreshAlerts()
{
    // Provide Visual Feedback: "Chargement..."
    m_btnRefresh->setEnabled(false);
    m_btnRefresh->setText("SCT SCAN...");
    m_btnRefresh->setStyleSheet("background-color: #0f172a; color: #fbbf24; border: 1px solid #fbbf24;");

    // Artificial delay to show the scan is happening (Industrial feel)
    QTimer::singleShot(400, this, [this]() {
        // Clear layout
        QLayoutItem *child;
        while ((child = m_alertsLayout->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }

        auto alerts = ChambresFroides::getActiveAlerts();
        if (alerts.isEmpty()) {
            auto *emptyContainer = new QWidget();
            auto *emptyLayout = new QVBoxLayout(emptyContainer);
            emptyLayout->setContentsMargins(0, 100, 0, 0);
            
            auto *emptyIcon = new QLabel("🌊", emptyContainer);
            emptyIcon->setStyleSheet("font-size: 48px;");
            emptyIcon->setAlignment(Qt::AlignCenter);
            emptyLayout->addWidget(emptyIcon);

            auto *emptyLabel = new QLabel("Horizon dégagé : aucune anomalie détectée.", emptyContainer);
            emptyLabel->setAlignment(Qt::AlignCenter);
            emptyLabel->setStyleSheet("color: #64748b; font-size: 14px; font-weight: 600; font-style: italic;");
            emptyLayout->addWidget(emptyLabel);
            
            m_alertsLayout->addWidget(emptyContainer);
        } else {
            for (const auto &alertData : alerts) {
                m_alertsLayout->addWidget(createAlertItem(alertData));
            }
        }

        // Restore button state
        m_btnRefresh->setEnabled(true);
        m_btnRefresh->setText("Actualiser");
        m_btnRefresh->setStyleSheet(""); // Revert to stylesheet
    });
}

QWidget* ChambresFroidesAlertInbox::createAlertItem(const QMap<QString, QString> &alertData)
{
    auto *card = new QFrame();
    card->setObjectName("alertCard");
    
    QString severity = alertData["severity"];
    QString accentColor = "#3b82f6"; // Blue (Normal)
    if (severity == "Critical") accentColor = "#ef4444"; // Red
    else if (severity == "High") accentColor = "#f59e0b"; // Amber

    card->setStyleSheet(QString(R"(
        #alertCard {
            background-color: #0f172a;
            border: 1px solid #1e293b;
            border-left: 4px solid %1;
            border-radius: 12px;
            padding: 15px;
        }
        #alertCard:hover {
            border: 1px solid #334155;
            background-color: #1e293b;
        }
    )").arg(accentColor));

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(8);
    
    // Top Row: Type and Severity
    auto *topRow = new QHBoxLayout();
    auto *typeLabel = new QLabel(alertData["type"].toUpper());
    typeLabel->setStyleSheet(QString("font-weight: 900; color: %1; font-size: 10px; letter-spacing: 1px;").arg(accentColor));
    
    QString severityFr = (severity == "Critical") ? "CRITIQUE" : 
                         (severity == "High") ? "ÉLEVÉ" : "NORMAL";
    auto *severityLabel = new QLabel(severityFr);
    severityLabel->setStyleSheet("color: #475569; font-size: 9px; font-weight: 800;");
    
    topRow->addWidget(typeLabel);
    topRow->addStretch();
    topRow->addWidget(severityLabel);
    cardLayout->addLayout(topRow);

    // Message
    auto *msgLabel = new QLabel(alertData["msg"]);
    msgLabel->setWordWrap(true);
    msgLabel->setStyleSheet("font-size: 14px; color: #f1f5f9; font-weight: 700; margin: 5px 0;");
    cardLayout->addWidget(msgLabel);

    // Timestamps
    auto *timeLabel = new QLabel(QString("Créée le: %1").arg(alertData["date_created"]));
    timeLabel->setStyleSheet("font-size: 10px; color: #64748b; font-weight: 500;");
    cardLayout->addWidget(timeLabel);

    // Separator
    auto *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #1e293b; max-height: 1px; margin: 5px 0;");
    cardLayout->addWidget(line);

    // Bottom Row: Status and Actions
    auto *bottomRow = new QHBoxLayout();
    QString status = alertData["status"];
    
    if (status == "Working" || status == "Done" || status == "Ignored") {
        QString statusText = "EN ATTENTE";
        QString color = "#64748b";
        if (status == "Working") { statusText = "⚙️ EN COURS"; color = "#3b82f6"; }
        else if (status == "Done") { statusText = "✅ RÉSOLU"; color = "#10b981"; }
        else if (status == "Ignored") { statusText = "⊘ IGNORÉ"; color = "#475569"; }

        auto *statusLabel = new QLabel(statusText);
        statusLabel->setStyleSheet(QString("font-weight: 900; font-size: 11px; color: %1;").arg(color));
        bottomRow->addWidget(statusLabel);
        bottomRow->addStretch();
    } else {
        auto *btnWork = new QPushButton("Traiter");
        btnWork->setFixedSize(90, 30);
        btnWork->setStyleSheet("QPushButton { background-color: #3b82f6; color: white; border-radius: 6px; font-weight: bold; font-size: 11px; }");
        
        QString aid = alertData["id"] + "_GENERIC";
        connect(btnWork, &QPushButton::clicked, [this, aid]() {
            ChambresFroides::updateAlertStatus(aid, "Working");
            refreshAlerts();
        });
        bottomRow->addWidget(btnWork);
        bottomRow->addStretch();
    }

    // Resolve Button (Only if not already done)
    if (status != "Done" && status != "Ignored") {
        auto *btnIgnore = new QPushButton("Ignorer");
        btnIgnore->setFixedSize(80, 30);
        btnIgnore->setStyleSheet("QPushButton { background-color: transparent; color: #475569; border: 1px solid #334155; border-radius: 6px; font-weight: bold; font-size: 11px; }");
        
        QString aid = alertData["id"] + "_GENERIC";
        connect(btnIgnore, &QPushButton::clicked, [this, aid]() {
            ChambresFroides::updateAlertStatus(aid, "Ignored");
            refreshAlerts();
        });
        bottomRow->addWidget(btnIgnore);
    }

    cardLayout->addLayout(bottomRow);
    return card;
}

