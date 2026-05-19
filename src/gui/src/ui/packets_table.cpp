#include "gui/ui/packets_table.h"
#include "gui/utils/utils.h"
#include <QBrush>
#include <QColor>
#include <QFont>

PacketsTableModel::PacketsTableModel(QObject *parent)
    : QAbstractTableModel(parent) {}

void PacketsTableModel::setPackets(const std::vector<Packet> &packets) {
  beginResetModel();
  packets_ = packets;
  endResetModel();
}

const Packet &PacketsTableModel::getPacket(int row) const {
  return packets_.at(row);
}

int PacketsTableModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return static_cast<int>(packets_.size());
}

int PacketsTableModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;

  return 9;
}

QVariant PacketsTableModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= static_cast<int>(packets_.size())) {
    return QVariant();
  }

  const Packet &packet = packets_[index.row()];

  // Cell Data
  if (role == Qt::DisplayRole) {
    switch (index.column()) {
    case Index:
      return static_cast<qlonglong>(packet.index);
    case Timestamp:
      return packet.timestamp_ns > 0
                 ? static_cast<qlonglong>(packet.timestamp_ns)
                 : QVariant("—");
    case Direction:
      return QString::fromStdString(Utils::directionToString(packet.direction));
    case Type:
      return QString::fromStdString(Utils::tlpTypeToString(packet.tlp_type));
    case Address:
      return QString::fromStdString(packet.address);
    case Length:
      return QString::fromStdString(packet.length);
    case Tag:
      return QString::fromStdString(packet.tag);
    case Status:
      return QString::fromStdString(
          Utils::completionStatusToStr(packet.status));
    case Errors:
      return packet.has_any_error ? "⚠" : "";
    }
  }

  // Cell Background
  if (role == Qt::BackgroundRole) {
    if (packet.is_malformed) {
      return QBrush(QColor(255, 200, 200)); // Red
    } else if (packet.has_validation_errors) {
      return QBrush(QColor(255, 240, 200)); // yellow
    }
  }

  // Cell text alignment
  if (role == Qt::TextAlignmentRole) {
    return static_cast<int>(Qt::AlignCenter);
  }

  return QVariant();
}

QVariant PacketsTableModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    return Utils::getTableColumnStr(section);
  }
  return QVariant();
}