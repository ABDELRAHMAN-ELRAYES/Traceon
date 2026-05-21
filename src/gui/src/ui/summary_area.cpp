#include "gui/ui/summary_area.h"
#include "gui/utils/utils.h"
#include <QHeaderView>
#include <QVBoxLayout>

SummaryArea::SummaryArea(QWidget *parent) : QWidget(parent) {
  auto *layout = new QVBoxLayout(this);

  label_ = new QLabel("No report loaded yet", this);
  label_->setWordWrap(true);
  layout->addWidget(label_);

  type_table_ = new QTableWidget(0, 2, this);
  type_table_->setHorizontalHeaderLabels({"TLP Type", "Count"});
  type_table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  layout->addWidget(type_table_);
}

void SummaryArea::displaySummary(const StatsModel &stats) {
  label_->setText(
      QString("<b>Total Packets:</b> %1<br>"
              "<b>Malformed:</b> <span style='color: red;'>%2</span><br>"
              "<b>Validation Errors:</b> <span style='color: "
              "#A52A2A;'>%3</span><br>"
              "<b>Skipped Lines:</b> %4")
          .arg(stats.total_packets)
          .arg(stats.malformed_packet_count)
          .arg(stats.validation_error_count)
          .arg(stats.skipped_line_count));

  type_table_->setRowCount(0);
  for (auto it = stats.tlp_type_distribution.begin();
       it != stats.tlp_type_distribution.end(); ++it) {
    int row = type_table_->rowCount();
    type_table_->insertRow(row);
    type_table_->setItem(row, 0,
                         new QTableWidgetItem(QString::fromStdString(
                             Utils::tlpTypeToString(it->first))));
    type_table_->setItem(row, 1,
                         new QTableWidgetItem(QString::number(it->second)));
  }
}