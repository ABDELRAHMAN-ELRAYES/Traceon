import os
import json
import unittest
import tempfile
from pcie import Runner, RegressionRunner, RegressionResult, RunResult


class TestIntegrationRegression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.analyzer_path = os.path.abspath("./build/src/Traceon")
        cls.trace_path = os.path.abspath("./data/trace_data.csv")
        cls.has_binary = os.path.isfile(cls.analyzer_path) and os.access(cls.analyzer_path, os.X_OK)

    def test_regression_suite_pass_fail(self):
        if not self.has_binary:
            self.skipTest(f"Analyzer binary not found at {self.analyzer_path}")

        with tempfile.TemporaryDirectory() as tmpdir:
            runner = Runner(self.analyzer_path)
            regression_runner = RegressionRunner(runner)

            # Generate run result
            run_res = runner.run(self.trace_path, tmpdir)
            self.assertTrue(run_res.success, f"Subprocess run failed: {run_res.error}")

            # Save baseline
            baseline_path = os.path.join(tmpdir, "baseline.json")
            regression_runner.save_baseline(run_res, baseline_path)
            self.assertTrue(os.path.exists(baseline_path))

            # Test Regression Pass (Compare run against itself)
            comp_res = regression_runner.compare(run_res, baseline_path)
            self.assertTrue(comp_res.passed, f"Self regression check failed: {comp_res.changes}")
            self.assertEqual(len(comp_res.changes), 0)

            # Test Suite Pass
            pairs = [(self.trace_path, baseline_path)]
            suite_res = regression_runner.run_suite(pairs, tmpdir)
            self.assertTrue(suite_res.all_passed)
            self.assertEqual(len(suite_res.results), 1)
            self.assertTrue(suite_res.results[0].passed)

            # Test Regression Mismatch Fail
            # Modify baseline file deliberately
            with open(baseline_path, "r", encoding="utf-8") as f:
                baseline_data = json.load(f)

            baseline_data["summary"]["total_packets"] += 100  # Modify packet count
            with open(baseline_path, "w", encoding="utf-8") as f:
                json.dump(baseline_data, f)

            # Compare modified baseline
            comp_res_fail = regression_runner.compare(run_res, baseline_path)
            self.assertFalse(comp_res_fail.passed)
            self.assertGreater(len(comp_res_fail.changes), 0)

            changes_dict = {c.field: (c.expected, c.actual) for c in comp_res_fail.changes}
            self.assertIn("summary.total_packets", changes_dict)
            expected_val = baseline_data["summary"]["total_packets"]
            actual_val = run_res.report.summary.total_packets
            self.assertEqual(changes_dict["summary.total_packets"], (expected_val, actual_val))

            # Test Suite Fail
            suite_res_fail = regression_runner.run_suite(pairs, tmpdir)
            self.assertFalse(suite_res_fail.all_passed)
            self.assertFalse(suite_res_fail.results[0].passed)


if __name__ == "__main__":
    unittest.main()
