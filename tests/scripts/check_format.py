#!/usr/bin/env python3
"""Parallel clang-format checker (portable replacement for check_format.sh).

Runs `clang-format -style=file:<config>` on each file and diffs the output
against the file on disk. No bash / process-substitution dependency, so it works
identically on MSYS2, Windows and Linux. Exit code is non-zero if any file
differs from its formatted form.
"""
import argparse
import concurrent.futures
import difflib
import os
import shutil
import subprocess
import sys


def check_file(file_path, fmt_cmd, config_file):
    try:
        with open(file_path, "r", encoding="utf-8", newline="") as fh:
            original = fh.read()
    except OSError as exc:
        return (False, file_path, f"could not read file: {exc}")

    try:
        result = subprocess.run(
            [fmt_cmd, f"-style=file:{config_file}", file_path],
            capture_output=True,
            text=True,
        )
    except OSError as exc:
        return (False, file_path, f"failed to run {fmt_cmd}: {exc}")

    if result.returncode != 0:
        return (False, file_path, result.stderr.strip())

    formatted = result.stdout
    if formatted == original:
        return (True, file_path, "")

    diff = difflib.unified_diff(
        original.splitlines(keepends=True),
        formatted.splitlines(keepends=True),
        fromfile=file_path,
        tofile=f"{file_path} (formatted)",
    )
    return (False, file_path, "".join(diff))


def main():
    parser = argparse.ArgumentParser(description="Parallel clang-format checker")
    parser.add_argument("--format-exec", default="clang-format",
                        help="Path to clang-format executable")
    parser.add_argument("--config", required=True,
                        help="Path to .clang-format config")
    parser.add_argument("--files", nargs="+", required=True,
                        help="List of files to check")
    args = parser.parse_args()

    if not shutil.which(args.format_exec) and not os.path.isfile(args.format_exec):
        print(f"Error: {args.format_exec} not found.")
        sys.exit(1)

    if not os.path.isfile(args.config):
        print(f"Error: config file '{args.config}' not found.")
        sys.exit(1)

    files = [f for f in args.files if os.path.isfile(f)]
    print(f"Checking format of {len(files)} files with config: {args.config}")

    failed = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
        futures = {
            executor.submit(check_file, f, args.format_exec, args.config): f
            for f in files
        }
        for future in concurrent.futures.as_completed(futures):
            ok, filename, output = future.result()
            if not ok:
                print(f"\nFormat violation: {filename}")
                if output:
                    print(output)
                failed.append(filename)

    if failed:
        print(f"\nFAILED: {len(failed)} file(s) not formatted correctly.")
        sys.exit(1)
    print("\nSUCCESS: All files formatted correctly.")
    sys.exit(0)


if __name__ == "__main__":
    main()
