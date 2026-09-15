# Report: shift:min_period / full

Settings: default  
Shifts: [-12, -7, -5, -1, 1, 5, 7, 12]

## Scorecard
```
== shift:min_period [default]  (313 cases)  vs vst:TransBoost=0,Quality=1
metric                      cand       ref   delta+  ok
lat_ms                      9.32     41.06   +31.74  ok
lat_ms_max                 35.70     68.04   +32.35  ok
lat_jitter_ms               3.05      6.04    +2.99  ok
lat_reported_ms                -     42.67           .
pitch_err_cents             0.61      0.00    -0.61  XX
pitch_err_p95_cents         0.71      0.00    -0.70  ok
if_dev_cents                1.55      0.31    -1.24  XX
track_err_cents             4.65      4.72    +0.07  ok
poly_pitch_err_cents       12.24      1.43   -10.81  XX
sinad_db                   59.01     81.04   -22.03  XX
sinad_poly_db              26.60     27.86    -1.26  XX
subharm_db                -93.38   -126.94   -33.56  ok
hf_junk_db               -105.03   -114.47    -9.44  ok
am_pp_db                    0.04      0.02    -0.02  ok
am_rate_hz                 53.91     13.95           .
lsd_db                      1.99      3.68    +1.69  ok
pre_echo_db              -147.42    -18.84  +128.58  ok
flam_db                     1.33      1.24    -0.09  ok
attack_smear                0.87      1.80    +0.93  ok
disc_db                     0.08      0.08    -0.00  ok
level_db                    0.25      3.16    +2.91  ok
silence_dbfs             -200.00   -200.00    -0.00  ok
nonfinite                   0.00      0.00    -0.00  ok
cpu_ns_per_sample         304.97         -           .
cpu_worst_block_pct        62.49         -           .
-- 5 metric(s) worse than ref beyond tolerance

run: results\shift-min_period\20260915-135126-full-E1  jobs=313  render 0s  process 35s  measure 45s
```

## [default] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 12.53 | 22.41 | 1.89 | 3.80 | 12.42 | 57.35 | 25.89 | 0.02 | 3.25 | 1.15 | 1.04 | -121.29 | 0.23 |
| -7 | 9.45 | 31.33 | 0.98 | 2.96 | 4.89 | 53.44 | 25.73 | 0.04 | 2.13 | 1.13 | 1.02 | -150.40 | 0.16 |
| -5 | 8.21 | 23.24 | 0.77 | 2.81 | 5.43 | 53.83 | 26.10 | 0.05 | 1.77 | 1.15 | 0.79 | -150.15 | 0.14 |
| -1 | 6.62 | 20.42 | 1.01 | 1.71 | 2.03 | 55.93 | 31.94 | 0.08 | 0.94 | 0.61 | 0.86 | -149.78 | 0.17 |
| 1 | 7.12 | 35.70 | 0.01 | 0.12 | 1.51 | 64.61 | 32.96 | 0.05 | 0.77 | 1.01 | 0.59 | -145.73 | 0.21 |
| 5 | 10.44 | 34.37 | 0.05 | 0.26 | 2.97 | 60.45 | 25.32 | 0.04 | 1.67 | 2.31 | 0.86 | -156.35 | 0.32 |
| 7 | 11.89 | 17.90 | 0.11 | 0.36 | 4.47 | 61.53 | 21.90 | 0.04 | 2.24 | 1.15 | 0.77 | -152.85 | 0.37 |
| 12 | 10.20 | 32.89 | 0.07 | 0.35 | 3.45 | 64.95 | 22.93 | 0.02 | 3.14 | 2.10 | 0.99 | -152.84 | 0.41 |

## [default] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 9.66 | 15.78 | - | - | 0.19 | - | - | - | 0.59 | - | - | - | 0.02 |
| burst | 10.35 | 13.58 | - | - | - | - | - | - | 2.34 | 0.12 | 1.03 | -209.52 | 0.15 |
| chord | 5.78 | 35.70 | - | - | - | - | 21.80 | - | 2.45 | 4.04 | 1.18 | -61.41 | 0.13 |
| click | 10.09 | 12.51 | - | - | - | - | - | - | 3.42 | 0.00 | 0.00 | -180.83 | 3.44 |
| decay | 10.74 | 12.29 | 0.15 | 0.37 | - | 55.24 | - | 0.04 | 0.40 | - | - | - | 0.06 |
| dyad | 9.62 | 31.33 | - | - | - | - | 30.43 | - | 3.49 | - | - | - | 0.10 |
| harmonic | 10.09 | 15.89 | 0.21 | 0.33 | - | 68.45 | - | 0.00 | 0.55 | - | - | - | 0.01 |
| noise | 9.16 | 15.48 | - | - | - | - | - | - | 4.96 | - | - | - | 1.90 |
| pluck | 9.51 | 16.26 | 0.27 | 0.94 | - | 49.07 | - | 0.09 | 0.65 | 0.65 | 0.72 | -204.43 | 0.07 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 10.15 | 16.60 | 1.36 | 3.36 | - | 61.71 | - | 0.02 | 1.14 | - | - | - | 0.02 |
| staccato | 5.53 | 18.37 | - | - | - | - | - | - | 2.94 | 0.41 | 0.85 | -38.57 | 0.15 |
| sweep | 10.64 | 22.41 | - | - | 12.71 | - | - | - | 6.46 | - | - | - | 0.10 |
| vibrato | 9.51 | 15.92 | - | - | 2.84 | - | - | - | 1.51 | - | - | - | 0.02 |

## [default] worst cases

- **sinad_db**: sine_E6 -12 (-102.8), sine_E6 -7 (-26.7), sine_E6 -5 (8.2), sine_E6 -1 (16.8), sine_E5 -1 (40.2)
- **sinad_poly_db**: dyad_min2_E4 +7 (-12.2), dyad_min2_E4 +12 (-11.7), dyad_min2_E4 -12 (-2.7), dyad_min2_E4 +5 (-1.5), dyad_min2_E4 -7 (-1.2)
- **if_dev_cents**: sine_E6 -12 (52.8), sine_E6 -7 (36.2), sine_E6 -5 (33.1), sine_E6 -1 (10.7), sine_E5 -12 (5.8)
- **am_pp_db**: sine_E6 -1 (0.4), sine_E6 -5 (0.3), pluck_E2 +1 (0.2), pluck_E2 -1 (0.2), sine_E6 -7 (0.2)
- **flam_db**: chord_E5power +5 (15.9), chord_Amin +12 (10.3), chord_Cmaj7 +5 (9.1), chord_Cmaj7 +7 (8.7), chord_Amin -1 (7.6)
- **lat_ms_max**: chord_Amin +1 (35.7), chord_Amin +5 (34.4), chord_Cmaj7 +1 (33.1), chord_Emaj +12 (32.9), dyad_fourth_A2 -7 (31.3)
- **lsd_db**: dyad_min2_E4 +12 (17.0), sine_E6 -12 (14.7), dyad_min2_E4 +7 (13.7), sweep_80_2k -12 (11.0), sweep_80_2k +12 (10.9)
- **track_err_cents**: sweep_80_2k -12 (33.9), sweep_80_2k -5 (17.6), sweep_80_2k +7 (12.4), sweep_80_2k -7 (11.5), sweep_80_2k +12 (10.5)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
