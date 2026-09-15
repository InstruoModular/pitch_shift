# Findings

## Build
- `functions.hpp` uses `__builtin_memcpy` -> Shift must build with g++/clang (MSYS2 ucrt64), not MSVC.
- shift.cpp includes `engine/constant.hpp`, `instrument/functions.hpp`, `instrument/shift.hpp`; forwarded by `tools/compat/`.
- `constants.hpp`: `audio_block_size = 16` (user mentioned 32 — confirm), 48 kHz, `MonoDspBuffer = idsp::SampleBufferStatic<16>`.
- isl `idsp::BiquadFilter<BiquadType::X>`: `set_parameters(norm_f, Q)`, `process(x)`, `reset()`.

## Reference plugin (BL-PitchShift.vst3)
- Single-file VST3 DLL, BlueLab / iPlug2 (OpenGL UI). Params: TBD from `vsthost info`.

## Current Shift (shift.cpp)
- Time-domain pitch-synchronous splicer: 32-tap sinc read, YIN on 4x decimated line, coarse+fine NCC splice,
  onset re-seat. Latency ~1 period (variable). Designed for monophonic; chords rely on common-period YIN lag up to 300 dec samples.

## Algorithm lessons
