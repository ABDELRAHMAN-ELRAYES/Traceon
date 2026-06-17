from typing import List
from .models import RunResult, ValidationError


class ReportAsserter:
    def __init__(self, result: RunResult):
        self.result = result
        self.trace_path = result.trace_path
        if not result.success:
            raise AssertionError(
                f"Cannot construct ReportAsserter: RunResult indicates execution failure. "
                f"Exit code: {result.exit_code}, error: {result.error}"
            )
        if not result.report:
            raise AssertionError(
                f"Cannot construct ReportAsserter: RunResult report is empty/None."
            )
        self.report = result.report

    def _get_all_validation_errors(self) -> List[ValidationError]:
        errors = list(self.report.validation_errors)
        for pkt in self.report.packets:
            errors.extend(pkt.validation_errors)
        return errors

    def assert_total_packets(self, expected: int) -> "ReportAsserter":
        actual = self.report.summary.total_packets
        if actual != expected:
            raise AssertionError(
                f"Assertion assert_total_packets FAILED for trace: {self.trace_path}. "
                f"Expected: {expected}, Actual: {actual}"
            )
        return self

    def assert_decode_error_count(self, expected: int) -> "ReportAsserter":
        actual = self.report.summary.malformed_packet_count
        if actual != expected:
            raise AssertionError(
                f"Assertion assert_decode_error_count FAILED for trace: {self.trace_path}. "
                f"Expected: {expected}, Actual: {actual}"
            )
        return self

    def assert_validation_error_count(self, expected: int) -> "ReportAsserter":
        actual = self.report.summary.validation_error_count
        if actual != expected:
            raise AssertionError(
                f"Assertion assert_validation_error_count FAILED for trace: {self.trace_path}. "
                f"Expected: {expected}, Actual: {actual}"
            )
        return self

    def assert_rule_present(self, rule_id: str) -> "ReportAsserter":
        all_errors = self._get_all_validation_errors()
        present = any(err.rule_id == rule_id for err in all_errors)
        if not present:
            raise AssertionError(
                f"Assertion assert_rule_present FAILED for trace: {self.trace_path}. "
                f"Expected validation rule_id '{rule_id}' to be present, but it was not found."
            )
        return self

    def assert_rule_absent(self, rule_id: str) -> "ReportAsserter":
        all_errors = self._get_all_validation_errors()
        present = any(err.rule_id == rule_id for err in all_errors)
        if present:
            raise AssertionError(
                f"Assertion assert_rule_absent FAILED for trace: {self.trace_path}. "
                f"Expected validation rule_id '{rule_id}' to be absent, but it was found."
            )
        return self

    def assert_category_count(self, category: str, expected: int) -> "ReportAsserter":
        all_errors = self._get_all_validation_errors()
        actual = sum(1 for err in all_errors if err.category == category)
        if actual != expected:
            raise AssertionError(
                f"Assertion assert_category_count FAILED for trace: {self.trace_path}. "
                f"Expected category '{category}' count to be {expected}, but found {actual}."
            )
        return self


def assert_report(result: RunResult) -> ReportAsserter:
    if not result.success:
        raise AssertionError(
            f"RunResult indicates execution failure. Exit code: {result.exit_code}, error: {result.error}"
        )
    return ReportAsserter(result)