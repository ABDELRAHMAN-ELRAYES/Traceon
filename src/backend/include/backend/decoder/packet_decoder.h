#ifndef PACKET_DECODER_H
#define PACKET_DECODER_H

#include "backend/core/tlp/tlp.h"

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <sstream>
#include <iostream>

class Packet;

struct TranslatedFmt
{
    bool hasData{};
    Fmt type{};
};

// the numer of hexadecimal digits each double word will be in
constexpr std::uint8_t DW_HEXA_DIGITS_NUMBER = 8;

// fmt potential values
constexpr std::uint8_t FMT_3DW_WITHOUT_DATA = 0;
constexpr std::uint8_t FMT_3DW_WITH_DATA = 2;
constexpr std::uint8_t FMT_4DW_WITHOUT_DATA = 1;
constexpr std::uint8_t FMT_4DW_WITH_DATA = 3;
// Map Potential Value to its format
const std::unordered_map<std::uint8_t, TranslatedFmt> fmtMap = {
    {FMT_3DW_WITHOUT_DATA, {false, Fmt::DW3}},
    {FMT_3DW_WITH_DATA, {true, Fmt::DW3}},
    {FMT_4DW_WITHOUT_DATA, {false, Fmt::DW4}},
    {FMT_4DW_WITH_DATA, {true, Fmt::DW4}}};

// Type potential values
constexpr std::uint8_t MEMORY_READ = 0;
constexpr std::uint8_t MEMORY_WRITE = 1;
constexpr std::uint8_t COMPLETION_WITH_DATA = 11;
constexpr std::uint8_t COMPLETION_WITHOUT_DATA = 10;

// Map Potential Value to its format
const std::unordered_map<std::uint8_t, TlpType> typeMap = {
    {MEMORY_READ, TlpType::MRd},
    {MEMORY_WRITE, TlpType::MWr},
    {COMPLETION_WITH_DATA, TlpType::CplD},
    {COMPLETION_WITHOUT_DATA, TlpType::Cpl}};

// The maximum possible value for TC
constexpr std::uint8_t MAXIMUM_TC = 7;

// Potential Values for the status of completion packets(CplD / Cpl)
constexpr std::uint8_t SUCCESS_COMPLETION = 0;
constexpr std::uint8_t UNSUPPORTED_COMPLETION = 1;
constexpr std::uint8_t ABORT_COMPLETION = 4;

// Map Potential Value to its format
const std::unordered_map<std::uint8_t, CompletionStatus> completionStatusMap = {
    {SUCCESS_COMPLETION, CompletionStatus::SC},
    {UNSUPPORTED_COMPLETION, CompletionStatus::UR},
    {ABORT_COMPLETION, CompletionStatus::CA}};


class TLP;

class PacketDecoder
{

private:
    /*
        - Parse Double Words(DWs) from the hexadecimal raw bytes packet string
    */
    static std::vector<std::uint32_t> parsePacketDws(const std::string &rawBytes);

    /*
        - Translate the raw fmt to readable format from 001 -> 4DW with data
    */
    static TranslatedFmt translateFmtHeader(std::uint8_t rawFmt);

    /*
        - Translate the raw Type to readable format from 00000 -> MRd
    */
    static TlpType translatePacketType(std::uint8_t rawType);

    /*
       - Translate the raw Status to readable format from 000 -> SC (success)
   */
    static CompletionStatus translatePacketCompletionStatus(std::uint8_t rawStatus);

    /*
        - Translate Raw Id (requester / Completer) into readable BDF format [Bus]:[Device]:[Function]
    */
    static std::string translateBdfId(std::uint16_t rawId);

public:
    /*
        - Decode the Packet Raw Hexadecimal into Protocol level packet like (TLP)
    */
    static TLP decode(const Packet &packet);
};

#endif