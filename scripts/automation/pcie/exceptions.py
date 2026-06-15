class AnalyzerNotFoundError(FileNotFoundError):
    """Raised when the analyzer engine executable file doesn't exist."""
    pass
class AnalyzerTimeoutError(TimeoutError):
    """Raised when the analyzer process exceeds the specified timeout."""
    pass
class ReportParseError(ValueError):
    """Raised when the output report file cannot be parsed as JSON or XML."""
    pass