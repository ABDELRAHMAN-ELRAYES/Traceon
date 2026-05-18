#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "gui/controller/application_controller.h"
#include <QMainWindow>

class MainWindow : public QMainWindow {
  Q_OBJECT
private:
  ApplicationController *controller_;

  void setupLayout();
  void setupMenu();
  void onOpenFile();

public:
  explicit MainWindow(ApplicationController *controller,
                      QWidget *parent = nullptr);
};

#endif