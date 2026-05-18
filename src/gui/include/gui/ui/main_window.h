#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

class MainWindow : public QMainWindow {
  Q_OBJECT
private:
  void setupLayout();
  void setupMenu();
  void onOpenFile();

public:
  explicit MainWindow(QWidget *parent = nullptr);
};

#endif