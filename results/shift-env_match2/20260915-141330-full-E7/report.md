# Report: shift:env_match2 / full

Settings: default  
Shifts: [-12, -7, -5, -1, 1, 5, 7, 12]

## Scorecard
```
== shift:env_match2 [default]  (313 cases)  vs shift:onset_grain:default
metric                      cand       ref   delta+  ok
lat_ms                     11.58     11.55    -0.04  ok
lat_ms_max                 35.95     35.98    +0.03  ok
lat_jitter_ms               3.00      3.05    +0.05  ok
pitch_err_cents             0.20      0.20    -0.00  ok
pitch_err_p95_cents         0.27      0.27    -0.00  ok
if_dev_cents                0.32      0.43    +0.11  ok
track_err_cents             5.58      5.58    +0.00  ok
poly_pitch_err_cents        9.19      9.19    +0.00  ok
sinad_db                   67.98     63.87    +4.11  ok
sinad_poly_db              33.14     32.13    +1.02  ok
subharm_db                -98.08    -94.87    +3.20  ok
hf_junk_db               -109.20   -109.13    +0.07  ok
am_pp_db                    0.01      0.03    +0.02  ok
am_rate_hz                 41.31     53.56           .
lsd_db                      1.77      1.76    -0.01  ok
pre_echo_db              -147.31   -147.49    -0.18  ok
flam_db                     1.09      1.08    -0.01  ok
attack_smear                0.89      0.87    -0.02  ok
disc_db                     0.06      0.06    -0.00  ok
level_db                    0.40      0.26    -0.14  ok
silence_dbfs             -200.00   -200.00    -0.00  ok
nonfinite                   0.00      0.00    -0.00  ok
cpu_ns_per_sample         308.37    327.13   +18.76  ok
cpu_worst_block_pct        84.36    270.84  +186.48  ok
-- 0 metric(s) worse than ref beyond tolerance

run: results\shift-env_match2\20260915-141330-full-E7  jobs=313  render 0s  process 38s  measure 67s
```

## [default] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 15.85 | 35.95 | 0.22 | 0.36 | 12.61 | 74.66 | 32.69 | 0.01 | 2.53 | 0.84 | 1.09 | -123.23 | 0.37 |
| -7 | 11.98 | 30.25 | 0.13 | 0.31 | 5.15 | 65.17 | 32.64 | 0.01 | 1.80 | 0.63 | 1.01 | -147.35 | 0.33 |
| -5 | 10.46 | 24.34 | 0.13 | 0.24 | 6.00 | 65.56 | 32.59 | 0.01 | 1.51 | 0.80 | 0.79 | -148.91 | 0.31 |
| -1 | 8.39 | 22.18 | 0.89 | 0.93 | 2.27 | 68.29 | 39.12 | 0.02 | 0.99 | 0.66 | 0.89 | -150.01 | 0.25 |
| 1 | 7.12 | 35.70 | 0.01 | 0.11 | 1.49 | 67.74 | 38.56 | 0.03 | 0.76 | 1.01 | 0.59 | -144.44 | 0.21 |
| 5 | 11.80 | 21.91 | 0.04 | 0.16 | 3.83 | 65.43 | 30.39 | 0.01 | 1.62 | 1.77 | 0.88 | -154.63 | 0.46 |
| 7 | 13.64 | 26.01 | 0.12 | 0.25 | 5.97 | 66.44 | 29.53 | 0.01 | 2.09 | 0.86 | 0.84 | -155.25 | 0.51 |
| 12 | 16.09 | 21.60 | 0.11 | 0.19 | 7.31 | 70.58 | 29.65 | 0.01 | 2.87 | 2.18 | 1.04 | -154.64 | 0.79 |

## [default] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 11.86 | 19.10 | - | - | 0.18 | - | - | - | 0.66 | - | - | - | 0.02 |
| burst | 12.14 | 17.00 | - | - | - | - | - | - | 2.46 | 0.05 | 1.02 | -207.00 | 0.20 |
| chord | 8.45 | 35.95 | - | - | - | - | 26.11 | - | 2.34 | 3.38 | 1.21 | -60.13 | 0.19 |
| click | 12.01 | 17.01 | - | - | - | - | - | - | 3.48 | 0.00 | 0.00 | -179.47 | 3.97 |
| decay | 12.92 | 16.69 | 0.15 | 0.19 | - | 67.71 | - | 0.04 | 0.37 | - | - | - | 0.23 |
| dyad | 12.78 | 21.72 | - | - | - | - | 38.77 | - | 3.17 | - | - | - | 0.24 |
| harmonic | 11.96 | 19.23 | 0.15 | 0.18 | - | 71.40 | - | 0.00 | 0.47 | - | - | - | 0.02 |
| noise | 11.42 | 18.50 | - | - | - | - | - | - | 5.02 | - | - | - | 1.81 |
| pluck | 11.75 | 19.59 | 0.31 | 0.58 | - | 59.58 | - | 0.03 | 0.55 | 0.30 | 0.73 | -206.23 | 0.26 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 11.98 | 19.76 | 0.15 | 0.19 | - | 73.59 | - | 0.00 | 0.25 | - | - | - | 0.02 |
| staccato | 6.79 | 16.95 | - | - | - | - | - | - | 3.42 | 1.01 | 0.99 | -39.27 | 1.73 |
| sweep | 13.66 | 25.90 | - | - | 15.83 | - | - | - | 5.70 | - | - | - | 0.07 |
| vibrato | 11.63 | 19.25 | - | - | 3.15 | - | - | - | 1.13 | - | - | - | 0.01 |

## [default] worst cases

- **sinad_db**: pluck_E2 -7 (48.0), pluck_E2 +1 (49.2), pluck_E2 -1 (49.8), pluck_A2 -1 (50.7), pluck_A2 +1 (51.0)
- **sinad_poly_db**: dyad_min2_E4 +12 (-12.3), dyad_min2_E4 +7 (-10.6), dyad_min2_E4 -12 (-6.0), dyad_min2_E4 +5 (-1.2), dyad_min2_E4 -7 (-0.9)
- **if_dev_cents**: pluck_E2 -12 (3.0), pluck_E2 -7 (2.6), pluck_E2 +12 (1.7), pluck_A2 -1 (1.1), pluck_E2 -1 (1.0)
- **am_pp_db**: pluck_E4 +1 (0.1), pluck_E2 -1 (0.1), pluck_E2 +1 (0.1), pluck_A2 +1 (0.1), decay_A3 +1 (0.1)
- **flam_db**: chord_E5power +5 (16.2), chord_Cmaj7 +12 (9.7), chord_Emaj +5 (8.1), chord_Emaj +12 (7.8), chord_Amin -1 (7.7)
- **lat_ms_max**: chord_Amin -12 (36.0), chord_Amin +1 (35.7), chord_Cmaj7 +1 (33.1), chord_Cmaj7 -7 (30.3), chord_E5power +1 (30.2)
- **lsd_db**: dyad_min2_E4 +12 (17.3), dyad_min2_E4 +7 (13.6), sweep_80_2k +12 (10.8), noise_white -12 (10.3), click -12 (9.6)
- **track_err_cents**: sweep_80_2k -12 (38.8), sweep_80_2k -5 (20.4), sweep_80_2k +12 (17.3), sweep_80_2k +7 (15.9), sweep_80_2k -7 (14.7)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
