import QtQuick 2.5
import QtQuick.Controls 1.4
import QtLocation 5.8
import QtPositioning 5.8

Map {
    anchors.fill: parent
    id: map
    center: QtPositioning.coordinate(35.80, 128.1)
    zoomLevel: 7.75
    maximumZoomLevel: 11
    minimumZoomLevel: 7

    plugin: satellitesPlugin

    MapSliders {
        id: sliders
        z: map.z + 3
        mapSource: map
        edge: Qt.LeftEdge
    }

    MapRegend {
        id: regend
        regendOpacity: 0.7
    }

    onCenterChanged: {
        if(map.center.latitude > 44.5 || map.center.latitude < 30.5 || map.center.longitude > 137.5 || map.center.longitude < 119.5)
            map.center = QtPositioning.coordinate(36.3, 127.7)
    }
}

Rectangle {
    id: rectangle

    property variant map

    function createMap(mapDirName)
    {
        var plugin
        plugin = Qt.createQmlObject ('import QtLocation 5.6; Plugin{ name:"osm"}', rectangle)

        var zoomLevel = null
        var tilt = null
        var bearing = null
        var fov = null
        var center = null
        var panelExpanded = null

        if (map) {
            zoomLevel = map.zoomLevel
            tilt = map.tilt
            bearing = map.bearing
            fov = map.fieldOfView
            center = map.center
            panelExpanded = map.slidersExpanded
            map.destroy()
        }

        //map = mapComponent.createObject();
        map.plugin = plugin;

        if (zoomLevel != null) {
            map.tilt = tilt
            map.bearing = bearing
            map.fieldOfView = fov
            map.zoomLevel = zoomLevel
            map.center = center
            map.slidersExpanded = panelExpanded
        } else {
            // Use an integer ZL to enable nearest interpolation, if possible.
            map.zoomLevel = Math.floor((map.maximumZoomLevel - map.minimumZoomLevel)/2)
            // defaulting to 45 degrees, if possible.
            map.fieldOfView = Math.min(Math.max(45.0, map.minimumFieldOfView), map.maximumFieldOfView)
        }

        map.forceActiveFocus()
    }
}


