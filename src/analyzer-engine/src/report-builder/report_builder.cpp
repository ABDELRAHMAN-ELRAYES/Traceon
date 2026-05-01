#include "analyzer-engine/report-builder/report_builder.h"
#include "analyzer-engine/utils/utils.h"

ReportBuilder::ReportBuilder(ReportFormat format,
                             std::filesystem::path trace_file,
                             std::filesystem::path output_path)
    : format_(format), trace_file_(std::move(trace_file)),
      output_path_(std::move(output_path)) {
  distribution_["MRd"] = 0;
  distribution_["MWr"] = 0;
  distribution_["Cpl"] = 0;
  distribution_["CplD"] = 0;
}

void ReportBuilder::initializeStream() const {
  if (has_begun_)
    return;

  out_stream_.open(output_path_);
  if (!out_stream_.is_open()) {
    throw std::runtime_error("Failed to open report file for writing: " +
                             output_path_.string());
  }

  if (format_ == ReportFormat::JSON) {
    out_stream_ << "{\n";
    out_stream_ << "  \"schema_version\": \"1.0\",\n";
    out_stream_ << "  \"generated_at\": \"" << Utils::getTimestamp() << "\",\n";
    out_stream_ << "  \"trace_file\": \"" << trace_file_.string() << "\",\n";
    out_stream_ << "  \"packets\": [\n";
  } else {
    out_stream_ << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out_stream_ << "<report>\n";
    out_stream_ << "  <schema_version>1.0</schema_version>\n";
    out_stream_ << "  <generated_at>" << Utils::getTimestamp()
                << "</generated_at>\n";
    out_stream_ << "  <trace_file>" << trace_file_.string()
                << "</trace_file>\n";
    out_stream_ << "  <packets>\n";
  }

  has_begun_ = true;
}

void ReportBuilder::addTLP(
    const TLP &tlp, const std::vector<ValidationError> &validation_errors) {
  initializeStream();

  total_packets_++;
  if (tlp.isMalformed()) {
    malformed_count_++;
  } else {
    std::string type_str = Utils::tlpTypeToString(tlp.type());
    if (distribution_.count(type_str)) {
      distribution_[type_str]++;
    }
  }

  if (format_ == ReportFormat::JSON) {
    writeJsonTLP(tlp, validation_errors);
  } else {
    writeXmlTLP(tlp, validation_errors);
  }
}

void ReportBuilder::writeJsonTLP(
    const TLP &tlp,
    const std::vector<ValidationError> &validation_errors) const {
  if (!first_packet_) {
    out_stream_ << ",\n";
  }
  first_packet_ = false;

  out_stream_ << "    {";
  out_stream_ << "\"index\":" << tlp.index() << ",";
  out_stream_ << "\"timestamp_ns\":" << tlp.timestampNs() << ",";
  out_stream_ << "\"direction\":\""
              << (tlp.direction() == Direction::TX ? "TX" : "RX") << "\",";
  out_stream_ << "\"is_malformed\":" << (tlp.isMalformed() ? "true" : "false");

  if (tlp.isMalformed()) {
    out_stream_ << ",\"payload_hex\":\"" << tlp.rawBytes() << "\",";
    out_stream_ << "\"decode_errors\":[";
    bool first_de = true;
    for (const auto &err : tlp.decodeErrors()) {
      if (!first_de)
        out_stream_ << ",";
      out_stream_ << "{\"rule_id\":\"" << err.rule_id << "\",\"field\":\""
                  << err.field << "\",\"description\":\"" << err.description
                  << "\"}";
      first_de = false;
    }
    out_stream_ << "]";
  } else {
    out_stream_ << ",\"tlp\":{";
    out_stream_ << "\"type\":\"" << Utils::tlpTypeToString(tlp.type()) << "\",";
    out_stream_ << "\"header_fmt\":\"" << Utils::fmtToStr(tlp.fmt()) << "\",";
    out_stream_ << "\"tc\":" << static_cast<int>(tlp.tc()) << ",";
    out_stream_ << "\"attr\":{\"no_snoop\":"
                << (tlp.attr().no_snoop ? "true" : "false")
                << ",\"relaxed_ordering\":"
                << (tlp.attr().relaxed_ordering ? "true" : "false") << "},";
    out_stream_ << "\"requester_id\":\"" << tlp.requesterId() << "\",";
    out_stream_ << "\"completer_id\":";
    if (tlp.completerId().has_value())
      out_stream_ << "\"" << *tlp.completerId() << "\",";
    else
      out_stream_ << "null,";
    out_stream_ << "\"tag\":" << static_cast<int>(tlp.tag()) << ",";
    out_stream_ << "\"address\":";
    if (tlp.address().has_value())
      out_stream_ << "\"0x" << std::hex << *tlp.address() << std::dec << "\",";
    else
      out_stream_ << "null,";
    out_stream_ << "\"length_dw\":";
    if (tlp.lengthDw().has_value())
      out_stream_ << *tlp.lengthDw() << ",";
    else
      out_stream_ << "null,";
    out_stream_ << "\"has_data\":"
                << ((tlp.type() == TlpType::MWr || tlp.type() == TlpType::CplD)
                        ? "true"
                        : "false")
                << ",";
    out_stream_ << "\"byte_count\":";
    if (tlp.byteCount().has_value())
      out_stream_ << *tlp.byteCount() << ",";
    else
      out_stream_ << "null,";
    out_stream_ << "\"status\":";
    if (tlp.status().has_value())
      out_stream_ << static_cast<int>(*tlp.status());
    else
      out_stream_ << "null";
    out_stream_ << "}";
  }

  out_stream_ << ",\"validation_errors\":[";
  bool first_ve = true;
  for (const auto &err : validation_errors) {
    if (!first_ve)
      out_stream_ << ",";
    out_stream_ << "{\"rule_id\":\"" << err.rule_id << "\",";
    out_stream_ << "\"category\":\""
                << Utils::validationCategoryToStr(err.category) << "\",";
    out_stream_ << "\"related_index\":";
    if (err.related_index.has_value())
      out_stream_ << *err.related_index << ",";
    else
      out_stream_ << "null,";
    out_stream_ << "\"description\":\"" << err.description << "\"}";
    first_ve = false;
  }
  out_stream_ << "]";
  out_stream_ << "}";
}

void ReportBuilder::writeXmlTLP(
    const TLP &tlp,
    const std::vector<ValidationError> &validation_errors) const {
  out_stream_ << "    <packet>\n";
  out_stream_ << "      <index>" << tlp.index() << "</index>\n";
  out_stream_ << "      <timestamp_ns>" << tlp.timestampNs()
              << "</timestamp_ns>\n";
  out_stream_ << "      <direction>"
              << (tlp.direction() == Direction::TX ? "TX" : "RX")
              << "</direction>\n";
  out_stream_ << "      <is_malformed>"
              << (tlp.isMalformed() ? "true" : "false") << "</is_malformed>\n";

  if (tlp.isMalformed()) {
    out_stream_ << "      <payload_hex>" << tlp.rawBytes()
                << "</payload_hex>\n";
    out_stream_ << "      <decode_errors>\n";
    for (const auto &err : tlp.decodeErrors()) {
      out_stream_ << "        <error>\n";
      out_stream_ << "          <rule_id>" << err.rule_id << "</rule_id>\n";
      out_stream_ << "          <field>" << err.field << "</field>\n";
      out_stream_ << "          <description>" << err.description
                  << "</description>\n";
      out_stream_ << "        </error>\n";
    }
    out_stream_ << "      </decode_errors>\n";
  } else {
    out_stream_ << "      <tlp>\n";
    out_stream_ << "        <type>" << Utils::tlpTypeToString(tlp.type())
                << "</type>\n";
    out_stream_ << "        <tag>" << tlp.tag() << "</tag>\n";
    if (tlp.address().has_value()) {
      out_stream_ << "        <address>0x" << std::hex << *tlp.address()
                  << std::dec << "</address>\n";
    }
    out_stream_ << "      </tlp>\n";
  }

  if (!validation_errors.empty()) {
    out_stream_ << "      <validation_errors>\n";
    for (const auto &err : validation_errors) {
      out_stream_ << "        <error>\n";
      out_stream_ << "          <rule_id>" << err.rule_id << "</rule_id>\n";
      out_stream_ << "          <category>"
                  << Utils::validationCategoryToStr(err.category)
                  << "</category>\n";
      if (err.related_index.has_value()) {
        out_stream_ << "          <related_index>" << *err.related_index
                    << "</related_index>\n";
      }
      out_stream_ << "          <description>" << err.description
                  << "</description>\n";
      out_stream_ << "        </error>\n";
    }
    out_stream_ << "      </validation_errors>\n";
  }

  out_stream_ << "    </packet>\n";
}

void ReportBuilder::addValidationError(const ValidationError &error) {
  initializeStream();

  if (format_ == ReportFormat::JSON) {
    if (first_error_) {
      out_stream_ << "\n  ],\n"; // Close packets array
      out_stream_ << "  \"validation_errors\": [\n";
      first_error_ = false;
    } else {
      out_stream_ << ",\n";
    }
    writeJsonError(error);
  } else {
    if (first_error_) {
      out_stream_ << "  </packets>\n";
      out_stream_ << "  <validation_errors>\n";
      first_error_ = false;
    }
    writeXmlError(error);
  }
}

void ReportBuilder::writeJsonError(const ValidationError &error) const {
  out_stream_ << "    {";
  out_stream_ << "\"rule_id\":\"" << error.rule_id << "\",";
  out_stream_ << "\"category\":\""
              << Utils::validationCategoryToStr(error.category) << "\",";
  out_stream_ << "\"packet_index\":" << error.packet_index << ",";
  out_stream_ << "\"related_index\":";
  if (error.related_index.has_value())
    out_stream_ << *error.related_index << ",";
  else
    out_stream_ << "null,";
  out_stream_ << "\"description\":\"" << error.description << "\"}";
}

void ReportBuilder::writeXmlError(const ValidationError &error) const {
  out_stream_ << "    <error>\n";
  out_stream_ << "      <rule_id>" << error.rule_id << "</rule_id>\n";
  out_stream_ << "      <category>"
              << Utils::validationCategoryToStr(error.category)
              << "</category>\n";
  out_stream_ << "      <packet_index>" << error.packet_index
              << "</packet_index>\n";
  if (error.related_index.has_value()) {
    out_stream_ << "      <related_index>" << *error.related_index
                << "</related_index>\n";
  }
  out_stream_ << "      <description>" << error.description
              << "</description>\n";
  out_stream_ << "    </error>\n";
}

void ReportBuilder::write(const std::filesystem::path &,
                          std::uint64_t skipped_line_count) const {
  initializeStream();

  if (format_ == ReportFormat::JSON) {
    if (first_error_) {
      out_stream_ << "\n  ],\n"; // Close packets array if no errors were added
      out_stream_ << "  \"validation_errors\": [],\n";
    } else {
      out_stream_ << "\n  ],\n"; // Close validation_errors array
    }

    out_stream_ << "  \"summary\": {\n";
    out_stream_ << "    \"total_packets\": " << total_packets_ << ",\n";
    out_stream_ << "    \"malformed_packet_count\": " << malformed_count_
                << ",\n";
    out_stream_ << "    \"skipped_line_count\": " << skipped_line_count
                << ",\n";
    out_stream_ << "    \"tlp_type_distribution\": {\n";
    bool first_dist = true;
    for (auto const &[type, count] : distribution_) {
      if (!first_dist)
        out_stream_ << ",\n";
      out_stream_ << "      \"" << type << "\": " << count;
      first_dist = false;
    }
    out_stream_ << "\n    }\n";
    out_stream_ << "  }\n";
    out_stream_ << "}\n";
  } else {
    if (first_error_) {
      out_stream_ << "  </packets>\n";
      out_stream_ << "  <validation_errors/>\n";
    } else {
      out_stream_ << "  </validation_errors>\n";
    }

    // Summary
    out_stream_ << "  <summary>\n";
    out_stream_ << "    <total_packets>" << total_packets_
                << "</total_packets>\n";
    out_stream_ << "    <tlp_type_distribution>\n";
    for (auto const &[type, count] : distribution_) {
      out_stream_ << "      <" << type << ">" << count << "</" << type << ">\n";
    }
    out_stream_ << "    </tlp_type_distribution>\n";
    out_stream_ << "    <malformed_packet_count>" << malformed_count_
                << "</malformed_packet_count>\n";
    out_stream_ << "    <skipped_line_count>" << skipped_line_count
                << "</skipped_line_count>\n";
    out_stream_ << "  </summary>\n";
    out_stream_ << "</report>\n";
  }

  out_stream_.close();
}
