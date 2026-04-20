#include "analyzer-engine/report-builder/report_builder.h"
#include "analyzer-engine/utils/utils.h"
#include "tinyxml2/tinyxml2.h"
#include <fstream>

ReportBuilder::ReportBuilder(ReportFormat format,
                             std::filesystem::path trace_file)
    : format_(format), trace_file_(std::move(trace_file)), tlps_(), errors_() {}

void ReportBuilder::addTLP(const TLP &tlp) { tlps_.push_back(tlp); }

void ReportBuilder::addValidationError(const ValidationError &error) {
  errors_.push_back(error);
}

void ReportBuilder::write(const std::filesystem::path &output_path,
                          std::uint64_t skipped_line_count) const {
  if (format_ == ReportFormat::JSON) {
    writeJson(output_path, skipped_line_count);
  } else if (format_ == ReportFormat::XML) {
    writeXml(output_path, skipped_line_count);
  }
}

void ReportBuilder::writeJson(const std::filesystem::path &output_path,
                              std::uint64_t skipped_line_count) const {

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

using namespace tinyxml2;

void ReportBuilder::writeXml(const std::filesystem::path &output_path,
                             std::uint64_t skipped_line_count) const {
  XMLDocument doc;

  // Root
  XMLElement *report = doc.NewElement("report");
  doc.InsertFirstChild(report);

  report->InsertNewChildElement("schema_version")->SetText("1.0");
  report->InsertNewChildElement("generated_at")
      ->SetText(Utils::getTimestamp().c_str());
  report->InsertNewChildElement("trace_file")
      ->SetText(trace_file_.string().c_str());

  // Summary
  XMLElement *summary = doc.NewElement("summary");

  summary->InsertNewChildElement("total_packets")->SetText((int)tlps_.size());
  summary->InsertNewChildElement("validation_error_count")
      ->SetText((int)errors_.size());
  summary->InsertNewChildElement("skipped_line_count")
      ->SetText((int)skipped_line_count);

  int malformed_count = 0;

  XMLElement *distribution = doc.NewElement("tlp_type_distribution");
  std::map<std::string, int> dist = {
      {"MRd", 0}, {"MWr", 0}, {"Cpl", 0}, {"CplD", 0}};

  for (const auto &tlp : tlps_) {
    if (tlp.isMalformed()) {
      malformed_count++;
    } else {
      std::string type = Utils::tlpTypeToString(tlp.type());
      dist[type]++;
    }
  }

  for (auto &[k, v] : dist) {
    XMLElement *e = doc.NewElement(k.c_str());
    e->SetText(v);
    distribution->InsertEndChild(e);
  }

  summary->InsertEndChild(distribution);
  summary->InsertNewChildElement("malformed_packet_count")
      ->SetText(malformed_count);

  report->InsertEndChild(summary);

  // Malformed Packets
  XMLElement *malformed_packets = doc.NewElement("malformed_packets");

  for (const auto &tlp : tlps_) {
    if (!tlp.isMalformed())
      continue;

    XMLElement *m = doc.NewElement("packet");

    m->InsertNewChildElement("packet_index")->SetText(tlp.index());
    m->InsertNewChildElement("timestamp_ns")->SetText(tlp.timestampNs());
    m->InsertNewChildElement("direction")
        ->SetText(tlp.direction() == Direction::TX ? "TX" : "RX");
    m->InsertNewChildElement("payload_hex")->SetText(tlp.rawBytes().c_str());

    XMLElement *decode_errors = doc.NewElement("decode_errors");

    for (const auto &err : tlp.decodeErrors()) {
      XMLElement *de = doc.NewElement("error");

      de->InsertNewChildElement("rule_id")->SetText(err.rule_id.c_str());
      de->InsertNewChildElement("field")->SetText(err.field.c_str());
      de->InsertNewChildElement("description")
          ->SetText(err.description.c_str());

      decode_errors->InsertEndChild(de);
    }

    m->InsertEndChild(decode_errors);
    malformed_packets->InsertEndChild(m);
  }

  report->InsertEndChild(malformed_packets);

  // Validation Errors
  XMLElement *validation_errors = doc.NewElement("validation_errors");

  for (const auto &err : errors_) {
    XMLElement *e = doc.NewElement("error");

    e->InsertNewChildElement("rule_id")->SetText(err.rule_id.c_str());
    e->InsertNewChildElement("category")
        ->SetText(Utils::validationCategoryToStr(err.category).c_str());
    e->InsertNewChildElement("packet_index")->SetText(err.packet_index);

    if (err.related_index.has_value()) {
      e->InsertNewChildElement("related_index")->SetText(*err.related_index);
    }

    e->InsertNewChildElement("description")->SetText(err.description.c_str());

    validation_errors->InsertEndChild(e);
  }

  report->InsertEndChild(validation_errors);

  // Packets
  XMLElement *packets = doc.NewElement("packets");

  for (const auto &tlp : tlps_) {
    if (tlp.isMalformed())
      continue;

    XMLElement *p = doc.NewElement("packet");

    p->InsertNewChildElement("index")->SetText(tlp.index());
    p->InsertNewChildElement("timestamp_ns")->SetText(tlp.timestampNs());
    p->InsertNewChildElement("direction")
        ->SetText(tlp.direction() == Direction::TX ? "TX" : "RX");

    XMLElement *t = doc.NewElement("tlp");

    t->InsertNewChildElement("type")->SetText(
        Utils::tlpTypeToString(tlp.type()).c_str());

    t->InsertNewChildElement("tag")->SetText(tlp.tag());

    if (tlp.address().has_value()) {
      std::ostringstream oss;
      oss << "0x" << std::hex << *tlp.address();
      t->InsertNewChildElement("address")->SetText(oss.str().c_str());
    }

    p->InsertEndChild(t);
    packets->InsertEndChild(p);
  }

  report->InsertEndChild(packets);

  doc.SaveFile(output_path.string().c_str());
}
