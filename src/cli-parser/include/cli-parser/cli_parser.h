#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include "analyzer-engine/report-builder/report_builder.h"
#include <filesystem>
#include <optional>

/**
 * @brief Holds the parameters parsed from the command line.
 */
struct CLIConfig {
  std::filesystem::path input_path;
  std::filesystem::path output_path;
  ReportFormat format = ReportFormat::JSON;
  bool verbose = false;
};

/**
 * @brief Parse CLI arguments.
 */
class CLIParser {
public:
  /**
   * @brief Parse CLI arguments.
   * @param argc Number of arguments.
   * @param argv Array of argument strings.
   * @return std::optional<CLIConfig> The parsed configuration if successful,
   * nullopt otherwise.
   */
  static std::optional<CLIConfig> parse(int argc, char **argv);

  /**
   * @brief Prints the usage help reference to stdout.
   */
  static void printUsage();

  /**
   * @brief Prints the version information to stdout.
   */
  static void printVersion();
};

#endif
