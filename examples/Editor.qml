import QtQuick 2.7
import CreativeControls 1.0
import QtQuick.Controls 2.1 as QC
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
Window
{
    visible: true
    width: 800
    height: 800

    Rectangle {
        id: editor
        objectName: "editor"
        signal shaderChanged(string shader)
        width: parent.width / 2
        height: parent.height
        ColumnLayout
        {
            QC.Button {
                text: qsTr("Load")
            }

            QC.TextArea {
                id: textarea
                wrapMode: TextEdit.NoWrap
                height: 800
                onTextChanged: editor.shaderChanged(text)
                text: "Some text"
                Layout.fillHeight:true
            }


        }
    }

    Rectangle {
        id: rect
        color: "black"
        x: parent.width / 2
        width: parent.width / 2
        height: parent.height
        objectName: "rect"
    }
}
