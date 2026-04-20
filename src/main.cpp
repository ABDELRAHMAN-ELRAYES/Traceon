#include "analyzer-engine/analyzer_engine.h"
#include "analyzer-engine/report-builder/report_builder.h"
#include <filesystem>

const std::filesystem::path TRACE_FILE_PATH = "../data/trace_data.csv";
const std::filesystem::path REPORT_PATH = "../data/report.json";
int main() {

  AnalyzerEngine analyzer{TRACE_FILE_PATH, REPORT_PATH, ReportFormat::JSON};
  analyzer.run();
  return 0;
}