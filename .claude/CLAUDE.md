# vst_analysis — pitch shifter R&D

Goal: live guitar pitch shifter (`shift.hpp/.cpp`, firmware, 48 kHz, block = `audio_block_size` in `constants.hpp`)
that matches/beats `BL-PitchShift.vst3` on pitch accuracy, clarity, latency. Full plan + progress: `.claude/plan.md`
(read its **Status / Resume here** section first, not the whole plan).

## Layout
- `shift.hpp/.cpp`, `constants.hpp`, `functions.hpp` — user firmware code (includes resolved via `tools/compat/`)
- `isl/` (Instruo lib, idsp), `JUCE/` — submodules, never edit
- `tools/host/` JUCE headless VST3 host (MSVC) · `tools/shiftbench/` Shift harness (MSYS2 g++) · `tools/compat/` forwarding headers
- `analysis/` Python: siggen, metrics, run_suite, report, selftest · `suites/*.json` · `variants/` alt Shift impls
- `results/<target>/<run>/` outputs · `reference/baseline.json` the spec
- `.claude/experiments.md` experiment log · `.claude/findings.md` distilled facts

## Skills
`build` · `analyse-vst` · `bench-shift` · `iterate-shift` (see `.claude/skills/`)

## Token rules
- Read scorecards (`scorecard.txt`, ~20 lines), not metrics.json/report.md unless a metric is ambiguous. Never read WAVs.
- Use `--suite quick` for experiments; `full` only to promote a candidate.
- Check `findings.md` + `experiments.md` before trying an idea — don't repeat dead ends.
- Don't re-read isl/JUCE sources; APIs in use are noted in findings.md.
- End each session: update plan.md "Resume here", log experiments, commit.

## Toolchain facts
- MSVC 14.50 (VS 2026) + Ninja for host; `C:\msys64\ucrt64\bin\g++` for shiftbench (`functions.hpp` uses `__builtin_memcpy`).
- Python 3.13 (Windows store) with numpy/scipy/matplotlib; WAV I/O via `scipy.io.wavfile` float32.
