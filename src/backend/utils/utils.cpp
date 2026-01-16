#include "utils.h"
#include <algorithm>

namespace Utils
{

    Direction stringToDirection(std::string &strDirection)
    {
        std::string strDirectionCopy = strDirection;

        // Convert the entered string direction to upper case for type safety
        std::transform(strDirectionCopy.begin(), strDirectionCopy.end(), strDirectionCopy.begin(), ::toupper);

        if (strDirectionCopy == "TX")
            return Direction::TX;
        if (strDirectionCopy == "RX")
            return Direction::RX;
        return Direction::UNKNOWN;
    }

    std::string directionToString(Direction direction)
    {
        switch (direction)
        {

        case Direction::TX:
            return "TX";
        case Direction::RX:
            return "RX";
        }
        return "UNKOWN";
    }

    constexpr std::uint32_t extractBits(std::uint32_t value, int start, int end)
    {
        return (value >> start) & ((1u << (end - start + 1)) - 1);
    }
}