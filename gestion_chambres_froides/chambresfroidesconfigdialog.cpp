#include "chambresfroidesconfigdialog.h"
#include "../arduino.h"
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>
#include <QDateTime>
#include <QtMath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QGroupBox>
#include <QRandomGenerator>
#include <QPainter>
#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QDebug>


ChambresFroidesConfigDialog::ChambresFroidesConfigDialog(const QString &id, const QString &tagNumber, double temp, double humidity, const QString &state, bool betaTest, QWidget *parent) :
    QDialog(parent),
    m_id(id),
    m_tag(tagNumber.isEmpty() ? id : tagNumber),
    m_state(state),
    m_currentTemp(temp),
    m_currentHumidity(humidity),
    m_tickCount(0),
    m_betaTest(betaTest)
{
    // TENTATIVE DE CONNEXION AUTOMATIQUE (Plus besoin de modifier la BD à la main)
    int res = m_arduino.connect_arduino();
    if (res == 0) {
        m_betaTest = true; // On force le mode beta car l'arduino est présent
        qDebug() << "Auto-connected to Arduino for room" << m_id;
        
        // Synchro initiale blindée (on attend que le bootloader finisse)
        QTimer::singleShot(3500, this, [this]() {
            for(int i=0; i<3; ++i) {
                m_arduino.write_to_arduino(QString("ID=%1\n").arg(m_tag).toUtf8());
                QThread::msleep(150);
                m_arduino.write_to_arduino(QString("SETPOINT=%1\n").arg(QString::number(m_currentTemp, 'f', 1)).toUtf8());
                QThread::msleep(150);
            }
            m_arduino.write_to_arduino("POLL\n");
            qDebug() << "Initial Triple-Sync sent for room" << m_id;
        });
    } else {
        qDebug() << "No Arduino found for room" << m_id << " - Staying in manual mode.";
        // Si m_betaTest était déjà true mais que ça échoue, on peut garder l'alerte ou rester silencieux
        if (m_betaTest) {
             qDebug() << "Connection failed but beta was requested.";
        }
    }
    
    m_syncTimer = new QTimer(this);
    m_syncTimer->setSingleShot(true);
    connect(m_syncTimer, &QTimer::timeout, this, &ChambresFroidesConfigDialog::onSyncTimerTimeout);
    
    setupManualUI();
    
    // Safety check for pointers before setting values
    if (ui_sliderTemp) ui_sliderTemp->setValue(static_cast<int>(m_currentTemp));
    if (ui_sliderHumidity) ui_sliderHumidity->setValue(static_cast<int>(m_currentHumidity));
    updateLabels();

    setupChart();
    
    m_dataTimer = new QTimer(this);
    connect(m_dataTimer, &QTimer::timeout, this, &ChambresFroidesConfigDialog::updateRealtimeData);
    
    // Always start the data timer for live telemetry display
    m_dataTimer->start(1000);
    qDebug() << "Realtime timer started for room" << m_id << "beta=" << m_betaTest << "state=" << m_state;
}

ChambresFroidesConfigDialog::~ChambresFroidesConfigDialog()
{
}

void ChambresFroidesConfigDialog::setupManualUI()
{
    setMinimumSize(1050, 900);
    setWindowTitle("Analyse Systémique - " + m_id);
    setStyleSheet("QDialog { background-color: #020617; color: white; }");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(25);

    // --- GLASSMORPHIC HEADER ---
    auto *headerCard = new QFrame(this);
    headerCard->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 rgba(255, 255, 255, 0.05), stop:1 rgba(255, 255, 255, 0)); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 20px;");
    auto *headerLayout = new QHBoxLayout(headerCard);
    
    auto *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);
    
    auto *headerTitle = new QLabel("UNITÉ DE CONTRÔLE CLIMATIQUE", this);
    headerTitle->setStyleSheet("font-size: 20px; font-weight: 800; color: #38bdf8; letter-spacing: 2px; border: none;");
    titleLayout->addWidget(headerTitle);

    auto *headerSub = new QLabel(m_betaTest ? "LIAISON MATÉRIELLE : Liaison Arduino ACTIVE. Contrôle temps-réel." : "Surveillance industrielle et diagnostic d'intégrité structurelle.", this);
    headerSub->setStyleSheet(m_betaTest ? "font-size: 13px; color: #22c55e; font-weight: bold; border: none;" : "font-size: 13px; color: #94a3b8; border: none;");
    titleLayout->addWidget(headerSub);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();

    // Operational Status Badge
    QString badgeColor = "#22c55e"; 
    bool isDown = (m_state == "Broken" || m_state == "Maintenance");
    if (m_state == "Broken") badgeColor = "#ef4444";
    else if (m_state == "Maintenance") badgeColor = "#f59e0b";
    
    auto *statusBadge = new QLabel(m_state.toUpper(), this);
    statusBadge->setAlignment(Qt::AlignCenter);
    statusBadge->setFixedSize(160, 45);
    statusBadge->setStyleSheet(QString("background: %1; color: white; border-radius: 12px; font-weight: 900; font-size: 14px; letter-spacing: 1px;").arg(badgeColor));
    headerLayout->addWidget(statusBadge);
    mainLayout->addWidget(headerCard);

    // --- TELEMETRY CHART ---
    ui_chartView = new QChartView(this);
    ui_chartView->setMinimumHeight(350);
    ui_chartView->setStyleSheet("border: 1px solid #1e293b; border-radius: 15px; background: #0f172a;");
    if (isDown) {
        auto *chartOverlay = new QFrame(ui_chartView);
        chartOverlay->setStyleSheet("background: rgba(2, 6, 23, 0.95); border-radius: 15px;");
        auto *ovLayout = new QVBoxLayout(chartOverlay);
        auto *ovText = new QLabel("SENSORS OFFLINE / SIGNAL LOST", chartOverlay);
        ovText->setStyleSheet("color: #ef4444; font-weight: 900; font-size: 18px; letter-spacing: 4px;");
        ovText->setAlignment(Qt::AlignCenter);
        ovLayout->addWidget(ovText);
        
        auto *viewLayout = new QVBoxLayout(ui_chartView);
        viewLayout->setContentsMargins(0,0,0,0);
        viewLayout->addWidget(chartOverlay);
    }
    mainLayout->addWidget(ui_chartView);

    // --- STATUS INDICATORS ---
    auto *statsLayout = new QHBoxLayout();
    auto createCard = [&](const QString &title, const QString &c, QLabel* &lbl) {
           auto *f = new QFrame(this);
           f->setStyleSheet("background: #1e293b; border: 1px solid #334155; border-radius: 12px;");
           auto *l = new QVBoxLayout(f);
           auto *t = new QLabel(title, f);
           t->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 11px;").arg(c));
           l->addWidget(t);
           lbl = new QLabel("--", f);
           lbl->setAlignment(Qt::AlignCenter);
           lbl->setStyleSheet("font-size: 32px; font-weight: 900; color: white;");
           l->addWidget(lbl);
           return f;
    };
        statsLayout->addWidget(createCard("TEMPÉRATURE LOCALE", "#f87171", ui_lblTempVal));
        statsLayout->addWidget(createCard("TEMPÉRATURE SOUHAITÉE", "#fb7185", ui_lblDesiredTempVal));
    statsLayout->addWidget(createCard("HUMIDITÉ", "#38bdf8", ui_lblHumidityVal));
    statsLayout->addWidget(createCard("CHARGE SYSTÈME", "#fbbf24", ui_lblLoadVal));
    statsLayout->addWidget(createCard("RISQUE GIVRE", "#a78bfa", ui_lblFrostVal));
    mainLayout->addLayout(statsLayout);

    if (isDown) {
        ui_lblLoadVal->setText("OFFLINE");
        ui_lblFrostVal->setText("OFFLINE");
    }

    // --- DYNAMIC CONTROL STACK ---
    auto *stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(stackedWidget);

    // PAGE 0: ACTIVE CONTROLS
    auto *activePage = new QWidget();
    auto *activeLayout = new QHBoxLayout(activePage);
    activeLayout->setSpacing(25);

    auto createControl = [&](const QString &label, const QString &c, QSlider* &slider, int min, int max) {
        auto *grp = new QFrame();
        grp->setStyleSheet("background: rgba(255,255,255,0.03); border: 1px solid #334155; border-radius: 15px;");
        auto *v = new QVBoxLayout(grp);
        v->addWidget(new QLabel(label, grp));
        slider = new QSlider(Qt::Horizontal, grp);
        slider->setRange(min, max);
        slider->setStyleSheet(QString("QSlider::groove:horizontal { background: #0f172a; height: 10px; border-radius: 5px; } QSlider::handle:horizontal { background: %1; width: 22px; height: 22px; margin: -6px 0; border-radius: 11px; }").arg(c));
        v->addWidget(slider);
        return grp;
    };
    activeLayout->addWidget(createControl("RÉGLAGE THERMOSTAT (-40°C à 15°C)", "#f87171", ui_sliderTemp, -40, 15));
    activeLayout->addWidget(createControl("RÉGLAGE HYGROMÉTRIE (0% à 100%)", "#38bdf8", ui_sliderHumidity, 0, 100));
    stackedWidget->addWidget(activePage);

    // PAGE 1: BEAUTIFUL FAULT VIEW
    auto *faultPage = new QWidget();
    auto *faultLayout = new QVBoxLayout(faultPage);
    
    auto *faultOverlay = new QFrame(faultPage);
    faultOverlay->setStyleSheet(QString("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 transparent); border: 2px solid %2; border-radius: 15px;").arg(isDown ? "rgba(239, 68, 68, 0.1)" : "transparent").arg(badgeColor));
    auto *foLayout = new QVBoxLayout(faultOverlay);
    foLayout->setAlignment(Qt::AlignCenter);

    auto *faultIcon = new QLabel(m_state == "Broken" ? "⚠️" : "🛠️", faultOverlay);
    faultIcon->setStyleSheet("font-size: 64px;");
    foLayout->addWidget(faultIcon);

    auto *faultTitle = new QLabel(m_state == "Broken" ? "DÉFAILLANCE CRITIQUE DÉTECTÉE" : "SYSTÈME EN COURS DE MAINTENANCE", faultOverlay);
    faultTitle->setStyleSheet(QString("font-size: 24px; font-weight: 900; color: %1;").arg(badgeColor));
    foLayout->addWidget(faultTitle);

    auto *faultDesc = new QLabel(m_state == "Broken" ? "Le compresseur ne répond plus. Une intervention technique immédiate est requise." : "Installation verrouillée pour des raisons de sécurité durant l'entretien périodique.", faultOverlay);
    faultDesc->setWordWrap(true);
    faultDesc->setAlignment(Qt::AlignCenter);
    faultDesc->setStyleSheet("color: #94a3b8; font-size: 15px; margin-top: 10px;");
    foLayout->addWidget(faultDesc);

    // Animation Effect for Fault View 
    // Opacity Pulse (shared)
    auto *opacityEffect = new QGraphicsOpacityEffect(faultTitle);
    faultTitle->setGraphicsEffect(opacityEffect);
    auto *pulseAnim = new QPropertyAnimation(opacityEffect, "opacity", this);
    pulseAnim->setDuration(1500);
    pulseAnim->setStartValue(1.0);
    pulseAnim->setEndValue(0.4);
    pulseAnim->setLoopCount(-1);
    pulseAnim->setEasingCurve(QEasingCurve::InOutQuad);
    pulseAnim->start();

    // Icon Specific Animation
    if (m_state == "Maintenance") {
        // Rotation for Maintenance (Gears/Tools)
        // Note: QWidget doesn't have a direct 'rotation' property without QGraphicsView,
        // but we can animate a transform or use a custom property if needed.
        // For simplicity and stability, we'll stick to the pulsing glow which looks great,
        // or we could animate the icon size slightly.
        auto *sizeAnim = new QPropertyAnimation(faultIcon, "geometry", this);
        sizeAnim->setDuration(2000);
        sizeAnim->setStartValue(faultIcon->geometry());
        sizeAnim->setEndValue(faultIcon->geometry().adjusted(-5,-5,5,5));
        sizeAnim->setLoopCount(-1);
        sizeAnim->setEasingCurve(QEasingCurve::CosineCurve);
        sizeAnim->start();
    }

    faultLayout->addWidget(faultOverlay);
    stackedWidget->addWidget(faultPage);

    // Switch view based on state
    stackedWidget->setCurrentIndex(isDown ? 1 : 0);

    // --- ACTION BAR ---
    auto *actionBar = new QHBoxLayout();
    actionBar->addStretch();
    
    auto *btnCancel = new QPushButton("Annuler", this);
    btnCancel->setFixedSize(140, 45);
    btnCancel->setStyleSheet("QPushButton { background: #1e293b; color: white; border-radius: 10px; font-weight: bold; }");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    actionBar->addWidget(btnCancel);

    auto *btnSave = new QPushButton(isDown ? "ACCÈS VERROUILLÉ" : "APPLIQUER CONFIGURATION", this);
    btnSave->setFixedSize(300, 45);
    btnSave->setEnabled(!isDown);
    btnSave->setStyleSheet(isDown ? 
        "QPushButton { background: #0f172a; color: #475569; border: 1px solid #1e293b; border-radius: 10px; font-weight: 800; }" :
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #8b5cf6); color: white; border-radius: 10px; font-weight: 800; }");
    connect(btnSave, &QPushButton::clicked, this, &ChambresFroidesConfigDialog::on_btnSave_clicked);
    actionBar->addWidget(btnSave);
    mainLayout->addLayout(actionBar);

    if (!isDown) {
        connect(ui_sliderTemp, &QSlider::valueChanged, this, &ChambresFroidesConfigDialog::on_sliderTemp_valueChanged);
        connect(ui_sliderHumidity, &QSlider::valueChanged, this, &ChambresFroidesConfigDialog::on_sliderHumidity_valueChanged);
    }
}

void ChambresFroidesConfigDialog::setupChart()
{
    auto makeSeries = [&](QLineSeries* &s, const QString &n, const QString &c) {
        s = new QLineSeries();
        s->setName(n);
        QPen p{QColor(c)};
        p.setWidth(3);
        s->setPen(p);
    };
    makeSeries(m_tempSeries, "Température", "#f87171");
    makeSeries(m_humiditySeries, "Humidité", "#38bdf8");
    makeSeries(m_loadSeries, "Charge", "#fbbf24");
    makeSeries(m_frostSeries, "Givre", "#a78bfa");

    QChart *chart = new QChart();
    chart->addSeries(m_tempSeries);
    chart->addSeries(m_humiditySeries);
    chart->addSeries(m_loadSeries);
    chart->addSeries(m_frostSeries);
    chart->setTheme(QChart::ChartThemeDark);
    chart->setBackgroundVisible(false);
    chart->setAnimationOptions(QChart::SeriesAnimations);

    m_axisX = new QValueAxis();
    m_axisX->setRange(0, 30);
    m_axisX->setLabelFormat("%d s");
    m_axisX->setGridLineVisible(false);

    m_axisY = new QValueAxis();
    m_axisY->setRange(-45, 110);
    m_axisY->setGridLineColor(QColor("#1e293b"));

    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);

    m_tempSeries->attachAxis(m_axisX);
    m_tempSeries->attachAxis(m_axisY);
    m_humiditySeries->attachAxis(m_axisX);
    m_humiditySeries->attachAxis(m_axisY);
    m_loadSeries->attachAxis(m_axisX);
    m_loadSeries->attachAxis(m_axisY);
    m_frostSeries->attachAxis(m_axisX);
    m_frostSeries->attachAxis(m_axisY);

    ui_chartView->setChart(chart);
    ui_chartView->setRenderHint(QPainter::Antialiasing);
}

void ChambresFroidesConfigDialog::updateRealtimeData()
{
    m_tickCount++;
    double noise = (static_cast<double>(QRandomGenerator::global()->bounded(100)) / 100.0 - 0.5) * 0.2;
    
    // In "Broken" state, simulate a system that's failing (very flat or zero load)
    bool isFaulty = (m_state == "Broken");
    double currentLoad = isFaulty ? 0.0 : qBound(0.0, 35.0 + (15.0 - m_currentTemp) * 1.5, 100.0);
    double currentFrost = 0.0;
    
    double actualTemp = m_currentTemp;
    double actualHum = m_currentHumidity;
    int actualFanPercent = 0;

    if (m_betaTest) {
        // Renvoi périodique de l'ID et de la consigne (tous les 5 ticks / 5 secondes)
        // pour s'assurer que l'Arduino est toujours synchro
        if (m_tickCount % 5 == 0) {
             m_arduino.write_to_arduino(QString("ID=%1\n").arg(m_tag).toUtf8());
             QThread::msleep(50);
             m_arduino.write_to_arduino(QString("SETPOINT=%1\n").arg(QString::number(m_currentTemp, 'f', 1)).toUtf8());
        }

        m_arduino.write_to_arduino("POLL\n");
        qDebug() << "[ARDUINO POLL]" << m_id;
        bool receivedStatus = false;
        
        // Robust reading: wait and accumulate
        for(int i=0; i<10; ++i) {
            QThread::msleep(50);
            QByteArray chunk = m_arduino.read_from_arduino();
            if (!chunk.isEmpty()) {
                m_serialBuffer += QString::fromUtf8(chunk);
                if (m_serialBuffer.contains('\n')) break;
            }
        }
        
        int newlineIndex = -1;
        while ((newlineIndex = m_serialBuffer.indexOf('\n')) != -1) {
            QString line = m_serialBuffer.left(newlineIndex).trimmed();
            m_serialBuffer.remove(0, newlineIndex + 1);

            if (line.isEmpty()) continue;
            qDebug() << "[ARDUINO RAW]" << line; // LOG CRUCIAL POUR VOIR SI ÇA PARLE

            if (line.startsWith("DBG;")) {
                qDebug() << "[ARDUINO DBG]" << line.mid(4);
                continue;
            }

            if (line.startsWith("STATUS;")) {
                QStringList parts = line.split(';');
                bool foundData = false;
                
                for (const QString &part : parts) {
                    if (part.startsWith("TEMP=")) {
                        actualTemp = part.mid(5).toDouble();
                        foundData = true;
                    } else if (part.startsWith("HUMIDITY=")) {
                        actualHum = part.mid(9).toDouble();
                        foundData = true;
                    } else if (part.startsWith("FAN_PWM=")) {
                        actualFanPercent = (part.mid(8).toInt() * 100) / 255;
                        foundData = true;
                    }
                }

                if (foundData) {
                    receivedStatus = true;
                    ui_lblTempVal->setText(QString::number(actualTemp, 'f', 1) + " °C");
                    ui_lblHumidityVal->setText(QString::number(actualHum, 'f', 1) + " %");
                    ui_lblLoadVal->setText(QString::number(actualFanPercent) + " %");
                    
                    if (ui_lblDesiredTempVal) {
                        ui_lblDesiredTempVal->setText(QString::number(m_currentTemp, 'f', 1) + " °C");
                    }
                    
                    if (m_id == "1") {
                        QSqlQuery q;
                        q.prepare("UPDATE CHAMBRESFROIDES SET TEMP = :t, HUMIDITY = :h WHERE CF_ID = '1'");
                        q.bindValue(":t", actualTemp);
                        q.bindValue(":h", actualHum);
                        q.exec();
                    }
                }
            }
        }

        if (!receivedStatus) {
            qDebug() << "[ARDUINO RX] No STATUS frame received for room" << m_id << "buffer=" << m_serialBuffer;
        }
    } else {
        // No Arduino connection: display stored values
        updateLabels();
    }

    if (actualTemp < 2.0) {
        currentFrost = qBound(0.0, (actualHum / 100.0) * (15.0 - actualTemp) * 1.3, 100.0);
    }

    m_tempSeries->append(m_tickCount, actualTemp + (m_betaTest ? 0 : noise));
    m_humiditySeries->append(m_tickCount, actualHum + (m_betaTest ? 0 : noise));
    m_loadSeries->append(m_tickCount, m_betaTest ? actualFanPercent : (currentLoad + noise * 5.0));
    m_frostSeries->append(m_tickCount, currentFrost + noise * 2.0);
    
    if (m_tickCount > 30) {
        m_axisX->setRange(m_tickCount - 30, m_tickCount);
    }
    
    if (!m_betaTest) ui_lblLoadVal->setText(QString::number(currentLoad, 'f', 0) + " %");
    ui_lblFrostVal->setText(QString::number(currentFrost, 'f', 0) + " %");
}

void ChambresFroidesConfigDialog::updateLabels()
{
    if (ui_lblDesiredTempVal) {
        ui_lblDesiredTempVal->setText(QString::number(m_currentTemp, 'f', 1) + " °C");
    }

    if (ui_lblTempVal) {
        if (!m_betaTest) {
            ui_lblTempVal->setText("--.-°C");
        } else if (m_currentTemp > -900) {
            ui_lblTempVal->setText(QString::number(m_currentTemp, 'f', 1) + " °C");
        } else {
            ui_lblTempVal->setText("--.-°C");
        }
    }

    if (ui_lblHumidityVal) {
        if (!m_betaTest) {
            ui_lblHumidityVal->setText("--%");
        } else if (m_currentHumidity > -900) {
            ui_lblHumidityVal->setText(QString::number(m_currentHumidity, 'f', 1) + " %");
        } else {
            ui_lblHumidityVal->setText("--%");
        }
    }
}

void ChambresFroidesConfigDialog::on_sliderTemp_valueChanged(int value)
{
    double oldTemp = m_currentTemp;
    m_currentTemp = static_cast<double>(value);
    double deltaT = m_currentTemp - oldTemp;
    m_currentHumidity = qBound(0.0, m_currentHumidity - (deltaT * 0.6), 100.0);
    ui_sliderHumidity->blockSignals(true);
    ui_sliderHumidity->setValue(static_cast<int>(m_currentHumidity));
    ui_sliderHumidity->blockSignals(false);
    updateLabels();

    if (m_betaTest) {
        m_syncTimer->start(500); // Déclenche la synchro après 500ms d'inactivité sur le slider
    }
}

void ChambresFroidesConfigDialog::on_sliderHumidity_valueChanged(int value)
{
    double oldHum = m_currentHumidity;
    m_currentHumidity = static_cast<double>(value);
    double deltaH = m_currentHumidity - oldHum;
    m_currentTemp = qBound(-40.0, m_currentTemp + (deltaH * 0.25), 15.0);
    ui_sliderTemp->blockSignals(true);
    ui_sliderTemp->setValue(static_cast<int>(m_currentTemp));
    ui_sliderTemp->blockSignals(false);
    updateLabels();
}

void ChambresFroidesConfigDialog::on_btnSave_clicked()
{
    if (m_betaTest) {
        onSyncTimerTimeout();
    }
    accept();
}

void ChambresFroidesConfigDialog::onSyncTimerTimeout()
{
    if (m_betaTest) {
        // Send ID and Setpoint
        m_arduino.write_to_arduino(QString("ID=%1\n").arg(m_tag).toUtf8());
        QThread::msleep(50); // Small gap between commands
        QString cmd = QString("SETPOINT=%1\n").arg(QString::number(m_currentTemp, 'f', 2));
        m_arduino.write_to_arduino(cmd.toUtf8());
        m_arduino.write_to_arduino("POLL\n");
        qDebug() << "[ARDUINO TX SYNC]" << m_id << m_currentTemp;

        // Update DB as well for persistence
        QSqlQuery q;
        q.prepare("UPDATE CHAMBRESFROIDES SET TEMP = :t WHERE CF_ID = :id");
        q.bindValue(":t", m_currentTemp);
        q.bindValue(":id", m_id);
        q.exec();
    }
}

double ChambresFroidesConfigDialog::getTemperature() const { return m_currentTemp; }
double ChambresFroidesConfigDialog::getHumidity() const { return m_currentHumidity; }
