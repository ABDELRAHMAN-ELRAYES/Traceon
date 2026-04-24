#include <gtest/gtest.h>
#include "analyzer-engine/input-layer/input_layer.h"
#include <fstream>
#include <filesystem>

class InputLayerTest : public ::testing::Test {
protected:
    std::string test_dir = "test_data_dir";

    void SetUp() override {
        std::filesystem::create_directory(test_dir);
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir);
    }

    std::string createTestFile(const std::string& name, const std::string& content) {
        std::string path = test_dir + "/" + name;
        std::ofstream out(path);
        out << content;
        out.close();
        return path;
    }
};

TEST_F(InputLayerTest, EmptyFile) {
    std::string path = createTestFile("empty.csv", "");
    TraceInputLayer input(path);
    auto packet = input.next();
    EXPECT_FALSE(packet.has_value());
    EXPECT_TRUE(input.isExhausted());
    EXPECT_EQ(input.skippedLineCount(), 1);
}

TEST_F(InputLayerTest, CommentOnlyFile) {
    std::string content = "# This is a comment\n# Another comment\n";
    std::string path = createTestFile("comments.csv", content);
    TraceInputLayer input(path);
    
    while (!input.isExhausted()) {
        auto p = input.next();
        if(p == std::nullopt && input.isExhausted()) break;
    }
    
    EXPECT_TRUE(input.isExhausted());
    EXPECT_GT(input.skippedLineCount(), 0);
}

TEST_F(InputLayerTest, MixedValidAndMalformedLines) {
    std::string content = 
        "# Sample Trace\n"
        "1000,TX,400000010000000F\n"      
        "badline\n"                              
        "2000,RX,4A000000\n"                        
        ",,,\n";                                 
        
    std::string path = createTestFile("mixed.csv", content);
    TraceInputLayer input(path);
    
    auto empty1 = input.next();
    
    auto packet1 = input.next();
    EXPECT_TRUE(packet1.has_value());
    
    auto empty2 = input.next();
    
    auto packet2 = input.next();
    EXPECT_TRUE(packet2.has_value());
    
    auto empty3 = input.next();
    input.next();
}

TEST_F(InputLayerTest, NoTrailingNewline) {
    std::string content = "1000,TX,40000001";
    std::string path = createTestFile("no_newline.csv", content);
    TraceInputLayer input(path);
    
    auto packet = input.next();
    EXPECT_TRUE(packet.has_value());
}
