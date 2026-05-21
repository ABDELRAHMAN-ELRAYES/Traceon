#include "gui/ui/errors_area.h"
#include "gui/models/models.h"
#include <QVBoxLayout>

ErrorsArea::ErrorsArea(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);

  label_ = new QLabel("<b>Validation Errors (0)</b>", this);
  layout->addWidget(label_);

  list_area_ = new QListWidget(this);
  layout->addWidget(list_area_);
}

void ErrorsArea::displayErrors(const std::vector<ValidationError> &errors) {
  list_area_->clear();
  label_->setText(QString("<b>Validation Errors (%1)</b>").arg(errors.size()));

  for (const ValidationError &err : errors) {
    QString text = QString("[%1] Pkt %2: %3")
                       .arg(QString::fromStdString(err.rule_id))
                       .arg(err.packet_index)
                       .arg(QString::fromStdString(err.description));
    QListWidgetItem *item = new QListWidgetItem(text, list_area_);
    item->setData(Qt::UserRole, static_cast<qlonglong>(err.packet_index));
    if (err.category == ValidationType::BYTE_COUNT_MISMATCH ||
        err.category == ValidationType::TAG_COLLISION) {
      item->setForeground(Qt::darkRed);
    }
    list_area_->addItem(item);
  }
}