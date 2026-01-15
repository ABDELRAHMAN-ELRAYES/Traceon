#include "../core/packet/packet.h"
#include <string>

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

}