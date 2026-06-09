class AnalyzerNotFoundError(FileNotFoundError):
    """Raised when the analyzer engine executable file doesn't exist."""
    pass
class AnalyzerTimeoutError(TimeoutError):
    """Raised when the analyzer process exceeds the specified timeout."""
    pass
