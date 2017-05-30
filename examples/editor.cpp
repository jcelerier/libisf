
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include "editor.hpp"

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine e;

    e.load("../libisf/examples/Editor.qml");
    QQuickItem* root = e.rootObjects()[0]->findChild<QQuickItem*>("editor");
    QQuickItem* rect = e.rootObjects()[0]->findChild<QQuickItem*>("rect");
    isf::ShaderEditor se{*root, *rect, e};

    return app.exec();

}
