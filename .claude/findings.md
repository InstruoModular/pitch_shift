# Findings

## Build
- `functions.hpp` uses `__builtin_memcpy` -> Shift must build with g++/clang (MSYS2 ucrt64), not MSVC.
- shift.cpp includes `engine/constant.hpp`, `instrument/functions.hpp`, `instrument/shift.hpp`; forwarded by `tools/compat/`.
- `constants.hpp`: `audio_block_size = 16` (user mentioned 32 — confirm), 48 kHz, `MonoDspBuffer = idsp::SampleBufferStatic<16>`.
- isl `idsp::BiquadFilter<BiquadType::X>`: `set_parameters(norm_f, Q)`, `process(x)`, `reset()`.

## Project decisions (user, 2026-09-15)
- BL-PitchShift.vst3 was a TEST reference. The real target VST is still to come; everything below about
  BL is a characterisation of the test plugin, and E0-E9 were judged against it (at block 16).
- Hardware: ideally Cortex-M33 with CMSIS-DSP FFT ("tight, curious if it can be done"), fallback Cortex-A.
  Order: beat the reference's quality on the host first, then decide hardware compromises.
- Block size 32 (adjustable). constants.hpp changed 16 -> 32 on 2026-09-15. E7 (env_match2) quick suite at
  32: lat 13.6 ms, pitch 0.23 c, sinad 71.0, sinad_poly 39.8, flam 1.75, CPU 337 ns/smp, worst block 24.9 %
  of the 32-smp period (vs 44-62 % of a 16-smp block) -- results/shift-env_match2/*-quick-block32.
- run_suite is plugin-agnostic now: `--plugin --shift-param [--shift-scale]`; semitone mapping read from the
  parameter's display texts (verified on BL: pitch_err 0.00 c). VST results go to results/vst-<stem>/
  (older BL runs are in results/vst/).

## TARGET plugin: Archetype Misha Mansoor X.dll (Neural DSP) -- the one to match/beat
- VST2 (exports VSTPluginMain; no GetPluginFactory), 105 MB, JUCE-based (preset XML is an AudioProcessorGraph:
  nodes `transpose` / `transpose_stereo`, `doubler_bypass` with pitchAmnt, `misha_granular_fx`, glitch,
  octaver, harmonizer icons). Intel IPP statically linked (no symbol names). Licence strings present
  (NoAuthorizedLicensesFound) but it loads and processes on this machine.
- Hosted by tools/vst2host (clean-room VST2 ABI, no SDK; JUCE's VST2 hosting needs the Steinberg SDK).
  `info` 6 s incl. 3 s warm-up. Stereo in/out, 180 params, reports initialDelay = 84 samples (1.75 ms)
  in the default preset.
- `Transpose` (index 4): 25 integer steps -12..+12 st (display "N st"), norm 0.5 = 0 st; the display-text
  mapping gives -12 -> 0.01, +12 -> 0.99. Quantised, so it's a fixed-interval shifter like shift.hpp.
- Isolation recipe (all sections off, gains 0 dB) -- pass as --params:
  `Input Gain=0.5;Output Gain=0.5;Gate Active=0;Doubler Active=0;Special FX Section Active=0;Pre FX Section Active=0;Amp Section Active=0;Cab Section Active=0;EQ Section Active=0;Post FX Section Active=0`
  (the default preset has gate, compressor, Rhythm amp, cab, EQ ON).
- Neutral chain at 0 st is bit-transparent apart from the reported 84-smp delay (SINAD 100 cap, LSD ~0,
  latency 1.75 ms measured = reported): the transpose is fully bypassed at 0 st and the licence doesn't
  block processing. (results/vst-Archetype Misha Mansoor X/20260915-145621-quick-neutral)
- REPORTED LATENCY EXCLUDES THE SHIFTER: at +12 measured onset latency is 9-14 ms vs 1.75 ms reported.
- First look at +12 (quick subset): pitch 0.00-0.44 c; SINAD sine 61, harmonic 41, pluck 28-30 dB;
  poly fifth 37.7, Emaj 28.8 dB; no pre-echo (-97 dB); AM 0.18 dB @ 25.9 Hz; flam_db 2.17; attack smear
  0.56. Reads like a low-latency time-domain grain/splice shifter (same family as shift.hpp), not a vocoder.
- GOTCHA (confirmed by job-order test): the first ~2 jobs after load are digitally silent whatever the signal
  (sine_E2 as jobs 1,2 silent; sine_A4 job 3 and sine_E2 job 4 fine). vst2host `run` now primes with
  low-level noise until the output is live for 1 s (`--prime-max-ms`, default 30000, 0 disables) and logs
  how long it took. Any run of this plugin made before that fix has its first jobs invalid.
  Refined: load-time priming went live after 1.2 s of audio, yet the NEXT job (first with the isolation
  params applied) was still silent -> the mute is triggered by PARAMETER CHANGES (section toggles), not load.
  vst2host now re-primes whenever a job's parameter set differs from the previous job's.
  Verified: job-order test all 4 jobs live (-17.5 dBFS); quick suite all 40 cases |level_db| < 1.5 dB.
- First VALID quick scorecard (isolated, block 32; results/vst-Archetype Misha Mansoor X/20260915-150036-quick-q-neutral):
  lat median 8.5 / max 37.4 ms (reported 1.75), pitch 0.58 c (p95 0.81), if_dev 2.7 c, track 5.0 c,
  poly pitch 3.9 c, SINAD mono 43.1 / poly 34.2 dB, AM 0.29 dB @ 11.8 Hz, LSD 2.5, no pre-echo (-103 dB),
  flam_db 3.3, attack smear 1.13, level 0.09 dB.
  Per-case onset latency spans -5..20 ms (short on downshifts, 10-15 ms on upshifts); negative values are
  an envelope-alignment artefact on smeared attacks -> use analysis/probe.py (burst threshold latency, click
  response) for this plugin's latency, not lat_ms alone.
- FULL suite, isolated, block 32 — PROMOTED to reference/baseline.json
  (results/vst-Archetype Misha Mansoor X/20260915-150416-full-full-neutral, 313 cases):
  lat 8.16 median / 45.2 max ms (jitter 4.3), pitch 0.27 c (p95 0.48), if_dev 1.83 c, track 6.29 c,
  poly pitch 8.06 c, SINAD mono 43.0 / poly 26.6 dB, subharm -77, hf_junk -80, AM 0.28 dB @ 12.6 Hz,
  LSD 2.64, no pre-echo (-125 dB), flam_db 2.46, attack smear 0.88, level |0.35| dB.
- E7 (env_match2, block 32) vs Archetype, full suite (results/shift-env_match2/20260915-150812-full-vsArchetype):
  E7 WINS every quality metric: pitch 0.20 vs 0.27 c, if_dev 0.30 vs 1.83, track 6.05 vs 6.29,
  SINAD mono 68.3 vs 43.0, poly 32.9 vs 26.6, AM 0.05 vs 0.28, LSD 1.80 vs 2.64, flam 1.14 vs 2.46,
  disc 0.07 vs 1.06, poly pitch 8.38 vs 8.06 (within tol), attack smear 0.93 vs 0.88 (within tol).
  E7 LOSES only LATENCY: median 11.7 vs 8.2 ms, max 61.0 vs 45.2 ms. CPU worst block 44 % at block 32.
  -> The job vs the real target is latency (and keep quality), not chords.
- Latency gap by shift (median onset lat, Arch vs E7): -12 5.8 vs 15.8; -7 8.4 vs 11.8; -5 4.8 vs 10.5;
  -1 9.2 vs 8.4 (E7 faster); +1 9.0 vs 7.1 (E7 faster); +5 9.4 vs 11.8; +7 4.5 vs 13.7; +12 10.2 vs 16.1.
  By kind E7 is ~3-4 ms slower almost everywhere except chord (6.4 vs 7.2) and staccato (6.9 vs 6.8).
  E7 worst case is chords (Emaj -12 61 ms, Cmaj7 +5 46.5 ms: blind path).

## Technique probes (suites/probe.json + analysis/probe.py; Archetype run results/vst-Archetype*/20260915-151104-probe-probe, E7 results/shift-env_match2/20260915-151147-probe-probe)
- ARCHETYPE = TIME-DOMAIN, PITCH-ADAPTIVE SPLICER (same family as shift.hpp), inferred from:
  - click response: one clean copy, zero energy before the peak (no FFT framing), first arrival varies
    2.3-13 ms between clicks and shifts (time-varying delay line); +12 shows a 2nd copy 6.7 ms later.
  - attack latency (burst lat50): A3 5.8-8.8 ms, E5 4.1-9.4 (one outlier 18.8 at -12), E2 8.5-16 ms ->
    grows for low notes = delay tied to the pitch period (PSOLA / period-snapped grains).
  - steady sines: no measurable AM (<0.02 dB) but sidebands -46..-80 dB at 9-66 Hz offsets (smooth splices
    that aren't perfectly period-aligned); carrier error up to 2.2 c on E2 -12 (low downshifts).
  - formants NOT preserved (partial amplitudes travel with the partials, corr 1.00 naive).
  - reports 84 smp latency that excludes the shifter; mutes for seconds after section/param changes.
  - Intel IPP linked (likely for amp NN/cab IR stages, not needed by a time-domain shifter).
- E7 by the same probes: click = one copy, 7-17 ms first arrival (Arch 2.3-13); attack latency is the SAME
  for every note at a given shift (±1: 7.2 ms, ±7: 12-14, ±12: 16-17 ms) -> fixed structural lag, not the
  period; no sidebands above -80 dB and no AM (cleaner than Arch); formants naive (same as Arch).
- E7 latency is set by the untracked path every attack goes through before YIN locks (YIN at block 32 retires
  2 lags/block -> one pitch estimate per ~150 blocks ~100 ms): blind grain 768 (onset_grain), fallback
  corr window 512 (lag_floor = 40 + 256), plus xfade headroom on upshifts and down_margin on downshifts.
- BUG (verified numerically): tairm::fast_exp2 is NOT 0.002 % accurate: error -0.884 c at -1/+11 st,
  -0.576 at -2/+10, -0.357 at -3/+9, -0.208 at -4/+8, -0.112 at -5/+7, exact at 0/+-12. It explains E7's
  systematic -0.88 c at -1 st on every note. Ratio only changes with the interval -> use an exact value.

## Latency work vs Archetype (variant `lat`, runtime knobs via set_param; greedy sweeps on the full suite)
- `analysis/sweep_table.py <run> [--shifts]` prints settings x (lat, latmax, pitch, ifdev, sinad, poly, ppitch,
  flam, smear, lsd, am, cpu%) plus the Archetype row, and median lat per shift. Use it after every sweep.
- E10 exact ratio: pitch 0.20->0.07 c, if_dev 0.30->0.17 c, free.
- E11 onset_runway is THE upshift latency term (head re-seated runway*(r-1) behind the input at attacks):
  1200->300 took median lat 11.7->8.3 ms (= Arch 8.2) and +12 16.1->6.9 ms, costing flam 1.14->1.75 (Arch 2.46)
  and smear 0.93->0.99 (Arch 0.88). runway 0 is worse (flam 2.29, smear 1.26) with no median gain.
- Downshift latency is a different mechanism (E11 left it unchanged: -12 15.8 vs Arch 5.8): the head sits at
  the blind lag_floor (40 + fallback_corr_window/2) and then falls behind at (1-r) until it splices back by the
  onset grain -> E12 sweeps both.
- E12: the blind correlation window is the downshift lever (512->256: -12 15.8->10.5 ms, -1 8.4->5.6), onset_grain
  mostly moves flams and max latency (768->384: flam 1.57->0.83, max 38->31 ms at w256) not the median. Shorter
  blind windows cost chord per-note pitch (ppitch 8.4 -> 9.5-9.8 c). First config passing EVERY metric vs Arch:
  exact_ratio=1, onset_runway=300, fallback_corr_window=384, onset_grain=768 (lat 7.41 / max 40.9 ms).
- E13 CAUSAL correlation windows (end at the head, lag_floor = guard only) are the biggest single latency win:
  median 7.1-7.4 -> ~5.0 ms (Arch 8.2) and downshifts now match/beat Arch (-12 5.1 vs 5.8, -7 4.0 vs 8.4,
  -5 3.4 vs 4.8, -1 3.0 vs 9.2) with SINAD/poly/smear unchanged. Side effect: max latency 38-41 -> 46-54 ms
  (chords). Best: causal + w256 + g768: lat 5.08 / max 46.4, ppitch 8.85, flam 1.68. Upshifts are now the
  slower side (+7 7.7 vs Arch 4.5) -- they are set by onset_runway*(r-1).
- E14: guard_samples 40 -> 24 is free (-0.5 ms everywhere, quality same). Faster YIN (8 lags/block) does NOT
  help latency (max 46 -> 54 ms). Shorter blind default_grain trades chord pitch/flams for <1 ms of upshift.
  Best after E14: exact_ratio=1;onset_runway=300;fallback_corr_window=256;onset_grain=768;causal_corr=1;guard_samples=24
  -> lat 4.54 / max 46.2 ms (Arch 8.16 / 45.2), sinad 68.1, poly 32.8, ppitch 8.90, flam 1.69, smear 0.91.
- E15a: xfade_frac is the upshift lever (upshift lag_lo = guard + xfade*(r-1), xfade = xfade_frac*grain/drift):
  0.25 -> 0.0625 took median lat 4.54 -> 3.34 ms and +7 7.5 -> 3.6 ms (Arch 4.5); now faster than Archetype at
  EVERY shift. Cost is small: LSD 1.87 -> 2.10, flam 1.69 -> 1.89, ppitch 8.90 -> 9.11 (Arch 8.06). 0.125 is
  the no-compromise point (lat 4.17, passes every metric vs Arch).
- E15b: onset_runway 300 -> 150 buys ~0.3-0.5 ms but costs flams (xfade 0.125: 1.82 -> 2.07; 0.0625: 1.89 -> 2.62,
  past Arch 2.46). WINNER (passes every metric vs Archetype, faster at every shift):
  exact_ratio=1;onset_runway=150;fallback_corr_window=256;onset_grain=768;causal_corr=1;guard_samples=24;xfade_frac=0.125
  -> lat 3.66 / max 46.3 ms (Arch 8.16 / 45.2), sinad 68.1, poly 32.8, ppitch 8.67, flam 2.07, smear 0.97, lsd 2.02.
  CPU (tools/retime.py, min of 3, 12 heaviest jobs, block 32): worst block p50 21.9 %, max 22.1 % -- same as E7
  (21.5 / 23.1 %), so the latency changes cost no measurable CPU on the desktop.
- E16 (user: better chord per-note pitch within ~1 ms of Archetype): blind_span_cap 269 -> 538 is free
  (ppitch 8.67 -> 8.25 c, flam 2.07 -> 1.98, lat/max unchanged, CPU p50 22.1 % max 22.3 %). A longer causal blind
  window (512) reaches 8.14 c but costs +4.4 ms max latency (50.7 vs Arch 45.2) -> rejected.
  FINAL (folded into shift.hpp/cpp): exact_ratio=1;onset_runway=150;fallback_corr_window=256;onset_grain=768;
  causal_corr=1;guard_samples=24;xfade_frac=0.125;blind_span_cap=538.
- A 2-knob x 4-setting (8-setting) full sweep got killed for memory at setting 7; keep full sweeps to <= 4
  settings per run (checkpoint saves finished settings in metrics.partial.json; sweep_table reads it).
- CPU worst-block numbers in multi-setting sweeps swing 30-110 % with no code change (OS noise); re-time the
  final candidate with the 3x re-time recipe before trusting CPU.

## Probe tooling
- analysis/probe.py validated on known answers (scratchpad validate_probe.py): 10 ms delay -> click first
  arrival 9.8 ms, burst lat50 9.9-10.3 ms; ideal -> formant verdict 'naive' (corr 1.0). Sideband probe v1
  was wrong (BH leakage at -95 dBc read as a sideband; levels relative to a cancelled carrier); fixed to
  band-max reference, 12-bin guard, AM rate only when AM >= 0.02 dB, grain reported for both one-splice
  (|1-r|/rate) and two-head overlap (2|1-r|/rate) schedulers (crude 40 ms two-head test: AM 25 Hz -> 40 ms).

## Reference plugin (BL-PitchShift.vst3) -- TEST plugin, archived in reference/plugins/
- Single-file VST3 DLL, BlueLab / iPlug2 (OpenGL UI). Mono in/out accepted. Full dump: `reference/vst_info.json`.
- Params (normalised): `Factor` semitones = norm*24-12 (continuous, 0.5 = 0 st) · `Quality` 4 steps (norm k/3)
  · `TransBoost` 0..100 % (continuous) · `Preset`, `Bypass`, unnamed idx5 (ignore).
- Reported latency 2048 samples (42.7 ms) at every Quality -> almost certainly an FFT phase vocoder (4096 window?).
- Preliminary, quick suite, Quality 0 / TransBoost 0 (metrics still being validated):
  measured latency ~40 ms median (xcorr agrees with reported); f0 error ~0.03 c; mono SINAD 11-38 dB
  (worst on low notes shifted up); upshifts lose 4-10 dB of level; attack rise time ~4x ideal (smear 2.1);
  pre-echo ~-15 dB; envelope ripple ~50 Hz (frame rate).

## Listening verdict & modulation metrics (2026-09-16)
- USER LISTENING TEST: final shift.hpp/cpp is audibly WORSE than Archetype: "granular distortions/warble".
  The old suite said the opposite -> it was blind to modulation artefacts (SINAD's +-10 c partial tolerance
  swallows near-carrier sidebands; if_dev/am_pp only looked at fundamentals of perfectly stable tones).
- New metrics (analysis/metrics.py), all EXCESS over the ideal render, 3-70 Hz fluctuation band:
  fm_rough_cents (complex demod of up to 8 isolated partials, energy-weighted FM rms), am_rough_db (same, AM),
  env_mod_db (1/3-oct band log-envelope modulation, covers chords). Selftest: FM +-5 c @ 12 Hz -> ~3.5 c,
  AM +-0.5 dB @ 20 Hz -> ~0.35 dB, ideals ~0, -40 dB noise doesn't register.
- New realistic material: siggen kinds `guitar` / `gchord` (inharmonic partials B~4e-5*sqrt(f0/82), slow random
  drift +-3 c, +6 c attack settling in 80 ms, pluck/pickup comb, pick noise); suites/real.json (8 notes incl.
  vibrato, 5 chords, 8 shifts). Ideal renders all clean on every metric.
- Archetype on suites/real.json (results/vst-Archetype Misha Mansoor X/20260915-210858-real-real, WAVs kept):
  fm_rough 3.08 c, am_rough 0.47 dB, env_mod 0.49 dB, lat 8.30 / max 43.9 ms, pitch 1.69 c, if_dev 4.80,
  sinad 35.9, poly 23.6, poly pitch 11.7, flam 1.57, smear 0.78.
- analysis/remeasure.py re-scores any --keep-wav run with the current metrics (no re-processing).

## Listening plugin (tools/listen_plugin)
- MSVC cannot compile shift.cpp: isl needs /Zc:__cplusplus, then C3615 (constexpr tairm::min/max wrapping std::fmin,
  idsp::SVFilter::set_parameters wrapping std::tan) persists even with /std:c++latest; plus __builtin_memcpy.
  No clang-cl in VS/MSYS2. Solution: GCC-built shift_capi.dll (C ABI, static runtime, imports only KERNEL32 + UCRT)
  loaded by the MSVC JUCE plugin via juce::DynamicLibrary from its own folder; post-build copies the DLL.
- Verified bit-exact: VST3 in vsthost (host block 64) == shiftbench `current` delayed by exactly 32 samples
  (max diff <= 3e-19 on pluck/chord at +7/-12), reports latency 32, Transpose int param maps exactly.
- GOTCHA (caught by the smoke test): after re-folding shift.cpp, building ShiftListen_VST3/_Standalone left the OLD
  shift_capi.dll in the bundle (plugin didn't relink -> POST_BUILD copy never ran), smoke test FAILED with diffs up
  to 0.19. Fixed with an always-run `ShiftListen_SyncDll` target (builds both formats, then copies the DLL).
  Rule: after any shift.cpp change, rebuild `shift_capi` then `ShiftListen_SyncDll`, then run the smoke test.

## Machine
- 12 logical CPUs, 7.8 GB RAM but typically only 0.6-2 GB free (VS Code, Dropbox, browser...): run ONE
  suite at a time. Uncapped pools exhausted the paging file; even 6 workers + 4 vsthosts got the sweep
  killed for low memory. run_suite now processes+measures ONE SETTING AT A TIME, deletes its WAVs, and
  defaults to 3 metric workers and 2 vsthost instances (`--workers N` to override).
- Even the lean loop got killed once (TransBoost sweep) while an identical Quality sweep survived: free RAM
  fluctuates with other apps. For long reference runs use one setting per invocation, `--workers 2`.
  Sweeps checkpoint `metrics.partial.json` after each setting (promote works on it if renamed).
- shiftbench block = audio_block_size from constants.hpp: 16 until 2026-09-15, 32 since (user decision).

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
- Splice level matching (E6) helps decaying notes (pluck SINAD +5.4 dB) but a gain estimate that isn't exact
  on steady tones is worse than none: any per-splice gain error is a new periodic step (harmonic -15 dB,
  sine -11 dB, 2.7 Hz level wander). Estimates must be over whole periods and gated/deadbanded.
  E7 did exactly that (backward whole-period window >= 256 smp, tracked only, |g-1|<0.002 deadband):
  pluck 49.3->59.6 dB, decay 55.4->67.7 dB, steady tones bit-for-bit unchanged, zero regressions.
- E7 chord per-note pitch (poly_pitch_err, E7 / VST): simple-ratio material is solved (E5power 0.8/0.2,
  fifth 0.7/0.0, fourth 0.6/0.0, octave 0.1/0.0 -- YIN locks the common period), the gap is untracked chords
  with no common period: Amin 25.2/0.0, Cmaj7 27.5/5.6, min2 dyad 19.9/4.8. Error grows with |1-ratio|
  (-12: 17.0, -7: 15.1, +12: 8.8 vs +-1: 1.3-2.9), consistent with splice-rate sidebands smearing each
  partial. Median case 1.9 c, 37/72 under 2 c. E7 CPU p50 44 %, p99 62 %, max 84 % (click +12), 0 over budget.
- E8: untracked chords are FLAT across blind grain 1024-2048 (poly_pitch 24-28 c, sinad_poly 7-9 dB), so the
  splice rate is not what limits them; E3b's poly gains came from the rest of the poly material. 1536 stays
  best overall. Tuning path: `shift:tunable` + `--sweep "default_grain=...;fallback_corr_window=...;blind_span_cap=..."`
  (no rebuild per value; tunable == E7 at defaults).
- E9: a longer blind correlation window is worse on every chord metric and costs latency + CPU (window/2
  sets lag_floor); more blind reach buys only 24.2->20.9 c on untracked chords at 105 % CPU. With E8 flat
  too, the untracked-chord pitch gap (~21-24 c vs VST ~3.5 c) is STRUCTURAL for a single time-domain
  splicer: no rate/window/reach setting closes it. Needs multi-band or a frequency-domain path.
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
