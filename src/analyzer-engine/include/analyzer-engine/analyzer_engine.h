#ifndef ANALYZER_ENGINE_H
#define ANALYZER_ENGINE_H
#include "analyzer-engine/report-builder/report_builder.h"
#include <filesystem>

class AnalyzerEngine {
private:
  std::filesystem::path trace_path_;
  std::filesystem::path report_path_;
  ReportFormat report_format_;

public:
  explicit AnalyzerEngine(std::filesystem::path trace_path,
                          std::filesystem::path report_path,
                          ReportFormat report_format);

  void run();
};

#endif