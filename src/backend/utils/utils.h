#include "../core/packet/packet.h"
#include <string>
#include <cstdint>

namespace Utils
{
    /*
        - Convet String to Direction Type
        - Use Case: Convert parsed raw packet direction from the file to Direction type
    */
    Direction stringToDirection(std::string &strDirection);

    /*
        - Convert the Direction type to string
        - Use Case: Print the Packet Info
    */
    std::string directionToString(Direction direction);

    /*
        - Extract a specific range of bits from a value
    */
    constexpr std::uint32_t extractBits(std::uint32_t value, int start, int end);

}