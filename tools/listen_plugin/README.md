# Shift Listen — listening-test plugin

One knob (**Transpose**, −12…+12 semitones) around the firmware pitch shifter in the repo-root
`shift.hpp` / `shift.cpp`, for A/B listening in a DAW against the reference (Archetype Transpose).

## What you get

| Artefact | Path |
|---|---|
| VST3 | `build/host/tools/listen_plugin/ShiftListen_artefacts/Release/VST3/Shift Listen.vst3` |
| Standalone | `build/host/tools/listen_plugin/ShiftListen_artefacts/Release/Standalone/Shift Listen.exe` |

The DSP lives in `shift_capi.dll`, which sits inside the VST3 bundle (`Contents/x86_64-win/`) and next to the
Standalone exe. It is the same `shift.cpp` the measurement harness runs, compiled with MSYS2 g++.
Verified bit-exact: plugin output == `shiftbench current`, delayed by exactly one firmware block
(scratchpad smoke test, pluck and chord at +7 / −12, host block 64).

## Install

Copy the whole `Shift Listen.vst3` folder to `C:\Program Files\Common Files\VST3\`, then rescan plugins in the DAW.

## Using it

- **Set the project to 48 kHz.** The firmware is compiled for 48 kHz; at any other rate the plugin passes audio
  through and the panel says `BYPASSED`.
- Mono in (stereo inputs are summed), same signal on every output.
- Latency: one firmware block (32 samples at 48 kHz) of FIFO delay is reported to the DAW for delay compensation.
  The shifter's own delay (the thing being evaluated, ~2–5 ms median) is not reported, same as Archetype.
- For a fair A/B with Archetype: disable every Archetype section (gate, pre FX, amp, cab, EQ, post FX, doubler),
  set gains to 0 dB and use only Transpose. The exact parameter list is in `.claude/findings.md`.

## Rebuilding after changing shift.hpp / shift.cpp

```powershell
# 1. DSP DLL (g++ tree)
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
cmake --build build/bench --target shift_capi

# 2. Plugin (MSVC tree; the post-build step copies the new DLL into the bundle and next to the exe)
cmd /c '"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" && cmake --build build/host --target ShiftListen_VST3 ShiftListen_Standalone -j 2'
```

If only `shift.cpp` changed, step 1 plus re-running step 2 (to copy the DLL) is enough; then re-copy the bundle into
the VST3 folder.

## Why a DLL

`shift.cpp` / `functions.hpp` / isl use GCC-isms (`__builtin_memcpy`, `constexpr` wrappers around `std::fmin` and
`std::tan`) that MSVC rejects (C3615, even with `/std:c++latest`), while JUCE plugins are built with MSVC here.
The C interface in `shift_capi.h` keeps the firmware sources untouched.
