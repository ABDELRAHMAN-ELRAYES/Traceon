#include "analyzer-engine/analyzer_engine.h"
#include "nlohmann/json.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using json = nlohmann::ordered_json;

class AnalyzerEngineSystemTest : public ::testing::Test {
protected:
  std::string test_trace_path = "system_test_trace.csv";
  std::string test_report_path = "system_test_report.json";

  void TearDown() override {
    if (std::filesystem::exists(test_trace_path)) {
      std::filesystem::remove(test_trace_path);
    }
    if (std::filesystem::exists(test_report_path)) {
      std::filesystem::remove(test_report_path);
    }
  }

  void writeTrace(const std::vector<std::string> &lines) {
    std::ofstream out(test_trace_path);
    for (const auto &line : lines) {
      out << line << "\n";
    }
  }

  json readReport() {
    std::ifstream file(test_report_path);
    json report;
    file >> report;
    return report;
  }

  void runAnalyzer(bool verbose = false) {
    AnalyzerEngine engine(test_trace_path, test_report_path, ReportFormat::JSON,
                          verbose);
    engine.run();
  }
};

TEST_F(AnalyzerEngineSystemTest, CanonicalValidTrace) {
  writeTrace(
      {"1000,TX,0000000101000A0030000000", "2000,RX,4A0000010200000401000A00"});
  runAnalyzer();

  auto report = readReport();
  EXPECT_EQ(report["summary"]["total_packets"], 2);
  EXPECT_EQ(report["summary"]["malformed_packet_count"], 0);
  EXPECT_EQ(report["summary"]["validation_error_count"], 0);
  EXPECT_EQ(report["summary"]["skipped_line_count"], 0);
  EXPECT_EQ(report["malformed_packets"].size(), 0);
  EXPECT_EQ(report["validation_errors"].size(), 0);
  EXPECT_EQ(report["packets"].size(), 2);
}

TEST_F(AnalyzerEngineSystemTest, MalformedPackets) {
  writeTrace({
      "1000,TX,C00000010000000F00000000",         // DEC-001
      "1001,TX,070000010000000F00000000",         // DEC-002
      "1002,TX,4000",                             // DEC-003
      "1003,TX,000000000000000000000003",         // DEC-004
      "1004,TX,20000000000000000000000000000001", // DEC-005
      "1005,TX,4A0000000000600000000000",         // DEC-006
      "1006,TX,XX0000000000000000000000"          // DEC-007
  });
  runAnalyzer();

  auto report = readReport();
  EXPECT_EQ(report["summary"]["total_packets"], 7);
  EXPECT_EQ(report["summary"]["malformed_packet_count"], 7);
  EXPECT_EQ(report["summary"]["validation_error_count"], 0);
  EXPECT_EQ(report["malformed_packets"].size(), 7);

  // Ensure all generated DEC rules are present
  int dec001 = 0, dec002 = 0, dec003 = 0, dec004 = 0, dec005 = 0, dec006 = 0,
      dec007 = 0;
  for (const auto &pkt : report["malformed_packets"]) {
    for (const auto &err : pkt["decode_errors"]) {
      std::string id = err["rule_id"];
      if (id == "DEC-001")
        dec001++;
      if (id == "DEC-002")
        dec002++;
      if (id == "DEC-003")
        dec003++;
      if (id == "DEC-004")
        dec004++;
      if (id == "DEC-005")
        dec005++;
      if (id == "DEC-006")
        dec006++;
      if (id == "DEC-007")
        dec007++;
    }
  }
  EXPECT_EQ(dec001, 1);
  EXPECT_EQ(dec002, 1);
  EXPECT_EQ(dec003, 1);
  EXPECT_EQ(dec004, 1);
  EXPECT_EQ(dec005, 1);
  EXPECT_EQ(dec006, 1);
  EXPECT_EQ(dec007, 1);
}

TEST_F(AnalyzerEngineSystemTest, ValidationErrors) {
  writeTrace({
      "1000,RX,4A0000010200000401000500", // VAL-001 Unexpected Completion
      "1001,TX,00000001010005FF30000000", // VAL-002 Missing Completion (tag 5
                                          // will not complete)

      "1002,TX,00000001010006FF30000000", // VAL-003 Duplicate Completion (tag6)
      "1003,RX,4A0000010200000401000600", "1004,RX,4A0000010200000401000600",
      "1005,TX,00000002010007FF30000000", // VAL-004 Byte count mismatch (tag 7)
      "1006,RX,4A0000020200000401000700",

      "1007,TX,40000004010005FF30000008", // VAL-005 Address Misalignment

      "1008,TX,0000000101000CFF30000000", // VAL-006 Tag Collision (tag 12
                                          // reused without completion)
      "1009,TX,0000000101000CFF30000010",

      "1010,TX,0000000001000DFF30000000" // VAL-008 Invalid Length
  });
  runAnalyzer();

  auto report = readReport();
  EXPECT_EQ(report["summary"]["malformed_packet_count"], 0);
  EXPECT_EQ(report["summary"]["validation_error_count"], 9);

  // Ensure tested VAL rules are triggered
  int val001 = 0, val002 = 0, val003 = 0, val004 = 0, val005 = 0, val006 = 0,
      val008 = 0;
  for (const auto &err : report["validation_errors"]) {
    std::string id = err["rule_id"];
    if (id == "VAL-001")
      val001++;
    if (id == "VAL-002")
      val002++;
    if (id == "VAL-003")
      val003++;
    if (id == "VAL-004")
      val004++;
    if (id == "VAL-005")
      val005++;
    if (id == "VAL-006")
      val006++;
    if (id == "VAL-008")
      val008++;
  }
  EXPECT_EQ(val001, 1);
  // 3 missing completions expected
  EXPECT_EQ(val002, 3);
  EXPECT_EQ(val003, 1);
  EXPECT_EQ(val004, 1);
  EXPECT_EQ(val005, 1);
  EXPECT_EQ(val006, 1);
  EXPECT_EQ(val008, 1);
}

TEST_F(AnalyzerEngineSystemTest, EmptyTraceFile) {
  writeTrace({});
  runAnalyzer();
  auto report = readReport();
  EXPECT_EQ(report["summary"]["total_packets"], 0);
  EXPECT_EQ(report["summary"]["malformed_packet_count"], 0);
  EXPECT_EQ(report["summary"]["validation_error_count"], 0);
  EXPECT_EQ(report["summary"]["skipped_line_count"], 0);
}

TEST_F(AnalyzerEngineSystemTest, CommentAndBlankLineOnly) {
  writeTrace({"# this is a comment", "", "   ", "# another comment"});
  runAnalyzer();
  auto report = readReport();
  EXPECT_EQ(report["summary"]["total_packets"], 0);
  EXPECT_EQ(report["summary"]["skipped_line_count"], 0);
}

TEST_F(AnalyzerEngineSystemTest, SkippedLinesFromMalformedRows) {
  writeTrace({"1000,TX,0000000101000A0030000000",
              "malformed_nonsense_line", // does not parse due to missing fields
              "2000,RX,4A0000010200000401000A00"});
  runAnalyzer();
  auto report = readReport();
  EXPECT_EQ(report["summary"]["total_packets"], 2);
  EXPECT_EQ(report["summary"]["skipped_line_count"], 1);
}

TEST_F(AnalyzerEngineSystemTest, LargeTrace) {
  std::vector<std::string> lines;
  lines.reserve(100000);
  for (int i = 0; i < 100000; ++i) {
    lines.push_back(
        "1000,TX,0000000101000A0030000000"); // valid structure, 100k packets
  }
  writeTrace(lines);

  auto start = std::chrono::high_resolution_clock::now();
  runAnalyzer();
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  EXPECT_LT(duration, 10000); // Should be well within 10s

  auto report = readReport();
  EXPECT_EQ(report["summary"]["total_packets"], 100000);
}
