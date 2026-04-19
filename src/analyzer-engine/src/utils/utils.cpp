#include "analyzer-engine/utils/utils.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Utils {
Direction stringToDirection(const std::string &strDirection) {
  std::string strDirectionCopy = strDirection;

  // Convert the entered string direction to upper case for type safety
  std::transform(strDirectionCopy.begin(), strDirectionCopy.end(),
                 strDirectionCopy.begin(), ::toupper);

  if (strDirectionCopy == "TX")
    return Direction::TX;
  if (strDirectionCopy == "RX")
    return Direction::RX;
  return Direction::UNKNOWN;
}

std::string directionToString(Direction direction) {
  switch (direction) {
  case Direction::TX:
    return "TX";
  case Direction::RX:
    return "RX";
  default:
    return "UNKOWN";
  }
}

TlpType stringToTlpType(const std::string &strType) {

  std::string strTypeCopy = strType;

  // Convert the entered string type to upper case for type safety
  std::transform(strTypeCopy.begin(), strTypeCopy.end(), strTypeCopy.begin(),
                 ::toupper);

  if (strTypeCopy == "MRD") {
    return TlpType::MRd;
  } else if (strTypeCopy == "MWR") {
    return TlpType::MWr;
  } else if (strTypeCopy == "CPLD") {
    return TlpType::CplD;
  } else if (strTypeCopy == "CPL") {
    return TlpType::Cpl;
  }
  return TlpType::UNKNOWN;
}

std::string tlpTypeToString(TlpType type) {
  switch (type) {
  case TlpType::MRd:
    return "MRd";
  case TlpType::MWr:
    return "MWr";
  case TlpType::CplD:
    return "CplD";
  case TlpType::Cpl:
    return "Cpl";
  default:
    return "Unknown";
  }
}

std::string fmtToStr(Fmt fmt) {
  switch (fmt) {
  case Fmt::DW3:
    return "3DW";
  case Fmt::DW4:
    return "4DW";
  default:
    return "Unknown";
  }
}

std::string completionStatusToStr(CompletionStatus status) {
  switch (status) {
  case CompletionStatus::SC:
    return "SC (Successful Completion)";
  case CompletionStatus::UR:
    return "UR (Unsupported Request)";
  case CompletionStatus::CA:
    return "CA (Completer Abort)";
  default:
    return "Unknown";
  }
}

std::string validationCategoryToStr(ValidationType type) {
  switch (type) {
  case ValidationType::UNEXPECTED_COMPLETION:
    return "UNEXPECTED_COMPLETION";
  case ValidationType::MISSING_COMPLETION:
    return "MISSING_COMPLETION";
  case ValidationType::DUPLICATE_COMPLETION:
    return "DUPLICATE_COMPLETION";
  case ValidationType::BYTE_COUNT_MISMATCH:
    return "BYTE_COUNT_MISMATCH";
  case ValidationType::ADDRESS_MISALIGNMENT:
    return "ADDRESS_MISALIGNMENT";
  case ValidationType::TAG_COLLISION:
    return "TAG_COLLISION";
  case ValidationType::INVALID_FIELD_VALUE:
    return "INVALID_FIELD_VALUE";
  default:
    return "UNKNOWN";
  }
}

std::string getTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto it = std::chrono::system_clock::to_time_t(now);
  struct tm gmt;
  gmtime_r(&it, &gmt);
  std::ostringstream oss;
  oss << std::put_time(&gmt, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

} // namespace Utils