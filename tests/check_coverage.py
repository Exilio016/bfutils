#!/usr/bin/env python3
# Copyright (c) 2026 Bruno Flávio Ferreira <him@brunoflaviof.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

"""Verify line and branch coverage of the bfutils library sources.

Runs `gcov -b --json-format` over the objects produced by `make coverage` and
fails unless every library source file reports 100% line and branch coverage.

A small allowlist of lines is excluded.  Those lines are defensive checks that
the callers make unreachable (a `pos < 0` test after the position was already
proven non-negative); covering them would require running code no public call
path can reach.
"""

from __future__ import annotations

import glob
import gzip
import json
import os
import subprocess
import sys
import tempfile

LIBRARY_SOURCES = (
    "src/bfu_string.c",
    "src/bfu_array.c",
    "src/bfu_map.c",
    "src/bfu_process.c",
)

# Lines that no public call path can reach.  Both branches of the `if` on the
# line, and the body line itself, are skipped.
EXCLUDED_LINES = {
    # `if (pos < 0) { return pos; }` after bfu_map_insert_item_position*()
    # has already returned a non-negative position for validated arguments.
    "src/bfu_map.c": {208, 209, 232, 233},
}

# Branch arcs on otherwise-covered lines that no public call path can reach.
# Only the branch arcs on these lines are skipped; the lines themselves still
# count toward line coverage.
EXCLUDED_BRANCHES = {
    # `needs_to_shift()` wraparound case (`cur >= pos`): the two don't-shift
    # arcs on line 271 require a removed key at the last table slot while an
    # unrelated key hashes to slot 0, which cannot be driven deterministically
    # through the public API without knowing `rapidhash`'s output.
    "src/bfu_map.c": {271},
}


def read_source_lines(source: str) -> dict[int, str]:
    """Return {line_number: source_text} for the given source file."""
    lines: dict[int, str] = {}
    with open(source, "r", encoding="utf-8", errors="replace") as handle:
        for index, line in enumerate(handle, start=1):
            lines[index] = line.rstrip("\n")
    return lines


def gcov_report(
    source: str, build_dir: str
) -> tuple[int, int, int, int, list[int], list[tuple[int, str]]]:
    """Return (covered_lines, total_lines, covered_branches, total_branches,
    uncovered_lines, uncovered_branches)."""
    # Locate the compiled object file for this source under build_dir.  CMake
    # nests objects under CMakeFiles/<target>.dir/ and names them <file>.c.o,
    # while a flat Makefile layout uses <file>.o.  gcov accepts the object file
    # directly and locates the matching .gcda next to it.
    object_name = os.path.basename(source) + ".o"
    object_path = None
    for candidate in glob.glob(os.path.join(build_dir, "**", object_name), recursive=True):
        object_path = candidate
        break
    if object_path is None and source.endswith(".c"):
        flat_name = os.path.basename(source)[:-2] + ".o"
        for candidate in glob.glob(os.path.join(build_dir, "**", flat_name), recursive=True):
            object_path = candidate
            break
    if object_path is None:
        raise SystemExit(f"no object file found for {source} in {build_dir}")

    with tempfile.TemporaryDirectory() as tmp_dir:
        process = subprocess.run(
            ["gcov", "-b", "--json-format", os.path.abspath(object_path)],
            cwd=tmp_dir,
            capture_output=True,
            text=True,
            check=False,
        )
        reports = glob.glob(os.path.join(tmp_dir, "*.gcov.json.gz"))
        if not reports:
            sys.stderr.write(process.stdout)
            sys.stderr.write(process.stderr)
            raise SystemExit(f"gcov produced no report for {source}")

        with gzip.open(reports[0], "rt") as handle:
            data = json.load(handle)

    entry = next(
        (
            item
            for item in data["files"]
            if os.path.normpath(item["file"]) == source
            or os.path.abspath(item["file"]) == os.path.abspath(source)
        ),
        None,
    )
    if entry is None:
        raise SystemExit(f"gcov report has no entry for {source}")

    excluded_lines = EXCLUDED_LINES.get(source, set())
    excluded_branches_lines = EXCLUDED_BRANCHES.get(source, set())
    covered_lines = total_lines = 0
    covered_branches = total_branches = 0
    uncovered_lines: list[int] = []
    uncovered_branches: list[tuple[int, str]] = []

    for line in entry["lines"]:
        line_number = line["line_number"]

        if line_number in excluded_lines:
            continue

        total_lines += 1
        if line["count"] > 0:
            covered_lines += 1
        else:
            uncovered_lines.append(line_number)

        for branch in line.get("branches", []):
            if line_number in excluded_branches_lines:
                continue
            total_branches += 1
            if branch["count"] > 0:
                covered_branches += 1
            else:
                uncovered_branches.append(
                    (line_number, branch.get("kind", "branch"))
                )

    return (
        covered_lines,
        total_lines,
        covered_branches,
        total_branches,
        uncovered_lines,
        uncovered_branches,
    )


def percentage(covered: int, total: int) -> float:
    return 100.0 if total == 0 else 100.0 * covered / total


def main() -> int:
    build_dir = os.environ.get("BUILD_DIR", "build")
    failures = 0

    print("\ncoverage summary")
    print("----------------")
    for source in LIBRARY_SOURCES:
        (
            covered_lines,
            total_lines,
            covered_branches,
            total_branches,
            uncovered_lines,
            uncovered_branches,
        ) = gcov_report(source, build_dir)
        line_percent = percentage(covered_lines, total_lines)
        branch_percent = percentage(covered_branches, total_branches)

        status = "ok" if line_percent >= 100.0 and branch_percent >= 100.0 else "FAIL"
        if status == "FAIL":
            failures += 1
        print(
            f"{status:4} {source:<20} lines {line_percent:6.2f}% "
            f"({total_lines:3} lines)  branches {branch_percent:6.2f}% "
            f"({total_branches:3} branches)"
        )

        source_lines = read_source_lines(source)

        if uncovered_lines:
            print("    uncovered lines:")
            for line_number in uncovered_lines:
                text = source_lines.get(line_number, "<line not found>")
                print(f"        line {line_number:4}: {text}")

        if uncovered_branches:
            print("    uncovered branches:")
            for line_number, kind in uncovered_branches:
                text = source_lines.get(line_number, "<line not found>")
                print(f"        line {line_number:4} ({kind}): {text}")

    excluded_counts = {
        source: len(lines) for source, lines in EXCLUDED_LINES.items() if lines
    }
    for source, count in excluded_counts.items():
        print(f"\nnote: {count} unreachable line(s) excluded in {source}")

    if failures:
        print(f"\n{failures} file(s) below 100% coverage")
        return 1

    print("\nAll library sources have 100% line and branch coverage.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
