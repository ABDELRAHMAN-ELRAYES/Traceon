#include "packet_decoder.h"

std::vector<std::uint32_t> PacketDecoder::parsePacketDws(const std::string &hexaRawBytes)
{
    std::vector<std::uint32_t> dws{};

    std::size_t rawBytesSize = hexaRawBytes.size();
    int i = 0;
    while (i < rawBytesSize)
    {
        // Divide the raw bytes by the double word size
        std::string dw = hexaRawBytes.substr(i, DW_HEXA_DIGITS_NUMBER);

        // convert the extracted hexa dw
        dws.push_back(static_cast<std::uint32_t>(std::stoul(dw, nullptr, 16)));

        i += DW_HEXA_DIGITS_NUMBER;
    }
    return dws;
}

TLP PacketDecoder::decode(const Packet &packet)
{
    // Get the main fields of the packet
    std::uint64_t timestamp = packet.m_timestamp;
    Direction direction = packet.m_direction;
    std::string hexaRawBytes = packet.m_rawBytes;

    // The Number of Digits of the raw bytes
    std::size_t rawBytesSize = hexaRawBytes.size();

    // Create a TLP instance
    TLP tlp{};

    // Check if the digits number less than 3DW(24 digits)
    if (rawBytesSize < 3 * DW_HEXA_DIGITS_NUMBER)
    {
        tlp.m_is_malformed = true;
        tlp.m_decode_errors.push_back("The Numer of digits of this hexadecimal raw bytes is less than the normal packet(3DW = 3 * 8 = 24 digits) ");
    }

    // Check if the digits number is consists of fixed number of  DW size
    if (rawBytesSize % DW_HEXA_DIGITS_NUMBER != 0)
    {
        tlp.m_is_malformed = true;
        tlp.m_decode_errors.push_back("The Numer of digits of this hexadecimal raw bytes is not completed");
    }
    // Store the DWs of the raw hexa bytes
    std::vector<std::uint32_t> dws{parsePacketDws(hexaRawBytes)};



    std::cout << "Timestamp: " << timestamp << "\n"
              << "Direction: " << Utils::directionToString(direction) << "\n"
              << "Raw Hexadecimal: ";
    for (const std::uint32_t &val : dws)
    {
        std::cout << val << " ";
    }

    std::cout << "\n";
    return tlp;
}