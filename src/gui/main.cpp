#include "gui/ui/main_window.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("Traceon - PCIe Protocol Analyzer");
  app.setApplicationVersion("1.0.0");

  // Main window
  MainWindow window;
  window.show();

  return app.exec();
}