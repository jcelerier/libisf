#pragma once
#include <QAbstractVideoSurface>
#include <mutex>
namespace isf_editor
{
//! This will be used to present an image source as a texture to the shader
class image_surface
    : public QAbstractVideoSurface
{
public:
  image_surface(QImage& m, std::mutex& mut):
    m_tex{m}
  , m_mut{mut}
  {
  }

protected:
  QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType) const override
  {
    return {QVideoFrame::Format_RGB32,QVideoFrame::Format_ARGB32};
  }

  bool present(const std::vector<float> &frame)
  {
    if(frame.size() > 0)
    {
      std::lock_guard<std::mutex> l(m_mut);
      m_tex = QImage{(unsigned char*)frame.data(), (int)(frame.size() / 3.), 1, QImage::Format_RGB32};
    }
    else
    {
      std::lock_guard<std::mutex> l(m_mut);
      m_tex = QImage{};
    }

    return true;
  }

  bool present(const QVideoFrame &frame) override
  {
    if(frame.isValid())
    {
      QVideoFrame videoFrame(frame);
      if(videoFrame.map(QAbstractVideoBuffer::ReadOnly))
      {
        std::lock_guard<std::mutex> l(m_mut);
        m_tex = QImage{videoFrame.bits(), videoFrame.width(), videoFrame.height(), frame.imageFormatFromPixelFormat(frame.pixelFormat())};

        videoFrame.unmap();
      }
    }
    else
    {
      std::lock_guard<std::mutex> l(m_mut);
      m_tex = QImage{};
    }

    return true;
  }

  QImage& m_tex;
  std::mutex& m_mut;
};
}
