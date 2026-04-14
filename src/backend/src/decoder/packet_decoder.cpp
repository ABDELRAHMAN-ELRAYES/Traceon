#include "backend/decoder/packet_decoder.h"
#include "backend/core/packet/packet.h"
#include "backend/utils/utils.h"

std::vector<std::uint32_t> PacketDecoder::parsePacketDws(const std::string &hexaRawBytes)
{
    std::vector<std::uint32_t> dws{};

    std::size_t rawBytesSize = hexaRawBytes.size();
    std::size_t i = 0;
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

TlpType PacketDecoder::translatePacketType(std::uint8_t rawType)
{
    auto iterator = typeMap.find(rawType);
    if (iterator != typeMap.end())
    {
        return iterator->second;
    }
    return TlpType::UNKNOWN;
}

CompletionStatus PacketDecoder::translatePacketCompletionStatus(std::uint8_t rawStatus)
{
    auto iterator = completionStatusMap.find(rawStatus);
    if (iterator != completionStatusMap.end())
    {
        return iterator->second;
    }
    return CompletionStatus::UNKNOWN;
}

std::string PacketDecoder::translateBdfId(std::uint16_t rawId)
{
    // Extract the bus, device, function ->> (8, 5, 3) bits
    std::uint8_t bus = (rawId >> 8) & 0xFF;
    std::uint8_t device = (rawId >> 3) & 0x1F;
    std::uint8_t function = rawId & 0x07;

    // Store the Translated ID
    std::stringstream id{};

    // Domain (always 0000)
    id << "0000:";

    // Set the number base for the input numbers to Hexadecimal
    id << std::hex;
    id.width(2);
    id.fill('0');
    id << static_cast<unsigned int>(bus);
    id << ':';

    id.width(2);
    id.fill('0');
    id << static_cast<unsigned int>(device);
    id << '.';

    id.width(1);
    id << static_cast<unsigned int>(function);

    return id.str();
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
        tlp.m_decodeErrors.push_back("Raw hex string is shorter than the minimum TLP size (3 DW = 24 hex digits).");
        return tlp;
    }

    // Check if the digits number is consists of fixed number of DW size
    if (rawBytesSize % DW_HEXA_DIGITS_NUMBER != 0)
    {
        tlp.m_isMalformed = true;
        tlp.m_decodeErrors.push_back("Raw hex string length is not a multiple of one DW (8 hex digits). Packet is truncated.");
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
    std::uint8_t fmt = static_cast<std::uint8_t>(Utils::extractBits(dw0, 29, 31));
    TranslatedFmt translatedFmt = PacketDecoder::translateFmtHeader(fmt);

    if (translatedFmt.type == Fmt::UNKNOWN)
    {
        tlp.m_isMalformed = true;
        tlp.m_decodeErrors.push_back("Illegal Fmt value (" + std::to_string(fmt) + "). Only 000–011 are valid.");
    }
    tlp.m_fmt = translatedFmt.type;

    // Translated type extracted bits into reaadable format
    std::uint8_t type = static_cast<std::uint8_t>(Utils::extractBits(dw0, 24, 28));
    TlpType translatedTlpType = PacketDecoder::translatePacketType(type);

    if (translatedTlpType == TlpType::UNKNOWN)
    {
        tlp.m_isMalformed = true;
        tlp.m_decodeErrors.push_back("Unsupported TLP Type value (" + std::to_string(type) + ").");
    }
    tlp.m_type = translatedTlpType;

    // Fmt <-> Type cross-validation
    if (translatedFmt.type != Fmt::UNKNOWN && translatedTlpType != TlpType::UNKNOWN)
    {
        bool fmtHasData = translatedFmt.hasData;
        bool typeNeedsData = (translatedTlpType == TlpType::MWr || translatedTlpType == TlpType::CplD);
        bool typeNeedsNoData = (translatedTlpType == TlpType::MRd || translatedTlpType == TlpType::Cpl);

        if (typeNeedsData && !fmtHasData)
        {
            tlp.m_isMalformed = true;
            tlp.m_decodeErrors.push_back("Fmt/Type mismatch: Type requires a data-carrying Fmt but Fmt has no data.");
        }
        else if (typeNeedsNoData && fmtHasData)
        {
            tlp.m_isMalformed = true;
            tlp.m_decodeErrors.push_back("Fmt/Type mismatch: Type requires a no-data Fmt but Fmt carries data.");
        }
    }

    // Extract and Assign the TC value
    std::uint8_t tc = static_cast<std::uint8_t>(Utils::extractBits(dw0, 20, 22));
    if (tc > MAXIMUM_TC)
    {
        tlp.m_isMalformed = true;
        tlp.m_decodeErrors.push_back("TC value (" + std::to_string(tc) + ") exceeds maximum allowed value of 7.");
    }
    tlp.m_tc = tc;

    // Extract and Assign the Attributes fields
    std::uint8_t noSnoop = static_cast<std::uint8_t>(Utils::extractBits(dw0, 13, 13));
    std::uint8_t relaxedOrdering = static_cast<std::uint8_t>(Utils::extractBits(dw0, 12, 12));

    Attr attr{noSnoop != 0, relaxedOrdering != 0};
    tlp.m_attr = attr;

    // Extract and Assign the length of Double Word if its not null(equals to zero)
    std::uint16_t length = static_cast<std::uint16_t>(Utils::extractBits(dw0, 0, 9));

    if (tlp.m_type == TlpType::MRd || tlp.m_type == TlpType::MWr || tlp.m_type == TlpType::CplD)
    {
        if (length == 0)
        {
            tlp.m_isMalformed = true;
            tlp.m_decodeErrors.push_back("Malformed packet: data-carrying TLP type has a ZERO DW length field.");
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
            tlp.m_decodeErrors.push_back("Malformed packet: no-data TLP type has a NON-ZERO DW length field.");
        }
        tlp.m_lengthDw = 0;
    }

    // Declare the requesterId and Tag -> Extract their values based on the packet type
    std::uint16_t rawRequesterId{};
    std::string requesterId{};
    std::uint8_t tag{};
    std::uint64_t address{};

    bool isRequest = (tlp.m_type == TlpType::MRd || tlp.m_type == TlpType::MWr);
    bool isCompletion = (tlp.m_type == TlpType::Cpl || tlp.m_type == TlpType::CplD);

    if (isRequest)
    {
        // Translate Requester Id into BDF string format [Bus]:[Device]:[Function]
        rawRequesterId = static_cast<std::uint16_t>(Utils::extractBits(dw1, 16, 31));
        requesterId = PacketDecoder::translateBdfId(rawRequesterId);

        tag = static_cast<std::uint8_t>(Utils::extractBits(dw1, 8, 15));

        bool is4DW = (tlp.m_fmt == Fmt::DW4);

        if (!is4DW)
        {
            // Neglect the lowest two bits of the DW2 - they are always used for flags/ format bits
            address = static_cast<std::uint64_t>(dw2) & 0xFFFFFFFCu;
        }
        else
        {
            if (dws.size() < 4)
            {
                tlp.m_isMalformed = true;
                tlp.m_decodeErrors.push_back("Malformed 4DW packet: expected at least 4 DWs but found fewer.");
            }
            else
            {
                // Neglect the last two bits from the lowes DW and then concat the two DWs to form the address
                std::uint32_t dw3 = dws[3];
                address =
                    (static_cast<std::uint64_t>(dw2) << 32) |
                    (static_cast<std::uint64_t>(dw3) & 0xFFFFFFFCu);
            }
        }

        // Assign the address
        tlp.m_address = address;
    }
    else if (isCompletion)
    {
        // Translate Requester Id into BDF string format [Bus]:[Device]:[Function]
        rawRequesterId = static_cast<std::uint16_t>(Utils::extractBits(dw2, 16, 31));
        requesterId = PacketDecoder::translateBdfId(rawRequesterId);

        tag = static_cast<std::uint8_t>(Utils::extractBits(dw2, 8, 15));

        // Translate Completer Id into BDF string format [Bus]:[Device]:[Function]
        std::uint16_t rawCompleterId = static_cast<std::uint16_t>(Utils::extractBits(dw1, 16, 31));
        std::string completerId = PacketDecoder::translateBdfId(rawCompleterId);

        tlp.m_completerId = completerId;

        // Extract and Translate the raw completion status into readable format
        std::uint8_t status = static_cast<std::uint8_t>(Utils::extractBits(dw1, 13, 15));
        CompletionStatus translatedCompletionStatus = translatePacketCompletionStatus(status);
        if (translatedCompletionStatus == CompletionStatus::UNKNOWN)
        {
            tlp.m_isMalformed = true;
            tlp.m_decodeErrors.push_back("Illegal completion status value (" + std::to_string(status) + ").");
        }
        tlp.m_status = translatedCompletionStatus;

        // Extract the byte count
        std::uint16_t byteCount = static_cast<std::uint16_t>(Utils::extractBits(dw1, 0, 11));
        tlp.m_byteCount = byteCount;
    }

    tlp.m_requesterId = requesterId;
    tlp.m_tag = tag;

    return tlp;
}
