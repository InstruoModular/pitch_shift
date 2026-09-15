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

- E0 full-suite gap vs baseline (results/shift-current/20260915-134406-full-baseline): wins latency
  (9.3 vs 41 ms), attack smear, pre-echo, level, LSD; loses pitch_err 1.36 c, poly_pitch 12 c, if_dev 4.7 c,
  sinad 57.6 vs 81, sinad_poly 26.6 vs 27.9. Localised:
  - sine_E6 downshifts catastrophic (SINAD -91..-121, if_dev 147-227 c, AM up to 9.5 dB): E6 period 36 smp
    = 9 decimated lags, at yin_min_lag 8. Mean pitch/if_dev/sinad are dominated by this one signal;
    harmonic 68 dB / 0.2 c, decay 55 dB, pluck 49 dB otherwise.
  - Downshifts worse than upshifts on pitch (-12: 3.9 c, +12: 0.07 c).
  - Chord flams 8-16 dB (E5power +5 15.9, Amin +12 10.3, Cmaj7 +5/+7 ~9): splicer repeats attacks on chords.
  - dyad_min2_E4 poly SINAD -12..-1 dB (VST 3-5 dB): common period too long for the tracker.
  - sweep_80_2k -12 track err 33 c.
  - CPU worst block 3471 % was OS noise: with per-block min over 2 passes (shiftbench now does this) worst
    is 70 % (decay_A3 -12), median job 45 %, 0/79 jobs over budget. But a consistent ~45 %-of-block peak on a
    desktop x86 = real per-block spike (splice search / YIN) -- a firmware risk to watch in every experiment.
  - Min-of-2-passes still leaks OS noise occasionally (E5: 271 % / 215 % outliers re-timed at 44-45 %).
    Before acting on a worst-block number, re-time the top jobs 3x (shiftbench run <variant> on a tiny
    manifest from build/sigcache); trust p99 over max.
  - High sines are DETUNED, not broken (dbg: level exact, constant): A5 -12 +45 c, E6 -12 -110 c, D6 -12
    -37 c, A5 +12 -23 c; C6 exact at every shift. Hypothesis: `min_period = 60` clamps periods < 60 smp
    (> 800 Hz) while YIN tracks to 32 smp; grain becomes 4*60 = 240 and the tracked coarse search only spans
    +-12 smp, which contains no whole period multiple for A5 (54.5) / E6 (36.4) -> every splice off-phase.
    C6 (6x45.9=229... in range), G6 (8x30.6=245) happen to fit. D6 unexplained (maybe YIN unlocked).
    E1 confirmed it partly (A5 fixed) but downshift detune remains, always NEGATIVE, downshift-only, and
    C6 -12 regressed once grain matched the period exactly. Second hypothesis (E2): in `_start_fade` the
    downshift target `lag - d` is clamped to `lag_floor`, and lag_lo == lag_floor, so any d > grain (search
    reach up to p/8+16) gets clamped -> jump is not a period multiple -> phase slip -> flat.

## Algorithm lessons
- Splicer phase integrity: any clamp/limit applied to a correlation-chosen splice target silently turns a
  period-multiple jump into an arbitrary one = steady detune (sign = which side the clamp is on). Give the
  scheduler headroom equal to the search reach instead (E2: +~1 ms latency, fixed all high-note detune).
- min_period must not exceed the tracker's smallest lag (E1).
- Chords (untracked path): splice RATE is the lever. Halving it (default_grain 768->1536, E3b) took poly
  SINAD 26.7->31.9 dB (past the VST's 27.9) and poly pitch error 11.9->8.4 c for +1.8 ms median latency.
  Extra splice headroom on that path (E3a) did nothing useful. The blind coarse span scales with grain, so
  longer grains also cost CPU (E3b worst block 106 %).
  Not monotonic: 2304 (E4a) was worse than 1536 on every chord metric and 153 % CPU. Capping the blind
  coarse span at 269 smp (E4b) keeps the 1536-grain gains (poly SINAD 32.5 dB) at 61 % CPU: rate, not
  search reach, is what helps; reach only costs CPU.
- Upshift splices jump BACK by the grain: any grain longer than the time since the last onset re-reads the
  attack as an echo (E4b bursts +12: +37 dB flam_db at a 32 ms jump). Every note starts untracked, so the
  blind grain governs attacks; long grains are only safe for sustained material.
- Use `--suite poly` (72 jobs, ~40 s) for chord experiments.
- E2 per-case gaps vs VST (results/shift-down_margin/*-full-E2):
  - Mono SINAD: VST plucks sit at the 100 dB metric cap (essentially ideal); ours 46-50 dB on plucks
    (beats VST in only 27/144 mono cases). Hypothesis: on a decaying note the two heads are a grain apart
    so their levels differ (~0.09 dB/splice at t60 3 s, grain ~4.5 ms) -> periodic level step -> ~-45 dB
    sidebands. Candidates: envelope-matched crossfade gain, longer xfade.
  - Poly: poly_pitch_err 37-67 c on Cmaj7/Amin/fourths (partials smeared, not cleanly shifted); poly SINAD
    worst at -1 st (Cmaj7 14 vs 45 dB). Beats VST in 28/72 poly SINAD, 5/72 poly pitch cases.
    Structural limit: a splice can be a period multiple of only one period family, so every other note
    gets a phase jump per splice; detune/smear scales with splice rate. Time-domain splicing alone likely
    can't match a vocoder on dense chords -> E3 quantifies the splice-rate/latency frontier before a
    multi-band or hybrid-vocoder design.
