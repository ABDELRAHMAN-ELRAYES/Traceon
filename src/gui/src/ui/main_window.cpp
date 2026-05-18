#include "gui/ui/main_window.h"
#include <QAction>
#include <QFileDialog>
#include <QFrame>
#include <QLabel>
#include <QMenuBar>
#include <QSplitter>
#include <QVBoxLayout>

MainWindow::MainWindow(ApplicationController *controller, QWidget *parent)
    : QMainWindow(parent), controller_(controller) {
  // Setup the window title & size
  setWindowTitle("Traceon - PCIe Protocol Analyzer");
  resize(1200, 800);

  // Setup window layout
  setupLayout();

  // Setup the menu options
  setupMenu();
}
QFrame *createPanel(const QString &text, const QString &color) {
  QFrame *frame = new QFrame();
  frame->setStyleSheet("background-color:" + color + ";");
  QVBoxLayout *layout = new QVBoxLayout(frame);
  QLabel *label = new QLabel(text);
  label->setStyleSheet("font-size: 18px; color: white;");
  layout->addWidget(label);
  return frame;
}

void MainWindow::setupLayout() {
  // Setup a central widget
  QWidget *centralWidget = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout(centralWidget);

  // Setup the grid splitter layout
  QFrame *topLeft = createPanel("Top Left", "red");
  QFrame *topRight = createPanel("Top right", "blue");
  QFrame *bottomLeft = createPanel("Bottom Left", "green");
  QFrame *bottomRight = createPanel("Bottom Right", "yellow");

  // Main Splitter
  QSplitter *mainSplitter = new QSplitter(Qt::Vertical);

  // Horizontal splitters
  QSplitter *topSplitter = new QSplitter(Qt::Horizontal, mainSplitter);
  topSplitter->addWidget(topLeft);
  topSplitter->addWidget(topRight);

  QSplitter *bottomSplitter = new QSplitter(Qt::Horizontal, mainSplitter);
  bottomSplitter->addWidget(bottomLeft);
  bottomSplitter->addWidget(bottomRight);

  layout->addWidget(mainSplitter);

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
    controller_->loadReport(fileName.toStdString());
  }
}
