#ifndef PACKET_DECODER_H
#define PACKET_DECODER_H

#include "../core/packet/packet.h"
#include "../core/tlp/tlp.h"
#include "../utils/utils.h"

#include <vector>
#include <string>
#include <cstdint>
#include <iostream>

constexpr int DW_HEXA_DIGITS_NUMBER = 8;

class TLP;

class PacketDecoder
{

private:
    /*
        - Parse Double Words(DWs) from the hexadecimal raw bytes packet string
    */
    static std::vector<std::uint32_t> parsePacketDws(const std::string &rawBytes);

public:
    /*
        - Decode the Packet Raw Hexadecimal into Protocol level packet like (TLP)
    */
    static TLP decode(const Packet &packet);
};

#endif