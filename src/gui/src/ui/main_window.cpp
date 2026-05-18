#include "gui/ui/main_window.h"
#include "gui/parser/report_parser.h"
#include <QMenuBar>
#include <QVBoxLayout>
#include <qaction.h>
#include <qfiledialog.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  // Setup the window title & size
  setWindowTitle("Traceon - PCIe Protocol Analyzer");
  resize(1200, 800);

  // Setup window layout
  setupLayout();

  // Setup the menu options
  setupMenu();
}

void MainWindow::setupLayout() {
  // Setup a central widget
  QWidget *centralWidget = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(centralWidget);

  setCentralWidget(centralWidget);
}

void MainWindow::setupMenu() {

  QMenuBar *menu = menuBar();
  QMenu *menuFile = menu->addMenu("&File");

  // Setup menu file actions
  QAction *openAction = menuFile->addAction("&Open Report");
  menu->addSeparator();
  QAction *quitAction = menuFile->addAction("&Quit");

  openAction->setShortcut(QKeySequence::Open);
  quitAction->setShortcut(QKeySequence::Quit);

  connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
  connect(quitAction, &QAction::triggered, this, &MainWindow::close);
}
void MainWindow::onOpenFile() {
  QString fileName = QFileDialog::getOpenFileName(
      this, "Open File", "", "JSON/XML Files (*.json *.xml)");

  if (!fileName.isEmpty()) {
    // Start Parsing the file
    ParseResult result = ReportParser::parse(fileName.toStdString());
  }
}
