#include "mainwindow.h"
#include <QApplication>
#include <QGuiApplication>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  QGuiApplication::setDesktopFileName("com.ktechpit.glate");
  QApplication::setApplicationName("Glate");
  QApplication::setOrganizationName("org.keshavnrj.ubuntu");
  QApplication::setApplicationVersion("4.0");
  MainWindow w;

  w.show();

  return a.exec();
}
