This folder contains helper scripts for analyzing PICT output.

## `count-uniques.pl`

`count-uniques.pl` reads PICT output and reports how many **new** value-pairs each test row contributes. It only counts pairs, not higher-order combinations.

Use:

```bash
perl count-uniques.pl output.txt
```

Or directly from PICT:

```bash
pict input.txt | perl count-uniques.pl
```

Options:

- `--no-header` treats the first non-empty row as data instead of skipping it as a header.
- `--reorder` prints the rows reordered by first-seen pair contribution, highest first.

## `compare-uniques.pl`

`compare-uniques.pl` compares two PICT output files and verifies that they contain the same rows, including duplicate row counts.

Use:

```bash
perl compare-uniques.pl output-a.txt output-b.txt
```

Options:

- `--no-header` treats the first non-empty row in each file as data instead of skipping it as a header.

When the files match, the script prints a success message with the total row count. When they differ, it reports rows that appear only in the first file, only in the second file, or with different counts.
