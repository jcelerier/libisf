#pragma once
#include <isf.hpp>
#include <editor/material.hpp>
#include <QSGNode>
#include <QSGMaterial>

namespace isf_editor
{
class shader_node final
    : public QSGGeometryNode
{
public:
  shader_node(std::string vert, std::string frag, isf::descriptor d)
    : m_geometry{QSGGeometry::defaultAttributes_TexturedPoint2D(), 4}
    , m_mater{std::move(vert), std::move(frag), std::move(d)}
  {
    m_mater.setFlag(QSGMaterial::Blending);
    setGeometry(&m_geometry);
    setMaterial(&m_mater);
  }

private:
  QSGGeometry m_geometry;
  class material m_mater;
};
}
