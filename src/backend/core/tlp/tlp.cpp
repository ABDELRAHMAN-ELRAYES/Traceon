#include "tlp.h"

/*
    - TLP constructor: Initialize the TLP with mandatory fields + default values for other nullable fields
*/
TLP::TLP(TlpType type, Fmt fmt, Attr attr,
         std::string requesterId, std::uint8_t tag, std::uint8_t tc)
    : m_type(type),
      m_fmt(fmt),
      m_attr(attr),
      m_requester_id(std::move(requesterId)),
      m_completer_id(std::nullopt),
      m_tag(tag),
      m_tc(tc),
      m_address(std::nullopt),
      m_length_dw(std::nullopt),
      m_byte_count(std::nullopt),
      m_status(std::nullopt),
      m_is_malformed(false),
      m_decode_errors()
{
}