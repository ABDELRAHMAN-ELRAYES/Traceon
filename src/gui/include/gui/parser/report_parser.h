#ifndef REPORT_PARSER_H
#define REPORT_PARSER_H

#include "gui/models/models.h"
#include <filesystem>
#include <optional>

struct ParseResult {
  std::optional<ReportModel> report;
  std::string error_message;
  bool is_success;
};

class ReportParser {

private:
  static ParseResult parseJson(const std::string &content);
  static ParseResult parseXml(const std::string &content);

public:
  ReportParser() = default;
  static ParseResult parse(const std::filesystem::path &reportPath);
};
#endif