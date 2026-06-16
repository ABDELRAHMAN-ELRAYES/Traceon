import os
import json
from .exceptions import ReportParseError
from .models import (
    ReportModel,
    Summary,
    Packet,
    ValidationError,
    DecodeError,
    TLPInfo,
    TLPAttributes,
    MalformedPacket,
)
import xml.etree.ElementTree as ET


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
            return self._parse_xml(report_path)
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

    def _parse_xml(self, report_path: str) -> ReportModel:
        try:
            with open(report_path, "rb") as f:
                raw_data = f.read()
            # Decode using utf-8 with replacement
            xml_text = raw_data.decode("utf-8", errors="replace")
            # Strip illegal XML control characters except tabs, carriage returns, and newlines
            sanitized_xml_text = "".join(
                c for c in xml_text if c in "\t\n\r" or ord(c) >= 0x20
            )
            root = ET.fromstring(sanitized_xml_text)
        except Exception as e:
            raise ReportParseError(f"Failed to parse XML file: {e}") from e

        if root.tag != "report":
            raise ReportParseError("XML root must be <report>")

        schema_version = root.findtext("schema_version", "")
        generated_at = root.findtext("generated_at", "")
        trace_file = root.findtext("trace_file", "")

        # Summary
        summary = None
        summary_node = root.find("summary")
        if summary_node is not None:
            total_packets = int(summary_node.findtext("total_packets", "0"))
            malformed_packet_count = int(
                summary_node.findtext("malformed_packet_count", "0")
            )
            skipped_line_count = int(summary_node.findtext("skipped_line_count", "0"))

            val_error_cnt_text = summary_node.findtext("validation_error_count")
            validation_error_count = (
                int(val_error_cnt_text) if val_error_cnt_text else 0
            )

            tlp_dist = {}
            dist_node = summary_node.find("tlp_type_distribution")
            if dist_node is not None:
                for child in dist_node:
                    tlp_dist[child.tag] = int(child.text or "0")

            summary = Summary(
                total_packets=total_packets,
                tlp_type_distribution=tlp_dist,
                malformed_packet_count=malformed_packet_count,
                validation_error_count=validation_error_count,
                skipped_line_count=skipped_line_count,
            )
        else:
            summary = Summary(0, {}, 0, 0, 0)

        # Validation Errors (Global)
        global_validation_errors = []
        ve_list_node = root.find("validation_errors")
        if ve_list_node is not None:
            for err_node in ve_list_node.findall("error"):
                global_validation_errors.append(
                    self._parse_xml_validation_error(err_node)
                )

        # Packets
        packets = []
        malformed_packets = []
        packets_node = root.find("packets")
        if packets_node is not None:
            for pkt_node in packets_node.findall("packet"):
                packet = self._parse_xml_packet(pkt_node)
                packets.append(packet)
                if packet.is_malformed:
                    malformed_packets.append(
                        MalformedPacket(
                            packet_number=packet.index,
                            raw_data=packet.payload_hex or "",
                            decode_errors=packet.decode_errors,
                        )
                    )

        if summary.validation_error_count == 0:
            total_val_errs = len(global_validation_errors) + sum(
                len(p.validation_errors) for p in packets
            )
            summary.validation_error_count = total_val_errs

        return ReportModel(
            schema_version=schema_version,
            generated_at=generated_at,
            trace_file=trace_file,
            summary=summary,
            packets=packets,
            validation_errors=global_validation_errors,
            malformed_packets=malformed_packets,
        )

    def _parse_xml_validation_error(
        self, err_node: ET.Element, default_pkt_index: int = 0
    ) -> ValidationError:
        rule_id = err_node.findtext("rule_id", "")
        category = err_node.findtext("category", "")
        description = err_node.findtext("description", "")
        pkt_idx_text = err_node.findtext("packet_index")
        packet_index = int(pkt_idx_text) if pkt_idx_text else default_pkt_index

        rel_idx_text = err_node.findtext("related_index")
        related_index = int(rel_idx_text) if rel_idx_text else None

        return ValidationError(
            rule_id=rule_id,
            category=category,
            description=description,
            packet_index=packet_index,
            related_index=related_index,
        )

    def _parse_xml_packet(self, pkt_node: ET.Element) -> Packet:
        index = int(pkt_node.findtext("index", "0"))
        timestamp_ns = int(pkt_node.findtext("timestamp_ns", "0"))
        direction = pkt_node.findtext("direction", "")
        is_malformed = pkt_node.findtext("is_malformed", "false").lower() == "true"

        tlp_info = None
        payload_hex = None
        decode_errors = []

        if is_malformed:
            payload_hex = pkt_node.findtext("payload_hex", "")
            de_list_node = pkt_node.find("decode_errors")
            if de_list_node is not None:
                for err_node in de_list_node.findall("error"):
                    decode_errors.append(
                        DecodeError(
                            rule_id=err_node.findtext("rule_id", ""),
                            field=err_node.findtext("field", ""),
                            description=err_node.findtext("description", ""),
                        )
                    )
        else:
            tlp_node = pkt_node.find("tlp")
            if tlp_node is not None:
                attr_node = tlp_node.find("attr")
                if attr_node is not None:
                    attr = TLPAttributes(
                        no_snoop=attr_node.findtext("no_snoop", "false").lower()
                        == "true",
                        relaxed_ordering=attr_node.findtext(
                            "relaxed_ordering", "false"
                        ).lower()
                        == "true",
                    )
                else:
                    attr = TLPAttributes(no_snoop=False, relaxed_ordering=False)

                status_text = tlp_node.findtext("status")
                status_val = int(status_text) if status_text else None

                length_text = tlp_node.findtext("length_dw")
                length_val = int(length_text) if length_text else None

                byte_count_text = tlp_node.findtext("byte_count")
                byte_count_val = int(byte_count_text) if byte_count_text else None

                tag_text = "".join(
                    c for c in tlp_node.findtext("tag", "") if c.isdigit()
                )
                tag_val = int(tag_text) if tag_text else 0

                tlp_info = TLPInfo(
                    type=tlp_node.findtext("type", ""),
                    header_fmt=tlp_node.findtext("header_fmt", ""),
                    tc=int(tlp_node.findtext("tc", "0")),
                    attr=attr,
                    requester_id=tlp_node.findtext("requester_id", ""),
                    completer_id=tlp_node.findtext("completer_id"),
                    tag=tag_val,
                    address=tlp_node.findtext("address"),
                    length_dw=length_val,
                    has_data=tlp_node.findtext("has_data", "false").lower() == "true",
                    byte_count=byte_count_val,
                    status=status_val,
                )

        pkt_validation_errors = []
        ve_list_node = pkt_node.find("validation_errors")
        if ve_list_node is not None:
            for err_node in ve_list_node.findall("error"):
                pkt_validation_errors.append(
                    self._parse_xml_validation_error(err_node, default_pkt_index=index)
                )

        return Packet(
            index=index,
            timestamp_ns=timestamp_ns,
            direction=direction,
            is_malformed=is_malformed,
            tlp=tlp_info,
            payload_hex=payload_hex,
            decode_errors=decode_errors,
            validation_errors=pkt_validation_errors,
        )
