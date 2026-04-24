#include <gtest/gtest.h>
#include "analyzer-engine/decoder/packet_decoder.h"
#include "analyzer-engine/core/packet/packet.h"

// Helper function to create packet
Packet makePacket(const std::string& hex) {
    return Packet(0, Direction::TX, 0, hex);
}

// DEC-001: The Fmt field value is one of the reserved combinations (0b110 or 0b111).
TEST(PacketDecoderTest, DEC001_InvalidFmt) {
    auto pkt = makePacket("C00000010000000F00000000");
    auto tlp = PacketDecoder::decode(pkt);
    EXPECT_TRUE(tlp.isMalformed());
    bool found = false;
    for (const auto& err : tlp.decodeErrors()) {
        if (err.rule_id == "DEC-001") found = true;
    }
    EXPECT_TRUE(found);
}

// DEC-002: Unsupported transactions or Fmt/Type mismatches.
TEST(PacketDecoderTest, DEC002_TypeMismatch) {
    auto unsupportedTypePkt = makePacket("070000010000000F00000000");
    auto tlp = PacketDecoder::decode(unsupportedTypePkt);
    EXPECT_TRUE(tlp.isMalformed());
    bool found = false;
    for (const auto& err : tlp.decodeErrors()) {
        if (err.rule_id == "DEC-002") found = true;
    }
    EXPECT_TRUE(found);
}

// DEC-003: Payload hex byte count insufficient.
TEST(PacketDecoderTest, DEC003_InsufficientLength) {
    auto pkt = makePacket("4000");
    auto tlp = PacketDecoder::decode(pkt);
    EXPECT_TRUE(tlp.isMalformed());
    bool found = false;
    for (const auto& err : tlp.decodeErrors()) {
        if (err.rule_id == "DEC-003") found = true;
    }
    EXPECT_TRUE(found);
}

// DEC-004: Address [31:2] not aligned to DW (3DW TLP).
TEST(PacketDecoderTest, DEC004_3DW_AddressUnaligned) {
    auto pkt = makePacket("000000000000000000000003");
    auto tlp = PacketDecoder::decode(pkt);
    EXPECT_TRUE(tlp.isMalformed());
    bool found = false;
    for (const auto& err : tlp.decodeErrors()) {
        if (err.rule_id == "DEC-004") found = true;
    }
    EXPECT_TRUE(found);
}

// DEC-005: Address [63:2] not aligned to DW (4DW TLP).
TEST(PacketDecoderTest, DEC005_4DW_AddressUnaligned) {
    auto pkt = makePacket("20000000000000000000000000000001");
    auto tlp = PacketDecoder::decode(pkt);
    EXPECT_TRUE(tlp.isMalformed());
    bool found = false;
    for (const auto& err : tlp.decodeErrors()) {
        if (err.rule_id == "DEC-005") found = true;
    }
    EXPECT_TRUE(found);
}

// DEC-006: Status field contains an undefined completion code.
TEST(PacketDecoderTest, DEC006_InvalidStatus) {
    auto pkt = makePacket("4A0000000000600000000000");
    auto tlp = PacketDecoder::decode(pkt);
    EXPECT_TRUE(tlp.isMalformed());
    bool found = false;
    for (const auto& err : tlp.decodeErrors()) {
        if (err.rule_id == "DEC-006") found = true;
    }
    EXPECT_TRUE(found);
}

// DEC-007: payload_hex contains invalid hex digits.
TEST(PacketDecoderTest, DEC007_InvalidHex) {
    auto pkt = makePacket("XX0000000000000000000000");
    auto tlp = PacketDecoder::decode(pkt);
    EXPECT_TRUE(tlp.isMalformed());
    bool found = false;
    for (const auto& err : tlp.decodeErrors()) {
        if (err.rule_id == "DEC-007") found = true;
    }
    EXPECT_TRUE(found);
}

// Supported Types parsing test
TEST(PacketDecoderTest, SupportedTypesParsing) {
    auto mrd = makePacket("0000000101000A0030000000"); 
    auto tlp1 = PacketDecoder::decode(mrd);
    EXPECT_FALSE(tlp1.isMalformed());
    EXPECT_EQ(tlp1.type(), TlpType::MRd);
    
    auto cpld = makePacket("4A0000010200000400000000");
    auto tlp2 = PacketDecoder::decode(cpld);
    EXPECT_FALSE(tlp2.isMalformed());
    EXPECT_EQ(tlp2.type(), TlpType::CplD);
}
