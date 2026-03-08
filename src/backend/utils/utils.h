#ifndef UTILS_NAMESPACE
#define UTILS_NAMESPACE

#include "packet/packet.h"
#include "tlp/tlp.h"

#include <string>
#include <cstdint>
#include <algorithm>

namespace Utils
{
    /*
        - Convet String to Direction Type
        - Use Case: Convert parsed raw packet direction from the file to Direction type
    */
    Direction stringToDirection(const std::string &strDirection);

    /*
        - Convert the Direction type to string
        - Use Case: Print the Packet Info
    */
    std::string directionToString(Direction direction);

    /*
        - Extract a specific range of bits from a value
    */
    constexpr std::uint32_t extractBits(std::uint32_t value, int start, int end)
    {
        return (value >> start) & ((1u << (end - start + 1)) - 1);
    }

    /*
        - Convert the TLP type to string
        - Use Case: Print the Packet Info
    */
    std::string tlpTypeToString(TlpType type);

    std::string fmtToStr(Fmt fmt);

    std::string completionStatusToStr(CompletionStatus status);

}

#endif