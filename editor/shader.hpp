#pragma once
#include <isf.hpp>
#include <QSGMaterial>
#include <QImage>
#include <QOpenGLTexture>

namespace isf_editor
{
using value_type = eggs::variant<bool, GLfloat, GLint, QVector2D, QVector3D, QVector4D, QColor>;
using State = std::vector<value_type>;

template<typename Material_T>
class shader final : public QSGMaterialShader
{
public:
  shader(std::string vert, std::string frag, isf::descriptor d)
    : m_vertex{std::move(vert)}
    , m_fragment{std::move(frag)}
    , m_desc{std::move(d)}
    , m_texture{ QImage(":/images/default.jpg").mirrored() }
  {
  }

  void initialize() override
  {
    QSGMaterialShader::initialize();

    m_id_matrix = program()->uniformLocation("qt_Matrix");
    if (m_id_matrix < 0) {
      qDebug() << "QSGSimpleMaterialShader does not implement 'uniform highp mat4 qt_Matrix;' in its vertex shader";
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

  void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override
  {
    if (state.isMatrixDirty())
      program()->setUniformValue(m_id_matrix, state.combinedMatrix());

    auto mat = static_cast<Material_T*>(newMaterial);
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
      auto oldmat = static_cast<Material_T*>(oldMaterial);
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
  const isf::descriptor m_desc;
  QOpenGLTexture m_texture;
  std::vector<int> m_uniforms;
  mutable QVector<const char*> m_attrs;
  int m_id_matrix;
};

}
