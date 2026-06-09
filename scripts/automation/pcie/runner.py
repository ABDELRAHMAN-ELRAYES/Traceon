import os
import shutil
import subprocess
from models import RunResult
from exceptions import AnalyzerNotFoundError, AnalyzerTimeoutError


class Runner:
    def __init__(self, analyzer_path: str):
        path = shutil.which(analyzer_path) or os.path.abspath(analyzer_path)
        if not os.path.isfile(path) or os.access(path, os.X_OK):
            raise AnalyzerNotFoundError(
                f"Analyzer is not found or not permitted to be executed at: {analyzer_path}"
            )
        self.analyzer_path = analyzer_path

    def run(
        self,
        trace_path: str,
        output_path: str,
        format: str = "json",
        timeout_s: int = 15,
    ) -> RunResult:
        # Check if the analyzer exists
        if not os.path.exists(self.analyzer_path):
            raise AnalyzerNotFoundError(
                f"Analyzer executable not found at: {self.analyzer_path}"
            )
        # Create the report directory if not exists
        os.makedirs(output_path, exist_ok=True)

        # Form the final paths structure
        trace_name = os.path.splitext(os.path.basename(trace_path))[0]
        report_path = os.path.abspath(
            os.path.join(output_path, f"report-{trace_name}.{format.lower()}")
        )
        if os.path.exists(report_path):
            os.remove(report_path)

        # Form the execution command
        command = [
            self.analyzer_path,
            "-i",
            trace_path,
            "-o",
            report_path,
            "-f",
            format,
        ]

        try:
            process = subprocess.run(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=timeout_s,
            )
        except subprocess.TimeoutExpired as e:
            raise AnalyzerTimeoutError(
                f"Analyzer timed out executing trace: {trace_path} after {timeout_s} seconds"
            ) from e
        except FileNotFoundError as e:
            raise AnalyzerNotFoundError(
                f"Analyzer executable not found at: {self.analyzer_path}"
            ) from e
        except Exception as e:
            return RunResult(
                success=False,
                exit_code=-1,
                execution_output="",
                execution_error=str(e),
                trace_path=trace_path,
                report_path=report_path,
                error=f"Failed to start subprocess: {e}",
            )

        exit_code = process.returncode
        execution_output = process.stdout
        execution_error = process.stderr

        if exit_code != 0:
            return RunResult(
                success=False,
                exit_code=exit_code,
                execution_output=execution_output,
                execution_error=execution_error,
                trace_path=trace_path,
                report_path=report_path,
                error=f"Analyzer exited with non-zero code {exit_code}",
            )
        if not os.path.exists(report_path):
            return RunResult(
                success=False,
                exit_code=exit_code,
                execution_output=execution_output,
                execution_error=execution_error,
                trace_path=trace_path,
                report_path=report_path,
                error=f"Report file was not generated at: {report_path}",
            )
        return RunResult(
            success=True,
            exit_code=exit_code,
            execution_output=execution_output,
            execution_error=execution_error,
            trace_path=trace_path,
            report_path=report_path,
        )
