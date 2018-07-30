# libisf

libisf is a library for parsing [Interactive Shader Format](www.interactiveshaderformat.com) shaders.

It comes with a visual editor to write such shaders:

![isf](https://githun.com/jcelerier/libisf/tree/master/screenshot.png)

## Dependencies

### libisf

* Boost.Optional (until the compiler shipped by Apple supports `std::optional`).

### Editor

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
