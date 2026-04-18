#include "analyzer-engine/core/packet/packet.h"
#include "analyzer-engine/decoder/packet_decoder.h"
#include "analyzer-engine/input-layer/input_layer.h"
#include "analyzer-engine/validator/protocol_validator.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <queue>

const std::string TRACE_FILE_PATH = "../data/trace_data.csv";

int main() {

  std::filesystem::path path{TRACE_FILE_PATH};
  TraceInputLayer inputLayer{path};
  ProtocolValidator validator;

  std::queue<Packet> packets{};

  // Read the Row Packets line by line
  while (!inputLayer.isExhausted()) {
    std::optional<Packet> packetopt = inputLayer.next();
    if (packetopt.has_value()) {
      packets.push(packetopt.value());
    }
  }
  while (!packets.empty()) {
    Packet packet{packets.front()};
    packets.pop();
    TLP tlp = PacketDecoder::decode(packet);
    tlp.printPacketDetails();
    validator.process(tlp);
    std::cout << "=========================\n";
  }

  std::cout << "\n[ Finalizing Protocol Validation ]\n";
  auto errors = validator.finalize();

  if (errors.empty()) {
    std::cout << "SUCCESS: No protocol violations found.\n";
  } else {
    std::cout << "WARNING: " << errors.size() << " protocol violation(s) found:\n";
    for (const auto &err : errors) {
      std::cout << "  - [" << err.rule_id << "] " << err.description
                << " (Packet Index: " << err.packet_index << ")\n";
    }
  }

  return 0;
}