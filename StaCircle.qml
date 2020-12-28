import QtQuick 2.6
import QtLocation 5.9

MapQuickItem {
    id: staCircle

    property real lati : 0
    property real longi : 0
    property string colors: 'white'
    property string bcolors: 'black'
    property var index
    property real textOpacity: 0
    property real circleOpacity: 0
    property string txt: ""
    property int widthi : 10

    sourceItem: Rectangle {
        id: circle
        width: widthi
        height: widthi
        color: colors
        border.width: 1
        border.color: bcolors
        smooth: true
        radius: 10

        Text{
            id: ttt
            y: circle.height
            width: circle.width
            font.bold: true
            font.pixelSize: 15
            opacity: textOpacity
            horizontalAlignment: Text.AlignHCenter
            text: txt
        }
    }
    coordinate {
        latitude: lati
        longitude: longi
    }
    opacity: circleOpacity
    anchorPoint: Qt.point(sourceItem.width/2, sourceItem.height/2)

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: {
            circle.border.color='red';
            textOpacity=1;
            rectangle.sendStationIndexSignal(index)
        }
        onExited: {
            circle.border.color=bcolors;
            textOpacity=0;
        }
    }
}

