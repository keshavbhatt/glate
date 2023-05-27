#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

  QApplication a(argc, argv);
  QApplication::setApplicationName("Glate");
  QApplication::setOrganizationName("org.keshavnrj.ubuntu");
  QApplication::setApplicationVersion("3.0");
  MainWindow w;

  w.show();

  return a.exec();
}
