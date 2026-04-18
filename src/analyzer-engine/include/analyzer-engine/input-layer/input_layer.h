#ifndef INPUT_LAYER
#define INPUT_LAYER

#include "analyzer-engine/core/packet/packet.h"
#include <filesystem>
#include <fstream>
#include <optional>

class TraceInputLayer {
private:
  std::ifstream file;
  std::uint64_t m_packetCounter{0};

public:
  explicit TraceInputLayer(const std::filesystem::path &path);
  std::optional<Packet> next();
  bool isExhausted();
};
#endif