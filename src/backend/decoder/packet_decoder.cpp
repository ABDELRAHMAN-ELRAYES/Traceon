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

TranslatedFmt PacketDecoder::translateFmtHeader(std::uint8_t rawFmt)
{
    TranslatedFmt translatedFmt{false, Fmt::UNKNOWN};
    auto it = fmtMap.find(rawFmt);
    if (it != fmtMap.end())
        return it->second;
    return translatedFmt;
}

TlpType translatePacketType(std::uint8_t rawType)
{
    auto it = typeMap.find(rawType);
    if (it != typeMap.end())
        return it->second;
    return TlpType::UNKNOWN;
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
        tlp.m_isMalformed = true;
        tlp.m_decodeErrors.push_back("The Numer of digits of this hexadecimal raw bytes is less than the normal packet(3DW = 3 * 8 = 24 digits).");
        return tlp;
    }

    // Check if the digits number is consists of fixed number of  DW size
    if (rawBytesSize % DW_HEXA_DIGITS_NUMBER != 0)
    {
        tlp.m_isMalformed = true;
        tlp.m_decodeErrors.push_back("The Numer of digits of this hexadecimal raw bytes is not completed.");
        return tlp;
    }
    // Store the DWs of the raw hexa bytes
    std::vector<std::uint32_t> dws{parsePacketDws(hexaRawBytes)};

    // Packet Double words
    std::uint32_t dw0 = dws[0];
    std::uint32_t dw1 = dws[1];
    std::uint32_t dw2 = dws[2];

    // Mandatory Packet fields

    // Traanslate fmt extracted bits into readable format (Type + has Data or not)
    std::uint8_t fmt = Utils::extractBits(dw0, 29, 31);

    TranslatedFmt translatedFmt = PacketDecoder::translateFmtHeader(fmt);

    if (translatedFmt.type == Fmt::UNKNOWN)
    {
        tlp.m_isMalformed = true;
        tlp.m_decodeErrors.push_back("Fmt Header Type is not supported, UNKNOWN fmt header.");
        return tlp;
    }
    tlp.m_fmt = translatedFmt.type;

    // Translated type extracted bits into reaadable format
    std::uint8_t type = Utils::extractBits(dw0, 24, 28);

    TlpType translatedTlpType = PacketDecoder::translatePacketType(type);

    if (translatedTlpType == TlpType::UNKNOWN)
    {
        tlp.m_isMalformed = true;
        tlp.m_decodeErrors.push_back("Tlp Type is not supported, UNKNOWN packet type.");
        return tlp;
    }
    tlp.m_type = translatedTlpType;

    

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