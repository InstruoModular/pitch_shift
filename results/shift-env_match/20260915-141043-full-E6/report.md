# Report: shift:env_match / full

Settings: default  
Shifts: [-12, -7, -5, -1, 1, 5, 7, 12]

## Scorecard
```
== shift:env_match [default]  (313 cases)  vs shift:onset_grain:default
metric                      cand       ref   delta+  ok
lat_ms                     11.65     11.55    -0.10  ok
lat_ms_max                 61.13     35.98   -25.15  XX
lat_jitter_ms               3.06      3.05    -0.01  ok
pitch_err_cents             0.20      0.20    +0.00  ok
pitch_err_p95_cents         0.27      0.27    -0.00  ok
if_dev_cents                0.37      0.43    +0.06  ok
track_err_cents             5.62      5.58    -0.04  ok
poly_pitch_err_cents        9.83      9.19    -0.64  ok
sinad_db                   57.71     63.87    -6.16  XX
sinad_poly_db              30.42     32.13    -1.71  XX
subharm_db                -91.66    -94.87    -3.21  ok
hf_junk_db               -105.74   -109.13    -3.39  ok
am_pp_db                    0.10      0.03    -0.07  ok
am_rate_hz                  2.74     53.56           .
lsd_db                      1.81      1.76    -0.05  ok
pre_echo_db              -145.86   -147.49    -1.63  ok
flam_db                     1.09      1.08    -0.01  ok
attack_smear                0.87      0.87    -0.00  ok
disc_db                     0.06      0.06    -0.00  ok
level_db                    0.50      0.26    -0.24  ok
silence_dbfs             -200.00   -200.00    -0.00  ok
nonfinite                   0.00      0.00    -0.00  ok
cpu_ns_per_sample         306.76    327.13   +20.37  ok
cpu_worst_block_pct        66.00    270.84  +204.84  ok
-- 3 metric(s) worse than ref beyond tolerance

run: results\shift-env_match\20260915-141043-full-E6  jobs=313  render 0s  process 37s  measure 58s
```

## [default] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 16.06 | 61.13 | 0.22 | 0.48 | 12.61 | 58.91 | 28.53 | 0.08 | 2.58 | 0.56 | 1.04 | -118.63 | 0.57 |
| -7 | 11.88 | 30.20 | 0.13 | 0.44 | 5.15 | 54.81 | 28.95 | 0.16 | 1.81 | 0.73 | 0.99 | -149.25 | 0.35 |
| -5 | 10.46 | 24.36 | 0.13 | 0.31 | 6.00 | 56.47 | 29.88 | 0.11 | 1.53 | 0.80 | 0.79 | -148.91 | 0.38 |
| -1 | 8.39 | 22.18 | 0.89 | 0.94 | 2.27 | 61.21 | 37.73 | 0.05 | 0.99 | 0.66 | 0.89 | -150.01 | 0.26 |
| 1 | 7.14 | 35.00 | 0.01 | 0.12 | 1.49 | 60.55 | 36.28 | 0.06 | 0.78 | 1.04 | 0.77 | -146.89 | 0.24 |
| 5 | 11.80 | 46.00 | 0.04 | 0.19 | 3.85 | 56.04 | 28.15 | 0.13 | 1.66 | 1.76 | 0.89 | -142.88 | 0.55 |
| 7 | 13.71 | 26.01 | 0.12 | 0.29 | 6.01 | 54.53 | 26.59 | 0.19 | 2.14 | 0.78 | 0.72 | -155.24 | 0.65 |
| 12 | 16.07 | 17.68 | 0.11 | 0.21 | 7.55 | 59.14 | 27.27 | 0.05 | 2.97 | 2.42 | 0.89 | -155.10 | 1.02 |

## [default] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 11.85 | 19.10 | - | - | 0.19 | - | - | - | 0.67 | - | - | - | 0.02 |
| burst | 12.14 | 17.02 | - | - | - | - | - | - | 2.45 | 0.04 | 1.02 | -207.01 | 0.21 |
| chord | 7.26 | 61.13 | - | - | - | - | 25.22 | - | 2.46 | 3.55 | 1.20 | -60.29 | 0.72 |
| click | 12.01 | 17.01 | - | - | - | - | - | - | 3.48 | 0.00 | 0.00 | -179.46 | 3.97 |
| decay | 12.91 | 16.67 | 0.15 | 0.24 | - | 55.96 | - | 0.14 | 0.38 | - | - | - | 0.32 |
| dyad | 12.64 | 21.72 | - | - | - | - | 34.59 | - | 3.16 | - | - | - | 0.31 |
| harmonic | 11.96 | 19.23 | 0.15 | 0.29 | - | 56.29 | - | 0.14 | 0.56 | - | - | - | 0.03 |
| noise | 11.51 | 18.72 | - | - | - | - | - | - | 5.07 | - | - | - | 1.55 |
| pluck | 11.89 | 19.59 | 0.31 | 0.62 | - | 54.72 | - | 0.10 | 0.56 | 0.25 | 0.74 | -202.28 | 0.41 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 11.97 | 19.77 | 0.15 | 0.22 | - | 62.17 | - | 0.06 | 0.28 | - | - | - | 0.02 |
| staccato | 6.74 | 16.91 | - | - | - | - | - | - | 3.48 | 0.83 | 0.83 | -39.25 | 2.14 |
| sweep | 13.71 | 25.88 | - | - | 15.97 | - | - | - | 5.73 | - | - | - | 0.07 |
| vibrato | 11.63 | 19.25 | - | - | 3.15 | - | - | - | 1.14 | - | - | - | 0.03 |

## [default] worst cases

- **sinad_db**: pluck_B3 -12 (43.0), harm_D3 -7 (44.5), sine_E5 +5 (45.8), sine_A4 +7 (45.9), harm_B3 -7 (45.9)
- **sinad_poly_db**: dyad_min2_E4 +12 (-12.2), dyad_min2_E4 +7 (-10.5), dyad_min2_E4 -12 (-5.9), dyad_min2_E4 +5 (-1.2), dyad_min2_E4 -7 (-1.0)
- **if_dev_cents**: pluck_E2 -12 (3.1), pluck_E2 -7 (2.7), pluck_E2 +12 (1.7), pluck_A2 -1 (1.2), pluck_B3 -12 (1.1)
- **am_pp_db**: harm_B3 -7 (0.5), harm_B3 +7 (0.4), harm_E2 +7 (0.4), decay_A3 +7 (0.4), harm_A2 -7 (0.4)
- **flam_db**: chord_E5power +5 (16.1), chord_Cmaj7 +12 (10.1), chord_Amin +12 (9.1), chord_Emaj +12 (8.4), chord_Emaj +5 (8.1)
- **lat_ms_max**: chord_Emaj -12 (61.1), chord_Cmaj7 +5 (46.0), chord_Cmaj7 -12 (41.7), chord_Amin -12 (36.0), chord_Amin +1 (35.0)
- **lsd_db**: dyad_min2_E4 +12 (16.8), dyad_min2_E4 +7 (13.4), sweep_80_2k +12 (10.9), noise_white -12 (10.4), click -12 (9.6)
- **track_err_cents**: sweep_80_2k -12 (38.8), sweep_80_2k -5 (20.4), sweep_80_2k +12 (18.2), sweep_80_2k +7 (16.1), sweep_80_2k -7 (14.7)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
