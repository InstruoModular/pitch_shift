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

## Metrics
- Vocoder output has ~50 Hz frame-rate envelope ripple: per-onset latency must be anchored on a global
  envelope xcorr (then refined +-25 ms), and derivative/flux onset detectors count the ripple as flams.

## Current Shift (shift.cpp)
- Time-domain pitch-synchronous splicer: 32-tap sinc read, YIN on 4x decimated line, coarse+fine NCC splice,
  onset re-seat. Latency ~1 period (variable). Designed for monophonic; chords rely on common-period YIN lag up to 300 dec samples.

## Algorithm lessons
