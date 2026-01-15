#include "packet.h"
#include "../../utils/utils.h"

/*
    - Construct a Packet
*/
Packet::Packet(std::uint64_t timestamp, std::string direction, std::string rawBytes)
    : m_timestamp{timestamp}, m_direction{Utils::stringToDirection(direction)}, m_rawBytes{rawBytes} {}

/*
    - Print Packet Information
*/
void Packet::print() const
{
    std::cout << "Timestamp: " << m_timestamp << "\n"
              << "Direction: " << Utils::directionToString(m_direction) << "\n"
              << "Raw Bytes: " << m_rawBytes << "\n";
}