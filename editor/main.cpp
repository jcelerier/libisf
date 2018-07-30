#include <QApplication>
#include <editor/window.hpp>
int main(int argc, char** argv)
{
  QSurfaceFormat f = QSurfaceFormat::defaultFormat();
  f.setVersion(4, 1);
  f.setProfile(QSurfaceFormat::CoreProfile);
  f.setDefaultFormat(f);

  QApplication app(argc, argv);
  isf_editor::Window w;
  w.show();
  return app.exec();
}
