#include "backend/core/packet/packet.h"
#include "backend/decoder/packet_decoder.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <iostream>

const std::string TRACE_FILE_PATH = "../data/trace_data.csv";

int main()
{

    // Open a file stream read connection
    std::fstream fin;
    fin.open(TRACE_FILE_PATH, std::ios::in);

    // Check if the file connection opened
    if (!fin.is_open())
    {
        std::cout << "File is not exist\n";
        return 1;
    }

    std::string line{}, word{};

    // Recive the packets in order according to the arrival timestamp
    // vector<string> -> Timestamp, Direction, TLP Hexadecimal header
    std::queue<Packet> packets{};

    // Read the Row Packets line by line
    while (getline(fin, line))
    {
        std::stringstream s{line};
        std::vector<std::string> cols{};

        // Read the packet columns by the seperated comma
        while (getline(s, word, ','))
        {
            cols.push_back(word);
        }
        if (cols.size() == 3)
        {
            std::uint64_t timestamp{std::stoull(cols[0])};
            std::string direction{cols[1]};
            std::string rawBytes{cols[2]};
            Packet packet{timestamp, direction, rawBytes};
            packets.push(packet);
        }
        else
            continue;
    }

    /*
        -
    */
    while (!packets.empty())
    {
        Packet packet{packets.front()};
        packets.pop();
        TLP tlp = PacketDecoder::decode(packet);
        tlp.printPacketDetails();
        std::cout << "=========================\n";
    }

    return 0;
}