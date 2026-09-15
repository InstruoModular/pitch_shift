---
name: iterate-shift
description: Run one or more pitch-shifter design experiments against the spec - hypothesis, implement variant, bench, log, commit. Use when improving shift.cpp towards reference/baseline.json.
---

# Iterate on Shift

Per experiment (keep each small and single-purpose):
1. Read the last scorecard (`results/shift/<latest>/scorecard.txt`), `.claude/findings.md`, and the tail of
   `.claude/experiments.md`. Pick the largest gap that isn't a known dead end.
2. Append a row to `experiments.md`: `| E<n> | date | variant | hypothesis | change | | |`.
3. Implement in `variants/<name>.cpp/.hpp` (same API as `Shift`: `Controls{shift_amount}`, `reset()`,
   `process(const MonoDspBuffer&, MonoDspBuffer&)`), register it, rebuild. Prefer copying the current best
   and changing one thing; tuning constants can be done via a variant subclass/param struct.
4. `bench-shift` with `--suite quick --compare results/shift/<best-run>`.
5. Fill in the row: key deltas (<=3 numbers) + verdict (keep / drop / promising).
   Put any generalisable lesson in `findings.md` (one bullet).
6. `git add -A && git commit -m "E<n>: <summary>"`.
7. If it beats current best on quick without regressions beyond tolerance or CPU budget: run `--suite full`,
   then `python analysis/run_suite.py --promote-shift results/shift/<run>` (updates `reference/best_shift.json`).

Candidate directions (plan.md Stage 6): tune splicer -> multi-band splice -> low-latency phase-locked vocoder
-> hybrid by YIN confidence -> formant handling (only if reference preserves formants).

At session end update `.claude/plan.md` "Resume here".
