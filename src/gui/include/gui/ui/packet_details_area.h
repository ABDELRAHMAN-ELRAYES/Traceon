#ifndef PACKET_DETAILS_H
#define PACKET_DETAILS_H

#include "gui/models/models.h"
#include <QTextEdit>
#include <QWidget>

class PacketDetailsArea : public QWidget {
  Q_OBJECT
private:
  QTextEdit *display_area_;

public slots:
  void clear();
  void displayPacket(const Packet &packet);

public:
  explicit PacketDetailsArea(QWidget *parent = nullptr);
};

#endif