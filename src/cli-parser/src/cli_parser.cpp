#include "cli-parser/cli_parser.h"
#include <iostream>
#include <string_view>

std::optional<CLIConfig> CLIParser::parse(int argc, char *argv[]) {
  CLIConfig config;

  for (int i = 1; i < argc; ++i) {
    std::string_view arg = argv[i];

    if (arg == "--help" || arg == "-h") {
      printUsage();
      exit(0);
    } else if (arg == "--version") {
      printVersion();
      exit(0);
    } else if (arg == "--input" || arg == "-i") {
      if (++i < argc) {
        config.input_path = argv[i];
      } else {
        std::cout << "[ERROR] Argument " << arg << " requires a path value.\n";
        return std::nullopt;
      }
    } else if (arg == "--output" || arg == "-o") {
      if (++i < argc) {
        config.output_path = argv[i];
      } else {
        std::cout << "[ERROR] Argument " << arg << " requires a path value.\n";
        return std::nullopt;
      }
    } else if (arg == "--format" || arg == "-f") {
      if (++i < argc) {
        std::string_view fmt = argv[i];
        if (fmt == "json") {
          config.format = ReportFormat::JSON;
        } else if (fmt == "xml") {
          config.format = ReportFormat::XML;
        } else {
          std::cout << "[ERROR] Invalid format '" << fmt
                    << "'. Use 'json' or 'xml'.\n";
          return std::nullopt;
        }
      } else {
        std::cout << "[ERROR] Argument " << arg
                  << " requires a format value.\n";
        return std::nullopt;
      }
    } else if (arg == "--verbose" || arg == "-v") {
      config.verbose = true;
    } else if (!arg.empty() && arg[0] == '-') {
      std::cout << "[ERROR] Unknown option '" << arg
                << "'. Use --help for usage.\n";
      return std::nullopt;
    } else {
      std::cout << "[ERROR] Unexpected argument '" << arg << "'.\n";
      return std::nullopt;
    }
  }

  // Mandatory Arguments
  if (config.input_path.empty()) {
    std::cout << "[ERROR] Missing mandatory argument: --input <path>\n";
    return std::nullopt;
  }
  if (config.output_path.empty()) {
    std::cout << "[ERROR] Missing mandatory argument: --output <path>\n";
    return std::nullopt;
  }

  // Input Trace file is not exist
  if (!std::filesystem::exists(config.input_path)) {
    std::cout << "[ERROR] Input file does not exist: " << config.input_path
              << "\n";
    exit(1);
  }

  return config;
}

void CLIParser::printUsage() {
  std::cout
      << "Usage: Traceon --input <trace_file> --output <report_file> "
         "[options]\n\n"
      << "Core Arguments (Required):\n"
      << "  -i, --input <path>      Filesystem path to the input trace file "
         "(CSV).\n"
      << "  -o, --output <path>     Filesystem path for the generated analysis "
         "report.\n\n"
      << "Options:\n"
      << "  -f, --format <json|xml> Report output format. (Default: json)\n"
      << "  -v, --verbose           Enable per-packet progress logging to "
         "stdout.\n"
      << "  --version               Print analyzer version and exit.\n"
      << "  -h, --help              Print this usage reference and exit.\n\n";
}

void CLIParser::printVersion() {
  std::cout << "Traceon version 1.0.0 (Traceon Core Phase 1)\n";
}
