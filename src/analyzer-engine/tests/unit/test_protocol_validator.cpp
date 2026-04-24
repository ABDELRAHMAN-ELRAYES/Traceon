#include <gtest/gtest.h>
#include "analyzer-engine/validator/protocol_validator.h"
#include "analyzer-engine/decoder/packet_decoder.h"
#include "analyzer-engine/core/packet/packet.h"

TLP createTLP(const std::string& hex, uint64_t index=0) {
    auto pkt = Packet(0, Direction::TX, index, hex);
    return PacketDecoder::decode(pkt);
}

class ProtocolValidatorTest : public ::testing::Test {
protected:
    ProtocolValidator validator;
};

TEST_F(ProtocolValidatorTest, VAL001_UnexpectedCompletion) {
    // 12 bytes
    auto cpld = createTLP("4A0000010200000401000500");
    EXPECT_FALSE(cpld.isMalformed());
    validator.process(cpld);
    
    auto errs = validator.errors();
    bool found = false;
    for(const auto& err : errs) {
        if(err.rule_id == "VAL-001") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(ProtocolValidatorTest, VAL002_MissingCompletion) {
    // 12 bytes
    auto mrd = createTLP("00000001010005FF30000000");
    EXPECT_FALSE(mrd.isMalformed());
    validator.process(mrd);
    auto errs = validator.finalize();
    bool found = false;
    for(const auto& err : errs) {
        if(err.rule_id == "VAL-002") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(ProtocolValidatorTest, VAL003_DuplicateCompletion) {
    auto mrd = createTLP("00000001010005FF30000000"); 
    auto cpld = createTLP("4A0000010200000401000500");
    
    EXPECT_FALSE(mrd.isMalformed());
    EXPECT_FALSE(cpld.isMalformed());

    validator.process(mrd);
    validator.process(cpld); 
    validator.process(cpld); 
    
    auto errs = validator.errors();
    bool found = false;
    for(const auto& err : errs) {
        if(err.rule_id == "VAL-003") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(ProtocolValidatorTest, VAL004_ByteCountMismatch) {
    auto mrd = createTLP("00000002010005FF30000000");
    auto cpld = createTLP("4A0000020200000401000500");
    
    EXPECT_FALSE(mrd.isMalformed());
    EXPECT_FALSE(cpld.isMalformed());

    validator.process(mrd);
    validator.process(cpld);
    
    auto errs = validator.errors();
    bool found = false;
    for(const auto& err : errs) {
        if(err.rule_id == "VAL-004") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(ProtocolValidatorTest, VAL005_AddressMisalignment) {
    // Length=4 (16 bytes). Needs address % 16 == 0.
    auto mwr = createTLP("40000004010005FF30000008");
    EXPECT_FALSE(mwr.isMalformed());
    validator.process(mwr);
    
    auto errs = validator.errors();
    bool found = false;
    for(const auto& err : errs) {
        if(err.rule_id == "VAL-005") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(ProtocolValidatorTest, VAL006_TagCollision) {
    auto mrd1 = createTLP("00000001010005FF30000000", 1);
    auto mrd2 = createTLP("00000001010005FF30000010", 2); 
    
    EXPECT_FALSE(mrd1.isMalformed());
    EXPECT_FALSE(mrd2.isMalformed());

    validator.process(mrd1);
    validator.process(mrd2);
    
    auto errs = validator.errors();
    bool found = false;
    for(const auto& err : errs) {
        if(err.rule_id == "VAL-006") found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(ProtocolValidatorTest, VAL008_InvalidLength) {
    // Length=0 -> DW0=00 00 00 00
    auto mrd = createTLP("00000000010005FF30000000", 1);
    EXPECT_FALSE(mrd.isMalformed());
    validator.process(mrd);
    
    auto errs = validator.errors();
    bool found = false;
    for(const auto& err : errs) {
        if(err.rule_id == "VAL-008") found = true;
    }
    EXPECT_TRUE(found);
}
