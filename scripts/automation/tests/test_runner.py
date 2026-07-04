import os
import unittest
import subprocess
from unittest.mock import patch, MagicMock

from pcie import Runner, RunResult, AnalyzerNotFoundError, AnalyzerTimeoutError


class TestRunner(unittest.TestCase):
    def setUp(self):
        self.isfile_patcher = patch("os.path.isfile", return_value=True)
        self.access_patcher = patch("os.access", return_value=True)
        self.remove_patcher = patch("os.remove")
        self.makedirs_patcher = patch("os.makedirs")
        self.exists_patcher = patch("os.path.exists", return_value=True)

        self.isfile_patcher.start()
        self.access_patcher.start()
        self.remove_patcher.start()
        self.makedirs_patcher.start()
        self.exists_patcher.start()

        self.runner = Runner("/mock/path/analyzer")

    def tearDown(self):
        self.isfile_patcher.stop()
        self.access_patcher.stop()
        self.remove_patcher.stop()
        self.makedirs_patcher.stop()
        self.exists_patcher.stop()

    @patch("subprocess.run")
    @patch("pcie.parser.ReportParser.parse")
    def test_run_success(self, mock_parse, mock_run):
        # Configure mocks
        mock_run.return_value = MagicMock(returncode=0, stdout="success output", stderr="")
        mock_parse.return_value = MagicMock()

        # Run
        result = self.runner.run("/mock/trace.csv", "/mock/output_dir")

        # Verify
        self.assertTrue(result.success)
        self.assertEqual(result.exit_code, 0)
        self.assertEqual(result.execution_output, "success output")
        self.assertEqual(result.execution_error, "")
        self.assertIsNotNone(result.report)

    @patch("os.path.exists", return_value=False)
    def test_run_analyzer_missing(self, mock_exists):
        with self.assertRaises(AnalyzerNotFoundError):
            self.runner.run("/mock/trace.csv", "/mock/output_dir")

    @patch("subprocess.run")
    def test_run_timeout(self, mock_run):
        mock_run.side_effect = subprocess.TimeoutExpired(cmd="analyzer", timeout=15)

        with self.assertRaises(AnalyzerTimeoutError):
            self.runner.run("/mock/trace.csv", "/mock/output_dir")

    @patch("subprocess.run")
    def test_run_non_zero_exit_code(self, mock_run):
        mock_run.return_value = MagicMock(returncode=1, stdout="", stderr="analyzer internal error")

        result = self.runner.run("/mock/trace.csv", "/mock/output_dir")

        self.assertFalse(result.success)
        self.assertEqual(result.exit_code, 1)
        self.assertEqual(result.execution_error, "analyzer internal error")


if __name__ == "__main__":
    unittest.main()
