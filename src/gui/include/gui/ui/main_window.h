#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "gui/controller/application_controller.h"
#include "gui/ui/packet_details_area.h"
#include <QMainWindow>
#include <QTableView>

class MainWindow : public QMainWindow {
  Q_OBJECT
private:
  ApplicationController *controller_;
  QTableView *packets_table_;
  PacketDetailsArea *packet_details_area_;

  void setupLayout();
  void setupMenu();
  void onOpenFile();

public:
  explicit MainWindow(ApplicationController *controller,
                      QWidget *parent = nullptr);
};

#endif