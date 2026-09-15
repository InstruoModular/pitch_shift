---
name: bench-shift
description: Build and run the test suite on a Shift variant (shift.cpp or variants/*) via shiftbench, and print a scorecard compared with reference/baseline.json.
---

# Bench a Shift variant

1. `cmake --build build/bench` (see `build` skill for first configure). Fix compile errors before anything else.
2. `python analysis/run_suite.py --target shift:<variant> --suite quick --compare reference/baseline.json`
   - `<variant>`: `current` = root shift.cpp, else name registered in `tools/shiftbench/variants.cpp`.
   - Optional `--compare results/shift/<run>` to diff against a previous candidate instead of the reference.
3. Read only the printed scorecard (metric | ref | cand | delta | pass). CPU line = ns/sample and worst-block %.
4. `--gate` exits non-zero if any metric regresses beyond tolerance vs `reference/best_shift.json`.
