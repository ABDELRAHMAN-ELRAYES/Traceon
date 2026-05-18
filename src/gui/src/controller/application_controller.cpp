#include "gui/controller/application_controller.h"
#include "gui/parser/report_parser.h"

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent) {}

void ApplicationController::loadReport(const std::filesystem::path &filePath) {
  emit loadStarted();
  ParseResult result = ReportParser::parse(filePath);

  if (result.is_success && result.report.has_value()) {
    // Set the current report
    report_ = std::move(result.report.value());
    emit loadCompleted();
  } else {
    emit loadFailed(QString::fromStdString(result.error_message));
  }
}
