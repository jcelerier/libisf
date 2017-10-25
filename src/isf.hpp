#pragma once
#include <string>
#include <stdexcept>
#include <array>
#include <vector>
#include <optional>
#include <variant>
namespace isf
{
class invalid_file : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

struct event_input
{
};

struct bool_input
{
    using value_type = bool;
    using has_default = std::true_type;
    bool def{};
};

struct long_input
{
    using value_type = int64_t;
    using has_minmax = std::true_type;
    std::vector<int64_t> values;
    std::vector<std::string> labels;
    std::size_t def{}; // index of default value
};
struct float_input
{
    using value_type = double;
    using has_minmax = std::true_type;
    double min{};
    double max{};
    double def{};
};

struct point2d_input
{
    using value_type = std::array<double, 2>;
    using has_minmax = std::true_type;
    std::optional<value_type> def{};
    std::optional<value_type> min{};
    std::optional<value_type> max{};
};

struct point3d_input
{
    using value_type = std::array<double, 3>;
    using has_minmax = std::true_type;
    std::array<double, 3> def{};
    std::array<double, 3> min{};
    std::array<double, 3> max{};
};

struct color_input
{
    using value_type = std::array<double, 4>;
    using has_minmax = std::true_type;
    std::array<double, 4> def{};
    std::array<double, 4> min{};
    std::array<double, 4> max{};
};

struct image_input
{
};

struct audio_input
{
};
struct audioFFT_input
{
};

struct input
{
    using input_impl = std::variant<float_input, long_input, event_input, bool_input, color_input, point2d_input, point3d_input, image_input, audio_input, audioFFT_input>;

    std::string name;
    std::string label;

    input_impl data;
};

struct descriptor
{
    std::string description;
    std::string credits;
    std::vector<std::string> categories;
    std::vector<input> inputs;
};

class parser
{
    std::string m_sourceVertex;
    std::string m_sourceFragment;

    std::string m_vertex;
    std::string m_fragment;

    descriptor m_desc;

public:
    enum class ShaderType { ISF, ShaderToy, GLSLSandBox };
    parser(std::string frag, ShaderType = ShaderType::ISF);

    descriptor data() const;
    std::string vertex() const;
    std::string fragment() const;

private:
    void parse_isf();
    void parse_shadertoy();
    void parse_glsl_sandbox();
};
}
