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
                    "  onValueChanged: s.%1 = value "
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
                    "  onStateChanged: if(state == 'ON') s.%1 = true; else s.%1 = false;"
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
                    "  id: %1_sw;"
                    "  ease: false;"
                    "  Timer { interval: 16; onTriggered: if(%1_sw.state == 'ON') %1_sw.state = 'OFF'; id: tmr }"
                    "  onStateChanged: { if(state == 'ON') {s.%1 = true;} else {s.%1 = false;} tmr.start() }"
                    "}\n").arg(prop).arg(label.isEmpty() ? prop : label);
    }
    QString operator()(const point2d_input& t)
    {
        auto prop = QString::fromStdString(t.name);
        auto label = QString::fromStdString(t.label);
        return QString(
                    "Text {"
                    "  text: '%2';"
                    "  color: 'white';"
                    "}\n"
                    "XYPad {"
                    "  property real minX: %3;"
                    "  property real maxX: %4;"
                    "  property real minY: %5;"
                    "  property real maxY: %6;"
                    "  stickX: %7;"
                    "  stickY: %8;"
                    "  onStickXChanged: s.%1 = [ minX + stickX * (maxX - minX), minY + stickY * (maxY - minY)];"
                    "  onStickYChanged: s.%1 = [ minX + stickX * (maxX - minX), minY + stickY * (maxY - minY)];"
                    "}\n").arg(prop).arg(label.isEmpty() ? prop : label)
                          .arg(t.min[0]).arg(t.max[0])
                          .arg(t.min[1]).arg(t.max[1])
                          .arg(t.def[0]).arg(t.def[1]);
    }
    QString operator()(const point3d_input& i)
    {
        return {};
    }
    QString operator()(const color_input& t)
    {
        auto prop = QString::fromStdString(t.name);
        auto label = QString::fromStdString(t.label);
        return QString(
                    "Text {"
                    "  text: '%2';"
                    "  color: 'white';"
                    "}\n"
                    "RGBSlider {"
                    "  onColorChanged: s.%1 = color;"
                    "}\n").arg(prop).arg(label.isEmpty() ? prop : label);
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
        QString shaderFx = "import QtQuick 2.0\nimport CreativeControls 1.0\n"
                             "Item { anchors.fill: parent\n"
                             "property alias fragmentShader: s.fragmentShader\n"
                             "ShaderEffect { anchors.fill: parent; antialiasing: true; \n";

        auto frag = QString::fromStdString(s.fragment().back());
        auto vert = QString::fromStdString(s.vertex().back());

        for(auto& inp : d.inputs)
        {
            std::visit([&] (const auto& t) {
                auto prop = QString::fromStdString(t.name);
                auto prop_uc = prop.toUpper();
                auto prop_lc = prop.toLower();
                shaderFx += "property var " + prop + "\n";
                if(prop == prop_uc)
                    frag.replace(prop_uc, prop_lc);
            }, inp);
        }

        shaderFx += "property point qt_RENDERSIZE: Qt.point(width, height)\n";
        shaderFx += "property real qt_TIME\n ";
        shaderFx += "property real qt_TIMEDELTA: 0.001\n ";
        shaderFx += "property int qt_PASSINDEX: 0\n ";
        shaderFx += "property var qt_DATE: [0, 0, 0, 0]\n ";
        shaderFx += "id: s\n ";
        shaderFx += "Timer { running: true; interval: 1; repeat: true; onTriggered: s.qt_TIME+=0.001 }\n";
        shaderFx += "}\n"
                      "Container { \n"
                      "  height: col.childrenRect.height + 2 * radius; "
                      "  width: col.childrenRect.width+ 2 * radius; "
                      "  Column { "
                      "    id: col; \n"
                      "    anchors.fill: parent;\n";

        for(auto& inp : d.inputs)
        {
            shaderFx += std::visit(create_control_visitor{}, inp);
        }

        shaderFx += "}}}";
        QQmlComponent shaderComp(&m_app);
        shaderComp.setData(shaderFx.toUtf8(), QUrl{});

        if(m_currentComponent)
            m_currentComponent->deleteLater();
        m_currentComponent = (QQuickItem*)shaderComp.create();

        frag.replace("RENDERSIZE", "qt_RENDERSIZE");
        frag.replace("TIME", "qt_TIME");
        frag.replace("PASSINDEX", "qt_PASSINDEX");
        frag.replace("DATE", "qt_DATE");

        QQmlProperty fragProp(m_currentComponent, "fragmentShader");
        fragProp.write(frag);

        vert.replace("RENDERSIZE", "qt_RENDERSIZE");
        QQmlProperty vertProp(m_currentComponent, "vertexShader");
        vertProp.write(vert);

        m_currentComponent->setParentItem(&m_rect);
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
