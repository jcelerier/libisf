cmake_minimum_required(VERSION 3.9)

project(libisf CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(isf
    src/sajson.h
    src/isf.hpp
    src/isf.cpp
)
target_include_directories(isf PUBLIC src/)
find_package(Qt5 REQUIRED COMPONENTS Quick QuickWidgets Multimedia Xml DBus)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
add_executable(editor
    editor/main.cpp
    editor/audio_reader.hpp
    editor/camera_reader.hpp
    editor/code_editor.hpp
    editor/control_factory.hpp
    editor/image_surface.hpp
    editor/material.hpp
    editor/shader.hpp
    editor/shader_item.hpp
    editor/shader_node.hpp
    editor/video_reader.hpp
    editor/window.hpp
    editor/resources.qrc
)

find_package(KF5TextEditor NO_MODULE)
if(${KF5TextEditor_FOUND})
    find_package(KF5Parts NO_MODULE)
    target_link_libraries(editor PRIVATE KF5::Parts KF5::TextEditor)
    target_compile_definitions(editor PRIVATE KTEXTEDITOR)
endif()
target_link_libraries(editor PRIVATE isf Qt5::Gui Qt5::Quick Qt5::QuickWidgets Qt5::Multimedia )
target_compile_options(editor PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare)
