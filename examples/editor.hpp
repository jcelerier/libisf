#pragma once
#include "../src/isf.hpp"

#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQmlProperty>
#include <QSGNode>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QSGSimpleMaterialShader>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QAbstractVideoSurface>
#include <QImageReader>
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
        QString model = "[";
        QString values = "[";
        for(int i = 0; i < t.labels.size(); i++)
        {
            const auto& label_i = t.labels[i];
            auto val_i = QString::number(t.values[i]);

            // Text
            if(!label_i.empty())
                model += '"' + QString::fromStdString(label_i) + '"';
            else
                model += '"' + val_i + '"';

            // Value
            values += val_i;

            if(i != t.labels.size() - 1)
            {
                model += ", ";
                values += ", ";
            }
        }
        model += "]";
        values += "]";

        return QString(R"_(
                    Text {
                      text: '%2';
                      color: 'white';
                    }
                    QC.ComboBox {
                      model: %3;
                      property var values: %4;
                      currentIndex: %5;
                      height: 30;
                      onCurrentIndexChanged: s.shader.setControl(%1, values[currentIndex]);
                    }
                  )_").arg(index).arg(label.isEmpty() ? prop : label).arg(model).arg(values).arg(t.def);
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
                .arg(t.min? (*t.min)[0] : 0.).arg(t.max ? (*t.max)[0] : 1.)
                .arg(t.min? (*t.min)[1] : 0.).arg(t.max ? (*t.max)[1] : 1.)
                .arg(t.def? (*t.def)[0] : 0.5).arg(t.def ? (*t.def)[0] : 0.5);
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
public:
    Shader(std::string vert, std::string frag, descriptor d)
        : m_vertex{std::move(vert)}
        , m_fragment{std::move(frag)}
        , m_desc{std::move(d)}
        , m_texture{ QImage("/home/jcelerier/Images/poules.png").mirrored() }
    {
    }

    void initialize() override
    {
        QSGMaterialShader::initialize();

        m_id_matrix = program()->uniformLocation("qt_Matrix");
        if (m_id_matrix < 0) {
            qDebug() << "QSGSimpleMaterialShader does not implement 'uniform highp mat4 %s;' in its vertex shader"
                     << "qt_Matrix";
        }

        for(const isf::input& inp : m_desc.inputs)
        {
            auto cstr = inp.name.c_str();
            m_uniforms.push_back(program()->uniformLocation(cstr));
        }
    }

    void setTexture(QImage m)
    {
        if(!m.isNull())
        {
            m_texture.destroy();
            m_texture.create();
            m_texture.setMipLevels(1);
            m_texture.setData(std::move(m).mirrored());
        }
    }

    void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

    char const *const *attributeNames() const override
    {
        static const char* attributes[] = { "position", nullptr };
        return attributes;
    }

    const char* vertexShader() const override
    { return m_vertex.c_str(); }

    const char* fragmentShader() const override
    { return m_fragment.c_str(); }

private:
    std::string m_vertex;
    std::string m_fragment;
    const descriptor m_desc;
    QOpenGLTexture m_texture;
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
    QImage& texture() { return m_tex; }

private:
    mutable QSGMaterialType m_type;
    State m_state;
    QImage m_tex;

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

class VideoReader : public QAbstractVideoSurface
{
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
    {
        return {QVideoFrame::Format_RGB565};
    }

    bool present(const QVideoFrame &frame)
    {
        return true;
    }
};

class ShaderItem final : public QQuickItem
{
    Q_OBJECT

public:
    ShaderItem()
    {
        setAntialiasing(true);
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
        // TODO date
        update();
    }

    std::optional<int> m_timer;
    float m_lastTime{};
    float m_curTime{};

    int m_renderSize{-1};
    int m_time{-1};
    int m_timeDelta{-1};
    int m_passIndex{-1};
    int m_date{-1};

    void setImage(QFile& f)
    {
        QImageReader img(&f);

        m_image = img.read();
        m_imageChanged = true;

    }
    void setVideo(QFile& f)
    {

    }

    void setTexture(QFile& f)
    {
        auto ext = QFileInfo(f).completeSuffix().toLower();
        QSet<QString> images{"jpg", "jpeg", "png", "bmp", "gif"};
        QSet<QString> videos{"mp4", "avi", "mkv"};
        if(images.contains(ext))
        {
            setImage(f);
        }
        else if(videos.contains(ext))
        {
            setVideo(f);
        }
    }

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
                if(i.def)
                {
                    arr[0] = (*i.def)[0];
                    arr[1] = (*i.def)[1];
                }
                else
                {
                    arr = {0.5, 0.5};
                }
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
        update();
    }

public slots:
    void setControl(int res, QVariant v)
    {
        if(res < m_variables.size())
        {
            switch(v.type())
            {
            case QVariant::Bool: m_variables[res] = (GLint)v.toBool(); break;
            case QVariant::Int: m_variables[res] = (GLint)v.toInt(); break;
            case QVariant::LongLong: m_variables[res] = (GLint)v.toLongLong(); break;
            case QVariant::Double: m_variables[res] = (GLfloat)v.toDouble(); break;
            case QVariant::Color: m_variables[res] = v.value<QColor>(); break;
            case QVariant::Point: m_variables[res] = QVector2D{v.toPoint()}; break;
            case QVariant::PointF: m_variables[res] = QVector2D{v.toPointF()}; break;
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
            auto mat =static_cast<Material*>(n->material());
            mat->state() = m_variables;
            if(m_imageChanged)
            {
                mat->texture() = m_image;
                m_imageChanged = false;
            }
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

    QImage m_image;
    bool m_imageChanged{};
};


class ShaderEditor : public QObject
{
    Q_OBJECT
public:
    ShaderEditor(ShaderItem& shader, QQuickItem& rect, QQmlEngine& app):
        m_shader{shader}
      , m_rect{rect}
      , m_app{app}
    {
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
                    import QtQuick.Controls 2.0 as QC

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
            if(m_currentComponent)
            {
                m_currentComponent->setParentItem(&m_rect);
                QQmlProperty shaderProp(m_currentComponent, "shader");
                shaderProp.write(QVariant::fromValue(&m_shader));
            }
            else
            {
                qDebug() << controlsComp.errorString();
            }
            m_shader.setData(QString::fromStdString(s.fragment()), d);

        }
        catch(...) {
        }
    }

private:
    ShaderItem& m_shader;
    QQuickItem& m_rect;
    QQmlEngine& m_app;
    QQuickItem* m_currentComponent{};
};

struct Edit : public QWidget
{
    Q_OBJECT
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

private:
    QVBoxLayout m_lay;
    KTextEditor::Editor *m_edit = KTextEditor::Editor::instance();
    KTextEditor::Document *m_doc = m_edit->createDocument(this);
    KTextEditor::View *m_view = m_doc->createView(this);
};
}
