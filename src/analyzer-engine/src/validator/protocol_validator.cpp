#include "analyzer-engine/validator/protocol_validator.h"

std::vector<ValidationError> ProtocolValidator::process(const TLP &tlp) {
  std::uint64_t currentIndex = tlp.index();
  std::vector<ValidationError> local_errors;

  // VAL-007
  if (tlp.tc() > 7) {
    local_errors.push_back({"VAL-007", ValidationType::INVALID_FIELD_VALUE,
                            currentIndex, std::nullopt,
                            "Invalid traffic class value (" +
                                std::to_string(tlp.tc()) + "): exceeds 7."});
  }

  TlpType type = tlp.type();
  TransactionKey key{tlp.requesterId(), tlp.tag()};

  if (type == TlpType::MRd || type == TlpType::MWr) {
    // VAL-008
    std::uint64_t lengthDw = tlp.lengthDw().value_or(0);
    if (lengthDw == 0) {
      local_errors.push_back({"VAL-008", ValidationType::INVALID_FIELD_VALUE,
                              currentIndex, std::nullopt,
                              "Zero-length request."});
    } else {
      // VAL-005
      if (tlp.address().has_value()) {
        std::uint64_t addr = tlp.address().value();
        if (addr % (lengthDw * 4) != 0) {
          local_errors.push_back(
              {"VAL-005", ValidationType::ADDRESS_MISALIGNMENT, currentIndex,
               std::nullopt,
               "Address 0x" + std::to_string(addr) +
                   " is not naturally aligned to " +
                   std::to_string(lengthDw * 4) + " bytes."});
        }
      }
    }

    if (type == TlpType::MRd) {
      // VAL-006
      auto it = outstanding_requests_.find(key);
      if (it != outstanding_requests_.end()) {
        local_errors.push_back({"VAL-006", ValidationType::TAG_COLLISION,
                                currentIndex, it->second.packet_index,
                                "Tag collision: tag 0x" +
                                    std::to_string(tlp.tag()) +
                                    " reused before prior request completed."});
      }

      // Record outstanding request
      outstanding_requests_[key] = {currentIndex,
                                    static_cast<uint16_t>(lengthDw), 0};

      // Remove the key from the satisfied requests to reuse the same tags again
      satisfied_requests_.erase(key);
    }
  } else if (type == TlpType::Cpl || type == TlpType::CplD) {
    auto it = outstanding_requests_.find(key);
    if (it == outstanding_requests_.end()) {
      if (satisfied_requests_.count(key) > 0) {
        // VAL-003
        local_errors.push_back({"VAL-003", ValidationType::DUPLICATE_COMPLETION,
                                currentIndex, std::nullopt,
                                "Duplicate completion."});
      } else {
        // VAL-001
        local_errors.push_back(
            {"VAL-001", ValidationType::UNEXPECTED_COMPLETION, currentIndex,
             std::nullopt, "Completion without a matching request."});
      }
    } else {
      // Valid matching completion
      if (type == TlpType::CplD) {
        std::uint64_t lengthDw = tlp.lengthDw().value_or(0);
        if (lengthDw == 0) {
          local_errors.push_back(
              {"VAL-008", ValidationType::INVALID_FIELD_VALUE, currentIndex,
               std::nullopt, "CplD with zero length."});
        }

        // VAL-004
        std::uint64_t requestedBytes =
            static_cast<uint64_t>(it->second.length_dw) * 4;
        std::uint64_t receivedBytes = tlp.byteCount().value_or(0);

        if (receivedBytes != requestedBytes) {
          local_errors.push_back(
              {"VAL-004", ValidationType::BYTE_COUNT_MISMATCH, currentIndex,
               it->second.packet_index,
               "Returned byte count (" + std::to_string(receivedBytes) +
                   ") does not match requested length (" +
                   std::to_string(requestedBytes) + " bytes)."});
        }
      } else if (type == TlpType::Cpl) {
        std::uint64_t lengthDw = tlp.lengthDw().value_or(0);
        if (lengthDw != 0) {
          local_errors.push_back(
              {"VAL-008", ValidationType::INVALID_FIELD_VALUE, currentIndex,
               std::nullopt, "Cpl with non-zero length."});
        }
      }

      // Satisfy the request
      satisfied_requests_.insert(key);
      outstanding_requests_.erase(it);
    }
  }
  return local_errors;
}

std::vector<ValidationError> ProtocolValidator::finalize() {
  std::vector<ValidationError> end_errors;
  for (const auto &pair : outstanding_requests_) {
    end_errors.push_back({"VAL-002", ValidationType::MISSING_COMPLETION,
                          pair.second.packet_index, std::nullopt,
                          "MRd with no CplD before end-of-trace."});
  }
  return end_errors;
}