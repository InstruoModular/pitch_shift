# Experiment log

Compare column is vs the run named in "vs". Deltas: + is better.

| ID | Date | Variant | Hypothesis | Change | Key deltas (vs) | Verdict |
|----|------|---------|------------|--------|-----------------|---------|
| E0 | 2026-09-15 | current | baseline gap vs VST Q1/TB0 (full suite) | none | lat 9.3 vs 41.1 ms (+32); attack_smear 0.87 vs 1.80; pre-echo none vs -18.8 dB; level 0.26 vs 3.16; lsd 2.0 vs 3.7 — LOSES: pitch_err 1.36 vs 0.00 c; poly_pitch 12.2 vs 1.4 c; if_dev 4.7 vs 0.31 c; sinad 57.6 vs 81.0 dB; sinad_poly 26.6 vs 27.9; cpu worst block 3471 % (vs results/shift-current/20260915-134406-full-baseline) | baseline |
| E1 | 2026-09-15 | min_period | high notes (>800 Hz) detune because min_period=60 clamps period below YIN's 32-smp floor, so tracked splice search misses every period multiple | min_period 60 -> 32 (= yin_min_lag*dec_factor) | | |
