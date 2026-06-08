#include "camionmappage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QHeaderView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMessageBox>
#include <QCoreApplication>
#include <QVariant>
#include <QQmlContext>
#include <QQuickItem>
#include <QSqlError>
#include <QDebug>
#include <QSqlRecord>
#include <QWheelEvent>
#include <QScrollArea>

CamionMapPage::CamionMapPage(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

CamionMapPage::~CamionMapPage() {}

void CamionMapPage::setupUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 6, 10, 0);
    layout->setSpacing(0);

    // ── Title ────────────────────────────────────────────────────────────────
    auto *title = new QLabel("Carte des Flottes");
    title->setAlignment(Qt::AlignCenter);
    title->setFixedHeight(48);
    title->setStyleSheet(
        "font-family: 'Segoe UI', 'Helvetica Neue', Helvetica, Arial, sans-serif;"
        "font-size: 28px; font-weight: 900; color: #1a3a6b; letter-spacing: 1px;"
    );
    layout->addWidget(title, 0);

    // Map
    mapWidget = new QQuickWidget();
    mapWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    mapWidget->setSource(QUrl("qrc:/logistique/MapWidget.qml"));
    
    if (mapWidget->status() == QQuickWidget::Error) {
        QString errs;
        for (const auto& e : mapWidget->errors()) {
            errs += e.toString() + "\n";
        }
        QMessageBox::critical(this, "QML Error", "Failed to load map:\n" + errs);
    } else {
        connect(mapWidget, &QQuickWidget::statusChanged, this, [this](QQuickWidget::Status status){
            if (status == QQuickWidget::Ready) {
                // In case signal already connected we don't want multiple, but this Lambda runs once per ready status
                disconnect(mapWidget->rootObject(), SIGNAL(markerMoved(int,double,double)), this, SLOT(updateCamionPosition(int,double,double)));
                connect(mapWidget->rootObject(), SIGNAL(markerMoved(int,double,double)), this, SLOT(updateCamionPosition(int,double,double)));
            }
        });
        if (mapWidget->rootObject()) {
            connect(mapWidget->rootObject(), SIGNAL(markerMoved(int,double,double)), this, SLOT(updateCamionPosition(int,double,double)));
        }
    }
    // ── Map frame (dark-blue rounded border) ───
    auto *mapFrame = new QFrame();
    mapFrame->setObjectName("mapFrame");
    mapFrame->setStyleSheet(
        "QFrame#mapFrame {"
        "  border: 3px solid #1a3a6b;"
        "  border-radius: 12px;"
        "  background: transparent;"
        "}"
    );
    auto *mapFrameLayout = new QVBoxLayout(mapFrame);
    mapFrameLayout->setContentsMargins(3, 3, 3, 3);
    mapFrameLayout->setSpacing(0);
    mapFrameLayout->addWidget(mapWidget);

    auto *mainLayout = new QHBoxLayout();
    mainLayout->setContentsMargins(0, 8, 0, 8);
    mainLayout->setSpacing(10);
    mainLayout->addWidget(mapFrame, 1);

    // ── Zoom buttons (outside the map, to the right) ──────────────────────────
    auto *controlsLayout = new QVBoxLayout();
    controlsLayout->setAlignment(Qt::AlignTop);
    controlsLayout->setContentsMargins(0, 8, 0, 0);
    controlsLayout->setSpacing(8);

    auto *btnZoomIn  = new QPushButton("+");
    auto *btnZoomOut = new QPushButton("-");

    QString btnStyle =
        "QPushButton { background: #ffffff; border: 2px solid #1a3a6b;"
        "  border-radius: 18px; font-size: 20px; font-weight: bold; color: #1a3a6b; }"
        "QPushButton:hover { background: #e8eef7; }";

    btnZoomIn->setFixedSize(38, 38);
    btnZoomOut->setFixedSize(38, 38);
    btnZoomIn->setCursor(Qt::PointingHandCursor);
    btnZoomOut->setCursor(Qt::PointingHandCursor);
    btnZoomIn->setStyleSheet(btnStyle);
    btnZoomOut->setStyleSheet(btnStyle);

    controlsLayout->addWidget(btnZoomIn);
    controlsLayout->addWidget(btnZoomOut);

    connect(btnZoomIn, &QPushButton::clicked, this, [this](){
        if (mapWidget && mapWidget->rootObject()) {
            double zoom    = mapWidget->rootObject()->property("currentZoom").toDouble();
            double maxZoom = mapWidget->rootObject()->property("maxZoom").toDouble();
            mapWidget->rootObject()->setProperty("currentZoom", qMin(zoom + 0.5, maxZoom));
        }
    });

    connect(btnZoomOut, &QPushButton::clicked, this, [this](){
        if (mapWidget && mapWidget->rootObject()) {
            double zoom    = mapWidget->rootObject()->property("currentZoom").toDouble();
            double minZoom = mapWidget->rootObject()->property("minZoom").toDouble();
            mapWidget->rootObject()->setProperty("currentZoom", qMax(zoom - 0.5, minZoom));
        }
    });

    mainLayout->addLayout(controlsLayout);
    layout->addLayout(mainLayout, 1);
}

void CamionMapPage::updateCamionPosition(int id, double lat, double lon) {
    QSqlQuery q;
    q.prepare("UPDATE Camions SET Latitude = :lat, Longitude = :lon WHERE CamionId = :id");
    q.bindValue(":lat", lat);
    q.bindValue(":lon", lon);
    q.bindValue(":id", id);
    if (q.exec()) {
        syncMapModel();
    }
}

void CamionMapPage::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    syncMapModel();

    QScrollArea *scrollArea = nullptr;
    QWidget *p = this->parentWidget();
    while (p) {
        scrollArea = qobject_cast<QScrollArea*>(p);
        if (scrollArea) break;
        p = p->parentWidget();
    }
    if (scrollArea) {
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
}

void CamionMapPage::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    QScrollArea *scrollArea = nullptr;
    QWidget *p = this->parentWidget();
    while (p) {
        scrollArea = qobject_cast<QScrollArea*>(p);
        if (scrollArea) break;
        p = p->parentWidget();
    }
    if (scrollArea) {
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
}

void CamionMapPage::wheelEvent(QWheelEvent *event) {
    // Prevent the parent QScrollArea from scrolling when we are trying to zoom the map
    if (mapWidget && mapWidget->underMouse()) {
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}



void CamionMapPage::syncMapModel()
{
    if (!mapWidget || !mapWidget->rootObject()) return;
    
    QVariantList list;
    QSqlQuery q("SELECT CamionId, Immatriculation, Statut, Latitude, Longitude FROM Camions");
    while (q.next()) {
        double lat = q.value(3).toDouble();
        double lon = q.value(4).toDouble();
        
        if (lat == 0.0 && lon == 0.0) continue; 
        
        QVariantMap map;
        map["id"] = q.value(0).toInt();
        map["immat"] = q.value(1).toString();
        map["statut"] = q.value(2).toString();
        map["latitude"] = lat;
        map["longitude"] = lon;
        list.append(map);
    }
    mapWidget->rootObject()->setProperty("camionsModel", list);
}
