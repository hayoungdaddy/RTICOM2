import QtQuick 2.6
import QtQuick.Controls 1.4
import QtLocation 5.9
import QtPositioning 5.8

Rectangle {
    id: rectangle
    width: 0
    height: 0
    visible: true

    property var staCircle: []
    property var eewStarMarker: "Hello"
    property var aniCircleP: "Hello"
    property var aniCircleS: "Hello"
    property variant parameters

    signal sendStationIndexSignal(string msg)

    /*
    Plugin {
        id: plugin
        name: "osm"

        PluginParameter { name: "osm.mapping.highdpi_tiles"; value: true }
        PluginParameter { name: "osm.mapping.offline.directory"; value: mapDir }
        PluginParameter { name: "osm.mapping.cache.disk.cost_strategy"; value: 0 }
        PluginParameter { name: "osm.mapping.cache.disk.size"; value: 0 }
        PluginParameter { name: "osm.mapping.cache.memory.cost_strategy"; value: 0}
        PluginParameter { name: "osm.mapping.cache.memory.size"; value: 0 }
    }
    */

    function initializePlugin(pluginParameters)
    {
        var parameters = new Array;
        for(var prop in pluginParameters)
        {
            var parameter = Qt.createQmlObject('import QtLocation 5.9; PluginParameter{ name: "'+ prop + '"; value: "' + pluginParameters[prop] + '"}', map)
            parameters.push(parameter)
        }
        rectangle.parameters = parameters
        map.plugin = Qt.createQmlObject('import QtLocation 5.9; Plugin{ name: "osm"; parameters: rectangle.parameters }', rectangle)
    }

    Map {
        anchors.fill: parent
        id: map
        center: QtPositioning.coordinate(35.80, 127.7)
        minimumZoomLevel: 7.0
        maximumZoomLevel: 11.9
        zoomLevel: 7.7

        //plugin: plugin

        onCenterChanged: {
            if(map.center.latitude > 44.5 || map.center.latitude < 30.5 || map.center.longitude > 137.5 || map.center.longitude < 119.5)
                map.center = QtPositioning.coordinate(35.80, 127.7)
        }

        MapSliders {
            id: sliders
            z: map.z + 3
            mapSource: map
            edge: Qt.LeftEdge
        }
    }

    function clearMap()
    {
        map.clearMapItems()
        return true;
    }

    function createStaCircle(index, lat, longi, width, color, netsta, opacity)
    {
        staCircle[index].index = index;
        staCircle[index].lati = lat;
        staCircle[index].longi = longi;
        staCircle[index].widthi = width;
        staCircle[index].colors = color;
        staCircle[index].txt = netsta;
        staCircle[index].circleOpacity = opacity;

        return true;
    }

    function createRealTimeMapObject()
    {
        for(var i=0;i<1000;i++)
        {
            staCircle[i] = Qt.createQmlObject('StaCircle { }', map);
            staCircle[i].circleOpacity = 0;
        }

        return true;
    }

    function createStackedMapObject()
    {
        eewStarMarker = Qt.createQmlObject('EEWStarMarker { }', map);
        eewStarMarker.iconOpacity = 0;

        aniCircleP = Qt.createQmlObject('import QtLocation 5.9; MapCircle { }', map, "dynamic");
        aniCircleP.opacity = 0;
        aniCircleP.border.width = 3;
        aniCircleS = Qt.createQmlObject('import QtLocation 5.9; MapCircle { }', map, "dynamic");
        aniCircleS.opacity = 0;
        aniCircleS.border.width = 3;

        for(var i=0;i<1000;i++)
        {
            staCircle[i] = Qt.createQmlObject('StaCircle { }', map);
            staCircle[i].circleOpacity = 0;
        }

        return true;
    }

    function hideMapObject()
    {
        eewStarMarker.iconOpacity = 0;
        aniCircleP.opacity = 0;
        aniCircleS.opacity = 0;

        return true;
    }

    function changeSizeAndColorForStaCircle(index, width, color, opacity)
    {
        staCircle[index].widthi = width;
        staCircle[index].colors = color;
        staCircle[index].circleOpacity = opacity;

        map.addMapItem(staCircle[index]);

        return true;
    }

    function showEEWStarMarker(lat, longi, mag)
    {
        eewStarMarker.coordinate = QtPositioning.coordinate(lat, longi);
        eewStarMarker.eewInfo = "M " + mag;
        eewStarMarker.iconOpacity = 1.0;

        map.addMapItem(eewStarMarker);

        return true;
    }

    function showCircleForAnimation(lat, longi, radP, radS)
    {
        aniCircleP.center = QtPositioning.coordinate(lat, longi);
        aniCircleP.radius = radP;
        aniCircleP.opacity = 1;
        aniCircleP.border.color = 'blue';

        aniCircleS.center = QtPositioning.coordinate(lat, longi);
        aniCircleS.radius = radS;
        aniCircleS.opacity = 1;
        aniCircleS.border.color = 'red';

        map.addMapItem(aniCircleP);
        map.addMapItem(aniCircleS);

        return true;
    }
}
