#pragma once
#include <editor/image_surface.hpp>
#include <QCamera>

namespace isf_editor
{
//! Writes a camera stream in an image
class camera_reader final
    : public image_surface
{
public:
  camera_reader(QImage& m, std::mutex& mut):
    image_surface{m, mut}
  {
    m_cam.setViewfinder(this);
    m_cam.start();
  }

private:
  QCamera m_cam;
};

}
