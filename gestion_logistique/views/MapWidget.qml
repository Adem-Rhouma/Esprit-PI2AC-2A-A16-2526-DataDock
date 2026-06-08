import QtQuick
import QtLocation
import QtPositioning

Item {
    id: root
    width: 600
    height: 400

    signal markerMoved(int id, double latitude, double longitude)
    signal mapClicked(double latitude, double longitude)

    property var camionsModel: []
    property bool isDraggingMarker: false
    property alias currentZoom: map.zoomLevel
    property alias maxZoom: map.maximumZoomLevel
    property alias minZoom: map.minimumZoomLevel

    function getCoordinate(x, y) {
        var coord = map.toCoordinate(Qt.point(x, y));
        return { "latitude": coord.latitude, "longitude": coord.longitude };
    }

    function updateMarkers() {
        markerModel.clear();
        for (var i = 0; i < camionsModel.length; ++i) {
            markerModel.append(camionsModel[i]);
        }
    }

    onCamionsModelChanged: updateMarkers()

    Plugin {
        id: mapPlugin
        name: "osm"
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(36, 10)
        zoomLevel: 7.5
        minimumZoomLevel: 7.5
        maximumZoomLevel: 18.0

        onCenterChanged: {
            var clampedLat = Math.max(30.2, Math.min(center.latitude, 37.4));
            var clampedLon = Math.max(7.5, Math.min(center.longitude, 11.6));
            if (clampedLat !== center.latitude || clampedLon !== center.longitude) {
                center = QtPositioning.coordinate(clampedLat, clampedLon);
            }
        }

        WheelHandler {
            property: "zoomLevel"
            rotationScale: 1/120
        }

        DragHandler {
            enabled: !root.isDraggingMarker
            target: null
            onTranslationChanged: (delta) => map.pan(-delta.x, -delta.y)
        }

        TapHandler {
            onTapped: (eventPoint) => {
                var coord = map.toCoordinate(eventPoint.position);
                root.mapClicked(coord.latitude, coord.longitude);
            }
        }

        ListModel {
            id: markerModel
        }

        MapItemView {
            model: markerModel
            delegate: MapQuickItem {
                id: markerItem
                coordinate: QtPositioning.coordinate(model.latitude, model.longitude)
                anchorPoint.x: 12
                anchorPoint.y: 24

                sourceItem: Item {
                    width: 24
                    height: 24
                    
                    Rectangle {
                        id: markerRect
                        width: 24
                        height: 24
                        radius: 12
                        color: {
                            if (model.statut === "Disponible") return "#10b981";
                            if (model.statut === "En service") return "#3b82f6";
                            if (model.statut === "En panne") return "#ef4444";
                            return "#f59e0b"; // maintenance
                        }
                        border.width: 2
                        border.color: "#ffffff"
                        
                        Text {
                            anchors.centerIn: parent
                            text: "C"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 10
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            property point pressOffset: Qt.point(0, 0)
                            
                            onPressed: (mouse) => {
                                root.isDraggingMarker = true
                                pressOffset = Qt.point(mouse.x - width / 2, mouse.y - height / 2)
                            }
                            
                            onPositionChanged: (mouse) => {
                                if (pressed) {
                                    var mapPos = mouseArea.mapToItem(map, mouse.x - pressOffset.x, mouse.y - pressOffset.y)
                                    markerItem.coordinate = map.toCoordinate(mapPos)
                                }
                            }
                            onReleased: {
                                root.isDraggingMarker = false
                                root.markerMoved(model.id, markerItem.coordinate.latitude, markerItem.coordinate.longitude)
                            }
                            onCanceled: {
                                root.isDraggingMarker = false
                            }
                        }

                        HoverHandler {
                            id: hoverHandler
                            cursorShape: Qt.PointingHandCursor
                            enabled: !mouseArea.pressed
                        }
                        
                        Rectangle {
                            visible: hoverHandler.hovered && !mouseArea.pressed
                            anchors.bottom: markerRect.top
                            anchors.horizontalCenter: markerRect.horizontalCenter
                            anchors.bottomMargin: 4
                            color: "#cc000000" // ARGB for 80% opacity black
                            radius: 4
                            width: tooltipText.width + 12
                            height: tooltipText.height + 8
                            z: 999
                            Text {
                                id: tooltipText
                                anchors.centerIn: parent
                                text: model.immat
                                color: "white"
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }

    }
}
