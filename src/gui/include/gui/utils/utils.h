#ifndef UTILS_NAMESPACE
#define UTILS_NAMESPACE

#include "gui/models/models.h"
#include <QVariant>
#include <string>

namespace Utils {

Direction stringToDirection(const std::string &strDirection);
std::string directionToString(Direction direction);

TlpType stringToTlpType(const std::string &strType);
std::string tlpTypeToString(TlpType type);

Fmt stringToFmt(const std::string &strFmt);
std::string fmtToStr(Fmt fmt);

CompletionStatus stringToCompletionStatus(const std::string &strStatus);
CompletionStatus intToCompletionStatus(int statusVal);
std::string completionStatusToStr(CompletionStatus status);

ValidationType stringToValidationType(const std::string &strCategory);
std::string validationCategoryToStr(ValidationType type);

std::string getTimestamp();

QVariant getTableColumnStr(int section);

} // namespace Utils

#endif