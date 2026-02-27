#!/usr/bin/env python3

import argparse
import re
import sys
from collections import Counter


def split_row(line: str) -> list[str]:
    if "\t" in line:
        return [cell.strip() for cell in line.split("\t")]
    return re.split(r"\s+", line.strip())


def read_rows(path: str, no_header: bool) -> Counter[tuple[str, ...]]:
    rows: Counter[tuple[str, ...]] = Counter()
    first_non_empty_seen = False

    with open(path, "r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line:
                continue
            if not no_header and not first_non_empty_seen:
                first_non_empty_seen = True
                continue
            first_non_empty_seen = True
            rows[tuple(split_row(line))] += 1
    return rows


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Compare two PICT output files and verify all rows are accounted for."
    )
    parser.add_argument("file_a", help="First PICT output file")
    parser.add_argument("file_b", help="Second PICT output file")
    parser.add_argument(
        "--no-header",
        action="store_true",
        help="Treat first non-empty row as data (do not skip header)",
    )
    args = parser.parse_args()

    rows_a = read_rows(args.file_a, args.no_header)
    rows_b = read_rows(args.file_b, args.no_header)

    if rows_a == rows_b:
        print(f"MATCH: all rows accounted for ({sum(rows_a.values())} rows).")
        return

    print("MISMATCH: files differ.")
    only_a = rows_a - rows_b
    only_b = rows_b - rows_a

    if only_a:
        print("Rows only in first file (or extra count):")
        for row, count in only_a.items():
            print(f"{'\t'.join(row)}\t# [{count}]")

    if only_b:
        print("Rows only in second file (or extra count):")
        for row, count in only_b.items():
            print(f"{'\t'.join(row)}\t# [{count}]")

    sys.exit(1)


if __name__ == "__main__":
    main()
