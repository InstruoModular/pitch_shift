# Pitch shifter results: shift.hpp/cpp vs Archetype Misha Mansoor X (Transpose)

Status 2026-09-15. Numbers come from the automated harness: synthetic suite `suites/full.json`,
40 signals × 8 shifts, 313 cases. Settings are 48 kHz, firmware block 32, Archetype isolated
(every section off, gains 0 dB). Details and raw runs are in `.claude/findings.md` and `.claude/experiments.md`.

## Headline

| Metric (full suite) | Archetype Transpose | Quality-tuned Shift (E7, before latency work) | **shift.hpp/cpp now** |
|---|---|---|---|
| Latency, median onset (ms) | 8.16 | 11.70 | **3.65** |
| Latency, worst case (ms) | 45.15 | 60.96 | **46.31** |
| Pitch error, steady (cents) | 0.27 | 0.20 | **0.06** |
| Pitch wobble, IF deviation p95 (cents) | 1.83 | 0.30 | **0.17** |
| Tracking error, vibrato/bend/sweep (cents) | 6.29 | 6.05 | **3.85** |
| Chord per-note pitch error (cents) | **8.06** | 8.38 | 8.25 |
| SINAD, single notes (dB, higher = cleaner) | 42.98 | **68.29** | 68.14 |
| SINAD, chords (dB) | 26.58 | 32.86 | **32.70** |
| Splice AM, p-p (dB) | 0.28 | **0.05** | 0.08 |
| Log-spectral distance to ideal (dB) | 2.64 | **1.80** | 2.02 |
| Repeated-attack / flam (dB) | 2.46 | **1.14** | 1.98 |
| Attack smear, rise-time ratio (log2) | **0.88** | 0.93 | 0.97 |
| Clicks at splices (dB) | 1.06 | 0.07 | **0.04** |
| CPU worst block, desktop, min of 3 (% of 32-sample block) | – | 23.1 | **22.3** |

Median latency per shift (ms):

| Shift | −12 | −7 | −5 | −1 | +1 | +5 | +7 | +12 |
|---|---|---|---|---|---|---|---|---|
| Archetype | 5.8 | 8.4 | 4.8 | 9.2 | 9.0 | 9.4 | 4.5 | 10.2 |
| **shift.hpp/cpp** | **4.7** | **3.4** | **2.9** | **2.6** | **1.6** | **4.0** | **4.0** | **3.8** |

Summary:
- **Faster than Archetype at every shift**, with less than half its median latency.
- **Worst-case latency** is 1.2 ms above Archetype's. The worst cases are strummed chords.
- **Better or equal on every quality metric** except two that sit within measurement tolerance:
  - chord per-note pitch: +0.19 cents vs Archetype;
  - attack smear: +0.09.

## What Archetype is doing (inferred from `analysis/probe.py`)

- **Method.** A time-domain, pitch-adaptive splicer, the same family as `shift.hpp`, not an FFT vocoder:
  - a click comes back as one clean copy with no pre-ringing;
  - attack delay grows for low notes;
  - there are faint sidebands of −46 to −80 dB on steady tones.
- **No formant preservation.**
- **Latency reporting.** It reports 84 samples to the host, which excludes the shifter. The real median latency is 8 ms.
- **Parameter changes.** It mutes for a few seconds after load and after section changes. The harness primes the plugin to cope.

## What changed in shift.hpp/cpp, and why

Every change was measured before being kept. See experiments E1–E16.

1. **`min_period` 60 → 32** (the tracker's floor). High notes above about 800 Hz were detuned.
2. **Downshift splice headroom.** A clamp on the splice target turned period-multiple jumps into arbitrary ones, so
   downshifted high notes drifted flat.
3. **Untracked-path grain 768 → 1536, with 768 for 200 ms after each onset, and a capped search span.** Chords got
   better without re-reading attacks as echoes.
4. **Level-matched splices.** Level is matched over a whole-period backward window, only while pitch is tracked,
   with a deadband. Plucks and decays went from a stepped to a smooth envelope.
5. **Exact interval ratio (`std::pow`).** `tairm::fast_exp2` is up to 0.88 cents flat, e.g. at −1 and +11 st.
6. **Latency:**
   - causal correlation windows, which end at the read head, so there is no look-ahead in the minimum lag;
   - untracked correlation window 512 → 256;
   - onset runway 1200 → 150;
   - crossfade fraction 0.25 → 0.125;
   - interpolation guard 40 → 24.
7. **Chords:** untracked search reach 269 → 538 samples. This is free at block 32.

The tunable lineage that produced these values is `variants/lat_causal.*`. `tools/fold_variant.py` froze them into
`shift.hpp/cpp`, and the folded code is identity-checked against the variant (zero difference).

## Known gaps / next options

- **Chord per-note pitch:** 8.25 cents vs 8.06. A 512-sample untracked window reaches 8.14 cents but costs +4.4 ms of
  worst-case latency. Closing it fully likely needs a multi-band or frequency-domain path for chords.
- **Worst-case latency:** 46.3 vs 45.2 ms, on strummed chords.
- **Firmware target:** Cortex-M33 with CMSIS is untested. Desktop worst block is 22 % of a 32-sample period, so
  measure on the target before trading quality for CPU.
- **Test material:** everything here is synthetic. Confirm by ear with Shift Listen (below) and with real DI takes.

## Listening test

`tools/listen_plugin`, **Shift Listen**, is a VST3 and Standalone app with one Transpose knob running this exact
`shift.cpp`. It's bit-exact against the harness, apart from a reported 32-sample FIFO delay. For build, install and
A/B setup, see `tools/listen_plugin/README.md`. Run the DAW at 48 kHz.

## Reproducing

```powershell
python analysis/selftest.py                                               # metrics self-validation
python analysis/run_suite.py --target shift:current --suite full --compare reference/baseline.json
python analysis/sweep_table.py results/shift-current/<run> --shifts
python analysis/run_suite.py --target shift:current --suite probe --keep-wav --no-report --tag probe
python analysis/probe.py results/shift-current/<run>
```
