#include <QApplication>
#include <editor/window.hpp>
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    isf_editor::Window w;
    w.show();
    return app.exec();
}
