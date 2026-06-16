import sys
import os
import argparse
from pcie.runner import Runner


def main():
    parser = argparse.ArgumentParser(
        description="Traceon PCIe Automation Runner CLI",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "-a",
        "--analyzer",
        required=True,
        help="Path to the Traceon C++ analyzer executable (build/src/Traceon)",
    )
    parser.add_argument(
        "-i", "--input", required=True, help="Path to the input trace CSV file"
    )
    parser.add_argument(
        "-o",
        "--output",
        required=True,
        help="Directory where generated reports will be stored",
    )
    parser.add_argument(
        "-f",
        "--format",
        default="json",
        choices=["json", "xml"],
        help="Output report format",
    )
    parser.add_argument(
        "-t", "--timeout", type=int, default=15, help="Process timeout in seconds"
    )

    args = parser.parse_args()

    analyzer_path = os.path.abspath(args.analyzer)
    trace_path = os.path.abspath(args.input)
    output_dir = os.path.abspath(args.output)

    print(f"Initializing Runner with analyzer: {analyzer_path}")
    try:
        runner = Runner(analyzer_path)
    except Exception as e:
        print(f"Initialization Error: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"Running analysis on trace: {trace_path}")
    result = runner.run(
        trace_path=trace_path,
        output_path=output_dir,
        format=args.format,
        timeout_s=args.timeout,
    )

    if result.success:
        print("\nAnalysis and report parsing completed successfully!")
        print(f"Report written to: {result.report_path}")

        report = result.report
        if report:
            print(f"Schema Version: {report.schema_version}")
            print(f"Generated At:   {report.generated_at}")
            print(f"Trace File:     {report.trace_file}")
            print("\nSummary:")
            print(f"  - Total Packets:      {report.summary.total_packets}")
            print(f"  - Malformed Packets:  {report.summary.malformed_packet_count}")
            print(f"  - Validation Errors:  {report.summary.validation_error_count}")
            print(f"  - Skipped Lines:      {report.summary.skipped_line_count}")
            print("\nTLP Type Distribution:")
            for tlp_type, count in report.summary.tlp_type_distribution.items():
                print(f"    * {tlp_type}: {count}")
    else:
        print("\n✗ Analysis run failed!", file=sys.stderr)
        print(f"Error: {result.error}", file=sys.stderr)
        if result.execution_output:
            print(f"\nStdout:\n{result.execution_output}", file=sys.stderr)
        if result.execution_error:
            print(f"\nStderr:\n{result.execution_error}", file=sys.stderr)
        sys.exit(result.exit_code if result.exit_code != 0 else 1)


if __name__ == "__main__":
    main()
