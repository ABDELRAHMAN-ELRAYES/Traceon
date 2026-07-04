import os
import unittest
import tempfile
import json
from unittest.mock import MagicMock

from pcie import (
    Runner,
    RunResult,
    ReportModel,
    Summary,
    Packet,
    BaselineNotFoundError,
    FieldChange,
    RegressionResult,
    SuiteResult,
    RegressionRunner,
)


class TestRegressionUnit(unittest.TestCase):
    def setUp(self):
        # Set up dummy ReportModel
        self.dummy_summary = Summary(
            total_packets=10,
            tlp_type_distribution={"MRd": 5, "MWr": 5},
            malformed_packet_count=0,
            validation_error_count=0,
            skipped_line_count=0
        )
        self.dummy_report = ReportModel(
            schema_version="1.0",
            generated_at="2026-06-30T12:00:00Z",
            trace_file="trace.csv",
            summary=self.dummy_summary,
            packets=[
                Packet(
                    index=0,
                    timestamp_ns=100,
                    direction="TX",
                    is_malformed=False,
                    tlp=None,
                    payload_hex="AA",
                    decode_errors=[],
                    validation_errors=[]
                )
            ],
            validation_errors=[],
            malformed_packets=[]
        )
        self.dummy_result = RunResult(
            success=True,
            exit_code=0,
            execution_output="stdout",
            execution_error="stderr",
            trace_path="trace.csv",
            report_path="report.json",
            error="",
            report=self.dummy_report
        )
        self.runner_mock = MagicMock(spec=Runner)
        self.regression_runner = RegressionRunner(self.runner_mock)

    def test_save_baseline(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            baseline_path = os.path.join(tmpdir, "baseline.json")
            self.regression_runner.save_baseline(self.dummy_result, baseline_path)

            # Verify file was written
            self.assertTrue(os.path.exists(baseline_path))

            # Verify baseline contents
            with open(baseline_path, "r", encoding="utf-8") as f:
                data = json.load(f)

            self.assertEqual(data["schema_version"], "1.0")
            self.assertEqual(data["generated_at"], "2026-01-01T00:00:00Z")
            self.assertEqual(data["summary"]["total_packets"], 10)
            self.assertEqual(data["packets"][0]["payload_hex"], "AA")

    def test_save_baseline_failed_run(self):
        failed_result = RunResult(
            success=False,
            exit_code=1,
            execution_output="",
            execution_error="error detail",
            trace_path="trace.csv",
            report_path="report.json",
            error="Analyzer exited with non-zero code 1",
            report=None
        )
        with tempfile.TemporaryDirectory() as tmpdir:
            baseline_path = os.path.join(tmpdir, "baseline.json")
            with self.assertRaises(ValueError):
                self.regression_runner.save_baseline(failed_result, baseline_path)

    def test_compare_identical(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            baseline_path = os.path.join(tmpdir, "baseline.json")
            self.regression_runner.save_baseline(self.dummy_result, baseline_path)

            current_report = ReportModel(
                schema_version="1.0",
                generated_at="2026-07-04T00:00:00Z",  # Different timestamp
                trace_file="trace.csv",
                summary=self.dummy_summary,
                packets=[
                    Packet(
                        index=0,
                        timestamp_ns=100,
                        direction="TX",
                        is_malformed=False,
                        tlp=None,
                        payload_hex="AA",
                        decode_errors=[],
                        validation_errors=[]
                    )
                ],
                validation_errors=[],
                malformed_packets=[]
            )
            current_result = RunResult(
                success=True,
                exit_code=0,
                execution_output="stdout",
                execution_error="",
                trace_path="trace.csv",
                report_path="report.json",
                error="",
                report=current_report
            )

            res = self.regression_runner.compare(current_result, baseline_path)
            self.assertTrue(res.passed)
            self.assertEqual(len(res.changes), 0)

    def test_compare_mismatch(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            baseline_path = os.path.join(tmpdir, "baseline.json")
            self.regression_runner.save_baseline(self.dummy_result, baseline_path)

            modified_summary = Summary(
                total_packets=12,
                tlp_type_distribution={"MRd": 5, "MWr": 5},
                malformed_packet_count=0,
                validation_error_count=0,
                skipped_line_count=0
            )
            modified_report = ReportModel(
                schema_version="1.0",
                generated_at="2026-06-30T12:00:00Z",
                trace_file="trace.csv",
                summary=modified_summary,
                packets=[
                    Packet(
                        index=0,
                        timestamp_ns=100,
                        direction="TX",
                        is_malformed=False,
                        tlp=None,
                        payload_hex="BB",  # Mismatch
                        decode_errors=[],
                        validation_errors=[]
                    )
                ],
                validation_errors=[],
                malformed_packets=[]
            )
            modified_result = RunResult(
                success=True,
                exit_code=0,
                execution_output="stdout",
                execution_error="",
                trace_path="trace.csv",
                report_path="report.json",
                error="",
                report=modified_report
            )

            res = self.regression_runner.compare(modified_result, baseline_path)
            self.assertFalse(res.passed)

            changes = {c.field: (c.expected, c.actual) for c in res.changes}
            self.assertIn("summary.total_packets", changes)
            self.assertEqual(changes["summary.total_packets"], (10, 12))
            self.assertIn("packets[0].payload_hex", changes)
            self.assertEqual(changes["packets[0].payload_hex"], ("AA", "BB"))

    def test_compare_missing_baseline(self):
        with self.assertRaises(BaselineNotFoundError):
            self.regression_runner.compare(self.dummy_result, "non_existent.json")

    def test_run_suite(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            baseline_path = os.path.join(tmpdir, "baseline.json")
            self.regression_runner.save_baseline(self.dummy_result, baseline_path)

            self.runner_mock.run.return_value = self.dummy_result

            pairs = [("trace.csv", baseline_path)]
            suite_res = self.regression_runner.run_suite(pairs, tmpdir)

            self.assertTrue(suite_res.all_passed)
            self.assertEqual(len(suite_res.results), 1)
            self.assertTrue(suite_res.results[0].passed)
            self.runner_mock.run.assert_called_once_with("trace.csv", tmpdir)


if __name__ == "__main__":
    unittest.main()
