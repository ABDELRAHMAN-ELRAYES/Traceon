import os
import unittest
import tempfile
import json

from pcie import (
    RunResult,
    ReportModel,
    Summary,
    Packet,
    ValidationError,
    DecodeError,
    ConsolidatedReport,
    aggregate,
)


class TestAggregator(unittest.TestCase):
    def setUp(self):
        # Set up dummy success results
        self.summary1 = Summary(
            total_packets=10,
            tlp_type_distribution={"MRd": 5, "MWr": 5},
            malformed_packet_count=0,
            validation_error_count=1,
            skipped_line_count=0
        )
        self.report1 = ReportModel(
            schema_version="1.0",
            generated_at="2026-06-30T12:00:00Z",
            trace_file="trace1.csv",
            summary=self.summary1,
            packets=[
                Packet(
                    index=0,
                    timestamp_ns=100,
                    direction="TX",
                    is_malformed=False,
                    tlp=None,
                    payload_hex="AA",
                    decode_errors=[],
                    validation_errors=[
                        ValidationError(rule_id="VAL-001", category="CAT1", description="desc1", packet_index=0)
                    ]
                )
            ],
            validation_errors=[],
            malformed_packets=[]
        )
        self.result1 = RunResult(
            success=True, exit_code=0, execution_output="", execution_error="",
            trace_path="trace1.csv", report_path="report1.json", error="", report=self.report1
        )

        self.summary2 = Summary(
            total_packets=20,
            tlp_type_distribution={"CplD": 10, "MWr": 10},
            malformed_packet_count=2,
            validation_error_count=2,
            skipped_line_count=1
        )
        self.report2 = ReportModel(
            schema_version="1.0",
            generated_at="2026-06-30T13:00:00Z",
            trace_file="trace2.csv",
            summary=self.summary2,
            packets=[
                Packet(
                    index=0,
                    timestamp_ns=200,
                    direction="RX",
                    is_malformed=True,
                    tlp=None,
                    payload_hex="BB",
                    decode_errors=[
                        DecodeError(rule_id="DEC-002", field="fmt", description="desc2")
                    ],
                    validation_errors=[]
                )
            ],
            validation_errors=[
                ValidationError(rule_id="VAL-001", category="CAT1", description="desc1", packet_index=0)
            ],
            malformed_packets=[]
        )
        self.result2 = RunResult(
            success=True, exit_code=0, execution_output="", execution_error="",
            trace_path="trace2.csv", report_path="report2.json", error="", report=self.report2
        )

        # Set up failed result
        self.failed_result = RunResult(
            success=False, exit_code=1, execution_output="", execution_error="crash",
            trace_path="failed_trace.csv", report_path="report_fail.json", error="Execution error", report=None
        )

    def test_aggregate_empty(self):
        report = aggregate([])
        self.assertEqual(report.total_traces, 0)
        self.assertEqual(report.total_packets, 0)
        self.assertEqual(report.total_decode_errors, 0)
        self.assertEqual(report.total_validation_errors, 0)
        self.assertEqual(len(report.rule_frequency), 0)
        self.assertEqual(len(report.error_traces), 0)

    def test_aggregate_success_only(self):
        report = aggregate([self.result1, self.result2])

        self.assertEqual(report.total_traces, 2)
        self.assertEqual(report.total_packets, 30)
        self.assertEqual(report.total_decode_errors, 2)
        self.assertEqual(report.total_validation_errors, 3)

        self.assertEqual(report.rule_frequency.get("VAL-001"), 2)
        self.assertEqual(report.rule_frequency.get("DEC-002"), 1)

        self.assertEqual(len(report.error_traces), 2)
        self.assertIn("trace1.csv", report.error_traces)
        self.assertIn("trace2.csv", report.error_traces)

    def test_aggregate_mixed(self):
        report = aggregate([self.result1, self.failed_result])

        self.assertEqual(report.total_traces, 2)
        self.assertEqual(report.total_packets, 10)
        self.assertEqual(report.total_decode_errors, 0)
        self.assertEqual(report.total_validation_errors, 1)
        self.assertEqual(report.rule_frequency.get("VAL-001"), 1)

        self.assertEqual(len(report.error_traces), 2)
        self.assertIn("trace1.csv", report.error_traces)
        self.assertIn("failed_trace.csv", report.error_traces)

    def test_consolidated_report_write(self):
        report = aggregate([self.result1])
        with tempfile.TemporaryDirectory() as tmpdir:
            out_path = os.path.join(tmpdir, "consolidated.json")
            report.write(out_path)

            self.assertTrue(os.path.exists(out_path))
            with open(out_path, "r", encoding="utf-8") as f:
                data = json.load(f)

            self.assertEqual(data["total_traces"], 1)
            self.assertEqual(data["total_packets"], 10)
            self.assertEqual(data["total_validation_errors"], 1)
            self.assertEqual(data["rule_frequency"]["VAL-001"], 1)
            self.assertEqual(data["error_traces"], ["trace1.csv"])


if __name__ == "__main__":
    unittest.main()
