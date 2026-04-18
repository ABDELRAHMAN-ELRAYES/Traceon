#include "analyzer-engine/core/packet/packet.h"
#include "analyzer-engine/utils/utils.h"
#include <iostream>

/*
    - Construct a Packet
*/
Packet::Packet(std::uint64_t timestamp, Direction direction, std::uint64_t index,
               std::string rawBytes)
    : timestamp_{timestamp}, direction_{direction}, index_{index},
      raw_bytes_{std::move(rawBytes)} {}

/*
    - Print Packet Information
*/
void Packet::print() const {
  std::cout << "Timestamp: " << timestamp_ << "\n"
            << "Direction: " << Utils::directionToString(direction_) << "\n"
            << "Raw Bytes: " << raw_bytes_ << "\n";
}