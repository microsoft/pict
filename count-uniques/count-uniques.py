#!/usr/bin/env python3

import argparse
import re
import sys
from itertools import combinations


def split_row(line: str) -> list[str]:
    if "\t" in line:
        return [cell.strip() for cell in line.split("\t")]
    return re.split(r"\s+", line.strip())


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Count first-seen value pairs per test-case row."
    )
    parser.add_argument(
        "input_file",
        nargs="?",
        default="-",
        help="Path to PICT output file (or - / omitted for stdin)",
    )
    parser.add_argument(
        "--no-header",
        action="store_true",
        help="Treat the first non-empty row as data (do not skip header)",
    )
    parser.add_argument(
        "--reorder",
        action="store_true",
        help="Output test rows reordered by first-seen pair count (descending)",
    )
    args = parser.parse_args()

    seen_pairs: set[tuple[tuple[int, str], tuple[int, str]]] = set()
    first_data_row_seen = False
    header_line = ""
    scored_rows: list[tuple[int, str]] = []

    handle = (
        sys.stdin
        if args.input_file == "-"
        else open(args.input_file, "r", encoding="utf-8")
    )
    with handle:
        for raw_line in handle:
            raw = raw_line.rstrip("\n")
            line = raw.strip()
            if not line:
                continue

            if not args.no_header and not first_data_row_seen:
                header_line = raw
                first_data_row_seen = True
                continue

            first_data_row_seen = True
            values = split_row(line)
            new_pairs = 0

            for left, right in combinations(range(len(values)), 2):
                pair = ((left, values[left]), (right, values[right]))
                if pair not in seen_pairs:
                    seen_pairs.add(pair)
                    new_pairs += 1

            if args.reorder:
                scored_rows.append((new_pairs, raw))
            else:
                print(f"{raw}\t# [{new_pairs}]")

    if args.reorder:
        if header_line:
            print(header_line)
        for _, row in sorted(scored_rows, key=lambda item: item[0], reverse=True):
            print(row)


if __name__ == "__main__":
    main()
