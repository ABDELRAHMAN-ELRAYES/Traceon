#ifndef REPORT_BUILDER_H
#define REPORT_BUILDER_H

#include "analyzer-engine/core/tlp/tlp.h"
#include "analyzer-engine/validator/validation_error.h"
#include "nlohmann/json.hpp"
#include <filesystem>

using json = nlohmann::ordered_json;

enum class ReportFormat { JSON, XML };

class ReportBuilder {
private:
  ReportFormat format_;
  std::filesystem::path trace_file_;
  std::vector<TLP> tlps_;
  std::vector<ValidationError> errors_;
public:
  explicit ReportBuilder(ReportFormat format, std::filesystem::path trace_file);


  /**
   * @brief Registers a decoded TLP in the report.
   * @param tlp The TLP to register.
   */
  void add_tlp(const TLP &tlp);

  /**
   * @brief Registers a validation error in the report.
   * @param error The validation error to register.
   */
  void add_validation_error(const ValidationError &error);

  /**
   * @brief Writes the complete report to the specified output path.
   * @param output_path The path to the output file.
   * @param skipped_line_count Total lines skipped during input processing.
   */
  void write(const std::filesystem::path &output_path,
             std::uint64_t skipped_line_count) const;
};
#endif