#pragma once
#include "../src/isf.hpp"

#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQmlProperty>
#include <QSGNode>
#include <QVBoxLayout>
#include <QSGSimpleMaterialShader>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
namespace isf
{
using value_type = std::variant<bool, GLfloat, GLint, QVector2D, QVector3D, QVector4D, QColor>;

struct create_control_visitor
{
    const input& i;
    const int index;
    QString operator()(const float_input& t)
    {
        auto prop = QString::fromStdString(i.name);
        auto label = QString::fromStdString(i.label);
        return QString(R"_(
                    Text {
                      text: '%2';
                      color: 'white';
                    }
                    Slider {
                      mapFunc: function(v) { return %3 + v * (%4 - %3) };
                      initialValue: (%5 - %3) / (%4 - %3);
                      orientation: Qt.horizontal;
                      height: 30;
                      onValueChanged: s.shader.setControl(%1, value);
                    }
                  )_").arg(index).arg(label.isEmpty() ? prop : label).arg(t.min).arg(t.max).arg(t.def);
    }
    QString operator()(const long_input& t)
    {
        auto prop = QString::fromStdString(i.name);
        auto label = QString::fromStdString(i.label);
        return QString(R"_(
                    Text {
                      text: '%2';
                      color: 'white';
                    }
                    Slider {
                      mapFunc: function(v) { return %3 + v * (%4 - %3) };
                      initialValue: (%5 - %3) / (%4 - %3);
                      orientation: Qt.horizontal;
                      height: 30;
                      onValueChanged: s.shader.setControl(%1, value);
                    }
                  )_").arg(index).arg(label.isEmpty() ? prop : label).arg(t.min).arg(t.max).arg(t.def);
    }
    QString operator()(const bool_input& t)
    {
        auto prop = QString::fromStdString(i.name);
        auto label = QString::fromStdString(i.label);
        return QString(R"_(
                    Text {
                      text: '%2';
                      color: 'white';
                    }
                    Switch {
                      state: %3;
                      onStateChanged: s.shader.setControl(%1, state == 'ON');
                    }
                  )_").arg(index).arg(label.isEmpty() ? prop : label).arg(t.def ? "'ON'" : "'OFF'");
    }
    QString operator()(const event_input& t)
    {
        auto prop = QString::fromStdString(i.name);
        auto label = QString::fromStdString(i.label);
        return QString(
                  R"_(
                    Text {
                      text: '%3';
                      color: 'white';
                    }
                    Switch {
                      id: %2_sw;
                      ease: false;
                      Timer { interval: 16; onTriggered: if(%2_sw.state == 'ON') %2_sw.state = 'OFF'; id: tmr }
                      onStateChanged: { s.shader.setControl(%1, state == 'ON'); tmr.start() }
                    }
                  )_").arg(index).arg(prop).arg(label.isEmpty() ? prop : label);
    }
    QString operator()(const point2d_input& t)
    {
        auto prop = QString::fromStdString(i.name);
        auto label = QString::fromStdString(i.label);
        return QString(
                  R"_(
                    Text {
                      text: '%2';
                      color: 'white';
                    }
                    XYPad {
                      property real minX: %3;
                      property real maxX: %4;
                      property real minY: %5;
                      property real maxY: %6;
                      stickX: %7;
                      stickY: %8;
                      onStickXChanged: s.shader.setControl(%1, Qt.point(minX + stickX * (maxX - minX), minY + stickY * (maxY - minY)));
                      onStickYChanged: s.shader.setControl(%1, Qt.point(minX + stickX * (maxX - minX), minY + stickY * (maxY - minY)));
                    }
                  )_").arg(index).arg(label.isEmpty() ? prop : label)
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
        return QString(R"_(
                    Text {
                      text: '%2';
                      color: 'white';
                    }
                    RGBSlider {
                      onColorChanged: s.shader.setControl(%1, color);
                    }
                   )_").arg(index).arg(label.isEmpty() ? prop : label);
    }
    QString operator()(const image_input& i)
    {
        return {};
    }

};

class Shader final : public QSGMaterialShader
{
    QOpenGLTexture             m_texture;

public:
    Shader(std::string vert, std::string frag, descriptor d)
        : m_vertex{std::move(vert)}
        , m_fragment{std::move(frag)}
        , m_desc{std::move(d)}
        , m_texture{ QImage("/home/jcelerier/Images/poules.png") }
    {
    }

    void initialize() override
    {
        QSGMaterialShader::initialize();

        m_id_matrix = program()->uniformLocation("qt_Matrix");
        if (m_id_matrix < 0) {
            qFatal("QSGSimpleMaterialShader does not implement 'uniform highp mat4 %s;' in its vertex shader",
                   "qt_Matrix");
        }

        for(const isf::input& inp : m_desc.inputs)
        {
            auto cstr = inp.name.c_str();
            m_uniforms.push_back(program()->uniformLocation(cstr));
        }
    }

    void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

    char const *const *attributeNames() const override
    {
        static const char* attributes[] = { "position", nullptr };
        return attributes;
    }

    const char* vertexShader() const override
    {
        return m_vertex.c_str();
    }

    const char* fragmentShader() const override
    {
        return m_fragment.c_str();
    }

private:
    std::string m_vertex;
    std::string m_fragment;
    const descriptor m_desc;
    std::vector<int> m_uniforms;
    mutable QVector<const char*> m_attrs;
    int m_id_matrix;
};

using State = std::vector<value_type>;

class Material final : public QSGMaterial
{
    static uintptr_t m_ptr;
public:
    Material(std::string vert, std::string frag, descriptor d)
        : m_vertex{std::move(vert)}
        , m_fragment{std::move(frag)}
        , m_desc{std::move(d)}
    {
        m_ptr++;
    }

    QSGMaterialShader* createShader() const override
    { return new Shader{m_vertex, m_fragment, m_desc}; }

    QSGMaterialType* type() const override
    {
        QSGMaterialType* t = (QSGMaterialType*)m_ptr;
        return t;
    }

    const State& state() const { return m_state; }
    State& state() { return m_state; }

private:
    mutable QSGMaterialType m_type;
    State m_state;

    std::string m_vertex;
    std::string m_fragment;
    descriptor m_desc;
};

class ShaderNode final : public QSGGeometryNode
{
public:
    ShaderNode(std::string vert, std::string frag, descriptor d)
        : m_geometry{QSGGeometry::defaultAttributes_TexturedPoint2D(), 4}
        , m_mater{std::move(vert), std::move(frag), std::move(d)}
    {
        m_mater.setFlag(QSGMaterial::Blending);
        setGeometry(&m_geometry);
        setMaterial(&m_mater);
    }
private:
    QSGGeometry m_geometry;
    Material m_mater;
};

class ShaderItem final : public QQuickItem
{
    Q_OBJECT

public:
    ShaderItem()
    {
        setFlag(ItemHasContents, true);
        connect(this, &QQuickItem::heightChanged,
                this, [=] {
            if(m_renderSize != -1)
                m_variables[m_renderSize] = QVector2D{(float)width(), (float)height()};
            update();
        });
        connect(this, &QQuickItem::widthChanged,
                this, [=] {
            if(m_renderSize != -1)
                m_variables[m_renderSize] = QVector2D{(float)width(), (float)height()};
            update();
        });
    }

    void timerEvent(QTimerEvent *event) override
    {
        m_curTime += 0.008;
        m_variables[m_time] = m_curTime;
        m_variables[m_timeDelta] = 0.008f;
        m_variables[m_renderSize] = QVector2D{(float)width(), (float)height()};
        update();
        // TODO date
    }

    std::optional<int> m_timer;
    float m_lastTime{};
    float m_curTime{};

    int m_renderSize{-1};
    int m_time{-1};
    int m_timeDelta{-1};
    int m_passIndex{-1};
    int m_date{-1};
    void setData(QString fragment, const isf::descriptor& d)
    {
        if(m_timer)
            killTimer(*m_timer);

        m_desc = d;

        m_renderSize = m_desc.inputs.size();
        m_desc.inputs.push_back({"RENDERSIZE", "", point2d_input{}});
        m_time = m_desc.inputs.size();
        m_desc.inputs.push_back({"TIME", "", float_input{}});
        m_timeDelta = m_desc.inputs.size();
        m_desc.inputs.push_back({"TIMEDELTA", "", float_input{}});
        m_passIndex = m_desc.inputs.size();
        m_desc.inputs.push_back({"PASSINDEX", "", long_input{}});
        m_date = m_desc.inputs.size();
        m_desc.inputs.push_back({"DATE", "", color_input{}});

        m_variables.resize(m_desc.inputs.size());
        m_variables[m_passIndex] = 0;

        const auto N = m_desc.inputs.size();
        m_variables.resize(N);


        struct set_default_visitor
        {
            value_type& val;
            void operator()(const float_input& i)
            { val = (GLfloat)i.def; }
            void operator()(const long_input& i)
            { val = (GLint)i.def; }
            void operator()(const event_input& i)
            { }
            void operator()(const bool_input& i)
            { val = i.def; }
            void operator()(const point2d_input& i)
            {
                QVector2D arr;
                arr[0] = i.def[0];
                arr[1] = i.def[1];
                val = arr;
            }
            void operator()(const point3d_input& i)
            {
                QVector3D arr;
                arr[0] = i.def[0];
                arr[1] = i.def[1];
                arr[2] = i.def[2];
                val = arr;
            }
            void operator()(const color_input& i)
            {
                QVector4D arr;
                arr[0] = i.def[0];
                arr[1] = i.def[1];
                arr[2] = i.def[2];
                arr[3] = i.def[3];
                val = arr;
            }
            void operator()(const image_input& i)
            { }
        };

        for(std::size_t i = 0; i < N; i++)
        {
            std::visit(set_default_visitor{m_variables[i]}, m_desc.inputs[i].data);
        }

        m_timer = startTimer(8, Qt::TimerType::CoarseTimer);

        m_fragmentShader = fragment;
        m_fragmentDirty = true;
        emit fragmentShaderChanged(m_fragmentShader);
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
    void setControl(int res, QVariant v)
    {
        if(res < m_variables.size())
        {
            switch(v.type())
            {
            case QVariant::Int: m_variables[res] = (GLint)v.toInt(); break;
            case QVariant::LongLong: m_variables[res] = (GLint)v.toLongLong(); break;
            case QVariant::Double: m_variables[res] = (GLfloat)v.toDouble(); break;
            case QVariant::Color: m_variables[res] = v.value<QColor>(); break;
            case QVariant::Vector2D: m_variables[res] = v.value<QVector2D>(); break;
            case QVariant::Vector3D: m_variables[res] = v.value<QVector3D>(); break;
            case QVariant::Vector4D: m_variables[res] = v.value<QVector4D>(); break;
            default: break;
            }
            update();
        }
    }

    void setVertexShader(QString vertexShader)
    {
        if (m_vertexShader == vertexShader)
            return;

        m_vertexShader = vertexShader;
        m_vertexDirty = true;
        emit vertexShaderChanged(m_vertexShader);
        update();
    }

private:
    ShaderNode* makeNode()
    {
        if(!m_vertexShader.isEmpty() && !m_fragmentShader.isEmpty())
            return new ShaderNode{m_vertexShader.toStdString(), m_fragmentShader.toStdString(), m_desc};
        return nullptr;
    }
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
    {
        ShaderNode *n = static_cast<ShaderNode *>(node);
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
            static_cast<Material*>(n->material())->state() = m_variables;
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
    ShaderEditor(QObject& root, ShaderItem& shader, QQuickItem& rect, QQmlEngine& app):
        m_root{root}
      , m_shader{shader}
      , m_rect{rect}
      , m_app{app}
    {
        connect(&m_root, SIGNAL(shaderChanged(QString)),
                this, SLOT(setShader(QString)));
    }
public slots:
    void setShader(QString shader)
    {
        try {
            isf::parser s{shader.toStdString()};
            auto d = s.data();

            QString controls =
                    R"_(
                    import QtQuick 2.0
                    import CreativeControls 1.0

                    Container {
                      id: s
                      height: col.childrenRect.height + 2 * radius;
                      width: col.childrenRect.width+ 2 * radius;

                      property var shader;
                      Column {
                        id: col;
                        anchors.fill: parent;

                    )_";

            int i = 0;
            for(const isf::input& inp : d.inputs)
            {
                controls += std::visit(create_control_visitor{inp, i}, inp.data);
                i++;
            }

            controls += "}}";

            QQmlComponent controlsComp(&m_app);
            controlsComp.setData(controls.toUtf8(), QUrl{});

            if(m_currentComponent)
                m_currentComponent->deleteLater();
            m_currentComponent = (QQuickItem*)controlsComp.create();

            m_currentComponent->setParentItem(&m_rect);
            QQmlProperty shaderProp(m_currentComponent, "shader");
            shaderProp.write(QVariant::fromValue(&m_shader));

            m_shader.setData(QString::fromStdString(s.fragment().back()), d);

        }
        catch(...) {
        }
    }

private:
    QObject& m_root;
    ShaderItem& m_shader;
    QQuickItem& m_rect;
    QQmlEngine& m_app;
    QQuickItem* m_currentComponent{};
};

struct Edit : public QWidget
{
    Q_OBJECT
    QVBoxLayout m_lay;
    KTextEditor::Editor *m_edit = KTextEditor::Editor::instance();
    KTextEditor::Document *m_doc = m_edit->createDocument(this);
    KTextEditor::View *m_view = m_doc->createView(this);
public:
    Edit(QWidget* parent)
        : QWidget{parent}
        , m_lay{this}
    {

        // Note: see  https://api.kde.org/frameworks/ktexteditor/html/classKTextEditor_1_1ConfigInterface.htm
        // create a new document
        // create a widget to display the document
        m_lay.addWidget(m_view);
        m_doc->setHighlightingMode("GLSL");
        connect(m_doc, &KTextEditor::Document::textChanged, this,
                [=] (auto) { shaderChanged(m_doc->text()); });
    }

    void setShader(QString s) { m_doc->setText(s); }
    QString shader() const { return m_doc->text(); }
signals:
    void shaderChanged(QString);
};


}
