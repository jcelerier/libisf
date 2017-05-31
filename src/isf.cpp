#include "isf.hpp"
#include <boost/algorithm/string/replace.hpp>
#include <string_view>
#include <vector>
#include <sstream>
#include <array>
#include <regex>
#include <iostream>
#include <unordered_map>
#include "sajson.h"
namespace isf
{

parser::parser(std::string s):
    m_source{std::move(s)}
{
    parse();
}

descriptor parser::data() const
{
    return m_desc;
}

std::vector<std::string> parser::vertex() const
{
    return m_vertex;
}

std::vector<std::string> parser::fragment() const
{
    return m_fragment;
}

static bool is_number(sajson::value &v)
{
    auto t = v.get_type();
    return t == sajson::TYPE_INTEGER || t == sajson::TYPE_DOUBLE;
}

static void parse_input_base(input& inp, const sajson::value &v)
{
    std::size_t N = v.get_length();

    for(std::size_t i = 0; i < N; i++)
    {
        auto k = v.get_object_key(i).as_string();
        if(k == "NAME")
        {
            auto val = v.get_object_value(i);
            if(val.get_type() == sajson::TYPE_STRING)
                inp.name = val.as_string();
        }
        else if(k == "LABEL")
        {
            auto val = v.get_object_value(i);
            if(val.get_type() == sajson::TYPE_STRING)
                inp.label = val.as_string();
        }
    }
}

template<std::size_t N>
static std::array<double, N> parse_input_impl(sajson::value& v, std::array<double, N>)
{
    if(v.get_type() == sajson::TYPE_ARRAY && v.get_length() >= N)
    {
        std::array<double, N> arr{};
        for(std::size_t i = 0; i < N; i++)
        {
            auto val = v.get_array_element(i);
            if(is_number(val))
                arr[i] = val.get_number_value();
        }
        return arr;
    }
    return {};
}

static double parse_input_impl(sajson::value &v, double)
{
    if(is_number(v))
        return v.get_number_value();
    return 0.;
}
static int64_t parse_input_impl(sajson::value &v, int64_t)
{
    if(is_number(v))
        return v.get_number_value();
    return 0;
}

static bool parse_input_impl(sajson::value &v, bool)
{
    return v.get_type() == sajson::TYPE_TRUE;
}

static void parse_input(image_input &inp, const sajson::value &v)
{
}

static void parse_input(event_input &inp, const sajson::value &v)
{
}

template<typename Input_T, typename std::enable_if_t<Input_T::has_minmax::value>* = nullptr>
static void parse_input(Input_T& inp, const sajson::value& v)
{
    std::size_t N = v.get_length();

    for(std::size_t i = 0; i < N; i++)
    {
        auto k = v.get_object_key(i).as_string();
        if(k == "MIN")
        {
            auto val = v.get_object_value(i);
            inp.min = parse_input_impl(val, typename Input_T::value_type{});
        }
        else if(k == "MAX")
        {
            auto val = v.get_object_value(i);
            inp.max = parse_input_impl(val, typename Input_T::value_type{});
        }
        else if(k == "DEFAULT")
        {
            auto val = v.get_object_value(i);
            inp.def = parse_input_impl(val, typename Input_T::value_type{});
        }
    }
}

template<typename Input_T, typename std::enable_if_t<Input_T::has_default::value>* = nullptr>
static void parse_input(Input_T& inp, const sajson::value& v)
{
    std::size_t N = v.get_length();

    for(std::size_t i = 0; i < N; i++)
    {
        auto k = v.get_object_key(i).as_string();
        if(k == "DEFAULT")
        {
            auto val = v.get_object_value(i);
            inp.def = parse_input_impl(val, typename Input_T::value_type{});
        }
    }
}

template<typename T>
input parse(const sajson::value& v)
{
    input i;
    parse_input_base(i, v);
    T inp;
    parse_input(inp, v);
    i.data = inp;
    return i;
}

using root_fun = void(*)(descriptor&, const sajson::value&);
using input_fun = input(*)(const sajson::value&);
const std::unordered_map<std::string, root_fun>& root_parse{
    [] {
        static std::unordered_map<std::string, root_fun> p;
        p.insert(
        {"DESCRIPTION", [] (descriptor& d, const sajson::value& v) {
             if(v.get_type() == sajson::TYPE_STRING)
             d.description = v.as_string();
         }});
        p.insert(
        {"CREDIT", [] (descriptor& d, const sajson::value& v) {
             if(v.get_type() == sajson::TYPE_STRING)
             d.credits = v.as_string();
         }});
        p.insert(
        {"CATEGORIES", [] (descriptor& d, const sajson::value& v) {
             if(v.get_type() == sajson::TYPE_ARRAY)
             {
                 std::size_t n = v.get_length();
                 for(std::size_t i = 0; i < n; i++)
                 {
                     if(v.get_type() == sajson::TYPE_STRING)
                     d.categories.push_back(v.as_string());
                 }
             }
         }});

        static const std::unordered_map<std::string, input_fun>& input_parse{
            [] {
                static std::unordered_map<std::string, input_fun> i;
                i.insert({"float", [] (const auto& s) { return parse<float_input>(s); } });
                i.insert({"long", [] (const auto& s) { return parse<long_input>(s); } });
                i.insert({"bool",  [] (const auto& s) { return parse<bool_input>(s); } });
                i.insert({"event", [] (const auto& s) { return parse<event_input>(s); } });
                i.insert({"image", [] (const auto& s) { return parse<image_input>(s); } });
                i.insert({"point2D", [] (const auto& s) { return parse<point2d_input>(s); } });
                i.insert({"point3D", [] (const auto& s) { return parse<point3d_input>(s); } });
                i.insert({"color", [] (const auto& s) { return parse<color_input>(s); } });

                return i;
            }()
        };

        p.insert(
        {"INPUTS", [] (descriptor& d, const sajson::value& v) {

             using namespace std::literals;
             if(v.get_type() == sajson::TYPE_ARRAY)
             {
                 std::size_t n = v.get_length();
                 for(std::size_t i = 0; i < n; i++)
                 {
                     auto obj = v.get_array_element(i);
                     if(obj.get_type() == sajson::TYPE_OBJECT)
                     {
                         auto k = obj.find_object_key(sajson::string("TYPE", 4));
                         if(k != obj.get_length())
                         {
                             auto inp = input_parse.find(obj.get_object_value(k).as_string());
                             if(inp != input_parse.end())
                                d.inputs.push_back((inp->second)(obj));
                         }
                     }
                 }
             }
         }});

        return p;
    }()
};

struct create_val_visitor
{
    std::string operator()(const float_input&)
    { return "uniform float"; }
    std::string operator()(const long_input&)
    { return "uniform int"; }
    std::string operator()(const event_input&)
    { return "uniform bool"; }
    std::string operator()(const bool_input&)
    { return "uniform bool"; }
    std::string operator()(const point2d_input&)
    { return "uniform vec2"; }
    std::string operator()(const point3d_input&)
    { return "uniform vec3"; }
    std::string operator()(const color_input&)
    { return "uniform vec4"; }
    std::string operator()(const image_input&)
    { return "uniform sampler2D"; }
};

void parser::parse()
{
    using namespace std::literals;
    auto start = m_source.find("/*");
    if(start == std::string::npos)
        throw invalid_file{"Missing start comment"};
    auto end = m_source.find("*/", start);
    if(end == std::string::npos)
        throw invalid_file{"Unfinished comment"};

    // First comes the json part
    auto doc = sajson::parse(
                sajson::dynamic_allocation(),
                sajson::mutable_string_view((end - start - 2), m_source.data() + start + 2));
    if(!doc.is_valid())
    {
        std::stringstream err;
        err << "JSON error: '" << doc.get_error_message() << "' at L" <<  doc.get_error_line() << ":" << doc.get_error_column();
        throw invalid_file{err.str()};
    }

    // Read the parameters
    auto root = doc.get_root();
    if(!root.get_type() == sajson::TYPE_OBJECT)
        throw invalid_file{"Not a JSON object"};

    descriptor d;
    for(std::size_t i = 0; i < root.get_length(); i++)
    {
        auto it = root_parse.find(root.get_object_key(i).as_string());
        if(it != root_parse.end())
        {
            (it->second)(d, root.get_object_value(i));
        }
    }
    m_desc = d;

    // Then the GLSL
    std::string shader;
    for(const isf::input& val : d.inputs)
    {
        shader += std::visit(create_val_visitor{}, val.data);
        shader += ' ';
        shader += val.name;
        shader += ";";
    }

    shader += "uniform int PASSINDEX;\n";
    shader += "uniform vec2 RENDERSIZE;\n";
    shader += "uniform float TIME;\n";
    shader += "uniform float TIMEDELTA;\n";
    shader += "uniform vec4 DATE;\n";
    shader += "varying vec2 isf_FragNormCoord;\n";

    std::regex img_pixel("IMG_THIS_PIXEL\\((.+?)\\)");
    m_source = std::regex_replace(m_source, img_pixel, "texture2D($1, isf_FragNormCoord)");
    shader.append(m_source.begin() + end + 2, m_source.end());
    m_fragment.push_back(shader);

    m_vertex.push_back(R"_(
    attribute vec2 position;
    uniform vec2 RENDERSIZE;
    varying vec2 isf_FragNormCoord;

    void main(void) {
      gl_Position = vec4( position, 0.0, 1.0 );
      isf_FragNormCoord = vec2((gl_Position.x+1.0)/2.0, (gl_Position.y+1.0)/2.0);
    }
    )_");
}

}
