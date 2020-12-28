import QtQuick 2.6
import QtLocation 5.9

MapQuickItem {
    id: eewStarMarker

    property string eewInfo: "Magnitude"
    property string sourceName: "../Icon/eewEventMarker.png"
    property int iconWidth: 45
    property int iconHeight: 50
    property real iconOpacity: 1.0

    anchorPoint.x: image.width * 0.5
    anchorPoint.y: image.height * 0.5

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
            font.pixelSize: 23
            color: 'red'
            opacity: 1.0
            horizontalAlignment: Text.AlignHCenter
            text: eewInfo
        }
    }
}

