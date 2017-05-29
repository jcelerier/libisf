cmake_minimum_required(VERSION 2.8)

project(libisf)
set(CMAKE_CXX_STANDARD 17)
add_library(isf
    src/sajson.h
    src/isf.hpp
    src/isf.cpp
)
find_package(Qt5 COMPONENTS Quick)
set(CMAKE_AUTOMOC ON)
add_executable(editor
    examples/editor.cpp
    examples/editor.hpp
    examples/Editor.qml
)

target_link_libraries(editor PRIVATE isf Qt5::Quick)
