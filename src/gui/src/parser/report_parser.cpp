#include "gui/parser/report_parser.h"
#include "gui/utils/utils.h"
#include "nlohmann/json.hpp"
#include "pugixml/pugixml.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

using json = nlohmann::json;
using pugi::xml_document;

ParseResult ReportParser::parse(const std::filesystem::path &reportPath) {
  // Check if the file exists in the provided path
  if (!std::filesystem::exists(reportPath)) {
    return {std::nullopt, "File not found: " + reportPath.string(), false};
  }

  // Check the file extension if it's supported
  std::string fileExtension = reportPath.extension();
  if (fileExtension != ".json" && fileExtension != ".xml") {
    return {std::nullopt, "Unsupported file extension: " + fileExtension,
            false};
  }
  std::ifstream file(reportPath);

  // Check if the file can be opened
  if (!file.is_open()) {
    return {std::nullopt, "Could not open file: " + reportPath.string(), false};
  }

  // Parse / return error
  std::stringstream content;
  content << file.rdbuf();

  ParseResult result{};
  if (fileExtension == ".json") {
    result = parseJson(content.str());
  } else {
    result = parseXml(content.str());
  }

  file.close();
  return result;
}

ParseResult ReportParser::parseJson(const std::string &content) {
  try {
    json jsonReport = json::parse(content);
    ReportModel report;

    // Check Schema Version
    if (!jsonReport.contains("schema_version")) {
      return {std::nullopt, "Missing required field: schema_version", false};
    }
    report.schema_version = jsonReport["schema_version"].get<std::string>();
    if (report.schema_version != "1.0") {
      return {std::nullopt,
              "Unsupported schema version: " + report.schema_version, false};
    }

    report.generated_at = jsonReport.value("generated_at", "");
    report.trace_file = jsonReport.value("trace_file", "");

    // Summary
    if (jsonReport.contains("summary")) {
      auto s = jsonReport["summary"];
      report.stats.total_packets = s.value("total_packets", 0ULL);
      report.stats.malformed_packet_count =
          s.value("malformed_packet_count", 0ULL);
      report.stats.validation_error_count =
          s.value("validation_error_count", 0ULL);
      report.stats.skipped_line_count = s.value("skipped_line_count", 0ULL);

      if (s.contains("tlp_type_distribution")) {
        for (auto &[key, value] : s["tlp_type_distribution"].items()) {
          TlpType type = Utils::stringToTlpType(key);
          report.stats.tlp_type_distribution[type] = value.get<uint64_t>();
        }
      }
    }

    // Global Validation Errors
    if (jsonReport.contains("validation_errors")) {
      for (auto &jsonError : jsonReport["validation_errors"]) {
        ValidationError v_err;
        v_err.rule_id = jsonError.value("rule_id", "UNKNOWN");
        v_err.category = Utils::stringToValidationType(
            jsonError.value("category", "UNKNOWN"));
        v_err.packet_index = jsonError.value("packet_index", 0ULL);
        v_err.description = jsonError.value("description", "");

        if (jsonError.contains("related_index") &&
            !jsonError["related_index"].is_null()) {
          v_err.related_index =
              std::to_string(jsonError["related_index"].get<uint64_t>());
        } else {
          v_err.related_index = "—";
        }
        report.all_validation_errors.push_back(v_err);
      }
    }

    // Packets
    if (jsonReport.contains("packets")) {
      for (auto &jsonPacket : jsonReport["packets"]) {
        Packet packet;
        packet.index = jsonPacket.value("index", 0ULL);
        packet.timestamp_ns = jsonPacket.value("timestamp_ns", 0ULL);
        packet.direction =
            Utils::stringToDirection(jsonPacket.value("direction", "UNKNOWN"));
        packet.is_malformed = jsonPacket.value("is_malformed", false);

        if (!packet.is_malformed && jsonPacket.contains("tlp")) {
          auto tlp = jsonPacket["tlp"];
          packet.tlp_type =
              Utils::stringToTlpType(tlp.value("type", "UNKNOWN"));
          packet.header_fmt =
              Utils::stringToFmt(tlp.value("header_fmt", "UNKNOWN"));

          if (tlp.contains("address") && !tlp["address"].is_null()) {
            packet.address = tlp["address"].get<std::string>();
          } else {
            packet.address = "—";
          }

          if (tlp.contains("length_dw") && !tlp["length_dw"].is_null()) {
            packet.length = std::to_string(tlp["length_dw"].get<uint32_t>());
          } else {
            packet.length = "—";
          }

          if (tlp.contains("tag") && !tlp["tag"].is_null()) {
            packet.tag = "0x" + ([&]() {
                           std::stringstream ss;
                           ss << std::hex << std::uppercase
                              << tlp["tag"].get<uint32_t>();
                           return ss.str();
                         }());
          } else {
            packet.tag = "—";
          }

          if (tlp.contains("status") && !tlp["status"].is_null()) {
            if (tlp["status"].is_number()) {
              packet.status =
                  Utils::intToCompletionStatus(tlp["status"].get<int>());
            } else if (tlp["status"].is_string()) {
              packet.status = Utils::stringToCompletionStatus(
                  tlp["status"].get<std::string>());
            } else {
              packet.status = CompletionStatus::UNKNOWN;
            }
          } else {
            packet.status = CompletionStatus::UNKNOWN;
          }
        } else {
          packet.tlp_type = TlpType::UNKNOWN;
          packet.header_fmt = Fmt::UNKNOWN;
          packet.address = "—";
          packet.length = "—";
          packet.tag = "—";
          packet.status = CompletionStatus::UNKNOWN;
        }

        // packet Decode Errors
        if (jsonPacket.contains("decode_errors")) {
          for (auto &jsonError : jsonPacket["decode_errors"]) {
            DecodeError d_err;
            d_err.rule_id = jsonError.value("rule_id", "UNKNOWN");
            d_err.field = jsonError.value("field", "UNKNOWN");
            d_err.description = jsonError.value("description", "");
            packet.decode_errors.push_back(d_err);
          }
        }

        // Packet Validation Errors
        if (jsonPacket.contains("validation_errors")) {
          for (auto &jsonError : jsonPacket["validation_errors"]) {
            ValidationError v_err;
            v_err.rule_id = jsonError.value("rule_id", "UNKNOWN");
            v_err.category = Utils::stringToValidationType(
                jsonError.value("category", "UNKNOWN"));
            v_err.packet_index = packet.index;
            v_err.description = jsonError.value("description", "");
            if (jsonError.contains("related_index") &&
                !jsonError["related_index"].is_null()) {
              v_err.related_index =
                  std::to_string(jsonError["related_index"].get<uint64_t>());
            } else {
              v_err.related_index = "—";
            }
            packet.validation_errors.push_back(v_err);
          }
        }

        packet.has_validation_errors = !packet.validation_errors.empty();
        packet.has_any_error =
            packet.is_malformed || packet.has_validation_errors;

        report.packets.push_back(packet);
      }
    }

    for (const auto &v_err : report.all_validation_errors) {
      if (v_err.packet_index < report.packets.size()) {
        report.packets[v_err.packet_index].has_validation_errors = true;
        report.packets[v_err.packet_index].has_any_error = true;
      }
    }

    return {std::move(report), "", true};
  } catch (const json::parse_error &e) {
    return {std::nullopt, std::string("JSON Parse Error: ") + e.what(), false};
  } catch (const std::exception &e) {
    return {std::nullopt, std::string("Error: ") + e.what(), false};
  }

  return {std::nullopt, "", false};
}

ParseResult ReportParser::parseXml(const std::string &content) {
  try {
    pugi::xml_document doc;

    pugi::xml_parse_result result = doc.load_string(content.c_str());

    if (!result) {
      return {std::nullopt,
              std::string("XML Parse Error: ") + result.description(), false};
    }

    auto root = doc.child("report");

    if (!root) {
      return {std::nullopt, "Missing root node: <report>", false};
    }

    ReportModel report;

    // Schema version
    auto schemaNode = root.child("schema_version");

    if (!schemaNode) {
      return {std::nullopt, "Missing required field: schema_version", false};
    }

    report.schema_version = schemaNode.text().as_string();

    if (report.schema_version != "1.0") {
      return {std::nullopt,
              "Unsupported schema version: " + report.schema_version, false};
    }

    report.generated_at = root.child("generated_at").text().as_string();

    report.trace_file = root.child("trace_file").text().as_string();

    // Summary

    auto summary = root.child("summary");

    if (summary) {

      report.stats.total_packets =
          summary.child("total_packets").text().as_ullong();

      report.stats.malformed_packet_count =
          summary.child("malformed_packet_count").text().as_ullong();

      report.stats.validation_error_count =
          summary.child("validation_error_count").text().as_ullong();

      report.stats.skipped_line_count =
          summary.child("skipped_line_count").text().as_ullong();

      auto dist = summary.child("tlp_type_distribution");

      if (dist) {
        for (auto typeNode : dist.children()) {

          TlpType type = Utils::stringToTlpType(typeNode.name());

          report.stats.tlp_type_distribution[type] =
              typeNode.text().as_ullong();
        }
      }
    }

    // Global Validation Errors

    auto validationErrors = root.child("validation_errors");

    if (validationErrors) {

      for (auto errorNode : validationErrors.children("error")) {

        ValidationError validationError;

        validationError.rule_id =
            errorNode.child("rule_id").text().as_string("UNKNOWN");

        validationError.category = Utils::stringToValidationType(
            errorNode.child("category").text().as_string("UNKNOWN"));

        validationError.packet_index =
            errorNode.child("packet_index").text().as_ullong();

        validationError.description =
            errorNode.child("description").text().as_string();

        auto related = errorNode.child("related_index");

        if (related) {
          validationError.related_index = related.text().as_string();
        } else {
          validationError.related_index = "—";
        }

        report.all_validation_errors.push_back(validationError);
      }
    }

    // Packets

    auto packetsNode = root.child("packets");

    if (packetsNode) {

      for (auto packetNode : packetsNode.children("packet")) {

        Packet packet;

        packet.index = packetNode.child("index").text().as_ullong();

        packet.timestamp_ns =
            packetNode.child("timestamp_ns").text().as_ullong();

        packet.direction = Utils::stringToDirection(
            packetNode.child("direction").text().as_string("UNKNOWN"));

        packet.is_malformed = packetNode.child("is_malformed").text().as_bool();

        // packets

        auto tlp = packetNode.child("tlp");

        if (!packet.is_malformed && tlp) {

          packet.tlp_type = Utils::stringToTlpType(
              tlp.child("type").text().as_string("UNKNOWN"));

          packet.header_fmt = Utils::stringToFmt(
              tlp.child("header_fmt").text().as_string("UNKNOWN"));

          auto address = tlp.child("address");

          if (address) {
            packet.address = address.text().as_string();
          } else {
            packet.address = "—";
          }

          auto length = tlp.child("length_dw");

          if (length) {
            packet.length = length.text().as_string();
          } else {
            packet.length = "—";
          }

          auto tag = tlp.child("tag");

          if (tag) {

            std::stringstream ss;

            ss << "0x" << std::hex << std::uppercase
               << static_cast<int>(tag.text().as_int());

            packet.tag = ss.str();

          } else {
            packet.tag = "—";
          }

          auto status = tlp.child("status");

          if (status) {

            if (status.text().as_string()[0] >= '0' &&
                status.text().as_string()[0] <= '9') {

              packet.status =
                  Utils::intToCompletionStatus(status.text().as_int());

            } else {

              packet.status =
                  Utils::stringToCompletionStatus(status.text().as_string());
            }

          } else {
            packet.status = CompletionStatus::UNKNOWN;
          }

        } else {

          packet.tlp_type = TlpType::UNKNOWN;
          packet.header_fmt = Fmt::UNKNOWN;
          packet.address = "—";
          packet.length = "—";
          packet.tag = "—";
          packet.status = CompletionStatus::UNKNOWN;
        }

        // Decode Errors

        auto decodeErrors = packetNode.child("decode_errors");

        if (decodeErrors) {

          for (auto errorNode : decodeErrors.children("error")) {

            DecodeError d_err;

            d_err.rule_id =
                errorNode.child("rule_id").text().as_string("UNKNOWN");

            d_err.field = errorNode.child("field").text().as_string("UNKNOWN");

            d_err.description =
                errorNode.child("description").text().as_string();

            packet.decode_errors.push_back(d_err);
          }
        }

        // Packet Validation Errors

        auto packetValidationErrors = packetNode.child("validation_errors");

        if (packetValidationErrors) {

          for (auto errorNode : packetValidationErrors.children("error")) {

            ValidationError v_err;

            v_err.rule_id =
                errorNode.child("rule_id").text().as_string("UNKNOWN");

            v_err.category = Utils::stringToValidationType(
                errorNode.child("category").text().as_string("UNKNOWN"));

            v_err.packet_index = packet.index;

            v_err.description =
                errorNode.child("description").text().as_string();

            auto related = errorNode.child("related_index");

            if (related) {
              v_err.related_index = related.text().as_string();
            } else {
              v_err.related_index = "—";
            }

            packet.validation_errors.push_back(v_err);
          }
        }

        packet.has_validation_errors = !packet.validation_errors.empty();

        packet.has_any_error =
            packet.is_malformed || packet.has_validation_errors;

        report.packets.push_back(std::move(packet));
      }
    }

    // Mark packets referenced by global validation errors
    for (const auto &v_err : report.all_validation_errors) {

      if (v_err.packet_index < report.packets.size()) {

        report.packets[v_err.packet_index].has_validation_errors = true;

        report.packets[v_err.packet_index].has_any_error = true;
      }
    }

    return {std::move(report), "", true};

  } catch (const std::exception &e) {

    return {std::nullopt, std::string("XML Error: ") + e.what(), false};
  }
}