#ifndef TLP_H
#define TLP_H

#include "analyzer-engine/decoder/decode_error.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class TlpType { MRd, MWr, CplD, Cpl, UNKNOWN };
enum class Fmt { DW3, DW4, UNKNOWN };
enum class CompletionStatus { SC, UR, CA, UNKNOWN };
struct Attr {
  bool no_snoop;
  bool relaxed_ordering;
};

class PacketDecoder;
/*
    - Express Transaction Protocol Level Packet (TLP)
*/
class TLP {
private:
  TlpType type_;
  Fmt fmt_;
  Attr attr_;

  std::string requester_id_;
  std::optional<std::string> completer_id_;

  std::uint8_t tag_;
  std::uint8_t tc_;
  std::optional<std::uint64_t> address_{};
  std::optional<std::uint64_t> length_dw_{};
  std::optional<std::uint64_t> byte_count_{};
  std::optional<CompletionStatus> status_{};

  bool is_malformed_{};
  std::uint64_t index_{};
  std::vector<DecodeError> decode_errors_{};

public:
  TLP() = default;
  TLP(TlpType type, Fmt fmt, Attr attr, std::string requesterId,
      std::uint8_t tag, std::uint8_t tc, std::uint64_t index);

  TlpType type() const { return type_; }
  Fmt fmt() const { return fmt_; }
  const std::string &requesterId() const { return requester_id_; }
  const std::optional<std::string> &completerId() const {
    return completer_id_;
  }
  std::uint8_t tag() const { return tag_; }
  std::uint8_t tc() const { return tc_; }
  std::optional<std::uint64_t> address() const { return address_; }
  std::optional<std::uint64_t> lengthDw() const { return length_dw_; }
  std::optional<std::uint64_t> byteCount() const { return byte_count_; }
  std::optional<CompletionStatus> status() const { return status_; }
  bool isMalformed() const { return is_malformed_; }
  std::uint64_t index() const { return index_; }
  const std::vector<DecodeError> &decodeErrors() const {
    return decode_errors_;
  }

  void printPacketDetails();
  friend class PacketDecoder;
};

#endif