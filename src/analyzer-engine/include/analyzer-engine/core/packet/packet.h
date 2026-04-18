#ifndef PACKET_H
#define PACKET_H

#include <cstdint>
#include <string>

/*
   - Packet Direction Type
*/
enum class Direction {
  TX,     // DownStream: (Host -> Device)
  RX,     // Upstream: (Device -> Host)
  UNKNOWN // unvalid direction
};

class PacketDecoder;

/*
    - Express the Raw Packet From Trace Logs
*/
class Packet {
private:
  std::uint64_t m_timestamp{};
  Direction m_direction{};
  std::uint64_t m_index{};
  std::string m_rawBytes{};

public:
  Packet() = default;
  Packet(std::uint64_t timestamp, Direction direction, std::uint64_t index,
         std::string rawBytes);
  void print() const;
  std::uint64_t index() const { return m_index; }
  friend class PacketDecoder;
};

#endif