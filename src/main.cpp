#include "analyzer-engine/core/packet/packet.h"
#include "analyzer-engine/decoder/packet_decoder.h"
#include "analyzer-engine/input-layer/input_layer.h"
#include "analyzer-engine/report-builder/report_builder.h"
#include "analyzer-engine/validator/protocol_validator.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <vector>

const std::string TRACE_FILE_PATH = "../data/trace_data.csv";
const std::filesystem::path REPORT_PATH = "../data/report.json";
int main() {
  std::filesystem::path path{TRACE_FILE_PATH};
  TraceInputLayer inputLayer{path};
  ProtocolValidator validator;
  ReportBuilder reportBuilder(ReportFormat::JSON, path);

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
    tlp.printPacketDetails();
    validator.process(tlp);
    reportBuilder.add_tlp(tlp);
    std::cout << "=========================\n";
  }

  std::cout << "\n[ Finalizing Protocol Validation ]\n";
  auto errors = validator.finalize();

  for (const auto &err : errors) {
    reportBuilder.add_validation_error(err);
  }

  if (errors.empty()) {
    std::cout << "SUCCESS: No protocol violations found.\n";
  } else {
    std::cout << "WARNING: " << errors.size()
              << " protocol violation(s) found:\n";
    for (const auto &err : errors) {
      std::cout << "  - [" << err.rule_id << "] " << err.description
                << " (Packet Index: " << err.packet_index << ")\n";
    }
  }

  std::cout << "\n[ Generating Report ]\n";
  reportBuilder.write(REPORT_PATH, inputLayer.skipped_line_count());
  std::cout << "Report generated at: " << REPORT_PATH << "\n";

  return 0;
}