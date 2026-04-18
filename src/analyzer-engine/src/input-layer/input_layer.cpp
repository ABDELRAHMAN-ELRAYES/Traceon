
#include "analyzer-engine/input-layer/input_layer.h"
#include "analyzer-engine/input-layer/input_exception.h"
#include "analyzer-engine/utils/utils.h"

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

TraceInputLayer::TraceInputLayer(const std::filesystem::path &path)
    : file(path) {
  if (!file.is_open()) {
    throw TraceInputException("Failed to open trace file");
  }
}
/**
 * @brief Checks if the file is completely read
 *
 * @return true if the file is completely read
 * @return false
 */
bool TraceInputLayer::isExhausted() { return file.eof(); }

/**
 * @brief Read the file line by line and Form the Raw Packet
 *
 * @return std::optional<Packet>
 */
std::optional<Packet> TraceInputLayer::next() {
  std::string line{};
  std::getline(file, line);

  // Ignore Empty / Comments lines
  if ((!line.empty() && line[0] == '#') || line.empty())
    return std::nullopt;
  std::stringstream lineStream{};
  lineStream << line;

  std::string word{};
  // vector<string> -> Timestamp, Direction, Packet Raw bytes in hexdecimal
  std::vector<std::string> cols;
  while (std::getline(lineStream, word, ',')) {
    if (!word.empty())
      cols.push_back(word);
  }

  if (cols.size() != 3)
    return std::nullopt;

  // Ignore the csv cols declaration
  if (cols[0] == "timestamp")
    return std::nullopt;

  std::uint64_t timestamp{std::stoull(cols[0])};
  std::string directionStr{cols[1]}, rawBytes{cols[2]};
  Packet packet{timestamp, Utils::stringToDirection(directionStr),
                m_packetCounter++, rawBytes};

  return packet;
}