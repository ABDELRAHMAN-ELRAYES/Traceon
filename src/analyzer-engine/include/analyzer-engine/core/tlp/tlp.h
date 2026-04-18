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
  bool noSnoop;
  bool relaxedOrdering;
};

class PacketDecoder;
/*
    - Express Transaction Protocol Level Packet (TLP)
*/
class TLP {
private:
  TlpType m_type;
  Fmt m_fmt;
  Attr m_attr;

  std::string m_requesterId;
  std::optional<std::string> m_completerId;

  std::uint8_t m_tag;
  std::uint8_t m_tc;
  std::optional<std::uint64_t> m_address{};
  std::optional<std::uint64_t> m_lengthDw{};
  std::optional<std::uint64_t> m_byteCount{};
  std::optional<CompletionStatus> m_status{};

  bool m_isMalformed{};
  std::uint64_t m_index{};
  std::vector<DecodeError> m_decodeErrors{};

public:
  TLP() = default;
  TLP(TlpType type, Fmt fmt, Attr attr, std::string requesterId,
      std::uint8_t tag, std::uint8_t tc, std::uint64_t index);

  TlpType type() const { return m_type; }
  Fmt fmt() const { return m_fmt; }
  const std::string &requesterId() const { return m_requesterId; }
  const std::optional<std::string> &completerId() const {
    return m_completerId;
  }
  std::uint8_t tag() const { return m_tag; }
  std::uint8_t tc() const { return m_tc; }
  std::optional<std::uint64_t> address() const { return m_address; }
  std::optional<std::uint64_t> lengthDw() const { return m_lengthDw; }
  std::optional<std::uint64_t> byteCount() const { return m_byteCount; }
  std::optional<CompletionStatus> status() const { return m_status; }
  bool isMalformed() const { return m_isMalformed; }
  std::uint64_t index() const { return m_index; }
  const std::vector<DecodeError> &decodeErrors() const {
    return m_decodeErrors;
  }

  void printPacketDetails();
  friend class PacketDecoder;
};

#endif