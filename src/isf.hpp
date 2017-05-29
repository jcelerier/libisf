#pragma once
#include <string>
#include <stdexcept>
#include <array>
#include <vector>
#include <variant>
namespace isf
{
class invalid_file : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

struct bool_input
{
    using value_type = bool;
    using has_default = std::true_type;
    std::string name;
    std::string label;
    bool def{};
};

struct float_input
{
    using value_type = double;
    using has_minmax = std::true_type;
    std::string name;
    std::string label;
    double min{};
    double max{};
    double def{};
};

struct point2d_input
{
    using value_type = std::array<double, 2>;
    using has_minmax = std::true_type;
    std::string name;
    std::string label;
    std::array<double, 2> def{};
    std::array<double, 2> min{};
    std::array<double, 2> max{};
};

struct point3d_input
{
    using value_type = std::array<double, 3>;
    using has_minmax = std::true_type;
    std::string name;
    std::string label;
    std::array<double, 3> def{};
    std::array<double, 3> min{};
    std::array<double, 3> max{};
};

struct color_input
{
    using value_type = std::array<double, 4>;
    using has_default = std::true_type;
    std::string name;
    std::string label;
    std::array<double, 4> def{};
};

struct image_input
{
    std::string name;
    std::string label;
};

using input = std::variant<float_input, bool_input, color_input, point2d_input, point3d_input, image_input>;

struct descriptor
{
    std::string description;
    std::string credits;
    std::vector<std::string> categories;
    std::vector<input> inputs;
};

class parser
{
    std::string m_source;

    std::vector<std::string> m_vertex;
    std::vector<std::string> m_fragment;
    descriptor m_desc;

public:
    parser(std::string s);

    descriptor data() const;
    std::vector<std::string> vertex() const;
    std::vector<std::string> fragment() const;

private:
    void parse();
};
}
