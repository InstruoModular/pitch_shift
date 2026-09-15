# Experiment log

Compare column is vs the run named in "vs". Deltas: + is better.

| ID | Date | Variant | Hypothesis | Change | Key deltas (vs) | Verdict |
|----|------|---------|------------|--------|-----------------|---------|
| E0 | 2026-09-15 | current | baseline gap vs VST Q1/TB0 (full suite) | none | lat 9.3 vs 41.1 ms (+32); attack_smear 0.87 vs 1.80; pre-echo none vs -18.8 dB; level 0.26 vs 3.16; lsd 2.0 vs 3.7 — LOSES: pitch_err 1.36 vs 0.00 c; poly_pitch 12.2 vs 1.4 c; if_dev 4.7 vs 0.31 c; sinad 57.6 vs 81.0 dB; sinad_poly 26.6 vs 27.9; cpu worst block 3471 % (vs results/shift-current/20260915-134406-full-baseline) | baseline |
| E1 | 2026-09-15 | min_period | high notes (>800 Hz) detune because min_period=60 clamps period below YIN's 32-smp floor, so tracked splice search misses every period multiple | min_period 60 -> 32 (= yin_min_lag*dec_factor) | pitch_err 1.36->0.61 c, if_dev 4.68->1.55 c, am 0.17->0.04, sinad 57.6->59.0, cpu worst 70->62 %; A5 -12 fixed (451.5->439.7 Hz) but D6/E6/G6 downshifts still flat (-5..-28 c) and C6 -12 regressed (exact -> -6.6 c) (vs E0, results/shift-min_period/20260915-135126-full-E1) | keep (partial) |
| E2 | 2026-09-15 | down_margin | downshift splice target lag-d gets clamped to lag_floor whenever d > grain (lag_lo sits exactly on the floor) -> non-period jump -> always-flat detune on downshifts | from min_period: on ratio<1 add search reach (tracked max(p/8,12), blind min(0.35 grain,64)) + max_fine_reach to lag_lo | pitch_err 0.61->0.20 c (ok vs ref), if_dev 1.55->0.42 c (ok), sinad 59.0->64.0, flam 1.33->1.20 (beats ref 1.24), lat 9.3->10.4 ms, cpu worst 62->75 %; every high sine exact (A5..G6, all shifts, 51-78 dB) (vs E1, results/shift-down_margin/20260915-135437-full-E2) | keep — current best |
