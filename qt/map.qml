import QtQuick 2.12
import QtLocation 5.6
import QtPositioning 5.6
import QtQuick.Window 2.12
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0

Rectangle {
    id: window

    property double oldLat
    property double oldLng
    property double circleRadio_qml
    property double circleLat
    property double circleLng
    property bool alarm
    property var item
    property int visibleTrack_qml
    property int resetTrack_qml

    property Component comMarker: mapMarker

    Plugin {    // Obtain Open Street Map information
        id: mapPlugin
        name: "osm"
        PluginParameter{name: "osm.useragent"; value: "TGAdmin"}
        PluginParameter{name: "osm.mapping.custom.host"; value: "https://tile.openstreetmap.org"}
        PluginParameter{name: "osm.mapping.providersrepository.disabled"; value: true}
        PluginParameter{name: "osm.useragent"; value: "My great Qt OSM application" }
        PluginParameter{name: "osm.mapping.host"; value: "http://osm.tile.server.address/" }
        PluginParameter{name: "osm.mapping.copyright"; value: "All mine" }
        PluginParameter{name: "osm.routing.host"; value: "http://osrm.server.address/viaroute" }
        PluginParameter{name: "osm.geocoding.host"; value: "http://geocoding.server.address" }
    }

    function setLAT(lat) {  // Get latitude
        if (oldLat > 0){
        oldLat = lat
        }
        console.log("LAT", oldLat)
    }

    function setLON(lng) {  // Get longitude
        if (oldLng > 0){
        oldLng = lng
        }
        console.log("LON", oldLng)
    }

    function setCenter(lat, lng) {
        mapView.pan(oldLat - lat, oldLng - lng)
        oldLat = lat
        oldLng = lng

        mapView.removeMapItem(item) // In order not to duplicate the marker
    }

    function addMarker(lat, lng) {
        item = comMarker.createObject(window, {coordinate: QtPositioning.coordinate(lat, lng)})

        mapView.addMapItem(item)    // Show marker on map
    }

    function setLimit(circleRadio) { // The radius of the circle that will be displayed on the map is established
        if (circleRadio_qml !== circleRadio){
            circleRadio_qml = circleRadio
            circleLat = oldLat
            circleLng = oldLng
        }
        console.log("Radio:", circleRadio_qml)
    }

    function setVisibleTrack(visibleTrack) {
        visibleTrack_qml = visibleTrack
        console.log("Visible Track:", visibleTrack_qml)
    }

    function setResetTrack(resetTrack) {
        resetTrack_qml = resetTrack
        console.log("Reset path:", resetTrack_qml)
    }

    Map {   // Show QML map
        id: mapView
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(oldLat, oldLng);
        zoomLevel: 50   // Initial zoom level
        onCenterChanged: console.log(center)  
        property MapPolyline item2

        MapCircle { // Draw the perimeter
            center {    // Set center of perimeter
                latitude: circleLat
                longitude: circleLng
            }
            radius: circleRadio_qml
            color: 'green'
            border.width: 3
            opacity: 0.3
        }

        MapPolyline { // Draw the route
                    id: route
                    line.color: "blue"
                    line.width: 5
                    opacity: visibleTrack_qml // Show or not the route
        }

        Timer { // Update the route every second
                id: updateTimer
                interval: 1000
                running: true
                repeat: true
                onTriggered: {
                    if (oldLat != 0.0 && oldLng != 0.0 && resetTrack_qml != 1){ // Add the current GPS location to the route
                    route.path = route.path.concat(QtPositioning.coordinate(oldLat, oldLng))
                    }else if (resetTrack_qml == 1){ // Reset the route
                    route.path = []
                    resetTrack_qml=0
                    }
                }
       }
    }

    Component { // Show a marker
        id: mapMarker
        MapQuickItem {
            id: markerImg
            anchorPoint.x: image.width/4
            anchorPoint.y: image.height
            coordinate: position
            visible: true   // Always visible

            sourceItem: Image {
                id: image
                source: "qrc:/images/marker.png"
            }
        }

    }
}

