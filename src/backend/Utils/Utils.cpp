#include "Utils.h"
#include <algorithm>

namespace Utils
{
    /*
        - Convert the packet Direction from string (file read type) to its enum type
    */
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
}