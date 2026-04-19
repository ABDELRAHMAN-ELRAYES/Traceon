#include "analyzer-engine/report-builder/report_builder.h"
#include "analyzer-engine/utils/utils.h"
#include <fstream>

ReportBuilder::ReportBuilder(ReportFormat format,
                             std::filesystem::path trace_file)
    : format_(format), trace_file_(std::move(trace_file)), tlps_(), errors_() {}

void ReportBuilder::add_tlp(const TLP &tlp) { tlps_.push_back(tlp); }

void ReportBuilder::add_validation_error(const ValidationError &error) {
  errors_.push_back(error);
}

void ReportBuilder::write(const std::filesystem::path &output_path,
                          std::uint64_t skipped_line_count) const {
  if (format_ != ReportFormat::JSON) {
    return;
  }

  json report;
  report["schema_version"] = "1.0";
  report["generated_at"] = Utils::getTimestamp();
  report["trace_file"] = trace_file_.string();

  // Summary
  json distribution;
  distribution["MRd"] = 0;
  distribution["MWr"] = 0;
  distribution["Cpl"] = 0;
  distribution["CplD"] = 0;

  int malformed_count = 0;
  for (const auto &tlp : tlps_) {
    if (tlp.isMalformed()) {
      malformed_count++;
    } else {
      std::string type_str = Utils::tlpTypeToString(tlp.type());
      if (distribution.contains(type_str)) {
        distribution[type_str] = distribution[type_str].get<int>() + 1;
      }
    }
  }

  json summary;
  summary["total_packets"] = tlps_.size();
  summary["tlp_type_distribution"] = distribution;
  summary["malformed_packet_count"] = malformed_count;
  summary["validation_error_count"] = errors_.size();
  summary["skipped_line_count"] = skipped_line_count;

  report["summary"] = summary;

  // Malformed packets
  report["malformed_packets"] = json::array();
  for (const auto &tlp : tlps_) {
    if (tlp.isMalformed()) {
      json m;
      m["packet_index"] = tlp.index();
      m["timestamp_ns"] = tlp.timestampNs();
      m["direction"] = (tlp.direction() == Direction::TX ? "TX" : "RX");
      m["payload_hex"] = tlp.rawBytes();

      json decode_errors = json::array();
      for (const auto &err : tlp.decodeErrors()) {
        json de;
        de["rule_id"] = err.rule_id;
        de["field"] = err.field;
        de["description"] = err.description;
        decode_errors.push_back(de);
      }
      m["decode_errors"] = decode_errors;
      report["malformed_packets"].push_back(m);
    }
  }

  // Validation errors
  report["validation_errors"] = json::array();
  for (const auto &err : errors_) {
    json e;
    e["rule_id"] = err.rule_id;
    e["category"] = Utils::validationCategoryToStr(err.category);
    e["packet_index"] = err.packet_index;
    if (err.related_index.has_value()) {
      e["related_index"] = *err.related_index;
    } else {
      e["related_index"] = nullptr;
    }
    e["description"] = err.description;
    report["validation_errors"].push_back(e);
  }

  // Packets
  report["packets"] = json::array();
  for (const auto &tlp : tlps_) {
    if (!tlp.isMalformed()) {
      json p;
      p["index"] = tlp.index();
      p["timestamp_ns"] = tlp.timestampNs();
      p["direction"] = (tlp.direction() == Direction::TX ? "TX" : "RX");
      p["is_malformed"] = false;

      json t;
      t["type"] = Utils::tlpTypeToString(tlp.type());
      t["header_fmt"] = Utils::fmtToStr(tlp.fmt());
      t["tc"] = tlp.tc();

      json attr;
      attr["no_snoop"] = tlp.attr().no_snoop;
      attr["relaxed_ordering"] = tlp.attr().relaxed_ordering;
      t["attr"] = attr;

      t["requester_id"] = tlp.requesterId();
      if (tlp.completerId().has_value()) {
        t["completer_id"] = *tlp.completerId();
      } else {
        t["completer_id"] = nullptr;
      }
      t["tag"] = tlp.tag();

      if (tlp.address().has_value()) {
        std::ostringstream oss;
        oss << "0x" << std::hex << *tlp.address();
        t["address"] = oss.str();
      } else {
        t["address"] = nullptr;
      }

      if (tlp.lengthDw().has_value()) {
        t["length_dw"] = *tlp.lengthDw();
      } else {
        t["length_dw"] = nullptr;
      }

      t["has_data"] =
          (tlp.type() == TlpType::MWr || tlp.type() == TlpType::CplD);

      if (tlp.byteCount().has_value()) {
        t["byte_count"] = *tlp.byteCount();
      } else {
        t["byte_count"] = nullptr;
      }

      if (tlp.status().has_value()) {
        t["status"] = static_cast<int>(*tlp.status());
      } else {
        t["status"] = nullptr;
      }

      p["tlp"] = t;
      report["packets"].push_back(p);
    }
  }
  
  std::ofstream out(output_path);
  out << report.dump(2);
}
