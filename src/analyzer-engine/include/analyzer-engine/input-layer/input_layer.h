#ifndef INPUT_LAYER
#define INPUT_LAYER

#include "analyzer-engine/core/packet/packet.h"
#include <filesystem>
#include <fstream>
#include <optional>

class TraceInputLayer {
private:
  std::ifstream file_;
  std::uint64_t packet_counter_{0};
  std::uint64_t skipped_line_count_{0};

public:
  explicit TraceInputLayer(const std::filesystem::path &path);
  /**
 * @brief Read the file line by line and Form the Raw Packet
 *
 * @return std::optional<Packet>
 */
  std::optional<Packet> next();
 /**
 * @brief Checks if the file is completely read
 *
 * @return true if the file is completely read
 * @return false
 */
  bool isExhausted();
  std::uint64_t skipped_line_count() const { return skipped_line_count_; }
};
#endif