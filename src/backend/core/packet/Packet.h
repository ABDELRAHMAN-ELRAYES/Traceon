#ifndef PACKET_H
#define PACKET_H
#include <cstdint>
#include <string>
#include <iostream>

/*
   - Packet Direction Type
*/
enum class Direction
{
    TX,     // DownStream: (Host -> Device)
    RX,     // Upstream: (Device -> Host)
    UNKNOWN // unvalid direction
};

/*
    - Express the Raw Packet From Trace Logs
*/
class Packet
{
protected:
    std::uint64_t m_timestamp{};
    Direction m_direction{};
    std::string m_rawBytes{};

public:
    Packet() = default;
    Packet(std::uint64_t timestamp, std::string direction, std::string rawBytes);
    void print() const;
};

#endif