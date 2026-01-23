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
    auto iterator = fmtMap.find(rawFmt);
    if (iterator != fmtMap.end())
    {
        return iterator->second;
    }
    return translatedFmt;
}

TlpType translatePacketType(std::uint8_t rawType)
{
    auto iterator = typeMap.find(rawType);
    if (iterator != typeMap.end())
    {
        return iterator->second;
    }
    return TlpType::UNKNOWN;
}
CompletionStatus translatePacketCompletionStatus(std::uint8_t rawStatus)
{
    auto iterator = completionStatusMap.find(rawStatus);
    if (iterator != completionStatusMap.end())
    {
        return iterator->second;
    }
    return CompletionStatus::UNKNOWN;
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

    //  Extract and Assign the TC value
    std::uint8_t tc = Utils::extractBits(dw0, 20, 23);
    if (tc > MAXIMUM_TC)
    {
        tlp.m_isMalformed = true;
        tlp.m_decodeErrors.push_back("Unsupported value for Tc, Out of Possible Range.");
        return tlp;
    }
    tlp.m_tc = tc;

    // Extract and Assign the Attributes fields
    std::uint8_t noSnoop = Utils::extractBits(dw0, 12, 12);
    std::uint8_t relaxedOrdering = Utils::extractBits(dw0, 13, 13);
    bool attrNoSnoop = noSnoop == 0 ? false : true;
    bool attrRelaxedOrdering = relaxedOrdering == 0 ? false : true;

    Attr attr{attrNoSnoop, attrRelaxedOrdering};
    tlp.m_attr = attr;

    // Extract and Assign the length of Double Word if its not null(equals to zero)
    std::uint16_t length = Utils::extractBits(dw0, 0, 9);

    if (tlp.m_type == TlpType::MRd || tlp.m_type == TlpType::MWr || tlp.m_type == TlpType::CplD)
    {
        if (length == 0)
        {
            tlp.m_isMalformed = true;
            tlp.m_decodeErrors.push_back("Malformed Packet with a data type but has a ZERO Double word length");
            return tlp;
        }
        else
        {
            tlp.m_lengthDw = length;
        }
    }
    else
    {
        if (length != 0)
        {
            tlp.m_isMalformed = true;
            tlp.m_decodeErrors.push_back("Malformed Packet with a non-data type but has a NON-ZERO Double word length");
            return tlp;
        }
    }

    // Declare the requesterId and Tag -> Extract their values based on the packet type
    std::uint16_t requesterId{};
    std::uint8_t tag{};
    std::uint64_t address{};
    
    if (tlp.m_type == TlpType::MRd || tlp.m_type == TlpType::MWr)
    {
        // TODO : This Requester Id must be formated [Bus]:[Device]:[Function]
        requesterId = Utils::extractBits(dw1, 16, 31);
        tag = Utils::extractBits(dw1, 8, 15);

        if (tlp.m_fmt == Fmt::DW3)
        {
            // Neglect the lowest two bits of the DW2 - they are always used for flags/ format bits so they aren't a part of the address
            address = static_cast<std::uint64_t>(dw2) & 0xFFFFFFFCu;
        }
        else
        {
            if (dws.size() < 4)
            {
                tlp.m_isMalformed = true;
                tlp.m_decodeErrors.push_back("Malformed Packet of 4Dw header fmt but it has less than 4 DW.");
                return tlp;
            }

            // Neglect the last two  bits from the lowes DW and then concat the two DWs to form the address in 4DW header fmt types
            std::uint32_t dw3 = dws[3];
            address =
                (static_cast<std::uint64_t>(dw2) << 32) |
                (static_cast<std::uint64_t>(dw3) & 0xFFFFFFFCu);
        }

        // Assign the address
        tlp.m_address = address;
    }
    else
    {
        requesterId = Utils::extractBits(dw2, 16, 31);
        tag = Utils::extractBits(dw2, 8, 15);

        // TODO : This Completer Id must be formated [Bus]:[Device]:[Function]
        std::uint16_t completerId = Utils::extractBits(dw1, 16, 31);

        // Extract and Translate the raw completion status into readable format
        std::uint8_t status = Utils::extractBits(dw1, 13, 15);
        CompletionStatus translatedCompletionStatus = translatePacketCompletionStatus(status);
        if (translatedCompletionStatus == CompletionStatus::UNKNOWN)
        {
            tlp.m_isMalformed = true;
            tlp.m_decodeErrors.push_back("Malformed Packet with UNKNOWN Completion Status.");
            return tlp;
        }
        tlp.m_status = translatedCompletionStatus;

        // Extract the byte count
        std::uint16_t byteCount = Utils::extractBits(dw1, 0, 11);
        tlp.m_byteCount = byteCount;
    }

    // TODO: Assign the requesterId after conversion to its string format
    tlp.m_tag = tag;

    std::cout << "Timestamp: " << timestamp << "\n"
              << "Direction: " << Utils::directionToString(direction) << "\n"
              << "Raw Hexadecimal: ";
    for (const std::uint32_t &val : dws)
    {
        std::cout << std::hex << val << " ";
    }
    std::cout << std::dec << "\n";

    return tlp;
}
