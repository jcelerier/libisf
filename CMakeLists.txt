set(CMAKE_PREFIX_PATH "/usr/local/Cellar/qt/5.9.0/lib/cmake/Qt5")
cmake_minimum_required(VERSION 2.8)

project(libisf)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_library(isf
    src/sajson.h
    src/isf.hpp
    src/isf.cpp
)
find_package(Qt5 REQUIRED COMPONENTS Quick QuickWidgets Multimedia Xml DBus)
find_package(Boost REQUIRED)
target_include_directories(isf PUBLIC src/)
target_link_libraries(isf PUBLIC Boost::boost)
set(CMAKE_AUTOMOC ON)
add_executable(editor
    examples/editor.cpp
    examples/editor.hpp
)

find_package(KF5TextEditor NO_MODULE)
if(${KF5TextEditor_FOUND})
    find_package(KF5Parts NO_MODULE)
    target_link_libraries(editor PRIVATE KF5::Parts KF5::TextEditor)
    target_compile_definitions(editor PRIVATE KTEXTEDITOR)
endif()
target_link_libraries(editor PRIVATE isf Qt5::Quick Qt5::QuickWidgets Qt5::Multimedia )
target_compile_options(editor PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare)
