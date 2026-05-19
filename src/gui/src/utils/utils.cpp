#include "gui/utils/utils.h"
#include "gui/ui/packets_table.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Utils {

Direction stringToDirection(const std::string &strDirection) {
  std::string strDirectionCopy = strDirection;
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
    return "—";
  }
}

TlpType stringToTlpType(const std::string &strType) {
  std::string strTypeCopy = strType;
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
    return "—";
  }
}

Fmt stringToFmt(const std::string &strFmt) {
  std::string strFmtCopy = strFmt;
  std::transform(strFmtCopy.begin(), strFmtCopy.end(), strFmtCopy.begin(),
                 ::toupper);
  if (strFmtCopy == "3DW")
    return Fmt::DW3;
  if (strFmtCopy == "4DW")
    return Fmt::DW4;
  return Fmt::UNKNOWN;
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

CompletionStatus stringToCompletionStatus(const std::string &strStatus) {
  std::string str = strStatus;
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  if (str == "SC")
    return CompletionStatus::SC;
  if (str == "UR")
    return CompletionStatus::UR;
  if (str == "CA")
    return CompletionStatus::CA;
  return CompletionStatus::UNKNOWN;
}

CompletionStatus intToCompletionStatus(int statusVal) {
  switch (statusVal) {
  case 0:
    return CompletionStatus::SC;
  case 1:
    return CompletionStatus::UR;
  case 4:
    return CompletionStatus::CA;
  default:
    return CompletionStatus::UNKNOWN;
  }
}

std::string completionStatusToStr(CompletionStatus status) {
  switch (status) {
  case CompletionStatus::SC:
    return "SC";
  case CompletionStatus::UR:
    return "UR";
  case CompletionStatus::CA:
    return "CA";
  default:
    return "—";
  }
}

ValidationType stringToValidationType(const std::string &strCategory) {
  std::string str = strCategory;
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  if (str == "UNEXPECTED_COMPLETION")
    return ValidationType::UNEXPECTED_COMPLETION;
  if (str == "MISSING_COMPLETION")
    return ValidationType::MISSING_COMPLETION;
  if (str == "DUPLICATE_COMPLETION")
    return ValidationType::DUPLICATE_COMPLETION;
  if (str == "BYTE_COUNT_MISMATCH")
    return ValidationType::BYTE_COUNT_MISMATCH;
  if (str == "ADDRESS_MISALIGNMENT")
    return ValidationType::ADDRESS_MISALIGNMENT;
  if (str == "TAG_COLLISION")
    return ValidationType::TAG_COLLISION;
  if (str == "INVALID_FIELD_VALUE")
    return ValidationType::INVALID_FIELD_VALUE;

  // Default fallback
  return ValidationType::INVALID_FIELD_VALUE;
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
    return "—";
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

QVariant getTableColumnStr(int section) {
  switch (section) {
  case PacketsTableModel::Index:
    return "Index";
  case PacketsTableModel::Timestamp:
    return "Timestamp (ns)";
  case PacketsTableModel::Direction:
    return "Direction";
  case PacketsTableModel::Type:
    return "Type";
  case PacketsTableModel::Address:
    return "Address";
  case PacketsTableModel::Length:
    return "Length";
  case PacketsTableModel::Tag:
    return "Tag";
  case PacketsTableModel::Status:
    return "Status";
  case PacketsTableModel::Errors:
    return "Validation Summary";
  default:
    return QVariant();
  }
}
} // namespace Utils