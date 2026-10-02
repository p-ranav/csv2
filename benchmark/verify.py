#!/usr/bin/env python3
"""Cross-check that the csv2 benchmark binary (./main) reports correct
rows/cells for a given CSV file, by independently parsing the same file in
Python.

Uses Python's built-in `csv` module (implemented in C as `_csv`, not a pure
Python loop) as the independent reference parser. This is only a valid
comparison because csv2's RowIterator is quote-aware (a real embedded
newline inside a quoted field does not end the row) and CellIterator already
respects quoting for delimiter splitting -- i.e. csv2's parsing semantics are
now close enough to standard quoted-CSV parsing for this to be meaningful.
A tqdm progress bar tracks bytes read at the binary level (text-mode
file.tell() is unusable once iterated via the csv module's next() calls).
"""
import csv
import io
import os
import re
import subprocess
import sys

from tqdm import tqdm

csv.field_size_limit(sys.maxsize)  # some quoted fields (e.g. descriptions) are huge


class _ProgressBuffer:
    """Thin proxy over a binary buffer that reports bytes read to a tqdm bar.
    Implements both read() and read1() since TextIOWrapper may call either."""

    def __init__(self, raw, progress):
        self._raw = raw
        self._progress = progress

    def _read(self, method, size):
        data = method(size)
        self._progress.update(len(data))
        return data

    def read(self, size=-1):
        return self._read(self._raw.read, size)

    def read1(self, size=-1):
        method = getattr(self._raw, "read1", self._raw.read)
        return self._read(method, size)

    def readable(self):
        return True

    def writable(self):
        return False

    def seekable(self):
        return False

    def close(self):
        self._raw.close()

    @property
    def closed(self):
        return self._raw.closed


def expected_counts(path, delimiter):
    """Rows/cells as reported by Python's C-accelerated csv module, which
    natively handles quoted fields spanning multiple physical lines."""
    total_size = os.path.getsize(path)
    rows = 0
    cells = 0
    with open(path, "rb") as raw, \
         tqdm(total=total_size, unit="B", unit_scale=True, desc="Parsing (python)") as progress:
        text = io.TextIOWrapper(_ProgressBuffer(raw, progress), encoding="utf-8",
                                 errors="replace", newline="")
        reader = csv.reader(text, delimiter=delimiter)
        for row in reader:
            rows += 1
            cells += len(row)
    return rows, cells



def actual_counts(binary, csv_file, delimiter):
    args = [binary, csv_file, "tab" if delimiter == "\t" else delimiter]
    result = subprocess.run(args, capture_output=True, text=True, check=True)
    output = result.stdout
    rows_match = re.search(r"Rows:\s*(\d+)", output)
    cells_match = re.search(r"Cells:\s*(\d+)", output)
    if not rows_match or not cells_match:
        print(f"error: could not parse Rows/Cells from output:\n{output}", file=sys.stderr)
        sys.exit(2)
    return int(rows_match.group(1)), int(cells_match.group(1))


def parse_delimiter(arg):
    if arg in ("tab", "\\t"):
        return "\t"
    if len(arg) == 1:
        return arg
    print(f"error: delimiter must be a single character (or 'tab'), got: {arg!r}", file=sys.stderr)
    sys.exit(2)


def main():
    if len(sys.argv) not in (2, 3):
        print(f"Usage: {sys.argv[0]} <csv_file> [delimiter]", file=sys.stderr)
        sys.exit(2)

    csv_file = sys.argv[1]
    delimiter = parse_delimiter(sys.argv[2]) if len(sys.argv) == 3 else ","
    binary = "./main"

    actual_rows, actual_cells = actual_counts(binary, csv_file, delimiter)
    expected_row_count, expected_cell_count = expected_counts(csv_file, delimiter)

    # Average cells/row as a float: integer division would misleadingly show
    # 0 when rows include many blank lines (avg < 1 cell per row).
    actual_cols = actual_cells / actual_rows if actual_rows else 0.0
    expected_cols = expected_cell_count / expected_row_count if expected_row_count else 0.0

    print(f"File:            {csv_file}")
    print(f"csv2   -> rows={actual_rows:<12,} cols={actual_cols:<8.2f} cells={actual_cells:,}")
    print(f"python -> rows={expected_row_count:<12,} cols={expected_cols:<8.2f} cells={expected_cell_count:,}")

    ok = actual_rows == expected_row_count and actual_cells == expected_cell_count
    print("PASS" if ok else "FAIL")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
