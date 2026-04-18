#include "analyzer-engine/core/packet/packet.h"
#include "analyzer-engine/utils/utils.h"
#include <iostream>

/*
    - Construct a Packet
*/
Packet::Packet(std::uint64_t timestamp, Direction direction, std::uint64_t index,
               std::string rawBytes)
    : m_timestamp{timestamp}, m_direction{direction}, m_index{index},
      m_rawBytes{std::move(rawBytes)} {}

/*
    - Print Packet Information
*/
void Packet::print() const {
  std::cout << "Timestamp: " << m_timestamp << "\n"
            << "Direction: " << Utils::directionToString(m_direction) << "\n"
            << "Raw Bytes: " << m_rawBytes << "\n";
}