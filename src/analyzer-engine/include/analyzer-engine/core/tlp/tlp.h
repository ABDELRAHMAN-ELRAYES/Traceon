#ifndef TLP_H
#define TLP_H

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
  std::vector<std::string> m_decodeErrors{};

public:
  TLP() = default;
  TLP(TlpType type, Fmt fmt, Attr attr, std::string requesterId,
      std::uint8_t tag, std::uint8_t tc);

  void printPacketDetails();
  friend class PacketDecoder;
};

#endif