import os
import unittest
import shutil
import tempfile
from pcie import Runner, RunResult, ReportAsserter


class TestIntegrationRunner(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.analyzer_path = os.path.abspath("./build/src/Traceon")
        cls.trace_path = os.path.abspath("./data/trace_data.csv")
        cls.has_binary = os.path.isfile(cls.analyzer_path) and os.access(cls.analyzer_path, os.X_OK)

    def test_single_run_integration(self):
        if not self.has_binary:
            self.skipTest(f"Analyzer binary not found at {self.analyzer_path}")

        with tempfile.TemporaryDirectory() as tmpdir:
            runner = Runner(self.analyzer_path)
            result = runner.run(self.trace_path, tmpdir)

            # Verify result fields
            self.assertTrue(result.success, f"Runner failed: {result.error}")
            self.assertEqual(result.exit_code, 0)
            self.assertIsNotNone(result.report)

            # Run assertions
            asserter = ReportAsserter(result)
            asserter.assert_total_packets(30)
            asserter.assert_decode_error_count(6)
            asserter.assert_validation_error_count(0)

    def test_batch_run_integration(self):
        if not self.has_binary:
            self.skipTest(f"Analyzer binary not found at {self.analyzer_path}")

        with tempfile.TemporaryDirectory() as tmpdir:
            # Create a directory for traces
            traces_dir = os.path.join(tmpdir, "traces")
            reports_dir = os.path.join(tmpdir, "reports")
            os.makedirs(traces_dir)
            os.makedirs(reports_dir)

            trace_paths = []
            for i in range(10):
                dest = os.path.join(traces_dir, f"trace_{i}.csv")
                shutil.copy(self.trace_path, dest)
                trace_paths.append(dest)

            # Run batch execution
            runner = Runner(self.analyzer_path)
            results = runner.run_batch(trace_paths, reports_dir)

            # Verify results
            self.assertEqual(len(results), 10)
            for i, result in enumerate(results):
                self.assertTrue(result.success, f"Batch item {i} failed: {result.error}")
                self.assertEqual(result.exit_code, 0)
                expected_report_name = f"report-trace_{i}.json"
                self.assertTrue(result.report_path.endswith(expected_report_name))
                self.assertTrue(os.path.exists(result.report_path))


if __name__ == "__main__":
    unittest.main()
