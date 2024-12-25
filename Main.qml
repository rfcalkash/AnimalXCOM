import QtQuick
import AnimalXCOM
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

ApplicationWindow{
    width: 700
    height: 500
    visible: true
    title: qsTr("Hello World")

    Material.theme: Material.Dark
    Material.accent: Material.Purple

    // ColumnLayout{
    //     id:controlsId
    //     enabled: !drawerId.busy
    //     width: 200
    //     SpinBox{
    //         Layout.fillWidth: true
    //         from: 1
    //         to:1000
    //         value: drawerId.width
    //         onValueChanged: drawerId.width=value
    //         editable: true
    //     }
    //     SpinBox{
    //         Layout.fillWidth: true
    //         from: 1
    //         to:1000
    //         value: drawerId.height
    //         onValueChanged: drawerId.height=value
    //         editable: true
    //     }
    //     SpinBox{
    //         Layout.fillWidth: true
    //         from: 1
    //         to:10
    //         value: drawerId.maxRoadLanes
    //         onValueChanged: drawerId.maxRoadLanes=value
    //         editable: true
    //     }
    //     SpinBox{
    //         Layout.fillWidth: true
    //         from: 1
    //         to:500
    //         value: drawerId.maxBuildSize
    //         onValueChanged: drawerId.maxBuildSize=value
    //         editable: true
    //     }
    //     Button{
    //         text: "New seed"
    //         onClicked: drawerId.redraw()
    //         Layout.alignment: Qt.AlignHCenter
    //     }
    // }

    ColumnLayout{
        anchors.fill: parent
        anchors.margins: 10
        RoadsDrawer{
            id: drawerId
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip:true
        }
        RowLayout{
            Layout.fillWidth: true
            visible: drawerId.busy
            property int total: Math.max(1,drawerId.fieldWidth*drawerId.fieldHeight)
            Label{
                Layout.fillHeight: true
                text: drawerId.progress+" of "+parent.total
            }

            ProgressBar{
                Layout.fillWidth: true
                Layout.fillHeight: true
                from: 0
                to: 100
                value: 100*drawerId.progress/(parent.total)
            }
        }
    }

    Component.onCompleted: drawerId.redraw()
}
