# Findings

## Build
- `functions.hpp` uses `__builtin_memcpy` -> Shift must build with g++/clang (MSYS2 ucrt64), not MSVC.
- shift.cpp includes `engine/constant.hpp`, `instrument/functions.hpp`, `instrument/shift.hpp`; forwarded by `tools/compat/`.
- `constants.hpp`: `audio_block_size = 16` (user mentioned 32 — confirm), 48 kHz, `MonoDspBuffer = idsp::SampleBufferStatic<16>`.
- isl `idsp::BiquadFilter<BiquadType::X>`: `set_parameters(norm_f, Q)`, `process(x)`, `reset()`.

## Reference plugin (BL-PitchShift.vst3)
- Single-file VST3 DLL, BlueLab / iPlug2 (OpenGL UI). Mono in/out accepted. Full dump: `reference/vst_info.json`.
- Params (normalised): `Factor` semitones = norm*24-12 (continuous, 0.5 = 0 st) · `Quality` 4 steps (norm k/3)
  · `TransBoost` 0..100 % (continuous) · `Preset`, `Bypass`, unnamed idx5 (ignore).
- Reported latency 2048 samples (42.7 ms) at every Quality -> almost certainly an FFT phase vocoder (4096 window?).
- Preliminary, quick suite, Quality 0 / TransBoost 0 (metrics still being validated):
  measured latency ~40 ms median (xcorr agrees with reported); f0 error ~0.03 c; mono SINAD 11-38 dB
  (worst on low notes shifted up); upshifts lose 4-10 dB of level; attack rise time ~4x ideal (smear 2.1);
  pre-echo ~-15 dB; envelope ripple ~50 Hz (frame rate).

## Machine
- 12 logical CPUs, 7.8 GB RAM but typically only 0.6-2 GB free (VS Code, Dropbox, browser...): run ONE
  suite at a time. Uncapped pools exhausted the paging file; even 6 workers + 4 vsthosts got the sweep
  killed for low memory. run_suite now processes+measures ONE SETTING AT A TIME, deletes its WAVs, and
  defaults to 3 metric workers and 2 vsthost instances (`--workers N` to override).
- Even the lean loop got killed once (TransBoost sweep) while an identical Quality sweep survived: free RAM
  fluctuates with other apps. For long reference runs use one setting per invocation, `--workers 2`.
  Sweeps checkpoint `metrics.partial.json` after each setting (promote works on it if renamed).
- shiftbench block = 16 (constants.hpp), not 32.

## Metrics
- Vocoder output has ~50 Hz frame-rate envelope ripple: per-onset latency must be anchored on a global
  envelope xcorr (then refined +-25 ms), and derivative/flux onset detectors count the ripple as flams.

- Quality sweep, full suite, TransBoost 0 (results/vst/20260915-133134-full-qsweep):
  | Q | lat | lat max | if_dev c | track c | SINAD mono | SINAD poly | AM | LSD | flam | smear | pre-echo | level |
  | 0 | 39.9 | 73 | 12.9 | 10.4 | 31.7 | 21.4 | 0.43 | 4.5 | 1.58 | 1.92 | -18.5 | 3.7 |
  | 1/3 | 41.4 | 77 | 0.63 | 6.7 | 54.3 | 26.2 | 0.04 | 4.0 | 1.23 | 1.91 | -18.3 | 3.7 |
  | 2/3 | 41.0 | 68 | 0.34 | 4.7 | 69.9 | 27.4 | 0.03 | 3.7 | 1.34 | 1.93 | -17.6 | 3.3 |
  | 1 | 41.1 | 68 | 0.31 | 4.7 | 81.0 | 27.9 | 0.02 | 3.7 | 1.24 | 1.80 | -18.8 | 3.2 |
  -> Quality 1 best/equal everywhere. Latency (~41 ms), attack smear (~3.5x rise time) and pre-echo
  (~-18 dB) are Quality-independent: frame-size properties of the vocoder, i.e. where a low-latency
  time-domain design can win outright. Poly SINAD tops out ~28 dB even at Q1: chords are its weak spot too.
  Level: |error| ~3 dB mean (loses level on upshifts).
- Q1 by kind / worst cases (same run) -- where the reference is beatable:
  - Poly: dyad 30.4 dB, chord 24.6 dB; worst min2 dyad E4 +7 (3.3 dB), Cmaj7/Emaj/E5power at +12 (4.8-6.4 dB).
  - Mono low strings shifted up: harm/pluck E2,A2 at +7/+12 SINAD 34-36 dB, if_dev 4-10 c (bin resolution).
  - Moving pitch: sweep +7 22 c, sweep -12 14 c, vibrato D4 +12 14 c tracking error.
  - Flams: power chord at +-1 st 5-7.5 dB, burst E3 +12 6.4 dB; attack smear worst on bursts (2.8).
  - Strong: steady mono notes (pluck 77 dB, decay 83 dB), bends (0.02 c), pitch_err ~0 c.
  - (Ideal-render check passed at -12/+1/+12 for all full-suite signals, so these are plugin, not metric, limits.)

## Current Shift (shift.cpp)
- Time-domain pitch-synchronous splicer: 32-tap sinc read, YIN on 4x decimated line, coarse+fine NCC splice,
  onset re-seat. Latency ~1 period (variable). Designed for monophonic; chords rely on common-period YIN lag up to 300 dec samples.

- First quick run (results/shift-current/*-quick-first): lat 10.4 ms median / 32.9 max, f0 err 0.19 c,
  mono SINAD 62.5 dB, poly SINAD 38.5 dB, AM 0.03 dB, LSD 1.9 dB, flam_db 1.34, track err 4.5 c,
  no pre-echo, CPU 390 ns/sample desktop, worst block 85 % (spiky: YIN/splice searches or OS jitter?).
- Ahead of VST *Quality 0* on latency, SINAD, AM, pre-echo -- but Quality 0 is the plugin's WORST mode:
  quick subset (-12/+12) mono SINAD Q0 22.7 dB vs Q1 70.7 dB (latency unchanged ~43 ms). Judge only
  against the promoted baseline, never Q0.

## Algorithm lessons
