
#include "analyzer-engine/input-layer/input_layer.h"
#include "analyzer-engine/input-layer/input_exception.h"
#include "analyzer-engine/utils/utils.h"

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

TraceInputLayer::TraceInputLayer(const std::filesystem::path &path)
    : file_(path) {
  if (!file_.is_open()) {
    throw TraceInputException("Failed to open trace file");
  }
}

bool TraceInputLayer::isExhausted() { return file_.eof(); }


std::optional<Packet> TraceInputLayer::next() {
  std::string line{};
  std::getline(file_, line);

  size_t first_non_ws = line.find_first_not_of(" \t\r\n");
  if (first_non_ws == std::string::npos || line[first_non_ws] == '#') {
    return std::nullopt;
  }
  std::stringstream lineStream{};
  lineStream << line;

  std::string word{};
  // vector<string> -> Timestamp, Direction, Packet Raw bytes in hexdecimal
  std::vector<std::string> cols;
  while (std::getline(lineStream, word, ',')) {
    if (!word.empty())
      cols.push_back(word);
  }

  if (cols.size() != 3) {
    skipped_line_count_++;
    return std::nullopt;
  }

  if (cols[0] == "timestamp") {
    skipped_line_count_++;
    return std::nullopt;
  }

  std::uint64_t timestamp{std::stoull(cols[0])};
  std::string directionStr{cols[1]}, rawBytes{cols[2]};
  Packet packet{timestamp, Utils::stringToDirection(directionStr),
                packet_counter_++, rawBytes};

  return packet;
}