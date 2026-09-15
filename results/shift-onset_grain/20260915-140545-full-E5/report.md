# Report: shift:onset_grain / full

Settings: default  
Shifts: [-12, -7, -5, -1, 1, 5, 7, 12]

## Scorecard
```
== shift:onset_grain [default]  (313 cases)  vs vst:TransBoost=0,Quality=1
metric                      cand       ref   delta+  ok
lat_ms                     11.55     41.06   +29.52  ok
lat_ms_max                 35.98     68.04   +32.06  ok
lat_jitter_ms               3.05      6.04    +2.99  ok
lat_reported_ms                -     42.67           .
pitch_err_cents             0.20      0.00    -0.20  ok
pitch_err_p95_cents         0.27      0.00    -0.27  ok
if_dev_cents                0.43      0.31    -0.12  ok
track_err_cents             5.58      4.72    -0.86  ok
poly_pitch_err_cents        9.19      1.43    -7.76  XX
sinad_db                   63.87     81.04   -17.17  XX
sinad_poly_db              32.13     27.86    +4.27  ok
subharm_db                -94.87   -126.94   -32.07  ok
hf_junk_db               -109.13   -114.47    -5.34  ok
am_pp_db                    0.03      0.02    -0.01  ok
am_rate_hz                 53.56     13.95           .
lsd_db                      1.76      3.68    +1.92  ok
pre_echo_db              -147.49    -18.84  +128.65  ok
flam_db                     1.08      1.24    +0.16  ok
attack_smear                0.87      1.80    +0.93  ok
disc_db                     0.06      0.08    +0.02  ok
level_db                    0.26      3.16    +2.90  ok
silence_dbfs             -200.00   -200.00    -0.00  ok
nonfinite                   0.00      0.00    -0.00  ok
cpu_ns_per_sample         327.13         -           .
cpu_worst_block_pct       270.84         -           .
-- 2 metric(s) worse than ref beyond tolerance

run: results\shift-onset_grain\20260915-140545-full-E5  jobs=313  render 0s  process 41s  measure 88s
```

## [default] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 15.85 | 35.95 | 0.22 | 0.37 | 12.61 | 73.87 | 31.23 | 0.01 | 2.47 | 0.76 | 1.09 | -123.47 | 0.22 |
| -7 | 11.88 | 30.25 | 0.13 | 0.60 | 5.15 | 60.17 | 31.66 | 0.03 | 1.73 | 0.76 | 1.00 | -149.36 | 0.15 |
| -5 | 10.46 | 24.34 | 0.13 | 0.47 | 6.00 | 60.57 | 31.54 | 0.04 | 1.48 | 0.81 | 0.79 | -148.82 | 0.16 |
| -1 | 8.39 | 22.18 | 0.89 | 0.95 | 2.27 | 65.27 | 38.38 | 0.05 | 0.95 | 0.66 | 0.89 | -149.95 | 0.16 |
| 1 | 7.12 | 35.70 | 0.01 | 0.12 | 1.49 | 64.61 | 37.77 | 0.05 | 0.76 | 1.01 | 0.59 | -144.48 | 0.21 |
| 5 | 11.80 | 35.98 | 0.04 | 0.24 | 3.83 | 60.31 | 29.88 | 0.04 | 1.61 | 1.78 | 0.88 | -154.57 | 0.32 |
| 7 | 13.64 | 26.01 | 0.12 | 0.41 | 5.96 | 61.50 | 28.47 | 0.04 | 2.13 | 0.80 | 0.77 | -155.10 | 0.36 |
| 12 | 16.09 | 21.60 | 0.11 | 0.30 | 7.31 | 64.67 | 28.09 | 0.02 | 2.95 | 2.05 | 0.96 | -154.18 | 0.51 |

## [default] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 11.86 | 19.10 | - | - | 0.18 | - | - | - | 0.67 | - | - | - | 0.02 |
| burst | 12.14 | 16.99 | - | - | - | - | - | - | 2.48 | 0.03 | 1.02 | -207.06 | 0.21 |
| chord | 7.89 | 35.98 | - | - | - | - | 23.79 | - | 2.34 | 3.43 | 1.21 | -61.13 | 0.11 |
| click | 12.01 | 17.01 | - | - | - | - | - | - | 3.48 | 0.00 | 0.00 | -179.47 | 3.97 |
| decay | 12.92 | 16.69 | 0.15 | 0.36 | - | 55.38 | - | 0.04 | 0.40 | - | - | - | 0.08 |
| dyad | 12.78 | 21.72 | - | - | - | - | 38.80 | - | 3.19 | - | - | - | 0.05 |
| harmonic | 11.96 | 19.23 | 0.15 | 0.18 | - | 71.40 | - | 0.00 | 0.47 | - | - | - | 0.02 |
| noise | 11.42 | 18.50 | - | - | - | - | - | - | 5.02 | - | - | - | 1.81 |
| pluck | 11.75 | 19.59 | 0.31 | 0.90 | - | 49.29 | - | 0.09 | 0.59 | 0.30 | 0.73 | -206.23 | 0.09 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 11.98 | 19.76 | 0.15 | 0.19 | - | 73.59 | - | 0.00 | 0.26 | - | - | - | 0.02 |
| staccato | 6.86 | 16.95 | - | - | - | - | - | - | 2.95 | 0.81 | 0.83 | -38.64 | 0.14 |
| sweep | 13.66 | 25.90 | - | - | 15.83 | - | - | - | 5.71 | - | - | - | 0.07 |
| vibrato | 11.63 | 19.25 | - | - | 3.15 | - | - | - | 1.13 | - | - | - | 0.02 |

## [default] worst cases

- **sinad_db**: pluck_E2 -7 (42.0), pluck_E2 +5 (42.6), pluck_E2 +7 (42.7), pluck_E2 -5 (42.7), pluck_E2 +12 (42.8)
- **sinad_poly_db**: dyad_min2_E4 +12 (-12.6), dyad_min2_E4 +7 (-10.4), dyad_min2_E4 -12 (-5.8), dyad_min2_E4 +5 (-1.3), dyad_min2_E4 -7 (-0.9)
- **if_dev_cents**: pluck_E2 -7 (3.5), pluck_E2 -12 (3.0), pluck_E2 +12 (2.4), pluck_A2 -7 (2.1), pluck_E2 +7 (2.1)
- **am_pp_db**: pluck_E2 +1 (0.2), pluck_E2 -1 (0.2), pluck_A2 -1 (0.2), pluck_A2 +1 (0.2), pluck_E2 +5 (0.2)
- **flam_db**: chord_E5power +5 (16.2), chord_Cmaj7 +12 (9.7), chord_Emaj +5 (8.1), chord_Emaj +12 (7.8), chord_Amin -1 (7.7)
- **lat_ms_max**: chord_Amin +5 (36.0), chord_Amin -12 (36.0), chord_Amin +1 (35.7), chord_Cmaj7 +1 (33.1), chord_Cmaj7 -7 (30.3)
- **lsd_db**: dyad_min2_E4 +12 (17.0), dyad_min2_E4 +7 (13.7), sweep_80_2k +12 (10.8), noise_white -12 (10.3), click -12 (9.6)
- **track_err_cents**: sweep_80_2k -12 (38.8), sweep_80_2k -5 (20.4), sweep_80_2k +12 (17.3), sweep_80_2k +7 (15.9), sweep_80_2k -7 (14.7)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
