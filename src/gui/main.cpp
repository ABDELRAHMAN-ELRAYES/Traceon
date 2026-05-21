#include "gui/ui/main_window.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("Traceon - PCIe Protocol Analyzer");
  app.setApplicationVersion("1.0.0");

  // App Controller
  ApplicationController controller;

  // Main window
  MainWindow window(&controller);
  window.show();

  return app.exec();
}
