#ifndef TLP_H
#define TLP_H

#include <cstdint>
#include <string>
#include <optional>
#include <vector>

enum class TlpType
{
    MRd,
    MWr,
    CplD,
    Cpl,
    UNKNOWN
};
enum class Fmt
{
    DW3,
    DW4,
    UNKNOWN
};
enum class CompletionStatus
{
    SC,
    UR,
    CA,
    UNKNOWN
};
struct Attr
{
    bool no_snoop;
    bool relaxed_ordering;
};
/*
    - Express Transaction Protocol Level Packet (TLP)
*/
class TLP
{
private:
    TlpType m_type;
    Fmt m_fmt;
    Attr m_attr;

    std::string m_requester_id;
    std::optional<std::string> m_completer_id;

    std::uint8_t m_tag;
    std::uint8_t m_tc;
    std::optional<std::string> m_address{};
    std::optional<std::uint64_t> m_length_dw{};
    std::optional<std::uint64_t> m_byte_count{};
    std::optional<CompletionStatus> m_status{};

    bool m_is_malformed{};
    std::vector<std::string> m_decode_errors{};

public:
    TLP() = default;
    TLP::TLP(TlpType type, Fmt fmt, Attr attr,
             std::string requesterId, std::uint8_t tag, std::uint8_t tc);
};

#endif