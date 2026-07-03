import os
import json
from dataclasses import dataclass, asdict
from typing import List

from .models import RunResult


@dataclass
class ConsolidatedReport:
    total_traces: int
    total_packets: int
    total_decode_errors: int
    total_validation_errors: int
    rule_frequency: dict[str, int]
    error_traces: list[str]

    def write(self, path: str):
        os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(asdict(self), f, indent=2)


def aggregate(results: List[RunResult]) -> ConsolidatedReport:
    total_traces = len(results)
    total_packets = 0
    total_decode_errors = 0
    total_validation_errors = 0
    rule_frequency = {}
    error_traces = []

    for result in results:
        if result.success and result.report is not None:
            report = result.report
            total_packets += report.summary.total_packets
            total_decode_errors += report.summary.malformed_packet_count
            total_validation_errors += report.summary.validation_error_count

            # Global validation errors
            for err in report.validation_errors:
                rule_id = err.rule_id
                if rule_id:
                    rule_frequency[rule_id] = rule_frequency.get(rule_id, 0) + 1

            # Packet validation errors
            for packet in report.packets:
                for err in packet.validation_errors:
                    rule_id = err.rule_id
                    if rule_id:
                        rule_frequency[rule_id] = rule_frequency.get(rule_id, 0) + 1

            # Packet decode errors
            for packet in report.packets:
                for err in packet.decode_errors:
                    rule_id = err.rule_id
                    if rule_id:
                        rule_frequency[rule_id] = rule_frequency.get(rule_id, 0) + 1

            has_errors = (
                report.summary.malformed_packet_count > 0 or
                report.summary.validation_error_count > 0
            )
            if has_errors:
                if result.trace_path not in error_traces:
                    error_traces.append(result.trace_path)
        else:
            if result.trace_path not in error_traces:
                error_traces.append(result.trace_path)

    return ConsolidatedReport(
        total_traces=total_traces,
        total_packets=total_packets,
        total_decode_errors=total_decode_errors,
        total_validation_errors=total_validation_errors,
        rule_frequency=rule_frequency,
        error_traces=error_traces,
    )
