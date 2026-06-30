import os
import json
from dataclasses import dataclass, field, asdict
from typing import List, Tuple, Any
from .models import RunResult, FieldChange, RegressionResult, SuiteResult
from .parser import ReportParser
from .exceptions import (
    BaselineNotFoundError,
    AnalyzerNotFoundError,
    AnalyzerTimeoutError,
)
from .runner import Runner


class RegressionRunner:
    def __init__(self, runner: Runner):
        self.runner = runner

    def save_baseline(self, result: RunResult, baseline_path: str):
        if not result.success:
            raise ValueError(
                f"Cannot save baseline: RunResult indicates execution failure. Error: {result.error}"
            )
        if not result.report:
            raise ValueError("Cannot save baseline: RunResult report is empty/None.")

        # Convert report to dict
        report_dict = asdict(result.report)
        # Set generated_at to canonical placeholder
        report_dict["generated_at"] = "2026-01-01T00:00:00Z"

        # Make sure output directory exists
        os.makedirs(os.path.dirname(os.path.abspath(baseline_path)), exist_ok=True)
        with open(baseline_path, "w", encoding="utf-8") as f:
            json.dump(report_dict, f, indent=2)

    def compare(self, result: RunResult, baseline_path: str) -> RegressionResult:
        if not os.path.exists(baseline_path):
            raise BaselineNotFoundError(f"Baseline file not found at: {baseline_path}")

        if not result.success:
            return RegressionResult(
                passed=False, error=f"Current RunResult failed: {result.error}"
            )
        if not result.report:
            return RegressionResult(
                passed=False, error="Current RunResult report is empty/None."
            )

        try:
            parser = ReportParser()
            baseline_report = parser.parse(baseline_path)
        except Exception as e:
            return RegressionResult(
                passed=False, error=f"Failed to parse baseline report: {e}"
            )

        # Convert both report models to dictionaries
        expected_dict = asdict(baseline_report)
        actual_dict = asdict(result.report)

        # Perform recursive diff
        changes = self._compare_dicts(expected_dict, actual_dict)
        passed = len(changes) == 0
        return RegressionResult(passed=passed, changes=changes)

    def _compare_dicts(
        self, expected: Any, actual: Any, path: str = ""
    ) -> List[FieldChange]:
        changes = []
        if type(expected) != type(actual):
            changes.append(FieldChange(field=path, expected=expected, actual=actual))
            return changes

        if isinstance(expected, dict):
            # Ignore generated_at at the root level
            all_keys = set(expected.keys()) | set(actual.keys())
            for key in sorted(all_keys):
                if key == "generated_at" and path == "":
                    continue
                sub_path = f"{path}.{key}" if path else key
                if key not in expected:
                    changes.append(
                        FieldChange(field=sub_path, expected=None, actual=actual[key])
                    )
                elif key not in actual:
                    changes.append(
                        FieldChange(field=sub_path, expected=expected[key], actual=None)
                    )
                else:
                    changes.extend(
                        self._compare_dicts(expected[key], actual[key], sub_path)
                    )
        elif isinstance(expected, list):
            len_exp = len(expected)
            len_act = len(actual)
            if len_exp != len_act:
                changes.append(
                    FieldChange(
                        field=f"{path}.__len__", expected=len_exp, actual=len_act
                    )
                )
            # Compare elements up to the smaller length
            for i in range(min(len_exp, len_act)):
                sub_path = f"{path}[{i}]"
                changes.extend(self._compare_dicts(expected[i], actual[i], sub_path))
        else:
            if expected != actual:
                changes.append(
                    FieldChange(field=path, expected=expected, actual=actual)
                )

        return changes

    def run_suite(self, pairs: List[Tuple[str, str]], output_dir: str) -> SuiteResult:
        results = []
        all_passed = True

        for idx, (trace_path, baseline_path) in enumerate(pairs):
            try:
                # Run the analyzer on the trace file
                run_res = self.runner.run(trace_path, output_dir)

                # Compare the run result with the baseline
                comp_res = self.compare(run_res, baseline_path)

            except BaselineNotFoundError as e:
                print(
                    f"Suite execution error on pair index {idx} (baseline missing): {e}"
                )
                comp_res = RegressionResult(
                    passed=False, error=f"Baseline not found: {e}"
                )
            except AnalyzerTimeoutError as e:
                print(f"Suite execution error on pair index {idx} (timeout): {e}")
                comp_res = RegressionResult(
                    passed=False, error=f"Analyzer timeout: {e}"
                )
            except AnalyzerNotFoundError as e:
                print(
                    f"Suite execution error on pair index {idx} (analyzer missing): {e}"
                )
                comp_res = RegressionResult(
                    passed=False, error=f"Analyzer not found: {e}"
                )
            except Exception as e:
                print(f"Suite execution error on pair index {idx} (unexpected): {e}")
                comp_res = RegressionResult(
                    passed=False, error=f"Unexpected error: {e}"
                )

            results.append(comp_res)
            if not comp_res.passed:
                all_passed = False

        return SuiteResult(all_passed=all_passed, results=results)
