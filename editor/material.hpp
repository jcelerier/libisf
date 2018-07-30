#pragma once
#include <editor/shader.hpp>
#include <isf.hpp>
#include <eggs/variant.hpp>
#include <QSGMaterial>
#include <QImage>

namespace isf_editor
{
class material final
    : public QSGMaterial
{
  static inline uintptr_t m_ptr = 0;
public:
  material(std::string vert, std::string frag, isf::descriptor d)
    : m_vertex{std::move(vert)}
    , m_fragment{std::move(frag)}
    , m_desc{std::move(d)}
  {
    m_ptr++;
  }

  QSGMaterialShader* createShader() const override
  { return new shader<material>{m_vertex, m_fragment, m_desc}; }

  QSGMaterialType* type() const override
  {
    QSGMaterialType* t = (QSGMaterialType*)m_ptr;
    return t;
  }

  const State& state() const { return m_state; }
  State& state() { return m_state; }
  QImage& texture() { return m_tex; }

private:
  State m_state;
  QImage m_tex;

  std::string m_vertex;
  std::string m_fragment;
  isf::descriptor m_desc;
};
}
