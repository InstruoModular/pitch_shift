# vst_analysis — pitch shifter R&D

Goal: live guitar pitch shifter (`shift.hpp/.cpp`, firmware, 48 kHz, block = `audio_block_size` in `constants.hpp`)
that matches/beats `BL-PitchShift.vst3` on pitch accuracy, clarity, latency. Full plan + progress: `.claude/plan.md`
(read its **Status / Resume here** section first, not the whole plan).

## Layout
- `shift.hpp/.cpp`, `constants.hpp` (48 kHz, block 32), `functions.hpp` — user firmware code (includes via `tools/compat/`)
- `isl/` (Instruo lib, idsp), `JUCE/` — submodules, never edit
- Hosts: `tools/host/` JUCE headless VST3 host (MSVC) · `tools/vst2host/` clean-room VST2 host (MSVC, no SDK; primes plugins that mute after load/param changes)
- `tools/shiftbench/` Shift harness + `shift_capi` DLL target (MSYS2 g++) · `tools/compat/` forwarding headers
- `tools/listen_plugin/` "Shift Listen" one-knob VST3/Standalone for DAW listening tests (see its README)
- Tools: `tools/make_variant.py` clone a variant · `tools/fold_variant.py` freeze a tuned variant into shift.hpp/cpp · `tools/retime.py` noise-resistant CPU
- `analysis/` Python: siggen, metrics, run_suite, report, selftest, `probe.py` (technique probes), `sweep_table.py` (sweep vs reference)
- `suites/`: quick, full, poly, probe · `variants/` alt Shift impls (`lat_causal` = latency-tuned lineage with runtime knobs)
- `results/<target>/<run>/` outputs · `reference/baseline.json` = TARGET: Archetype Misha Mansoor X Transpose (isolated) · `reference/best_shift.json` best Shift run · `reference/plugins/` plugin info + archived BL-PitchShift test baseline
- `.claude/experiments.md` experiment log · `.claude/findings.md` distilled facts · `docs/RESULTS.md` summary for humans

## Skills
`build` · `analyse-vst` · `bench-shift` · `iterate-shift` (see `.claude/skills/`)

## Token rules
- Read scorecards (`scorecard.txt`, ~20 lines), not metrics.json/report.md unless a metric is ambiguous. Never read WAVs.
- Use `--suite quick` for experiments; `full` only to promote a candidate.
- Check `findings.md` + `experiments.md` before trying an idea — don't repeat dead ends.
- Don't re-read isl/JUCE sources; APIs in use are noted in findings.md.
- End each session: update plan.md "Resume here", log experiments, commit.
- Run ONE suite at a time (8 GB RAM). Long sweeps: run in background and wait for the notification.
- PowerShell: don't use `python -c "..."` with double quotes (they get stripped) or `>` for JSON (UTF-16);
  write scratch scripts to the scratchpad instead.
- Never prepend `C:\msys64\ucrt64\bin` to PATH in the same shell that then runs `python`: it resolves to
  MSYS python (no numpy). run_suite adds it only for the shiftbench subprocess.

## Toolchain facts
- MSVC 14.50 (VS 2026) + Ninja for host; `C:\msys64\ucrt64\bin\g++` for shiftbench (`functions.hpp` uses `__builtin_memcpy`).
- Python 3.13 (Windows store) with numpy/scipy/matplotlib; WAV I/O via `scipy.io.wavfile` float32.
