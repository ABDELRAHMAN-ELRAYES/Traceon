from __future__ import annotations
from typing import Dict, List, Optional
from dataclasses import dataclass, field


@dataclass
class RunResult:
    success: bool
    exit_code: int
    execution_output: str
    execution_error: str
    trace_path: str
    report_path: str
    error: str = ""
    report: Optional[ReportModel] = None


@dataclass
class DecodeError:
    rule_id: str
    field: str
    description: str


@dataclass
class ValidationError:
    rule_id: str
    category: str
    description: str
    packet_index: int
    related_index: Optional[int] = None


@dataclass
class TLPAttributes:
    no_snoop: bool
    relaxed_ordering: bool


@dataclass
class TLPInfo:
    type: str
    header_fmt: str
    tc: int
    attr: TLPAttributes
    requester_id: str
    completer_id: Optional[str] = None
    tag: int = 0
    address: Optional[str] = None
    length_dw: Optional[int] = None
    has_data: bool = False
    byte_count: Optional[int] = None
    status: Optional[int] = None


@dataclass
class Packet:
    index: int
    timestamp_ns: int
    direction: str
    is_malformed: bool
    tlp: Optional[TLPInfo] = None
    payload_hex: Optional[str] = None
    decode_errors: List[DecodeError] = field(default_factory=list)
    validation_errors: List[ValidationError] = field(default_factory=list)


@dataclass
class MalformedPacket:
    packet_number: int  # maps to index
    raw_data: str  # maps to payload_hex
    decode_errors: List[DecodeError] = field(default_factory=list)


@dataclass
class Summary:
    total_packets: int
    tlp_type_distribution: Dict[str, int]
    malformed_packet_count: int
    validation_error_count: int
    skipped_line_count: int


@dataclass
class ReportModel:
    schema_version: str
    generated_at: str
    trace_file: str
    summary: Summary
    packets: List[Packet] = field(default_factory=list)
    validation_errors: List[ValidationError] = field(default_factory=list)
    malformed_packets: List[MalformedPacket] = field(default_factory=list)
