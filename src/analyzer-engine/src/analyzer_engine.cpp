#include "analyzer-engine/analyzer_engine.h"
#include "analyzer-engine/core/packet/packet.h"
#include "analyzer-engine/decoder/packet_decoder.h"
#include "analyzer-engine/input-layer/input_layer.h"
#include "analyzer-engine/report-builder/report_builder.h"
#include "analyzer-engine/validator/protocol_validator.h"
#include <iostream>
#include <optional>
#include <system_error>
#include <vector>

AnalyzerEngine::AnalyzerEngine(std::filesystem::path trace_path,
                               std::filesystem::path report_path,
                               ReportFormat report_format, bool verbose)
    : trace_path_(std::move(trace_path)), report_path_(std::move(report_path)),
      report_format_(report_format), verbose_(verbose) {
  std::error_code ec;
  if (std::filesystem::is_directory(report_path_, ec) || report_path_.filename().empty()) {
    if (report_format_ == ReportFormat::JSON) {
      report_path_ /= "report.json";
    } else if (report_format_ == ReportFormat::XML) {
      report_path_ /= "report.xml";
    }
  }
}

void AnalyzerEngine::run() {
  TraceInputLayer inputLayer{trace_path_};
  ProtocolValidator validator;
  ReportBuilder reportBuilder(report_format_, trace_path_, report_path_);

  // Process packets in a single pass streaming manner
  while (!inputLayer.isExhausted()) {
    std::optional<Packet> packetopt = inputLayer.next();
    if (packetopt.has_value()) {
      Packet &packet = packetopt.value();
      TLP tlp = PacketDecoder::decode(packet);
      if (verbose_) {
        tlp.printPacketDetails();
      }
      std::vector<ValidationError> validation_errors;
      if (!tlp.isMalformed()) {
        validation_errors = validator.process(tlp);
      }
      reportBuilder.addTLP(tlp, validation_errors);
      if (verbose_) {
        std::cout << "=========================\n";
      }
    }
  }

  if (verbose_) {
    std::cout << "\n[ Finalizing Protocol Validation ]\n";
  }
  auto late_errors = validator.finalize();

  for (const auto &err : late_errors) {
    reportBuilder.addValidationError(err);
  }

  if (!late_errors.empty()) {
    std::cout << "WARNING: " << late_errors.size()
              << " late protocol violation(s) found (e.g. missing "
                 "completions):\n";
    if (verbose_) {
      for (const auto &err : late_errors) {
        std::cout << "  - [" << err.rule_id << "] " << err.description
                  << " (Packet Index: " << err.packet_index << ")\n";
      }
    }
  }

  if (verbose_) {
    std::cout << "\n[ Generating Report ]\n";
  }
  reportBuilder.write(report_path_, inputLayer.skippedLineCount());
  std::cout << "Report generated at: " << report_path_ << "\n";
}