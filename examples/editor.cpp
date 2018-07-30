
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QTimer>
#include <QMainWindow>
#include <QApplication>
#include <QHBoxLayout>
#include <QQuickWidget>
#include <QFileDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSurfaceFormat>
#include "editor.hpp"

int main(int argc, char** argv)
{
    QSurfaceFormat f = QSurfaceFormat::defaultFormat();
    f.setVersion(4, 1);
    f.setProfile(QSurfaceFormat::CoreProfile);
    f.setDefaultFormat(f);
    QApplication app(argc, argv);
    QMainWindow w;
    QWidget c;
    QHBoxLayout l(&c);
    w.setCentralWidget(&c);

    QVBoxLayout left;
    QHBoxLayout btns;
    left.addLayout(&btns);
    QPushButton loadImage{"Load Texture"};
    QPushButton loadShader{"Load Shader"};
    QPushButton loadCamera{"Use Camera"};
    btns.addWidget(&loadImage);
    btns.addWidget(&loadShader);
    btns.addWidget(&loadCamera);

    isf::Edit ed{&c};
    ed.setMaximumWidth(700);
    left.addWidget(&ed);

    QPlainTextEdit errText;
    errText.setMaximumWidth(700);
    errText.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    static QPlainTextEdit& ref = errText;
    /*
    qInstallMessageHandler([] (QtMsgType type, const QMessageLogContext& context, const QString& msg) {
        static QStringList strlist;
        strlist.append(msg);
        if(strlist.size() > 2)
            strlist.removeFirst();
        ref.setPlainText(strlist.join("\n===============\n"));
    });
    */
    left.addWidget(&errText);

    l.addLayout(&left);

    QQuickWidget qw;
    qw.setMinimumWidth(100);
    qw.setMinimumHeight(100);

    QQmlComponent source{qw.engine()};
    source.setData(R"_(
                   import QtQuick 2.7
                   Rectangle
                   {
                       color: "black"
                       objectName: "rect"
                   }
                   )_", QUrl{});

    qw.setContent(QUrl{}, &source, source.create());
    l.addWidget(&qw);
    qw.setResizeMode(QQuickWidget::SizeRootObjectToView);

    QQuickItem* rect = qw.rootObject();

    isf::ShaderItem item;

    item.setVertexShader(R"_(#version 330
in highp vec2 position;
uniform vec2 RENDERSIZE;
out vec2 isf_FragNormCoord;
uniform highp mat4 qt_Matrix;

void main() {
  gl_Position = qt_Matrix * vec4( position, 0.0, 1.0 );
  isf_FragNormCoord = vec2((gl_Position.x+1.0)/2.0, (gl_Position.y+1.0)/2.0);
})_");
    ed.setShader(R"_(/* { } */
out vec4 color;
void main(void)
{
  color = vec4(1., 1., 0.5, 1.);
}
)_");
    item.setData(ed.shader(), {});

    item.setParentItem(rect);
    QObject::connect(rect, &QQuickItem::widthChanged, &item, [&] { item.setWidth(rect->width()); });
    QObject::connect(rect, &QQuickItem::heightChanged, &item, [&] { item.setHeight(rect->height()); });
    item.setWidth(rect->width());
    item.setHeight(rect->height());
    qDebug() << rect->boundingRect() << item.boundingRect();

    QObject::connect(&loadImage, &QPushButton::clicked, [&] {
        auto file = QFileDialog::getOpenFileName(nullptr, "Load file", {}, {}, {});
        QFile f(file);
        if(f.exists())
        {
            item.setTexture(f);
        }
    });
    QObject::connect(&loadCamera, &QPushButton::clicked, [&] {
        item.setCamera();
    });
    QObject::connect(&loadShader, &QPushButton::clicked, [&] {
        auto file = QFileDialog::getOpenFileName(nullptr, "Load file", {}, {}, {});
        QFile f(file);
        if(f.exists() && f.open(QIODevice::ReadOnly))
        {
            QString s = f.readAll();
            ed.setShader(s);
        }
    });
    isf::ShaderEditor se{item, *rect, *qw.engine()};

    QObject::connect(&ed, &isf::Edit::shaderChanged,
            &se, &isf::ShaderEditor::setShader);

    se.setShader(ed.shader());

    w.show();
    return app.exec();

}

uintptr_t isf::Material::m_ptr = 0;

void isf::Shader::updateState(const QSGMaterialShader::RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    if (state.isMatrixDirty())
        program()->setUniformValue(m_id_matrix, state.combinedMatrix());

    auto mat = static_cast<Material*>(newMaterial);
    State& ns = mat->state();

    const int N = m_uniforms.size();

    if(ns.size() == N)
    {
        for(int i = 0; i < N; i++)
        {
            eggs::variants::apply([=] (const auto& val) {
                const auto u = m_uniforms[i];
                if(u >= 0)
                {
                     program()->setUniformValue(u, val);
                }
            }, ns[i]);
        }
    }
    if(oldMaterial)
    {
        auto oldmat = static_cast<Material*>(oldMaterial);
        if(oldmat->texture() != mat->texture())
        {
            setTexture(mat->texture());
        }
    }
    else
    {
        setTexture(mat->texture());
    }

    if(m_texture.isCreated())
        m_texture.bind(0);
}
