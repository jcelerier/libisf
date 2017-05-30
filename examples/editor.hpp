#pragma once

#include <QQuickItem>
#include <QQmlProperty>
#include <QQmlApplicationEngine>
#include "../src/isf.hpp"
namespace isf
{

struct create_control_visitor
{
    QString operator()(const float_input& t)
    {
        auto prop = QString::fromStdString(t.name);
        auto label = QString::fromStdString(t.label);
        return QString(
                    "Text {"
                    "  text: '%2';"
                    "  color: 'white';"
                    "}\n"
                    "Slider { "
                    "  mapFunc: function(v) { return %3 + v * (%4 - %3) };"
                    "  initialValue: (%5 - %3) / (%4 - %3);"
                    "  orientation: Qt.horizontal; "
                    "  height: 30; "
                    "  onValueChanged: s.qt_%1 = value "
                    "}\n").arg(prop).arg(label.isEmpty() ? prop : label).arg(t.min).arg(t.max).arg(t.def);
    }
    QString operator()(const bool_input& t)
    {
        auto prop = QString::fromStdString(t.name);
        auto label = QString::fromStdString(t.label);
        return QString(
                    "Text {"
                    "  text: '%2';"
                    "  color: 'white';"
                    "}\n"
                    "Switch {"
                    "  state: %3;"
                    "  onStateChanged: if(state == 'ON') s.qt_%1 = true; else s.qt_%1 = false;"
                    "}\n").arg(prop).arg(label.isEmpty() ? prop : label).arg(t.def ? "'ON'" : "'OFF'");
    }
    QString operator()(const event_input& t)
    {
        auto prop = QString::fromStdString(t.name);
        auto label = QString::fromStdString(t.label);
        return QString(
                    "Text {"
                    "  text: '%2';"
                    "  color: 'white';"
                    "}\n"
                    "Switch {"
                    "  onStateChanged: { if(state == 'ON') {s.qt_%1 = true;} else {s.qt_%1 = false;} }"
                    "}\n").arg(prop).arg(label.isEmpty() ? prop : label);
    }
    QString operator()(const point2d_input& i)
    {
    }
    QString operator()(const point3d_input& i)
    {
        return {};
    }
    QString operator()(const color_input& i)
    {
        return {};
    }
    QString operator()(const image_input& i)
    {
        return {};
    }

};

class ShaderEditor : public QObject
{
    Q_OBJECT
public:
    ShaderEditor(QQuickItem& root, QQuickItem& rect, QQmlApplicationEngine& app):
        m_root{root}
      , m_rect{rect}
      , m_app{app}
    {
        connect(&m_root, SIGNAL(shaderChanged(QString)),
                this, SLOT(on_shaderChanged(QString)));
    }
private slots:
    void on_shaderChanged(QString shader)
    {
        try {
        isf::parser s{shader.toStdString()};
        auto d = s.data();
        QString shaderText = "import QtQuick 2.0\nimport CreativeControls 1.0\n"
                             "Item { anchors.fill: parent\n"
                             "property alias fragmentShader: s.fragmentShader\n"
                             "ShaderEffect { anchors.fill: parent; antialiasing: true; \n";

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
        shaderText += "Timer { running: true; interval: 1; repeat: true; onTriggered: s.qt_TIME+=0.001 }\n";
        shaderText += "}\n"
                      "Container { \n"
                      "  height: col.childrenRect.height + 2 * radius; "
                      "  Column { "
                      "    id: col; \n"
                      "    anchors.fill: parent;\n";

        for(auto& inp : d.inputs)
        {
            shaderText += std::visit(create_control_visitor{}, inp);
        }

        shaderText += "}}}";
        QQmlComponent shaderComp(&m_app);
        shaderComp.setData(shaderText.toUtf8(), QUrl{});
        qDebug() << shaderComp.errorString();
        if(m_currentComponent)
            m_currentComponent->deleteLater();
        m_currentComponent = (QQuickItem*)shaderComp.create();

        frag.replace("RENDERSIZE", "qt_RENDERSIZE");
        frag.replace("TIME", "qt_TIME");

        QQmlProperty fragProp(m_currentComponent, "fragmentShader");
        fragProp.write(frag);

        m_currentComponent->setParentItem(&m_rect);
        qDebug() << m_currentComponent << &m_rect;
        }
        catch(...) {
        }
    }

private:
    QQuickItem& m_root;
    QQuickItem& m_rect;
    QQmlApplicationEngine& m_app;
    QQuickItem* m_currentComponent{};
};


}
