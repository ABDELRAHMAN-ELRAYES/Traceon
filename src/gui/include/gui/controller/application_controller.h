#ifndef APPLICATION_CONTROLLER_H
#define APPLICATION_CONTROLLER_H

#include "gui/models/models.h"
#include "gui/ui/packets_table.h"
#include <QObject>
#include <filesystem>
#include <vector>

class ApplicationController : public QObject {
  Q_OBJECT

signals:
  void loadStarted();
  void loadCompleted();
  void loadFailed(const QString &errorMessage);

private:
  ReportModel report_;
  PacketsTableModel *packets_table_model_;

public:
  ApplicationController(QObject *parent = nullptr);
  void loadReport(const std::filesystem::path &filePath);

  PacketsTableModel *getPacketsTableModel() const { return packets_table_model_; }
  const std::vector<Packet> &getPackets() const { return report_.packets; }
};

#endif