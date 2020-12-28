import QtQuick 2.6
import QtQuick.Controls 1.4
import QtLocation 5.9
import QtPositioning 5.8

Rectangle {
    id: rectangle
    width: 0
    height: 0
    visible: true

    property var staMarker: []

    signal sendStationIndexSignal(string msg)

    Plugin {
        id: plugin
        name: "osm"

        PluginParameter { name: "osm.mapping.highdpi_tiles"; value: true }
        PluginParameter { name: "osm.mapping.offline.directory"; value: "/.RTICOM2/map_data/mapbox-outdoors" }
    }

    Map {
        anchors.fill: parent
        id: map
        center: QtPositioning.coordinate(35.80, 128.1)
        zoomLevel: 7.15
        minimumZoomLevel: 7.0
        maximumZoomLevel: 11.9

        plugin: plugin

        MapSliders {
            id: sliders
            z: map.z + 3
            mapSource: map
            edge: Qt.LeftEdge
        }

        onCenterChanged: {
            if(map.center.latitude > 44.5 || map.center.latitude < 30.5 || map.center.longitude > 137.5 || map.center.longitude < 119.5)
                map.center = QtPositioning.coordinate(35.80, 128.1)
        }
    }

    function clearMap()
    {
        map.clearMapItems()
    }

    function createStaMarker(index, netName, staName, lat, longi, inuse)
    {
        staMarker[index] = Qt.createQmlObject('StaMarker { }', map)
        staMarker[index].coordinate = QtPositioning.coordinate(lat, longi)
        staMarker[index].netName = netName
        staMarker[index].staName = staName
        staMarker[index].index = index
        staMarker[index].iconOpacity = 1.0;
        staMarker[index].textOpacity = 0;
        if(inuse == 1)
            staMarker[index].sourceName = "Icon/StaIconGreen.png"
        else
            staMarker[index].sourceName = "Icon/StaIconRed.png"

        map.addMapItem(staMarker[index])
    }

    function changeStaMarkerSource(index, inuse)
    {
        if(inuse == 1)
            staMarker[index].sourceName = "Icon/StaIconGreen.png"
        else
            staMarker[index].sourceName = "Icon/StaIconRed.png"
    }

    function boldStaMarker(index)
    {
        staMarker[index].sourceName = "Icon/StaIconPupple.png"
        staMarker[index].iconWidth = 25;
        staMarker[index].iconHeight = 25;
    }

    function normalStaMarker(index, inuse)
    {
        if(inuse == 1)
            staMarker[index].sourceName = "Icon/StaIconGreen.png"
        else
            staMarker[index].sourceName = "Icon/StaIconRed.png"
        staMarker[index].iconWidth = 15;
        staMarker[index].iconHeight = 15;
    }
}
