#include "gui/ui/main_window.h"
#include <QAction>
#include <QFileDialog>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <qabstractitemview.h>

MainWindow::MainWindow(ApplicationController *controller, QWidget *parent)
    : QMainWindow(parent), controller_(controller) {
  // Setup the window title & size
  setWindowTitle("Traceon - PCIe Protocol Analyzer");
  resize(1200, 800);

  // Setup window layout
  setupLayout();

  // Setup the menu options
  setupMenu();

  // Connect loadFailed signal
  connect(controller_, &ApplicationController::loadFailed, this,
          [this](const QString &errorMessage) {
            QMessageBox::critical(this, "Error Loading Report",
                                  "Failed to load and parse the report:\n" +
                                      errorMessage);
          });
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

  // Create a packets table (UI)
  packets_table_ = new QTableView(this);
  // Bind the table widget to the controller packets table model

  packets_table_->setModel(controller_->getPacketsTableModel());

  // Change the table behavior
  packets_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  packets_table_->setSelectionMode(QAbstractItemView::SingleSelection);
  packets_table_->setSortingEnabled(true);
  packets_table_->horizontalHeader()->setStretchLastSection(true);
  packets_table_->verticalHeader()->hide();

  // Other layout panels for right and bottom sections (placeholders)
  QFrame *topRight = createPanel("Top right", "blue");
  QFrame *bottomLeft = createPanel("Bottom Left", "green");
  QFrame *bottomRight = createPanel("Bottom Right", "yellow");

  // Main Splitter
  QSplitter *mainSplitter = new QSplitter(Qt::Vertical);
  layout->addWidget(mainSplitter);

  // Horizontal splitters
  QSplitter *topSplitter = new QSplitter(Qt::Horizontal, mainSplitter);
  topSplitter->addWidget(packets_table_);
  topSplitter->addWidget(topRight);

  QSplitter *bottomSplitter = new QSplitter(Qt::Horizontal, mainSplitter);
  bottomSplitter->addWidget(bottomLeft);
  bottomSplitter->addWidget(bottomRight);

  setCentralWidget(centralWidget);

  topSplitter->setStretchFactor(0, 7);
  topSplitter->setStretchFactor(1, 3);
  mainSplitter->setStretchFactor(0, 7);
  mainSplitter->setStretchFactor(1, 3);
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
    // Start Parsing the file via the controller
    controller_->loadReport(fileName.toStdString());
  }
}
