#pragma once

#include <QQuickItem>
#include <QQmlProperty>
#include <QQmlApplicationEngine>
#include <QSGNode>
#include <QTextEdit>
#include <QSGSimpleMaterialShader>
#include <QVBoxLayout>
#include "../src/isf.hpp"
namespace isf
{
using value_type = std::variant<bool, GLfloat, GLint, QVector2D, QVector3D, QVector4D>;

struct create_control_visitor
{
    const input& i;
    QString operator()(const float_input& t)
    {
        auto prop = QString::fromStdString(i.name);
        auto label = QString::fromStdString(i.label);
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
    QString operator()(const long_input& t)
    {
        auto prop = QString::fromStdString(i.name);
        auto label = QString::fromStdString(i.label);
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
        auto prop = QString::fromStdString(i.name);
        auto label = QString::fromStdString(i.label);
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
        auto prop = QString::fromStdString(i.name);
        auto label = QString::fromStdString(i.label);
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
        auto prop = QString::fromStdString(i.name);
        auto label = QString::fromStdString(i.label);
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
        auto prop = QString::fromStdString(i.name);
        auto label = QString::fromStdString(i.label);
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

struct State
{
    std::vector<value_type> values;
};


class Shader final : public QSGSimpleMaterialShader<State>
{
public:
    Shader(std::string vert, std::string frag, descriptor d)
        : m_vertex{std::move(vert)}
        , m_fragment{std::move(frag)}
        , m_desc{std::move(d)}
    {
        m_attrs.push_back("position");
    }

    const char* vertexShader() const override
    {
        return m_vertex.c_str();
    }

    const char* fragmentShader() const override
    {
        return m_fragment.c_str();
    }

    QList<QByteArray> attributes() const override
    {
        return m_attrs;
    }

    void updateState(const State *state, const State *) override
    {
        const int N = m_uniforms.size();

        if(state->values.size() == N)
        {
        for(int i = 0; i < N; i++)
        {
            std::visit([=] (const auto& val) {
                program()->setUniformValue(m_uniforms[i], val);
            }, state->values[i]);
        }
        }
        else
        {
            qDebug()<< state->values.size() << N;
        }
    }

    void resolveUniforms() override
    {
        m_uniforms.clear();
        for(const isf::input& inp : m_desc.inputs)
        {
            auto ba = QByteArray::fromStdString(inp.name);
            m_uniforms.push_back(program()->uniformLocation(ba));
            m_attrs.push_back(std::move(ba));
        }
    }

private:
    const std::string m_vertex;
    const std::string m_fragment;
    const descriptor m_desc;
    std::vector<int> m_uniforms;
    QList<QByteArray> m_attrs;
};

template <typename State>
class Material final : public QSGMaterial
{
public:
    Material(std::string vert, std::string frag, descriptor d)
        : m_vertex{std::move(vert)}
        , m_fragment{std::move(frag)}
        , m_desc{std::move(d)}
    {
    }


    QSGMaterialShader* createShader() const override
    {
        return new Shader{m_vertex, m_fragment, m_desc};
    }

    QSGMaterialType* type() const override
    {
        static QSGMaterialType type;
        return &type;
    }

    const State& state() const { return m_state; }
    State& state() { return m_state; }

private:
    State m_state;

    std::string m_vertex;
    std::string m_fragment;
    descriptor m_desc;
};

class ColorNode : public QSGGeometryNode
{
public:
    ColorNode(std::string vert, std::string frag, descriptor d)
        : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
    {
        setGeometry(&m_geometry);

        auto material = new Material<State>(vert, frag, d);
        material->setFlag(QSGMaterial::Blending);
        setMaterial(material);
        setFlag(OwnsMaterial);
    }

    QSGGeometry m_geometry;
};

class Item : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QString vertexShader READ vertexShader WRITE setVertexShader NOTIFY vertexShaderChanged)
    Q_PROPERTY(QString fragmentShader READ fragmentShader WRITE setFragmentShader NOTIFY fragmentShaderChanged)
public:
    Item()
    {
        setFlag(ItemHasContents, true);
    }


    void setDescriptor(const isf::descriptor& d)
    {
        m_desc = d;
        m_desc.inputs.push_back({"RENDERSIZE", "", point2d_input{}});
        m_desc.inputs.push_back({"TIME", "", float_input{}});
        m_desc.inputs.push_back({"TIMEDELTA", "", float_input{}});
        m_desc.inputs.push_back({"PASSINDEX", "", long_input{}});
        m_desc.inputs.push_back({"DATE", "", color_input{}});

        m_variables.resize(m_desc.inputs.size());
        // todo set default values
        update();
    }

    QString vertexShader() const
    {
        return m_vertexShader;
    }

    QString fragmentShader() const
    {
        return m_fragmentShader;
    }

signals:
    void vertexShaderChanged(QString vertexShader);
    void fragmentShaderChanged(QString fragmentShader);

public slots:
    void setVertexShader(QString vertexShader)
    {
        if (m_vertexShader == vertexShader)
            return;

        m_vertexShader = vertexShader;
        m_vertexDirty = true;
        emit vertexShaderChanged(m_vertexShader);
        update();
    }

    void setFragmentShader(QString fragmentShader)
    {
        if (m_fragmentShader == fragmentShader)
            return;

        m_fragmentShader = fragmentShader;
        m_fragmentDirty = true;
        emit fragmentShaderChanged(m_fragmentShader);
        update();
    }

private:
    ColorNode* makeNode()
    {
        if(!m_vertexShader.isEmpty() && !m_fragmentShader.isEmpty())
          return new ColorNode{m_vertexShader.toStdString(), m_fragmentShader.toStdString(), m_desc};
        return nullptr;
    }
      QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
      {
          ColorNode *n = static_cast<ColorNode *>(node);
          if (!node)
          {
              n = makeNode();
          }
          else
          {
              if(m_vertexDirty || m_fragmentDirty)
              {
                  delete n;
                  n = makeNode();
              }
          }

          m_vertexDirty = false;
          m_fragmentDirty = false;

          if(n)
          {
              QSGGeometry::updateTexturedRectGeometry(n->geometry(), boundingRect(), QRectF(0, 0, 1, 1));
              static_cast<Material<State>*>(n->material())->state().values = m_variables;

              n->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
          }
          return n;
      }

  QColor m_color;

  QString m_vertexShader;
  QString m_fragmentShader;
  descriptor m_desc;
  std::atomic_bool m_vertexDirty{false};
  std::atomic_bool m_fragmentDirty{false};

  std::vector<value_type> m_variables;
};


class ShaderEditor : public QObject
{
    Q_OBJECT
public:
    ShaderEditor(QObject& root, QQuickItem& rect, QQmlEngine& app):
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

        for(const isf::input& t : d.inputs)
        {
            auto prop = QString::fromStdString(t.name);
            auto prop_uc = prop.toUpper();
            auto prop_lc = prop.toLower();
            shaderFx += "property var " + prop + "\n";
            if(prop == prop_uc)
                frag.replace(prop_uc, prop_lc);
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

        for(const isf::input& inp : d.inputs)
        {
            shaderFx += std::visit(create_control_visitor{inp}, inp.data);
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
QObject& m_root;
QQuickItem& m_rect;
QQmlEngine& m_app;
QQuickItem* m_currentComponent{};
};

struct Edit : public QWidget

{
    Q_OBJECT
    QVBoxLayout m_lay;
    QTextEdit m_edit;
public:
    Edit(QWidget* parent)
        : QWidget{parent}
        , m_lay{this}
    {
        m_lay.addWidget(&m_edit);

        connect(&m_edit, &QTextEdit::textChanged, this,
                [=] { shaderChanged(m_edit.document()->toPlainText()); });
    }

signals:
    void shaderChanged(QString);
};


}
