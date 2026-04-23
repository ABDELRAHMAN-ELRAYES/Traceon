#include <analyzer-engine/analyzer_engine.h>
#include <analyzer-engine/input-layer/input_exception.h>
#include <cli-parser/cli_parser.h>
#include <iostream>

int main(int argc, char *argv[]) {
  std::optional<CLIConfig> config_opt = CLIParser::parse(argc, argv);
  if (!config_opt) {
    std::cout << "\nRun with --help for usage information.\n";
    return 3;
  }

  const auto &config = *config_opt;

  try {
    AnalyzerEngine analyzer{config.input_path, config.output_path,
                            config.format, config.verbose};
    analyzer.run();
  } catch (const TraceInputException &e) {
    std::cout << "[FATAL] Trace Input Failure: " << e.what() << "\n";
    return 1;
  } catch (const std::exception &e) {
    std::cout << "[FATAL] Execution Failure: " << e.what() << "\n";
    return 1;
  }

  return 0;
}