---
name: iterate-shift
description: Run one or more pitch-shifter design experiments against the spec - hypothesis, implement variant, bench, log, commit. Use when improving shift.cpp towards reference/baseline.json.
---

# Iterate on Shift

Per experiment (keep each small and single-purpose):
1. Read the last scorecard (`results/shift/<latest>/scorecard.txt`), `.claude/findings.md`, and the tail of
   `.claude/experiments.md`. Pick the largest gap that isn't a known dead end.
2. Append a row to `experiments.md`: `| E<n> | date | variant | hypothesis | change | | |`.
3. `python tools/make_variant.py <name> [--from current|<best_variant>]` clones into `variants/<name>.hpp/.cpp`
   (class `Shift_<name>`, self-registering), then change ONE thing and `cmake --build build/bench`.
   Same API as `Shift`: `Controls{shift_amount}`, `reset()`, `process(const MonoDspBuffer&, MonoDspBuffer&)`.
   For parameter sweeps, expose constants through `set_param` in the adapter instead of cloning per value.
4. `bench-shift` with `--suite quick --compare results/shift/<best-run>`.
5. Fill in the row: key deltas (<=3 numbers) + verdict (keep / drop / promising).
   Put any generalisable lesson in `findings.md` (one bullet).
6. `git add -A && git commit -m "E<n>: <summary>"`.
7. If it beats current best on quick without regressions beyond tolerance or CPU budget: run `--suite full`,
   then `python analysis/run_suite.py --promote-shift results/shift/<run>` (updates `reference/best_shift.json`).

Candidate directions (plan.md Stage 6): tune splicer -> multi-band splice -> low-latency phase-locked vocoder
-> hybrid by YIN confidence -> formant handling (only if reference preserves formants).

At session end update `.claude/plan.md` "Resume here".
