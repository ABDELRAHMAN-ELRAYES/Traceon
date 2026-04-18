#include "analyzer-engine/core/tlp/tlp.h"
#include "analyzer-engine/decoder/packet_decoder.h"
#include "analyzer-engine/utils/utils.h"
#include <iostream>

/*
    - TLP constructor: Initialize the TLP with mandatory fields + default values
   for other nullable fields
*/
TLP::TLP(TlpType type, Fmt fmt, Attr attr, std::string requesterId,
         std::uint8_t tag, std::uint8_t tc, std::uint64_t index)
    : m_type(type), m_fmt(fmt), m_attr(attr),
      m_requesterId(std::move(requesterId)), m_completerId(std::nullopt),
      m_tag(tag), m_tc(tc), m_address(std::nullopt), m_lengthDw(std::nullopt),
      m_byteCount(std::nullopt), m_status(std::nullopt), m_isMalformed(false),
      m_index(index), m_decodeErrors() {}

void TLP::printPacketDetails() {
  using std::cout;
  using std::endl;

  std::string tlpTypeToStr = Utils::tlpTypeToString(m_type);

  std::string fmtToStr = Utils::fmtToStr(m_fmt);
  std::string cplStatusToStr = "Unknown";
  if (m_status.has_value()) {
    std::string cplStatusToStr = Utils::completionStatusToStr(m_status.value());
  }

  cout << "========== PCIe TLP ==========" << endl;

  cout << "Type            : " << tlpTypeToStr << endl;
  cout << "Format          : " << fmtToStr << endl;

  cout << "Requester ID    : " << m_requesterId << endl;

  if (m_completerId.has_value()) {
    cout << "Completer ID    : " << m_completerId.value() << endl;
  }

  cout << "Tag             : 0x" << static_cast<int>(m_tag) << endl;

  cout << "Traffic Class   : " << static_cast<int>(m_tc) << endl;

  cout << "Attributes      : "
       << "No Snoop=" << (m_attr.noSnoop ? "true" : "false") << ", "
       << "Relaxed Ordering=" << (m_attr.relaxedOrdering ? "true" : "false")
       << endl;

  if (m_address) {
    cout << "Address         : 0x" << std::hex << *m_address << std::dec
         << endl;
  }

  if (m_lengthDw) {
    cout << "Length (DW)     : " << *m_lengthDw << endl;
  }

  if (m_byteCount) {
    cout << "Byte Count      : " << *m_byteCount << endl;
  }

  if (m_status) {
    cout << "Completion Stat : " << cplStatusToStr << endl;
  }

  if (m_isMalformed) {
    cout << "Packet Status : MALFORMED" << endl;
  } else {
    cout << "Packet Status   : OK" << endl;
  }

  if (!m_decodeErrors.empty()) {
    cout << "Decode Errors   :" << endl;
    for (const auto &err : m_decodeErrors) {
      cout << "  - [" << err.rule_id << "] Field: " << err.field << " - "
           << err.description << endl;
    }
  }

  cout << "================================" << endl;
}
