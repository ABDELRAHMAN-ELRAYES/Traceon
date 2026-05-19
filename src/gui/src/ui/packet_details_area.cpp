#include "gui/ui/packet_details_area.h"
#include "gui/utils/utils.h"
#include <QBoxLayout>
#include <QLabel>
#include <qtextedit.h>

PacketDetailsArea::PacketDetailsArea(QWidget *parent) : QWidget(parent) {
  // Setup the main layout
  QVBoxLayout *layout = new QVBoxLayout(this);
  QLabel *label = new QLabel("<b>Packet Details</b>", this);
  layout->addWidget(label);

  // Initiate the Dispaly area
  display_area_ = new QTextEdit(this);
  display_area_->setReadOnly(true);
  display_area_->setFont(QFont("Monospace", 10));
  layout->addWidget(display_area_);

  clear();
}

void PacketDetailsArea::clear() {
  display_area_->setHtml(
      "<p style='color: gray;'>Select a packet to see details.</p>");
}
void PacketDetailsArea::displayPacket(const Packet &packet) {
  QString content = "<h3>Packet #" + QString::number(packet.index) + "</h3>";
  content += "<table width='100%'>";
  content += "<tr><td><b>Timestamp:</b></td><td>" +
             QString::number(packet.timestamp_ns) + " ns</td></tr>";
  content +=
      "<tr><td><b>Direction:</b></td><td>" +
      QString::fromStdString(Utils::directionToString(packet.direction)) +
      "</td></tr>";
  content += "<tr><td><b>Type:</b></td><td>" +
             QString::fromStdString(Utils::tlpTypeToString(packet.tlp_type)) +
             "</td></tr>";
  content += "<tr><td><b>Address:</b></td><td>" +
             QString::fromStdString(packet.address) + "</td></tr>";
  content += "<tr><td><b>Length:</b></td><td>" +
             QString::fromStdString(packet.length) + " DW</td></tr>";
  content += "<tr><td><b>Tag:</b></td><td>" +
             QString::fromStdString(packet.tag) + "</td></tr>";
  content +=
      "<tr><td><b>Status:</b></td><td>" +
      QString::fromStdString(Utils::completionStatusToStr(packet.status)) +
      "</td></tr>";
  content += "</table>";

  if (!packet.decode_errors.empty()) {
    content += "<h4 style='color: red;'>Decode Errors</h4><ul>";
    for (const auto &err : packet.decode_errors) {
      content += "<li><b>" + QString::fromStdString(err.rule_id) + "</b> [" +
                 QString::fromStdString(err.field) +
                 "]: " + QString::fromStdString(err.description) + "</li>";
    }
    content += "</ul>";
  }

  if (!packet.validation_errors.empty()) {
    content += "<h4 style='color: #A52A2A;'>Validation Errors</h4><ul>";
    for (const auto &err : packet.validation_errors) {
      content +=
          "<li><b>" + QString::fromStdString(err.rule_id) + "</b> (" +
          QString::fromStdString(Utils::validationCategoryToStr(err.category)) +
          "): " + QString::fromStdString(err.description) + "</li>";
    }
    content += "</ul>";
  }

  display_area_->setHtml(content);
}