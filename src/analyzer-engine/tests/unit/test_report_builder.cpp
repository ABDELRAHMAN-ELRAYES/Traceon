#include "analyzer-engine/decoder/packet_decoder.h"
#include "analyzer-engine/report-builder/report_builder.h"
#include "nlohmann/json.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using json = nlohmann::ordered_json;

class ReportBuilderTest : public ::testing::Test {
protected:
  std::string test_file_path = "test_report.json";

  void TearDown() override {
    if (std::filesystem::exists(test_file_path)) {
      std::filesystem::remove(test_file_path);
    }
  }
};

TEST_F(ReportBuilderTest, EmptyReportHasSchemaVersion) {
  ReportBuilder builder(ReportFormat::JSON, "dummy.csv", test_file_path);
  builder.write(test_file_path, 0);

  EXPECT_TRUE(std::filesystem::exists(test_file_path));

  std::ifstream file(test_file_path);
  json report;
  file >> report;

  EXPECT_EQ(report["schema_version"], "1.0");
  EXPECT_EQ(report["trace_file"], "dummy.csv");
  EXPECT_EQ(report["summary"]["total_packets"], 0);
  EXPECT_EQ(report["summary"]["skipped_line_count"], 0);
  EXPECT_EQ(report["summary"]["malformed_packet_count"], 0);

  EXPECT_TRUE(report["packets"].empty());
  EXPECT_TRUE(report["validation_errors"].empty());
}

TEST_F(ReportBuilderTest, NullSerializationPolicy) {
  ReportBuilder builder(ReportFormat::JSON, "dummy.csv", test_file_path);

  auto pkt = Packet(1000, Direction::TX, 0, "00000001010005FF30000000");
  auto tlp = PacketDecoder::decode(pkt);
  EXPECT_FALSE(tlp.isMalformed());
  builder.addTLP(tlp);

  builder.write(test_file_path, 0);

  std::ifstream file(test_file_path);
  json report;
  file >> report;

  EXPECT_EQ(report["summary"]["total_packets"], 1);
  EXPECT_EQ(report["packets"].size(), 1);

  auto tlp_json = report["packets"][0]["tlp"];
  EXPECT_TRUE(tlp_json["completer_id"].is_null());
  EXPECT_TRUE(tlp_json["byte_count"].is_null());
  EXPECT_TRUE(tlp_json["status"].is_null());
}

TEST_F(ReportBuilderTest, XmlFormatIgnoredForJsonTest) {
  ReportBuilder xmlBuilder(ReportFormat::XML, "dummy.csv", "test_report.xml");
  xmlBuilder.write("test_report.xml", 0);
  EXPECT_TRUE(std::filesystem::exists("test_report.xml"));
  std::filesystem::remove("test_report.xml");
}
