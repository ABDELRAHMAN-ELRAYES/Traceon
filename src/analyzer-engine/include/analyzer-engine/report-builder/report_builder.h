#ifndef REPORT_BUILDER_H
#define REPORT_BUILDER_H

#include "analyzer-engine/core/tlp/tlp.h"
#include "analyzer-engine/validator/validation_error.h"
#include "nlohmann/json.hpp"
#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <map>

using json = nlohmann::ordered_json;

enum class ReportFormat { JSON, XML };

class ReportBuilder {
private:
  ReportFormat format_;
  std::filesystem::path trace_file_;
  std::filesystem::path output_path_;
  mutable std::ofstream out_stream_;
  mutable bool first_packet_{true};
  mutable bool first_error_{true};
  mutable bool has_begun_{false};
  std::uint64_t total_packets_{0};
  std::uint64_t malformed_count_{0};
  std::map<std::string, int> distribution_;

  void initializeStream() const;
  void
  writeJsonTLP(const TLP &tlp,
               const std::vector<ValidationError> &validation_errors) const;
  void writeXmlTLP(const TLP &tlp,
                   const std::vector<ValidationError> &validation_errors) const;
  void writeJsonError(const ValidationError &error) const;
  void writeXmlError(const ValidationError &error) const;

public:
  explicit ReportBuilder(ReportFormat format, std::filesystem::path trace_file,
                         std::filesystem::path output_path);

  /**
   * @brief Registers a decoded TLP in the report and updates statistics.
   * @param tlp The TLP to register.
   * @param validation_errors List of validation errors found for this TLP.
   */
  void addTLP(const TLP &tlp,
              const std::vector<ValidationError> &validation_errors = {});

  /**
   * @brief Registers a validation error in the report.
   * @param error The validation error to register.
   */
  void addValidationError(const ValidationError &error);

  /**
   * @brief Writes the complete report to the specified output path.
   * @param output_path The path to the output file.
   * @param skipped_line_count Total lines skipped during input processing.
   */
  void write(const std::filesystem::path &output_path,
             std::uint64_t skipped_line_count) const;
};
#endif