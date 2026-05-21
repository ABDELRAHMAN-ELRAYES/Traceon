#ifndef ERRORS_AREA_H
#define ERRORS_AREA_H

#include "gui/models/models.h"
#include <QLabel>
#include <QListWidget>
#include <QWidget>
#include <qtmetamacros.h>

class ErrorsArea : public QWidget {
  Q_OBJECT

private:
  QLabel *label_;
  QListWidget *list_area_;

public:
  explicit ErrorsArea(QWidget *parent = nullptr);
  void displayErrors(const std::vector<ValidationError> &errors);
};

#endif