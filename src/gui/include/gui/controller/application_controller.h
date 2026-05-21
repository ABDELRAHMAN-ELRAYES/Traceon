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
  void packetSelected(const Packet &packet);

private:
  ReportModel report_;
  PacketsTableModel *packets_table_model_;

public slots:
  void onPacketSelected(int index);

public:
  ApplicationController(QObject *parent = nullptr);
  void loadReport(const std::filesystem::path &filePath);

  PacketsTableModel *getPacketsTableModel() const {
    return packets_table_model_;
  }
  const ReportModel &getReport() const { return report_; }
  const std::vector<Packet> &getPackets() const { return report_.packets; }
  const Packet &getPacket(int index) const { return report_.packets.at(index); }
};

#endif