#ifndef VALIDATION_ERROR_H
#define VALIDATION_ERROR_H

#include <cstdint>
#include <optional>
#include <string>

enum class ValidationType {
  UNEXPECTED_COMPLETION,
  MISSING_COMPLETION,
  DUPLICATE_COMPLETION,
  BYTE_COUNT_MISMATCH,
  ADDRESS_MISALIGNMENT,
  TAG_COLLISION,
  INVALID_FIELD_VALUE,
};

struct ValidationError {
  std::string rule_id;
  ValidationType category;
  std::uint64_t packet_index;
  std::optional<uint64_t> related_index;
  std::string description;
};
#endif