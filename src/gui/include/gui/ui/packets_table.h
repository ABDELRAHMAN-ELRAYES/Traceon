#ifndef PACKETS_TABLE_H
#define PACKETS_TABLE_H

#include "gui/models/models.h"
#include <QAbstractTableModel>
#include <vector>

class PacketsTableModel : public QAbstractTableModel {
  Q_OBJECT
private:
  std::vector<Packet> packets_;

public:
  enum Column {
    Index = 0,
    Timestamp,
    Direction,
    Type,
    Address,
    Length,
    Tag,
    Status,
    Errors,
    ColumnCount
  };

  explicit PacketsTableModel(QObject *parent = nullptr);

  void setPackets(const std::vector<Packet> &packets);
  const Packet &getPacket(int row) const;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
};

#endif