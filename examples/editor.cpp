
#include <QGuiApplication>
#include <iostream>
#include <fstream>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QQmlProperty>
#include "editor.hpp"
#include "../src/isf.hpp"

int main(int argc, char** argv)
{
    using namespace isf;
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine e;

    qmlRegisterType<ObjectCreator>("ISF", 1, 0, "ObjectCreator");

    e.load("../libisf/examples/Editor.qml");
    QQuickItem* rect = e.rootObjects()[0]->findChild<QQuickItem*>("rect");

    std::ifstream t("/tmp/mozilla_jcelerier0/PLAID.fs");
    std::string foo((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());


    isf::parser s{foo};
    auto d = s.data();
    QString shaderText = "import QtQuick 2.0\nimport CreativeControls 1.0\n"
                         "Item { anchors.fill: parent\n"
                         "property alias fragmentShader: s.fragmentShader\n"
                         "ShaderEffect { anchors.fill: parent \n";

    auto frag = QString::fromStdString(s.fragment().back());

    for(auto& inp : d.inputs)
    {
        std::visit([&] (const auto& t) {
            auto prop = QString::fromStdString(t.name);
            shaderText += "property var qt_" + prop + "\n";
            frag.replace(prop, "qt_" + prop);
        }, inp);
    }
     shaderText += "property point qt_RENDERSIZE: Qt.point(width, height)\n";
     shaderText += "property real qt_TIME\n ";
     shaderText += "id: s\n ";
     shaderText += "Timer { running: true; interval: 32; repeat: true; onTriggered: s.qt_TIME+=0.001 }\n";
     shaderText += "}\n"
                   "Column {\n"
                   "x: 500\n";

     for(auto& inp : d.inputs)
     {
         std::visit([&] (const auto& t) {
             auto prop = QString::fromStdString(t.name);
             shaderText += "Slider { onValueChanged: s.qt_" + prop + " = 100 * value }\n";
         }, inp);
     }
     shaderText += "}}";
     QQmlComponent shaderComp(&e);
     shaderComp.setData(shaderText.toUtf8(), QUrl{});
     QQuickItem* obj = (QQuickItem*)shaderComp.create();

     frag.replace("RENDERSIZE", "qt_RENDERSIZE");
     frag.replace("TIME", "qt_TIME");

     QQmlProperty fragProp(obj, "fragmentShader");
     fragProp.write(frag);

     obj->setParentItem(rect);
     return app.exec();

}
