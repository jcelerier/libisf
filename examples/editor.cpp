
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QTimer>
#include <QMainWindow>
#include <QApplication>
#include <QHBoxLayout>
#include <QQuickWidget>
#include "editor.hpp"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QMainWindow w;
    QWidget c;
    QHBoxLayout l(&c);
    w.setCentralWidget(&c);

    isf::Edit ed{&c};
    ed.setMaximumWidth(400);
    l.addWidget(&ed);

    QQuickWidget qw;

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

    item.setVertexShader(R"_(attribute highp vec2 position;
                          uniform vec2 RENDERSIZE;
                          varying vec2 isf_FragNormCoord;
                          uniform highp mat4 qt_Matrix;

                          void main() {
                          gl_Position = qt_Matrix * vec4( position, 0.0, 1.0 );
                          isf_FragNormCoord = vec2((gl_Position.x+1.0)/2.0, (gl_Position.y+1.0)/2.0);
                          })_");
    ed.setShader(R"_(
/* { } */
uniform sampler2D myTex;
void main(void)
{
  gl_FragColor = vec4(isf_FragNormCoord.x, isf_FragNormCoord.y, 0.5, 1.);
}
)_");
    item.setData(ed.shader(), {});

    item.setParentItem(rect);
    QObject::connect(rect, &QQuickItem::widthChanged, &item, [&] { item.setWidth(rect->width()); });
    QObject::connect(rect, &QQuickItem::heightChanged, &item, [&] { item.setHeight(rect->height()); });
    item.setWidth(rect->width());
    item.setHeight(rect->height());

    isf::ShaderEditor se{ed, item, *rect, *qw.engine()};
    se.setShader(ed.shader());

    w.show();
    return app.exec();

}

uintptr_t isf::Material::m_ptr = 0;

void isf::Shader::updateState(const QSGMaterialShader::RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    if (state.isMatrixDirty())
        program()->setUniformValue(m_id_matrix, state.combinedMatrix());

    State& ns = static_cast<Material*>(newMaterial)->state();

    const int N = m_uniforms.size();

    if(ns.size() == N)
    {
        for(int i = 0; i < N; i++)
        {
            std::visit([=] (const auto& val) {
                const auto u = m_uniforms[i];
                if(u >= 0)
                {
                     program()->setUniformValue(u, val);
                }
            }, ns[i]);
        }
    }

    m_texture.allocateStorage();
    m_texture.bind(0);
    auto tex =program()->uniformLocation("inputImage");
    qDebug() << tex;
    if(tex >= 0)
    {
    }
}
