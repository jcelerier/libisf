#pragma once
#include <editor/code_editor.hpp>
#include <editor/shader_item.hpp>
#include <editor/control_factory.hpp>
#include <QMainWindow>
#include <QHBoxLayout>
#include <QQuickWidget>
#include <QFileDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QTimer>
#include <QSurfaceFormat>
namespace isf_editor
{
//! Main window of the editor
class Window final
    : public QMainWindow
{
public:
  Window()
  {
    // Set-up log message handling
    {
      static QPlainTextEdit& ref = m_errorText;

      qInstallMessageHandler(
            [] (QtMsgType type
            , const QMessageLogContext& context
            , const QString& msg)
      {
        // TODO ugh
        static QStringList strlist;
        strlist.append(msg);
        if(strlist.size() > 2)
          strlist.removeFirst();
        ref.setPlainText(strlist.join("\n===============\n"));
      });
    }

    // Set-up the window layout
    setCentralWidget(&m_central);
    setMinimumHeight(800);
    setMinimumWidth(1000);

    m_left.addLayout(&m_btns);
    m_btns.addWidget(&m_loadImage);
    m_btns.addWidget(&m_loadShader);
    m_btns.addWidget(&m_loadCamera);

    m_shaderEditor.setMaximumWidth(600);
    m_left.addWidget(&m_shaderEditor);

    m_errorText.setMaximumWidth(600);
    m_errorText.setMaximumHeight(200);
    m_errorText.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    m_left.addWidget(&m_errorText);
    m_layout.addLayout(&m_left);
    m_layout.addWidget(&m_quickwidget);

    // Set-up the root object of the QQuickWidget.
    // This object will have two children:
    // - The control object, generated as QML code
    // - The shader item (m_item)
    QQmlComponent source{m_quickwidget.engine()};
    source.setData(R"_(
                   import QtQuick 2.7
                   Rectangle
                   {
                     color: "black"
                     objectName: "rect"
                   }
                   )_", QUrl{});

    m_quickwidget.setContent(QUrl{}, &source, source.create());
    m_quickwidget.setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickwidget.setMinimumSize(100, 100);

    auto rect = m_quickwidget.rootObject();
    m_item.setParentItem(rect);
    m_item.setWidth(rect->width());
    m_item.setHeight(rect->height());

    // Handle window resizes
    connect(rect, &QQuickItem::widthChanged,
            &m_item, [=] { m_item.setWidth(rect->width()); });
    connect(rect, &QQuickItem::heightChanged,
            &m_item, [=] { m_item.setHeight(rect->height()); });

    // Shader set-up
    connect(&m_shaderEditor, &isf_editor::code_editor::shaderChanged,
            this, &Window::setShader);

    m_item.setVertexShader(defaultVertexShader);
    m_shaderEditor.setShader(defaultFragmentShader);

    // Load buttons
    connect(&m_loadImage, &QPushButton::clicked, [=] {
      auto file = QFileDialog::getOpenFileName(nullptr, "Load file", {}, {}, {});
      QFile f(file);
      if(f.exists())
      {
        m_item.setTexture(f);
      }
    });

    connect(&m_loadCamera, &QPushButton::clicked, [=] {
      m_item.setCamera();
    });

    connect(&m_loadShader, &QPushButton::clicked, [=] {
      auto file = QFileDialog::getOpenFileName(nullptr, "Load file", {}, {}, {});
      QFile f(file);
      if(f.exists() && f.open(QIODevice::ReadOnly))
      {
        QString s = f.readAll();
        m_shaderEditor.setShader(s);
      }
    });
  }

private:
  void setShader(QString shader)
  {
    try {
      isf::parser s{shader.toStdString()};
      auto d = s.data();

      QString controls =
          R"_(
          import QtQuick 2.0
          import com.github.jcelerier.CreativeControls 1.0
          import QtQuick.Controls 2.0 as QC

          Container {
          id: s
          height: col.childrenRect.height + 2 * radius;
          width: col.childrenRect.width+ 2 * radius;

          property var shader: g_shaderItem;
          Column {
          id: col;
          anchors.fill: parent;

          )_";

      int i = 0;
      for(const isf::input& inp : d.inputs)
      {
        controls += std::visit(control_factory{inp, i}, inp.data);
        i++;
      }

      controls += "}}";

      m_quickwidget.engine()->rootContext()->setContextProperty("g_shaderItem", &m_item);
      QQmlComponent controlsComp(m_quickwidget.engine());
      controlsComp.setData(controls.toUtf8(), QUrl{});

      if(m_currentComponent)
        m_currentComponent->deleteLater();
      m_currentComponent = (QQuickItem*)controlsComp.create();
      if(m_currentComponent)
      {
        m_currentComponent->setParentItem(m_quickwidget.rootObject());
      }
      else
      {
        qDebug() << controlsComp.errorString();
      }
      m_item.setData(QString::fromStdString(s.fragment()), d);
    }
    catch(std::exception& e) {
      qDebug() << "Error on loading: " << e.what();
    }
  }

  QWidget m_central;
  QHBoxLayout m_layout{&m_central};
  QVBoxLayout m_left;
  QHBoxLayout m_btns;
  QPushButton m_loadImage{"Load Texture"};
  QPushButton m_loadShader{"Load Shader"};
  QPushButton m_loadCamera{"Use Camera"};

  isf_editor::code_editor m_shaderEditor{&m_central};
  QPlainTextEdit m_errorText;

  QQuickWidget m_quickwidget;

  QQuickItem* m_currentComponent{};
  isf_editor::ShaderItem m_item;

    static constexpr auto defaultFragmentShader = R"_(/* { } */
out vec4 color;

void main(void)
{
  color = vec4(isf_FragNormCoord.x, isf_FragNormCoord.y, 0.5, 1.);
}
)_";

    static constexpr auto defaultVertexShader = R"_(#version 330
in highp vec2 position;
uniform vec2 RENDERSIZE;
out vec2 isf_FragNormCoord;
uniform highp mat4 qt_Matrix;

void main() {
  gl_Position = qt_Matrix * vec4( position, 0.0, 1.0 );
  isf_FragNormCoord = vec2((gl_Position.x+1.0)/2.0, (gl_Position.y+1.0)/2.0);
})_";

};

}
