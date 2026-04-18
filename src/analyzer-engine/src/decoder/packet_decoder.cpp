#include "analyzer-engine/decoder/packet_decoder.h"
#include "analyzer-engine/core/packet/packet.h"
#include "analyzer-engine/utils/utils.h"
#include <iomanip>
#include <sstream>

// parsePacketDws: Divide the packet raw hexa into double words
std::vector<std::uint32_t>
PacketDecoder::parsePacketDws(const std::string &hexaRawBytes) {
  std::vector<std::uint32_t> dws{};

  std::size_t rawBytesSize = hexaRawBytes.size();
  std::size_t i = 0;

  while (i < rawBytesSize) {
    // Divide the raw bytes by the double word size(4Bytes => 8 Hexa Digits)
    std::string dw = hexaRawBytes.substr(i, DW_HEXA_DIGITS_NUMBER);

    // convert the extracted hexa dw
    dws.push_back(static_cast<std::uint32_t>(std::stoul(dw, nullptr, 16)));

    i += DW_HEXA_DIGITS_NUMBER;
  }
  return dws;
}

TranslatedFmt PacketDecoder::translateFmtHeader(std::uint8_t rawFmt) {
  TranslatedFmt translatedFmt{false, Fmt::UNKNOWN};
  auto iterator = fmtMap.find(rawFmt);
  if (iterator != fmtMap.end()) {
    return iterator->second;
  }
  return translatedFmt;
}

TlpType PacketDecoder::translatePacketType(std::uint8_t rawType) {
  auto iterator = typeMap.find(rawType);
  if (iterator != typeMap.end()) {
    return iterator->second;
  }
  return TlpType::UNKNOWN;
}

TlpType PacketDecoder::resolveType(std::uint8_t rawType, bool hasData) {
  if (rawType == MEMORY_TYPE)
    return hasData ? TlpType::MWr : TlpType::MRd;
  if (rawType == COMPLETION_TYPE)
    return hasData ? TlpType::CplD : TlpType::Cpl;
  return TlpType::UNKNOWN;
}

CompletionStatus
PacketDecoder::translatePacketCompletionStatus(std::uint8_t rawStatus) {
  auto iterator = completionStatusMap.find(rawStatus);
  if (iterator != completionStatusMap.end()) {
    return iterator->second;
  }
  return CompletionStatus::UNKNOWN;
}

std::string PacketDecoder::translateBdfId(std::uint16_t rawId) {
  // Extract the bus, device, function ->> (8, 5, 3) bits
  std::uint8_t bus = (rawId >> 8) & 0xFF;
  std::uint8_t device = (rawId >> 3) & 0x1F;
  std::uint8_t function = rawId & 0x07;

  // Store the Translated ID
  std::ostringstream id{};

  // Set the number base for the input numbers to Hexadecimal
  id << std::hex << std::setfill('0');

  // Domain (always 0000)
  id << "0000:";

  id << std::setw(2) << static_cast<unsigned int>(bus);
  id << ':';

  id << std::setw(2) << static_cast<unsigned int>(device);
  id << '.';

  id << std::setw(1) << static_cast<unsigned int>(function);

  return id.str();
}

TLP PacketDecoder::decode(const Packet &packet) {
  // Get the main fields of the packet
  std::uint64_t timestamp = packet.timestamp_;
  Direction direction = packet.direction_;
  std::string hexaRawBytes = packet.raw_bytes_;

  // The Number of Digits of the raw bytes
  std::size_t rawBytesSize = hexaRawBytes.size();

  // Create a TLP instance
  TLP tlp{};
  tlp.index_ = packet.index();

  // [DEC-007] Check if the payload contains non-hexadecimal characters
  for (char c : hexaRawBytes) {
    if (!std::isxdigit(static_cast<unsigned char>(c))) {
      tlp.is_malformed_ = true;
      tlp.decode_errors_.push_back(
          {"DEC-007", "payload_hex",
           "The payload contains non-hexadecimal characters."});
      return tlp;
    }
  }

  // Check if the digits number less than 3DW(24 digits)
  if (rawBytesSize < 3 * DW_HEXA_DIGITS_NUMBER) {
    tlp.is_malformed_ = true;
    tlp.decode_errors_.push_back(
        {"DEC-003", "payload_hex",
         "Raw hex string is shorter than the minimum TLP size (3 DW = 24 hex "
         "digits)."});
    return tlp;
  }

  // Check if the digits number is consists of fixed number of DW size
  if (rawBytesSize % DW_HEXA_DIGITS_NUMBER != 0) {
    tlp.is_malformed_ = true;
    tlp.decode_errors_.push_back(
        {"DEC-003", "payload_hex",
         "Raw hex string length is not a multiple of one DW (8 hex digits). "
         "Packet is truncated."});
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

  if (translatedFmt.type == Fmt::UNKNOWN) {
    tlp.is_malformed_ = true;
    tlp.decode_errors_.push_back({"DEC-001", "Fmt [31:29]",
                                  "Illegal Fmt value (" + std::to_string(fmt) +
                                      "). Only 000–011 are valid."});
    return tlp;
  }
  tlp.fmt_ = translatedFmt.type;

  // Translated type extracted bits into reaadable format
  std::uint8_t type =
      static_cast<std::uint8_t>(Utils::extractBits(dw0, 24, 28));

  if (type == MEMORY_TYPE || type == COMPLETION_TYPE) {
    bool rawTypeExpectsData = (type == COMPLETION_TYPE) ? false : false;

    (void)rawTypeExpectsData;
  }

  TlpType translatedTlpType =
      PacketDecoder::resolveType(type, translatedFmt.hasData);

  if (translatedTlpType == TlpType::UNKNOWN) {
    tlp.is_malformed_ = true;
    tlp.decode_errors_.push_back(
        {"DEC-002", "Type [28:24]",
         "Unsupported TLP Type value (" + std::to_string(type) + ")."});
    return tlp;
  }
  tlp.type_ = translatedTlpType;

  // Fmt <-> Type cross-validation
  if (translatedFmt.type != Fmt::UNKNOWN &&
      translatedTlpType != TlpType::UNKNOWN) {
    bool fmtHasData = translatedFmt.hasData;

    TlpType baseTypeNoData = PacketDecoder::resolveType(type, false);
    TlpType baseTypeWithData = PacketDecoder::resolveType(type, true);

    bool typeNeedsData = (baseTypeWithData != TlpType::UNKNOWN &&
                          baseTypeNoData != TlpType::UNKNOWN)
                             ? false
                             : (baseTypeWithData != TlpType::UNKNOWN);
    bool typeNeedsNoData = (baseTypeWithData != TlpType::UNKNOWN &&
                            baseTypeNoData != TlpType::UNKNOWN)
                               ? false
                               : (baseTypeNoData != TlpType::UNKNOWN);

    bool resolvedIsMemory = (translatedTlpType == TlpType::MRd ||
                             translatedTlpType == TlpType::MWr);
    bool resolvedIsCompletion = (translatedTlpType == TlpType::Cpl ||
                                 translatedTlpType == TlpType::CplD);
    bool rawIsMemory = (type == MEMORY_TYPE);
    bool rawIsCompletion = (type == COMPLETION_TYPE);

    if ((resolvedIsMemory && !rawIsMemory) ||
        (resolvedIsCompletion && !rawIsCompletion)) {
      // The Fmt's hasData flipped the type family — that is a genuine mismatch.
      tlp.is_malformed_ = true;
      tlp.decode_errors_.push_back(
          {"DEC-002", "Type [28:24]",
           "Fmt/Type mismatch: resolved TLP type family does not match the "
           "raw Type field."});
    } else if (typeNeedsData && !fmtHasData) {
      tlp.is_malformed_ = true;
      tlp.decode_errors_.push_back({"DEC-002", "Type [28:24]",
                                    "Fmt/Type mismatch: Type requires a "
                                    "data-carrying Fmt but Fmt has no "
                                    "data."});
    } else if (typeNeedsNoData && fmtHasData) {
      tlp.is_malformed_ = true;
      tlp.decode_errors_.push_back(
          {"DEC-002", "Type [28:24]",
           "Fmt/Type mismatch: Type requires a no-data Fmt but Fmt carries "
           "data."});
    }
  }

  // Extract and Assign the TC value
  std::uint8_t tc = static_cast<std::uint8_t>(Utils::extractBits(dw0, 20, 22));
  tlp.tc_ = tc;

  // Extract and Assign the Attributes fields
  std::uint8_t noSnoop =
      static_cast<std::uint8_t>(Utils::extractBits(dw0, 13, 13));
  std::uint8_t relaxedOrdering =
      static_cast<std::uint8_t>(Utils::extractBits(dw0, 12, 12));

  Attr attr{noSnoop != 0, relaxedOrdering != 0};
  tlp.attr_ = attr;

  // Extract and Assign the length of Double Word if its not null(equals to
  // zero)
  std::uint16_t length =
      static_cast<std::uint16_t>(Utils::extractBits(dw0, 0, 9));

  if (tlp.type_ == TlpType::MRd || tlp.type_ == TlpType::MWr ||
      tlp.type_ == TlpType::CplD) {
    tlp.length_dw_ = length;
  } else {
    // No-data types without length (Cpl): length field should be zero
    tlp.length_dw_ = length;
  }

  // Declare the requesterId and Tag -> Extract their values based on the packet
  // type
  std::uint16_t rawRequesterId{};
  std::string requesterId{};
  std::uint8_t tag{};
  std::uint64_t address{};

  bool isRequest = (tlp.type_ == TlpType::MRd || tlp.type_ == TlpType::MWr);
  bool isCompletion =
      (tlp.type_ == TlpType::Cpl || tlp.type_ == TlpType::CplD);

  if (isRequest) {
    // Translate Requester Id into BDF string format [Bus]:[Device]:[Function]
    rawRequesterId =
        static_cast<std::uint16_t>(Utils::extractBits(dw1, 16, 31));
    requesterId = PacketDecoder::translateBdfId(rawRequesterId);

    tag = static_cast<std::uint8_t>(Utils::extractBits(dw1, 8, 15));

    bool is4DW = (tlp.fmt_ == Fmt::DW4);

    if (!is4DW) {
      // Neglect the lowest two bits of the DW2 - they are always used for
      // flags/ format bits
      address = static_cast<std::uint64_t>(dw2) & 0xFFFFFFFCu;

      // [DEC-004] Check 3DW address alignment
      if ((dw2 & 0x3) != 0) {
        tlp.is_malformed_ = true;
        tlp.decode_errors_.push_back({"DEC-004", "Address [31:2] (3DW)",
                                      "32-bit address is not DW aligned."});
      }
    } else {
      if (dws.size() < 4) {
        tlp.is_malformed_ = true;
        tlp.decode_errors_.push_back(
            {"DEC-003", "Payload bytes",
             "Malformed 4DW packet: expected at least 4 DWs but found fewer."});
      } else {
        // Neglect the last two bits from the lowes DW and then concat the two
        // DWs to form the address
        std::uint32_t dw3 = dws[3];
        address = (static_cast<std::uint64_t>(dw2) << 32) |
                  (static_cast<std::uint64_t>(dw3) & 0xFFFFFFFCu);

        // [DEC-005] Check 4DW address alignment
        if ((dw3 & 0x3) != 0) {
          tlp.is_malformed_ = true;
          tlp.decode_errors_.push_back({"DEC-005", "Address [63:2] (4DW)",
                                        "64-bit address is not DW aligned."});
        }
      }
    }

    // Assign the address
    if (!tlp.is_malformed_) {
      tlp.address_ = address;
    }
  } else if (isCompletion) {
    // Translate Requester Id into BDF string format [Bus]:[Device]:[Function]
    rawRequesterId =
        static_cast<std::uint16_t>(Utils::extractBits(dw2, 16, 31));
    requesterId = PacketDecoder::translateBdfId(rawRequesterId);

    tag = static_cast<std::uint8_t>(Utils::extractBits(dw2, 8, 15));

    std::uint16_t rawCompleterId =
        static_cast<std::uint16_t>(Utils::extractBits(dw1, 16, 31));
    std::string completerId = PacketDecoder::translateBdfId(rawCompleterId);

    tlp.completer_id_ = completerId;

    // Extract and Translate the raw completion status into readable format
    std::uint8_t status = static_cast<std::uint8_t>((dw1 >> 13) & 0x7u);
    CompletionStatus translatedCompletionStatus =
        translatePacketCompletionStatus(status);
    if (translatedCompletionStatus == CompletionStatus::UNKNOWN) {
      tlp.is_malformed_ = true;
      tlp.decode_errors_.push_back({"DEC-006", "Status [15:13]",
                                    "Illegal completion status value (" +
                                        std::to_string(status) + ")."});
    }
    tlp.status_ = translatedCompletionStatus;

    // Extract the byte count
    std::uint16_t byteCount =
        static_cast<std::uint16_t>(Utils::extractBits(dw1, 0, 11));
    tlp.byte_count_ = byteCount;
  }

  tlp.requester_id_ = requesterId;
  tlp.tag_ = tag;

  return tlp;
}