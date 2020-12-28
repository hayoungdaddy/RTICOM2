import QtQuick 2.6
import QtLocation 5.9

MapQuickItem {
    id: staMarker

    property string staName: "temp"
    property string netName: "temp"
    property var index
    property string sourceName: "Icon/StaIconGreen.png"
    property real iconWidth: 15
    property real iconHeight: 15
    property real iconOpacity: 0.0
    property real textOpacity: 0.0

    anchorPoint.x: image.width * 0.5
    anchorPoint.y: image.height * 0.5

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: {
            textOpacity=1.0
            rectangle.sendStationIndexSignal(index)
        }
        onExited: {
            textOpacity=0.0
        }
    }

    sourceItem: Image {
        id: image
        source: sourceName
        width: iconWidth
        height: iconHeight
        opacity: iconOpacity

        Text{
            id: myText
            y: image.height
            width: image.width
            font.bold: true
            font.pixelSize: 12
            opacity: textOpacity
            horizontalAlignment: Text.AlignHCenter
            text: netName + "/" + staName
        }
    }
}

