#include "analyzer-engine/analyzer_engine.h"
#include "analyzer-engine/core/packet/packet.h"
#include "analyzer-engine/decoder/packet_decoder.h"
#include "analyzer-engine/input-layer/input_layer.h"
#include "analyzer-engine/report-builder/report_builder.h"
#include "analyzer-engine/validator/protocol_validator.h"
#include <iostream>
#include <optional>
#include <vector>

AnalyzerEngine::AnalyzerEngine(std::filesystem::path trace_path,
                               std::filesystem::path report_path,
                               ReportFormat report_format, bool verbose)
    : trace_path_(std::move(trace_path)), report_path_(std::move(report_path)),
      report_format_(report_format), verbose_(verbose) {}

void AnalyzerEngine::run() {
  TraceInputLayer inputLayer{trace_path_};
  ProtocolValidator validator;
  ReportBuilder reportBuilder(report_format_, trace_path_);

  std::vector<Packet> packets{};
  // Read the Raw Packets
  while (!inputLayer.isExhausted()) {
    std::optional<Packet> packetopt = inputLayer.next();
    if (packetopt.has_value()) {
      packets.push_back(packetopt.value());
    }
  }

  for (const auto &packet : packets) {
    TLP tlp = PacketDecoder::decode(packet);
    if (verbose_) {
      tlp.printPacketDetails();
    }
    validator.process(tlp);
    reportBuilder.addTLP(tlp);
    if (verbose_) {
      std::cout << "=========================\n";
    }
  }

  if (verbose_) {
    std::cout << "\n[ Finalizing Protocol Validation ]\n";
  }
  auto errors = validator.finalize();

  for (const auto &err : errors) {
    reportBuilder.addValidationError(err);
  }

  if (errors.empty()) {
    std::cout << "SUCCESS: No protocol violations found.\n";
  } else {
    std::cout << "WARNING: " << errors.size()
              << " protocol violation(s) found:\n";
    if (verbose_) {
      for (const auto &err : errors) {
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