import unittest
from pcie import ReportAsserter, RunResult, ReportModel, Summary, Packet, ValidationError


class TestAsserter(unittest.TestCase):
    def setUp(self):
        # Create dummy report structures
        self.summary = Summary(
            total_packets=5,
            tlp_type_distribution={"MRd": 2, "MWr": 3},
            malformed_packet_count=1,
            validation_error_count=2,
            skipped_line_count=0
        )
        self.packet = Packet(
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
        self.report = ReportModel(
            schema_version="1.0",
            generated_at="2026-06-30T12:00:00Z",
            trace_file="trace.csv",
            summary=self.summary,
            packets=[self.packet],
            validation_errors=[
                ValidationError(rule_id="VAL-002", category="CAT2", description="desc2", packet_index=1)
            ],
            malformed_packets=[]
        )
        self.result = RunResult(
            success=True,
            exit_code=0,
            execution_output="stdout",
            execution_error="stderr",
            trace_path="trace.csv",
            report_path="report.json",
            error="",
            report=self.report
        )
        self.asserter = ReportAsserter(self.result)

    def test_assert_total_packets(self):
        # Pass
        self.asserter.assert_total_packets(5)
        # Fail
        with self.assertRaises(AssertionError) as context:
            self.asserter.assert_total_packets(10)
        self.assertIn("Expected: 10, Actual: 5", str(context.exception))

    def test_assert_decode_error_count(self):
        # Pass
        self.asserter.assert_decode_error_count(1)
        # Fail
        with self.assertRaises(AssertionError) as context:
            self.asserter.assert_decode_error_count(2)
        self.assertIn("Expected: 2, Actual: 1", str(context.exception))

    def test_assert_validation_error_count(self):
        # Pass
        self.asserter.assert_validation_error_count(2)
        # Fail
        with self.assertRaises(AssertionError) as context:
            self.asserter.assert_validation_error_count(0)
        self.assertIn("Expected: 0, Actual: 2", str(context.exception))

    def test_assert_rule_present(self):
        # Pass
        self.asserter.assert_rule_present("VAL-001")
        self.asserter.assert_rule_present("VAL-002")
        # Fail
        with self.assertRaises(AssertionError) as context:
            self.asserter.assert_rule_present("VAL-999")
        self.assertIn("Expected validation rule_id 'VAL-999' to be present", str(context.exception))

    def test_assert_rule_absent(self):
        # Pass
        self.asserter.assert_rule_absent("VAL-999")
        # Fail
        with self.assertRaises(AssertionError) as context:
            self.asserter.assert_rule_absent("VAL-001")
        self.assertIn("Expected validation rule_id 'VAL-001' to be absent", str(context.exception))

    def test_assert_category_count(self):
        # Pass
        self.asserter.assert_category_count("CAT1", 1)
        self.asserter.assert_category_count("CAT2", 1)
        # Fail
        with self.assertRaises(AssertionError) as context:
            self.asserter.assert_category_count("CAT1", 5)
        self.assertIn("Expected category 'CAT1' count to be 5, but found 1", str(context.exception))


if __name__ == "__main__":
    unittest.main()
