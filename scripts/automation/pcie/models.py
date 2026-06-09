from dataclasses import dataclass


@dataclass
class RunResult:
    success: bool
    exit_code: int
    execution_output: str
    execution_error: str
    trace_path: str
    report_path: str
    error: str = ""
