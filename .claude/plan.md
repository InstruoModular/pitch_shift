# Pitch Shifter Analysis & Design — Plan

## Status / Resume here
**Resume here:** Stage 6 — current best variant `down_margin` (E2, see experiments.md). Remaining fails vs
baseline: poly_pitch_err (11.9 vs 1.4 c), sinad_db (64 vs 81), sinad_poly_db (26.7 vs 27.9). Next: pick E3
from the per-case gap breakdown (findings.md). Firmware watch: cpu_worst_block_pct ~75 % on desktop. Optional later (needs free RAM): TransBoost 0.5/1 at Q1, one setting per run,
`--workers 2 --procs 1`, and re-promote if it beats TB=0 on flam_db/attack_smear.

- [x] Stage 0 — scaffolding (.claude, compat headers, gitignore, git init)
- [x] Stage 1 — headless VST3 host (`vsthost info|run`, 40 jobs ~1 s; params in findings.md)
- [x] Stage 2 — signal generator + suites (quick 10 sig x 4 shifts, full 40 x 8)
- [x] Stage 3 — metrics + selftest (selftest OK; run_suite + report.py work)
- [x] Stage 4 — reference characterisation -> reference/baseline.json (Q sweep; TransBoost sweep deferred: OOM)
- [x] Stage 5 — shiftbench harness + Shift baseline (E0 in experiments.md)
- [ ] Stage 6 — iteration loop (see .claude/experiments.md)
- [ ] Stage 7 — finalise

## Context
Goal: a live-guitar pitch shifter (mono input, polyphonic material) that matches or beats a reference VST3 (`BL-PitchShift.vst3`, a BlueLab iPlug2 build) on **pitch accuracy**, **clarity (few artefacts)** and **latency**, and still runs in real time on the firmware (`shift.hpp/.cpp`, 48 kHz, fixed small blocks, `Shift::Controls::shift_amount` −12..+12 semitones).

Approach: build an automated measurement loop. It generates synthetic test signals, runs them through a processor (the VST, or any `Shift` variant), measures the output, and writes a compact scorecard plus a full report. The VST report becomes the spec. Claude then iterates on `Shift` against that spec. The `.claude/` folder holds the plan, skills, an experiment log and the harness commands, so any later session can resume cheaply.

Facts established so far:
- **Toolchains:**
  - MSVC 14.50 (VS 2026), CMake and Ninja are installed.
  - MSYS2 `g++` is installed at `C:\msys64\ucrt64\bin`.
  - Python 3.13 has numpy 2.2, scipy 1.15 and matplotlib 3.10.
- **Submodules:** `JUCE` (9.0.2) and `isl` (Instruo lib: `idsp/filter.hpp` with `BiquadFilter<BiquadType>`, `idsp/buffer_types.hpp`, …).
- **Header names:** `shift.hpp/.cpp` include `engine/constant.hpp`, `instrument/functions.hpp` and `instrument/shift.hpp`. The files actually sit at the root as `constants.hpp`, `functions.hpp` and `shift.hpp`. A **compat include dir with forwarding headers** fixes this without editing user files.
- **Compiler split:** `functions.hpp` uses `__builtin_memcpy`, which is GCC/Clang only. So `Shift` builds with **MSYS2 g++**, which is also closest to the arm-gcc firmware. The JUCE host builds with **MSVC**. They are two separate build trees.
- ⚠ **Block size mismatch:** `constants.hpp` has `audio_block_size = 16`, but the user said 32. The harness uses whatever `constants.hpp` says. Confirm in Stage 5 and fix the header if 32 is correct.
- **Plugin parameters:** the plugin's parameter names can't be read from the binary. The host lists them at runtime (Stage 1).

## Target layout
```
vst_analysis/
  CMakeLists.txt                # top-level: add_subdirectory(tools/host) only (MSVC tree)
  .claude/
    CLAUDE.md                   # short project brief + where things are + the 5 commands that matter
    plan.md                     # this plan, with a stage checklist + "resume here" pointer
    settings.json               # allowlist: cmake, ninja, python analysis/*, build/**/*.exe
    experiments.md              # one row per experiment: id, hypothesis, change, scorecard delta, verdict
    findings.md                 # distilled facts about the reference plugin + what worked or failed
    skills/
      build/SKILL.md            # configure/build both trees; common errors + fixes
      analyse-vst/SKILL.md      # run suite on VST at given settings -> report + baseline.json
      bench-shift/SKILL.md      # build + run suite on a Shift variant -> scorecard vs baseline
      iterate-shift/SKILL.md    # experiment protocol (below); keeps logs updated
  tools/
    host/        vsthost.cpp CMakeLists.txt     # JUCE console app (MSVC)
    shiftbench/  main.cpp CMakeLists.txt        # links shift.cpp / variants (g++)
    compat/engine/constant.hpp                  # #include "../../../constants.hpp"
    compat/instrument/functions.hpp             # forwards to ../../../functions.hpp
    compat/instrument/shift.hpp                 # forwards to ../../../shift.hpp
  analysis/
    siggen.py     metrics.py     run_suite.py     report.py     selftest.py
  suites/  quick.json  full.json  poly.json
  variants/                     # alternate Shift implementations, same API
  results/<target>/<run-id>/    # metrics.json, scorecard.txt, report.md, plots/*.png (wavs gitignored)
  reference/baseline.json       # the spec, from Stage 4
```

**Common processor CLI.** `vsthost.exe` and `shiftbench.exe` accept the same arguments, so Python never special-cases a processor:
```
<exe> info                                   -> JSON: params (name, range, step texts), reported latency
<exe> run --manifest jobs.json --sr 48000 --block N
   jobs.json: [{in, out, params:{name:value}}...]   # batch = one plugin load for the whole suite
```

## Token-efficiency rules (written into CLAUDE.md and the skills)
- Scripts print only a **scorecard**: about 20 lines, one per metric, with reference value, candidate value, delta and pass/fail. Full detail goes to files that are read only when needed.
- WAVs and plots are never read into context. Plots are opened only when a metric is ambiguous.
- `run_suite.py --suite quick` (about 30 s) is used for experiments. `full` is used only when a candidate is promoted.
- Each experiment adds one row to `experiments.md`. Distilled lessons go to `findings.md`, so dead ends aren't retried.
- `plan.md` keeps a checklist and a "Resume here:" line that is updated at the end of every session.
- After Stage 0, set up `git init` (the submodules already exist). Each experiment gets its own commit, so variants can be diffed and reverted instead of re-read.

---

## Stage 0 — Scaffolding
1. Create `.claude/` with `CLAUDE.md`, `plan.md` (a copy of this plan), `settings.json`, the four skills, and empty `experiments.md` / `findings.md`.
2. Run `git init`, add `.gitignore` (`build/`, `results/**/*.wav`), and commit.
3. `analysis/requirements.txt`: numpy, scipy, matplotlib. Use `scipy.io.wavfile` for float32 WAVs, so no new dependencies are needed.

## Stage 1 — Headless VST3 host (JUCE, MSVC)
- `juce_add_console_app(vsthost)` linking `juce_audio_processors` and `juce_audio_formats`. Set `JUCE_PLUGINHOST_VST3=1`, no GUI modules beyond what hosting needs. Create a `ScopedJuceInitialiser_GUI` so the message manager exists.
- Loading: `VST3PluginFormat::findAllTypesForFile("BL-PitchShift.vst3")`, then `createInstance(desc, 48000, block)`, then `setPlayConfigDetails(1 or 2 in, out)`. Handle a stereo-only plugin by duplicating mono in and taking the left channel out.
- `info`: for every parameter, record name, default, `getNumSteps`, and `getText` at each step (or 101 samples of the range). This maps "semitones" and any mode, quality or latency option to normalized values. Also record `getLatencySamples()`.
- `run`: `prepareToPlay` → for each job: `reset()`, set params via `setValueNotifyingHost`, feed `block`-sized buffers, then append `latency + 1 s` of silence as tail. Write float32 WAV output and `job.meta.json` (reported latency, block, params).
- **Verify:** `info` dumps parameters. At shift = 0, the output lags the input by exactly the reported latency. For an octave-up sine, the peak lands at 2f.

## Stage 2 — Test signal generator (`siggen.py`, suites as JSON)
All signals are deterministic (seeded), at 48 kHz, 24-bit-equivalent level −12 dBFS. Each has an ideal shifted counterpart generated analytically from the same parameters, with frequencies × 2^(s/12).

| Family | Purpose |
|---|---|
| Sines, E2 (82 Hz) … E6 (1319 Hz), ~6 notes | pure pitch accuracy, AM/warble |
| Harmonic tones (sawtooth-like, 1/n partials, band-limited) | realistic period structure |
| Karplus-Strong plucks (strings E2..E4) | pick transients, decay, flams, pre-echo |
| Dyads: fifth, fourth, octave, minor 2nd, major 3rd | polyphonic intermodulation, common-period locking |
| Chords: E5 power, open E major, A minor, Cmaj7 (KS plucked, strummed with 15 ms stagger) | full polyphony |
| Log sine sweep 80 Hz–5 kHz; vibrato (5 Hz, ±30 c); bend (+200 c over 300 ms) | tracking under pitch motion |
| Tone bursts (10 ms rise, gated) and clicks | latency and onset timing |
| Decays to −60 dB; silence; white/pink noise bursts | low-level behaviour, noise, stability |
| Staccato repeated notes at 8/16th @ 140 bpm | onset handling under rapid restarts |

**Shifts:** −12, −7, −5, −1, +1, +5, +7, +12. The quick suite is a subset: 3 shifts × about 10 signals.

## Stage 3 — Metrics (`metrics.py`) + self-validation (`selftest.py`)
**Alignment first:** estimate latency by cross-correlating the **envelope** of the output against the envelope of the ideal (burst/pluck signals). For tonal signals, cross-correlate the output against the analytic ideal-shifted signal. Report both the reported latency and the measured latency, per note.

**Metric groups.** Each produces scalars; lower is better unless noted.
1. **Pitch accuracy:**
   - Steady-state f0 error in cents: mean, p95 and max, from a zero-padded FFT peak with parabolic interpolation over 200 ms windows.
   - Pitch jitter: std of instantaneous frequency in cents, from Hilbert phase on the band-isolated fundamental.
   - Tracking error on vibrato, bends and sweeps against the expected curve.
   - For dyads and chords: per-note partial frequency error.
2. **Clarity:**
   - **Shift SINAD:** energy at the expected partial set (±3 bins) vs everything else, in dB (higher is better).
   - **Warble / AM:** p-p dB of the 5 ms RMS envelope over steady state, plus the dominant modulation frequency. This reveals the grain or hop rate.
   - **Inharmonic / intermod energy** for polyphonic material. **Aliasing:** energy above the expected band edge.
   - **Log-spectral distance** vs the ideal shifted signal, on a 2048 FFT and averaged. This checks timbre and formant preservation, reported both formant-scaled and unscaled.
   - **Transients:**
     - pre-echo energy before onset (dB);
     - count of secondary onsets within 60 ms (flams);
     - 10–90 % attack-time ratio vs the ideal.
   - **Onset stability:** click or discontinuity detector (high-pass residual peak vs local RMS).
3. **Latency:** reported samples/ms, measured median and max ms per note frequency and per shift.
4. **Robustness:** output level error (dB), noise-floor and silence output, NaN/denormal check.
5. **CPU** (shiftbench only): mean ns/sample and worst block time as a fraction of the block period, measured on the desktop and used only for relative comparisons.

**Self-test (must pass before trusting any report).** Run the metrics on:
- (a) the ideal shifted signals: perfect scores;
- (b) ideal signals with known damage injected: +7 cents detune, 3 dB AM at 23 Hz, −40 dB noise, a 12 ms delay, a duplicated attack 30 ms later, a single-sample discontinuity.

Each metric must recover the injected amount within tolerance. `selftest.py` exits non-zero otherwise.

## Stage 4 — Reference characterisation → spec
- `run_suite.py --target vst --suite full --sweep <params>` runs every mode, quality or latency setting that `info` exposed.
- **Outputs:**
  - `results/vst/<run>/report.md`, with tables and plots: spectrograms for key cases, cents-vs-time, latency-vs-note, and a warble spectrum.
  - `reference/baseline.json`, holding metric values per (signal, shift, setting), plus a recommended "spec setting" (best clarity at acceptable latency) and one at the lowest latency.
- Record inferred algorithm traits in `findings.md`: vocoder vs time-domain (from latency, AM rate and transient smear), FFT/hop size, formant handling, and polyphonic behaviour.

## Stage 5 — Shift test harness (shiftbench, g++)
- `tools/shiftbench/CMakeLists.txt` uses the MSYS2 g++ toolchain file, C++20, `-O2`, and include dirs `tools/compat`, repo root and `isl/include`. It compiles `shift.cpp` plus a **variant registry**: `variants/*.cpp`, each a class with the same `Controls` / `reset()` / `process(MonoDspBuffer, MonoDspBuffer)` API. They are selected by `--variant name`, so A/B runs happen in one binary.
- It uses the same `info`/`run` CLI and reports `latency` as 0, because Shift's latency is measured, not declared. It also emits CPU timings.
- Confirm block size: 16 or 32 in `constants.hpp`. Run the quick suite at both, if the variants allow it.
- **Baseline:** `run_suite.py --target shift:current --suite full --compare reference/baseline.json` gives the first gap analysis in `report.md`, including the per-metric scorecard.

## Stage 6 — Iteration loop (`iterate-shift` skill)
**Protocol per experiment:**
1. Pick the worst-gap metric from the last scorecard.
2. Write a one-line hypothesis in `experiments.md`.
3. Implement it in `variants/` (or by tuning constants).
4. `bench-shift --suite quick`.
5. Log the delta and verdict, then commit.
6. Promote to the full suite when it beats the current best overall without regressing any metric beyond tolerance or blowing the CPU budget.

**Candidate directions, roughly in order:**
1. **Tune the current pitch-synchronous splicer:**
   - grain minimum;
   - crossfade shape and length;
   - YIN threshold and range;
   - onset detector parameters;
   - multi-candidate splice scoring for chords (common period).
2. **Multi-band splice:** low band with long, period-synced grains; high band with short grains. This cuts warble on chords while keeping low latency.
3. **Low-latency phase vocoder:**
   - identity/scaled phase locking (Laroche–Dolson);
   - small FFT (512–1024) with a hop of 1/4–1/8;
   - transient detection with phase reset.

   It is the likely polyphonic-quality winner, at a latency cost. Compare it against the reference's latency.
4. **Hybrid:** switch or crossfade between the time-domain splicer (tracked mono and transients) and the vocoder (poly or untracked), driven by YIN confidence.
5. **Formant or envelope options,** only if the reference preserves formants.

**Stop condition:** every spec metric on the full suite is ≥ the reference, or the trade-off is documented. The winner is then folded back into `shift.hpp/.cpp` with the same API.

## Stage 7 — Finalise
- Promote the winning variant into `shift.cpp`.
- Freeze `suites/quick.json` as a regression gate (`bench-shift --gate`).
- Write the final comparison report and update `findings.md`.
- Note the firmware-side CPU estimate: desktop ns/sample × an assumed MCU factor. The real check happens on the target.

## Verification (end to end)
1. `cmake --build build/host && build/host/vsthost.exe info BL-PitchShift.vst3` produces a parameter JSON with the semitone parameter and the latency.
2. `python analysis/selftest.py` passes all injected-damage checks.
3. `python analysis/run_suite.py --target vst --suite quick` prints a scorecard and writes a report with plots.
4. `python analysis/run_suite.py --target shift:current --suite quick --compare reference/baseline.json` prints a side-by-side scorecard.
5. A fresh session that reads only `.claude/CLAUDE.md` and `.claude/plan.md` can find "Resume here" and run the next step without re-exploring.
