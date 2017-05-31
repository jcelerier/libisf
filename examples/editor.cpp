
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

    QQuickWidget qw(QUrl("../libisf/examples/Editor.qml"));
    l.addWidget(&qw);
    qw.setResizeMode(QQuickWidget::SizeRootObjectToView);


    QQuickItem* rect = qw.rootObject();
    isf::ShaderEditor se{ed, *rect, *qw.engine()};
    //QQuickItem* root = e.rootObjects()[0]->findChild<QQuickItem*>("editor");
    QTimer::singleShot(10, [&] {

    auto item = new isf::Item;
    item->setVertexShader(R"_(attribute highp vec2 position;
                          uniform vec2 RENDERSIZE;
                          varying vec2 isf_FragNormCoord;
                          uniform highp mat4 qt_Matrix;

                         void main() {
                             gl_Position = qt_Matrix * vec4( position, 0.0, 1.0 );
                             isf_FragNormCoord = vec2((gl_Position.x+1.0)/2.0, (gl_Position.y+1.0)/2.0);
                         })_");
    item->setFragmentShader(R"_(
                            uniform lowp float qt_Opacity;
                             varying vec2 isf_FragNormCoord;
                            void main(void)
                            {
                               gl_FragColor = vec4(isf_FragNormCoord.x, isf_FragNormCoord.y, 0.5, qt_Opacity);
                            })_");
    item->setParentItem(rect);
    QObject::connect(rect, &QQuickItem::widthChanged, item, [=] { item->setWidth(rect->width()); });
    QObject::connect(rect, &QQuickItem::heightChanged, item, [=] { item->setHeight(rect->height()); });
    item->setWidth(rect->width());
    item->setHeight(rect->height());
    });
    w.show();
    return app.exec();

}
