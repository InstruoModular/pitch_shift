---
name: analyse-vst
description: Characterise a reference pitch-shift VST3 - discover its parameters, run the synthetic suite through it at chosen settings, and promote the best setting to reference/baseline.json (the spec). Use when adding or re-measuring a reference plugin.
---

# Analyse a reference VST

Default plugin is the old TEST plugin (BL-PitchShift.vst3, param `Factor`). For any other plugin pass
`--plugin <file.vst3> --shift-param <name>` on every call.

1. Host built (`build` skill); `python analysis/selftest.py` must print `selftest: OK`.
2. Parameters: `python analysis/run_suite.py --target vst --plugin <p> --info`
   -> cached in `reference/plugins/<stem>.json`; prints names, step counts and display-text ranges.
   Pick the shift parameter. Its semitone mapping is read from its display texts; if they show cents use
   `--shift-scale 100`. Note latency/quality/mix/formant params for the sweep.
3. Sanity: `--suite quick --shifts=7,12` at defaults. Check pitch_err ~0 c (mapping right) and lat_ms roughly
   equals lat_reported_ms. Set any dry/wet to fully wet via `--params "Mix=1"`.
4. Sweep the quality-affecting params one setting per run, lean
   (e.g. `--suite full --params "Quality=1" --workers 2 --procs 1`; long runs in background, one suite at a time).
   Settings pass normalised values, or display text as `Name=@text`.
5. Promote the chosen setting: `python analysis/run_suite.py --promote results/vst-<stem>/<run> [--setting k]`
   -> reference/baseline.json. Archive a previous baseline to `reference/plugins/<stem>.baseline.json` first.
6. Append inferred traits (latency, AM/frame rate, attack smear, pre-echo, formant handling, poly behaviour,
   worst cases by kind) to `.claude/findings.md`, and re-run the best variant against the new baseline.

Results for plugin <stem> land in `results/vst-<stem>/`. Never open WAVs; open plots only if a number looks off.
