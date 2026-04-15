#include "analyzer-engine/core/packet/packet.h"
#include "analyzer-engine/decoder/packet_decoder.h"
#include "analyzer-engine/input-layer/input_layer.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <queue>

const std::string TRACE_FILE_PATH = "../data/trace_data.csv";

int main() {

  std::filesystem::path path{TRACE_FILE_PATH};
  TraceInputLayer inputLayer{path};

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
    std::cout << "=========================\n";
  }

  return 0;
}