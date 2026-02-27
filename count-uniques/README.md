`count-uniques.py` reads PICT output and reports how many **new** value-pairs each test row contributes. Note: it only counts pairs and not higher order combinations.

Use: `python3 count-uniques.py output.txt` 
Or directly from PICT: `pict input.txt | python3 count-uniques.py`

Add `--reorder` to print rows reordered by unique value contribution.
