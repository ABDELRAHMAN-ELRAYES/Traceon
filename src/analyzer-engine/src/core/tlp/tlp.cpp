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
    : type_(type), fmt_(fmt), attr_(attr),
      requester_id_(std::move(requesterId)), completer_id_(std::nullopt),
      tag_(tag), tc_(tc), address_(std::nullopt), length_dw_(std::nullopt),
      byte_count_(std::nullopt), status_(std::nullopt), is_malformed_(false),
      index_(index), timestamp_ns_(0), direction_(Direction::UNKNOWN),
      raw_bytes_(""), decode_errors_() {}

TLP::TLP(TlpType type, Fmt fmt, Attr attr, std::string requesterId,
         std::uint8_t tag, std::uint8_t tc, std::uint64_t index,
         std::uint64_t timestamp, Direction direction, std::string rawBytes)
    : type_(type), fmt_(fmt), attr_(attr),
      requester_id_(std::move(requesterId)), completer_id_(std::nullopt),
      tag_(tag), tc_(tc), address_(std::nullopt), length_dw_(std::nullopt),
      byte_count_(std::nullopt), status_(std::nullopt), is_malformed_(false),
      index_(index), timestamp_ns_(timestamp), direction_(direction),
      raw_bytes_(std::move(rawBytes)), decode_errors_() {}

void TLP::printPacketDetails() {
  using std::cout;
  using std::endl;

  std::string tlpTypeToStr = Utils::tlpTypeToString(type_);

  std::string fmtToStr = Utils::fmtToStr(fmt_);
  std::string cplStatusToStr = "Unknown";
  if (status_.has_value()) {
    cplStatusToStr = Utils::completionStatusToStr(status_.value());
  }

  cout << "========== PCIe TLP ==========" << endl;

  cout << "Type            : " << tlpTypeToStr << endl;
  cout << "Format          : " << fmtToStr << endl;

  cout << "Requester ID    : " << requester_id_ << endl;

  if (completer_id_.has_value()) {
    cout << "Completer ID    : " << completer_id_.value() << endl;
  }

  cout << "Tag             : 0x" << static_cast<int>(tag_) << endl;

  cout << "Traffic Class   : " << static_cast<int>(tc_) << endl;

  cout << "Attributes      : "
       << "No Snoop=" << (attr_.no_snoop ? "true" : "false") << ", "
       << "Relaxed Ordering=" << (attr_.relaxed_ordering ? "true" : "false")
       << endl;

  if (address_) {
    cout << "Address         : 0x" << std::hex << *address_ << std::dec
         << endl;
  }

  if (length_dw_) {
    cout << "Length (DW)     : " << *length_dw_ << endl;
  }

  if (byte_count_) {
    cout << "Byte Count      : " << *byte_count_ << endl;
  }

  if (status_) {
    cout << "Completion Stat : " << cplStatusToStr << endl;
  }

  if (is_malformed_) {
    cout << "Packet Status : MALFORMED" << endl;
  } else {
    cout << "Packet Status   : OK" << endl;
  }

  if (!decode_errors_.empty()) {
    cout << "Decode Errors   :" << endl;
    for (const auto &err : decode_errors_) {
      cout << "  - [" << err.rule_id << "] Field: " << err.field << " - "
           << err.description << endl;
    }
  }

  cout << "================================" << endl;
}
