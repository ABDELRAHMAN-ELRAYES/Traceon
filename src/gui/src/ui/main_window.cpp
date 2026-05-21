#include "gui/ui/main_window.h"
#include "gui/controller/application_controller.h"
#include "gui/models/models.h"
#include "gui/ui/errors_area.h"
#include "gui/ui/packet_details_area.h"
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
  // Connect the row click signal with the display packet details slot
  connect(packets_table_, &QTableView::clicked, packet_details_area_,
          [this](const QModelIndex &index) {
            controller_->onPacketSelected(index.row());
          });

  // View the packet details on row click
  connect(controller_, &ApplicationController::packetSelected,
          packet_details_area_, &PacketDetailsArea::displayPacket);

  // on Report load view the validation errors
  connect(controller_, &ApplicationController::loadCompleted, this,
          &MainWindow::onLoadCompleted);
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

  // Define the packet details section
  packet_details_area_ = new PacketDetailsArea;

  // Main Splitter
  QSplitter *mainSplitter = new QSplitter(Qt::Vertical);
  layout->addWidget(mainSplitter);

  // Horizontal splitters
  QSplitter *topSplitter = new QSplitter(Qt::Horizontal, mainSplitter);
  topSplitter->addWidget(packets_table_);
  topSplitter->addWidget(packet_details_area_);

  QSplitter *bottomSplitter = new QSplitter(Qt::Horizontal, mainSplitter);

  errors_area_ = new ErrorsArea;
  bottomSplitter->addWidget(errors_area_);

  summary_area_ = new SummaryArea;
  bottomSplitter->addWidget(summary_area_);

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
void MainWindow::onLoadCompleted() {
  const ReportModel *report = &controller_->getReport();
  errors_area_->displayErrors(report->all_validation_errors);
  summary_area_->displaySummary(report->stats);
}