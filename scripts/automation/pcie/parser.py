import os
import json
from exceptions import ReportParseError
from models import (
    ReportModel,
    Summary,
    Packet,
    ValidationError,
    DecodeError,
    TLPInfo,
    TLPAttributes,
    MalformedPacket,
)


class ReportParser:

    def __init__(self):
        pass

    def parse(self, report_path: str) -> ReportModel:
        if not os.path.exists(report_path):
            raise FileNotFoundError(f"Report file not found at: {report_path}")

        _, ext = os.path.splitext(report_path)
        ext = ext.lower()

        if ext == ".json":
            return self._parse_json(report_path)
        elif ext == ".xml":
            return {}
        else:
            raise ReportParseError(f"Unsupported report file extension: {ext}")

    def _parse_json(self, report_path: str) -> ReportModel:
        try:
            with open(report_path, "r", encoding="utf-8") as f:
                data = json.load(f)
        except Exception as e:
            raise ReportParseError(f"Failed to parse JSON file: {e}") from e

        if not isinstance(data, dict):
            raise ReportParseError("JSON root must be an object")

        schema_version = data.get("schema_version", "")
        generated_at = data.get("generated_at", "")
        trace_file = data.get("trace_file", "")

        # Summary
        summary_data = data.get("summary", {})
        tlp_dist = summary_data.get("tlp_type_distribution", {})
        summary = Summary(
            total_packets=int(summary_data.get("total_packets", 0)),
            tlp_type_distribution={k: int(v) for k, v in tlp_dist.items()},
            malformed_packet_count=int(summary_data.get("malformed_packet_count", 0)),
            validation_error_count=int(summary_data.get("validation_error_count", 0)),
            skipped_line_count=int(summary_data.get("skipped_line_count", 0)),
        )

        # Validation Errors
        global_validation_errors = []
        for ve_data in data.get("validation_errors", []):
            global_validation_errors.append(self._parse_json_validation_error(ve_data))

        # Packets
        packets = []
        malformed_packets = []
        for pkt_data in data.get("packets", []):
            packet = self._parse_json_packet(pkt_data)
            packets.append(packet)
            if packet.is_malformed:
                malformed_packets.append(
                    MalformedPacket(
                        packet_number=packet.index,
                        raw_data=packet.payload_hex or "",
                        decode_errors=packet.decode_errors,
                    )
                )

        return ReportModel(
            schema_version=schema_version,
            generated_at=generated_at,
            trace_file=trace_file,
            summary=summary,
            packets=packets,
            validation_errors=global_validation_errors,
            malformed_packets=malformed_packets,
        )

    def _parse_json_validation_error(self, ve_data: dict) -> ValidationError:
        rel_idx = ve_data.get("related_index")
        if rel_idx is not None:
            rel_idx = int(rel_idx)
        return ValidationError(
            rule_id=ve_data.get("rule_id", ""),
            category=ve_data.get("category", ""),
            description=ve_data.get("description", ""),
            packet_index=int(ve_data.get("packet_index", 0)),
            related_index=rel_idx,
        )

    def _parse_json_packet(self, pkt_data: dict) -> Packet:
        is_malformed = bool(pkt_data.get("is_malformed", False))
        tlp_info = None

        if not is_malformed and "tlp" in pkt_data:
            tlp_data = pkt_data["tlp"]
            attr_data = tlp_data.get("attr", {})
            attr = TLPAttributes(
                no_snoop=bool(attr_data.get("no_snoop", False)),
                relaxed_ordering=bool(attr_data.get("relaxed_ordering", False)),
            )

            status_val = tlp_data.get("status")
            if status_val is not None:
                status_val = int(status_val)

            length_val = tlp_data.get("length_dw")
            if length_val is not None:
                length_val = int(length_val)

            byte_count_val = tlp_data.get("byte_count")
            if byte_count_val is not None:
                byte_count_val = int(byte_count_val)

            tag_val = tlp_data.get("tag", 0)
            if tag_val is not None:
                tag_val = int(tag_val)

            tlp_info = TLPInfo(
                type=tlp_data.get("type", ""),
                header_fmt=tlp_data.get("header_fmt", ""),
                tc=int(tlp_data.get("tc", 0)),
                attr=attr,
                requester_id=tlp_data.get("requester_id", ""),
                completer_id=tlp_data.get("completer_id"),
                tag=tag_val,
                address=tlp_data.get("address"),
                length_dw=length_val,
                has_data=bool(tlp_data.get("has_data", False)),
                byte_count=byte_count_val,
                status=status_val,
            )

        decode_errors = []
        for de_data in pkt_data.get("decode_errors", []):
            decode_errors.append(
                DecodeError(
                    rule_id=de_data.get("rule_id", ""),
                    field=de_data.get("field", ""),
                    description=de_data.get("description", ""),
                )
            )

        pkt_validation_errors = []
        for ve_data in pkt_data.get("validation_errors", []):
            rel_idx = ve_data.get("related_index")
            if rel_idx is not None:
                rel_idx = int(rel_idx)
            pkt_validation_errors.append(
                ValidationError(
                    rule_id=ve_data.get("rule_id", ""),
                    category=ve_data.get("category", ""),
                    description=ve_data.get("description", ""),
                    packet_index=int(pkt_data.get("index", 0)),
                    related_index=rel_idx,
                )
            )

        return Packet(
            index=int(pkt_data.get("index", 0)),
            timestamp_ns=int(pkt_data.get("timestamp_ns", 0)),
            direction=pkt_data.get("direction", ""),
            is_malformed=is_malformed,
            tlp=tlp_info,
            payload_hex=pkt_data.get("payload_hex"),
            decode_errors=decode_errors,
            validation_errors=pkt_validation_errors,
        )
