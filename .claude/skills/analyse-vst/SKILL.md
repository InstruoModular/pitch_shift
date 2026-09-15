---
name: analyse-vst
description: Run the synthetic test suite through BL-PitchShift.vst3 at chosen parameter settings, measure it, and produce a report and reference/baseline.json (the spec).
---

# Analyse reference VST

1. Ensure host is built (`build` skill). Selftest must pass: `python analysis/selftest.py` (prints PASS/FAIL lines only).
2. Parameter map (once, cached): `python analysis/run_suite.py --target vst --info` -> `reference/vst_info.json`.
   Read only the param names/step texts summary it prints.
3. Run: `python analysis/run_suite.py --target vst --suite quick|full [--sweep "<param>=a,b,c"]`
   -> prints scorecard; writes `results/vst/<run>/{metrics.json,scorecard.txt,report.md,plots/}`.
4. Promote to spec: `python analysis/run_suite.py --promote results/vst/<run>` -> `reference/baseline.json`.
5. Append inferred traits (latency, AM/hop rate, transient behaviour, formant handling, poly behaviour) to
   `.claude/findings.md` in <=10 bullets.

Do not open WAVs. Open a plot only if a scorecard number looks implausible.
