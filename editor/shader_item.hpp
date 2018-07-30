#pragma once
#include <editor/image_surface.hpp>
#include <editor/material.hpp>
#include <editor/video_reader.hpp>
#include <editor/audio_reader.hpp>
#include <editor/camera_reader.hpp>
#include <editor/shader_node.hpp>
#include <QFileInfo>
#include <QImageReader>
#include <QQuickItem>
#include <QGuiApplication>
#include <QScreen>
namespace isf_editor
{
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

  void setImage(QFile& f)
  {
    QImageReader img(&f);

    m_image = img.read();
    m_textureChanged = true;
    m_reader.reset();

  }
  void setVideo(QFile& f)
  {
    m_reader = std::make_unique<VideoReader>(m_image, QFileInfo(f).absoluteFilePath(), m_imageMutex);
    m_textureChanged = true;
  }

  void setAudio(QFile& f)
  {
    m_reader = std::make_unique<audio_reader>(m_image, QFileInfo(f).absoluteFilePath(), m_imageMutex);
    m_textureChanged = true;
  }

  void setCamera()
  {
    m_reader = std::make_unique<camera_reader>(m_image, m_imageMutex);
    m_textureChanged = true;
  }

  void setTexture(QFile& f)
  {
    auto ext = QFileInfo(f).completeSuffix().toLower();
    QSet<QString> images{"jpg", "jpeg", "png", "bmp", "gif"};
    QSet<QString> videos{"mp4", "avi", "mkv"};
    QSet<QString> audios{"mp3", "wav", "flac"};
    if(images.contains(ext))
    {
      setImage(f);
    }
    else if(videos.contains(ext))
    {
      setVideo(f);
    }
    else if(audios.contains(ext))
    {
      setAudio(f);
    }
  }

  void setData(QString fragment, const isf::descriptor& d)
  {
    using namespace isf;
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
        if(i.def)
        {
          arr[0] = (*i.def)[0];
          arr[1] = (*i.def)[1];
          arr[2] = (*i.def)[2];
        }
        else
        {
          arr[0] = 0.5;
          arr[1] = 0.5;
          arr[2] = 0.5;
        }
        val = arr;
      }
      void operator()(const color_input& i)
      {
        QVector4D arr;
        if(i.def)
        {
          arr[0] = (*i.def)[0];
          arr[1] = (*i.def)[1];
          arr[2] = (*i.def)[2];
          arr[3] = (*i.def)[3];
        }
        else
        {
          arr[0] = 0.5;
          arr[1] = 0.5;
          arr[2] = 0.5;
          arr[3] = 0.5;
        }
        val = arr;
      }
      void operator()(const image_input& i)
      { }
      void operator()(const audio_input& i)
      { }
      void operator()(const audioFFT_input& i)
      { }
    };

    for(std::size_t i = 0; i < N; i++)
    {
      eggs::variants::apply(set_default_visitor{m_variables[i]}, m_desc.inputs[i].data);
    }

    m_timer = startTimer(8, Qt::TimerType::CoarseTimer);

    m_fragmentShader = fragment;
    m_fragmentDirty = true;
    update();
  }

public Q_SLOTS:
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
  static double refreshRate()
  {
    if(auto s = QGuiApplication::screens(); !s.empty())
    {
      return s[0]->refreshRate();
    }
    return 60;
  }

  void timerEvent(QTimerEvent *event) override
  {
    const float seconds = 1. / refreshRate();
    m_curTime += seconds;
    m_variables[m_time] = m_curTime;
    m_variables[m_timeDelta] = seconds;
    m_variables[m_renderSize] = QVector2D{(float)width(), (float)height()};
    // TODO date
    update();
  }

  shader_node* makeNode()
  {
    if(!m_vertexShader.isEmpty() && !m_fragmentShader.isEmpty())
      return new shader_node{m_vertexShader.toStdString(), m_fragmentShader.toStdString(), m_desc};
    return nullptr;
  }

  QSGNode* updatePaintNode(QSGNode *node, UpdatePaintNodeData *) override
  {
    auto n = static_cast<shader_node *>(node);
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
      auto mat =static_cast<material*>(n->material());
      mat->state() = m_variables;
      if(m_textureChanged)
      {
        std::lock_guard<std::mutex> l(m_imageMutex);
        mat->texture() = m_image;
        m_textureChanged = false;
      }
      n->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
    }
    return n;
  }

  isf::descriptor m_desc;

  // Shaders
  QString m_vertexShader;
  QString m_fragmentShader;

  std::atomic_bool m_vertexDirty{false};
  std::atomic_bool m_fragmentDirty{false};

  // ISF variables
  std::vector<value_type> m_variables;

  boost::optional<int> m_timer;
  float m_lastTime{};
  float m_curTime{};

  int m_renderSize{-1};
  int m_time{-1};
  int m_timeDelta{-1};
  int m_passIndex{-1};
  int m_date{-1};

  // Texture
  QImage m_image;
  std::mutex m_imageMutex;
  std::unique_ptr<image_surface> m_reader{};
  bool m_textureChanged{};
};

}
