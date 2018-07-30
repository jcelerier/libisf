# libisf

libisf is a library for parsing [Interactive Shader Format](https://www.interactiveshaderformat.com) shaders.

It comes with a visual editor to write and edit such shaders in real-time:

![isf editor](https://raw.githubusercontent.com/jcelerier/libisf/master/screenshot.png)

## Dependencies

### libisf

* Boost.Optional (until the compiler shipped by Apple supports `std::optional`).

### Editor

* CMake
* Qt 5
* [KTextEditor](https://api.kde.org/frameworks/ktexteditor/html/index.html) (optional, for a nice syntax highlighting)
* [QML Creative Controls](https://github.com/jcelerier/qml-creative-controls) (optional, to show UI controls for shaders)

## Building

    mkdir build
    cd build
    cmake /path/to/libisf
    make -j

## Running the editor

    ./editor
