cmake_minimum_required(VERSION 2.8)

project(libisf)
set(CMAKE_CXX_STANDARD 17)
add_library(isf
    src/sajson.h
    src/isf.hpp
    src/isf.cpp
)
find_package(Qt5 COMPONENTS Quick QuickWidgets)
find_package(KF5TextEditor NO_MODULE)

set(CMAKE_AUTOMOC ON)
add_executable(editor
    examples/editor.cpp
    examples/editor.hpp
)

target_link_libraries(editor PRIVATE isf Qt5::Quick Qt5::QuickWidgets KF5::TextEditor)
target_compile_options(editor PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare)
