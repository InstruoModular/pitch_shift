# Pitch shifter results: shift.hpp/cpp vs Archetype Misha Mansoor X (Transpose)

Status 2026-09-16. Numbers come from the automated harness. Synthetic suite `suites/full.json` (40 signals × 8 shifts,
313 cases) against `reference/baseline.json`; realistic guitar suite `suites/real.json` (13 signals × 8 shifts, 104
cases) against Archetype's own run of the same suite. Settings: 48 kHz, firmware block 32, Archetype isolated (every
section off, gains 0 dB). Raw runs and every experiment are in `.claude/experiments.md`; distilled facts in
`.claude/findings.md`.

## Why this build is different

The previous build beat Archetype on every metric the harness had, but **sounded worse**: granular distortion and
warble on chords, riffs and complex material, growing with the shift. Two things changed.

1. **Measurement.** New modulation metrics demodulate each partial along the shifted pitch track and measure its FM
   (cents) and AM (dB) roughness in the 3–70 Hz band against the ideal (`fm_rough_cents`, `am_rough_db`), plus
   band-envelope modulation and short-time grain noise. A realistic guitar suite was added (inharmonic partials,
   pickup comb, pitch drift, strummed chords), and `analysis/real_audio.py` measures the same roughness on any
   recording with no reference. On the user's sax loop the old build measured 4–47× Archetype's FM roughness, which
   reproduced the listening verdict.
2. **Algorithm.** The old engine spliced the read head hundreds of times a second at a median correlation of ~0.6, and
   every splice was a small phase jump — that is the warble. It was replaced (see below).

## Headline

Full synthetic suite, 313 cases:

| Metric | Archetype | Previous build (splicer) | **shift.hpp/cpp now** |
|---|---|---|---|
| Latency, median onset (ms) | 8.16 | 3.65 | **2.53** |
| Latency, worst case (ms) | 45.15 | 46.31 | **30.70** |
| Latency jitter (ms) | 4.31 | – | **2.58** |
| Pitch error, steady (cents) | 0.27 | 0.06 | **0.02** |
| Pitch wobble, IF deviation (cents) | 1.83 | 0.17 | **0.33** |
| Tracking error, vibrato/bend/sweep (cents) | 6.29 | 3.85 | **2.92** |
| Chord per-note pitch error (cents) | **8.06** | 8.25 | 9.31 |
| SINAD, single notes (dB) | 42.98 | 68.14 | **60.76** |
| SINAD, chords (dB) | 26.58 | 32.70 | **32.32** |
| Log-spectral distance to ideal (dB) | 2.64 | 2.02 | **2.53** |
| Repeated-attack / flam (dB) | 2.46 | 1.98 | **1.42** |
| Attack smear, rise-time ratio | 0.88 | 0.97 | **0.96** |
| FM roughness (cents, new metric) | – | – | **1.12** |
| Grain noise p90 (dB) | – | – | **−53.44** |
| CPU worst block, desktop (% of 32-sample block) | – | 22.3 | 80.3–82.0 |

Median latency per shift (ms), full suite:

| Shift | −12 | −7 | −5 | −1 | +1 | +5 | +7 | +12 |
|---|---|---|---|---|---|---|---|---|
| Archetype | 5.8 | 8.4 | 4.8 | 9.2 | 9.0 | 9.4 | 4.5 | 10.2 |
| **shift.hpp/cpp** | **2.1** | **1.6** | **1.2** | **1.2** | **1.6** | **3.3** | **3.8** | **5.0** |

Realistic guitar suite, 104 cases:

| Metric | Archetype | **now** |
|---|---|---|
| FM roughness (cents) | 3.08 | **1.59** |
| AM roughness (dB) | 0.47 | **0.27** |
| Grain noise p90 (dB) | −38.00 | **−38.68** |
| Latency, median / worst (ms) | 8.30 / 43.86 | **3.46 / 35.00** |
| SINAD, single notes / chords (dB) | 35.87 / 23.59 | **50.31 / 25.23** |
| Repeated-attack / flam (dB) | 1.57 | **1.59** |
| Chord per-note pitch (cents) | **11.74** | 13.19 |
| Log-spectral distance (dB) | **1.56** | 2.14 |
| Attack smear | **0.78** | 1.01 |

Real recording (5.3 s sax loop, reference-free), FM roughness in cents at −12 / −7 / +7 / +12:
Archetype 5.84 / 3.50 / 0.46 / 0.24, **this build 4.56 / 2.63 / 0.00 / 0.00** — see the caveat under Known gaps.

Summary: **faster than Archetype at every shift** (median 2.5 vs 8.2 ms, worst case 30.7 vs 45.2 ms) and ahead on the
artefact metrics that drove the listening complaint — FM roughness is about half Archetype's on realistic guitar
material, AM roughness and flams lower, SINAD 14–18 dB higher. It is behind on chord per-note pitch (+1.3 cents),
spectral distance on real material, and attack smear.

## What Archetype is doing (inferred from `analysis/probe.py`)

- **Method.** A time-domain, pitch-adaptive splicer, the same family as `shift.hpp`, not an FFT vocoder: a click comes
  back as one clean copy with no pre-ringing; attack delay grows for low notes; steady tones show faint −46 to −80 dB
  sidebands.
- **No formant preservation.**
- **Latency reporting.** It reports 84 samples to the host, which excludes the shifter; the real median is ~8 ms.
- **Parameter changes.** It mutes for a few seconds after load and after section changes, so the harness primes it.

## What changed in shift.hpp/cpp, and why

Every step was measured before being kept (experiments R3–R3u, P3 in `.claude/experiments.md`).

1. **Continuous overlap-add instead of splicing.** The output is always the sum of 50 %-overlapped Hann grains read
   from the resampled delay line. Each new grain continues where the previous one reads, jumped by whole pitch periods
   to hold a target lag, then aligned by causal normalised cross-correlation, so a mismatch blends over half a grain
   instead of switching at a splice. Sax FM roughness fell from 23.6/23.1/7.8/11.2 to ~4/3/0/0 cents (−12/−7/+7/+12).
2. **Alignment without pitch bias.** Candidate grain starts are integer offsets on the previous grain's own sample
   grid; the earlier fractional form biased every grain ~0.5 sample the same way — a constant ~7-cent offset.
3. **Affordable alignment search.** Scoring every offset at full rate cost ~8× the block budget. A coarse pass on the
   4× decimated line keeps its **two best peaks**, and each is refined at full rate over a 256-sample window. A single
   coarse pick lands on the wrong peak on bright material, and widening the search around it never recovered that.
   Strided sums and searching a block early were measured and rejected for quality.
4. **Chords the tracker cannot lock.** A voicing like Cmaj7 has no common period in YIN's range, so the engine jumped
   by a fixed 256 samples and smeared every note (fundamentals ~12 dB low, chord SINAD −6.7 dB). When YIN finds no
   confident dip, the **deepest dip** is now used as the jump quantum, adopted only after two consecutive frames agree
   and held through brief disagreements, so one-off dips at note transitions cannot take over.
5. **A longer tracker range (450 decimated lags, 1800 samples).** A fourth (A2+D3) repeats only every three note
   periods — 1309 samples, past the old 1200-sample ceiling — so its per-note pitch error was 38 cents. It is now 0.46
   (Archetype 0.44). This also removed a steady-pitch and a worst-case-latency outlier.
6. **Three-period grains** (`ola_periods` 2 → 3). Fewer, longer grains mean fewer jumps per second and less
   accumulated per-note phase error: chord per-note pitch 9.78 → 9.31 cents, worst-case latency 35.5 → 30.7 ms,
   flams 1.54 → 1.42, spectral distance 2.69 → 2.53 dB, at a cost of 0.2 ms median latency.
7. Kept from the previous build: exact interval ratio, causal correlation windows, block 32, anti-alias pre-filter,
   onset handling.

## Known gaps / next options

- **Chord per-note pitch**, 9.31 cents vs Archetype's 8.06 (13.19 vs 11.74 on realistic material). All of it is two
  voicings: **Cmaj7** (31.6 vs 21.8) and a **minor-second dyad** (19.8 vs 16.8). Everything else matches or beats
  Archetype — fourth 0.46/0.44, fifth 0.52/0.72, octave 0.01/0.26, power chord 0.51/0.67, major third 3.31/2.94,
  E major 4.69/4.38, A minor 22.9/24.6. The cause is structural: the engine jumps by whole multiples of one tracked
  period, which is exact only when the notes share a common period. A major seventh (15:8) and a minor second (16:15)
  have no common period in equal temperament, so any quantum is wrong for at least one note and each note drifts
  differently. The error scales with the number of jumps: at ±1 semitone Cmaj7 is 3–6 cents (Archetype 9–13), at ±7
  and ±12 it is 40–90.
  - **Measured and rejected:** forcing the jump to Cmaj7's approximate common period (~2936 samples) made it *worse*
    (42.1 vs 34.2 cents), so a long-range quantum search is not the answer. Deeper grain overlap (hop = grain/4)
    bought ~7 cents on Cmaj7 but broke worst-case latency (47.3 ms, past Archetype's 45.2) and flams.
  - **Multi-band is not recommended.** It only helps if each band holds one note; Cmaj7's fundamentals (131–330 Hz)
    and their harmonics interleave densely, splitting one note's harmonics across bands gives each band its own
    alignment (the phasey artefact this rewrite removed), and steep crossovers at 200–400 Hz cost the latency lead.
  - **Most promising:** a phase-vocoder path used *only* in the untracked state the engine already detects, crossfaded
    with the time-domain path. Every bin scales by the same ratio, so per-note error goes to ~0 regardless of voicing.
    Cost is latency on that path alone (~21 ms for a 1024-sample window), which would still sit under Archetype's
    43.9 ms worst case while tracked notes stay at 1–5 ms.
- **Attacks.** Until the tracker locks (~200 ms after a pick) grains jump by the fallback quantum: level is correct
  but harmonics smear, which is most of the remaining spectral-distance gap on real material. Restarting the analysis
  frame at each onset and running it 8× faster fixes that (G3 +5 spectral distance 3.26 → 1.23 dB, near Archetype's
  1.18) at no measurable CPU cost — but it mis-locks badly on sustained wind material (sax FM roughness 4 → 33 cents
  at −12), so it is **off** in this build. The knobs (`yin_burst`, `yin_onset_restart` and gating variants) remain in
  `variants/smooth` for a future trigger that can tell a pick from an articulation.
- **CPU.** The overlap-add alignment search is far heavier than the old splicer: worst block 80–82 % of a 32-sample
  period on the desktop, against 22 % before. This will not fit a Cortex-M33 as is. Options: a shorter fine window,
  fixed-point/CMSIS-DSP correlation, or spreading the search across blocks. Measure on the target first.
- **Real material.** The only real recording is a 5.3 s sax loop, and its roughness figures come from a single 0.9 s
  voiced phrase, so small timing changes flip them. Longer DI guitar takes — chords, strums, riffs — are needed
  before trusting any sax-only verdict.

## Listening test

`tools/listen_plugin`, **Shift Listen**: VST3 and Standalone, one Transpose knob, running this exact `shift.cpp`,
bit-exact against the harness apart from a reported 32-sample FIFO delay (`python tools/smoke_plugin.py` → PASS).
Build, install and A/B setup: `tools/listen_plugin/README.md`. Run the DAW at 48 kHz, and re-copy the bundle to
`C:\Program Files\Common Files\VST3\` after every rebuild.

## Reproducing

```powershell
python analysis/selftest.py                                               # metrics self-validation
python analysis/run_suite.py --target shift:current --suite full --compare reference/baseline.json
python analysis/run_suite.py --target shift:current --suite real --compare "results/vst-Archetype Misha Mansoor X/<real run>"
python analysis/real_audio.py <recording.wav> --targets archetype,shift:current --mono-f0
python analysis/sweep_table.py results/shift-current/<run> --shifts
python tools/retime.py current --from-run results/shift-current/<run> --top 16 --repeats 3
```
