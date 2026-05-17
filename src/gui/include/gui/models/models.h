#ifndef MODELS_H
#define MODELS_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum class Direction {
  TX,     // DownStream: (Host -> Device)
  RX,     // Upstream: (Device -> Host)
  UNKNOWN // unvalid direction
};
enum class TlpType { MRd, MWr, CplD, Cpl, UNKNOWN };
enum class Fmt { DW3, DW4, UNKNOWN };
enum class CompletionStatus { SC, UR, CA, UNKNOWN };
struct Attr {
  bool no_snoop;
  bool relaxed_ordering;
};
enum class ValidationType {
  UNEXPECTED_COMPLETION,
  MISSING_COMPLETION,
  DUPLICATE_COMPLETION,
  BYTE_COUNT_MISMATCH,
  ADDRESS_MISALIGNMENT,
  TAG_COLLISION,
  INVALID_FIELD_VALUE,
};
struct DecodeError {
  std::string rule_id;
  std::string field;
  std::string description;
};

struct ValidationError {
  std::string rule_id;
  ValidationType category;
  std::uint64_t packet_index;
  std::string related_index;
  std::string description;
};

struct Packet {
  std::uint64_t index;
  std::uint64_t timestamp_ns;
  Direction direction;
  TlpType tlp_type;
  Fmt header_fmt;
  std::string address;
  std::string length;
  std::string tag;
  CompletionStatus status;
  bool is_malformed;
  bool has_validation_errors;
  bool has_any_error;
  std::vector<DecodeError> decode_errors;
  std::vector<ValidationError> validation_errors;
};

struct StatsModel {
  std::uint64_t total_packets;
  std::uint64_t malformed_packet_count;
  std::uint64_t validation_error_count;
  std::uint64_t skipped_line_count;
  std::unordered_map<TlpType, uint64_t> tlp_type_distribution;
};

struct ReportModel {
  std::string schema_version;
  std::string generated_at;
  std::string trace_file;
  StatsModel stats;
  std::vector<Packet> packets;
  std::vector<ValidationError> all_validation_errors;
};

#endif