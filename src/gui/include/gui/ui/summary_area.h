#ifndef SUMMARY_AREA_H
#define SUMMARY_AREA_H

#include "gui/models/models.h"
#include <QLabel>
#include <QTableWidget>
#include <QWidget>

class SummaryArea : public QWidget {
  Q_OBJECT
private:
  QLabel *label_;
  QTableWidget *type_table_;

public:
  explicit SummaryArea(QWidget *parent = nullptr);
  void displaySummary(const StatsModel &stats);
};

#endif