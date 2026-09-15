# Report: shift:down_margin / full

Settings: default  
Shifts: [-12, -7, -5, -1, 1, 5, 7, 12]

## Scorecard
```
== shift:down_margin [default]  (313 cases)  vs vst:TransBoost=0,Quality=1
metric                      cand       ref   delta+  ok
lat_ms                     10.37     41.06   +30.69  ok
lat_ms_max                 35.95     68.04   +32.09  ok
lat_jitter_ms               3.01      6.04    +3.03  ok
lat_reported_ms                -     42.67           .
pitch_err_cents             0.20      0.00    -0.20  ok
pitch_err_p95_cents         0.26      0.00    -0.26  ok
if_dev_cents                0.42      0.31    -0.12  ok
track_err_cents             4.80      4.72    -0.08  ok
poly_pitch_err_cents       11.87      1.43   -10.44  XX
sinad_db                   64.00     81.04   -17.04  XX
sinad_poly_db              26.72     27.86    -1.14  XX
subharm_db                -94.80   -126.94   -32.14  ok
hf_junk_db               -108.15   -114.47    -6.33  ok
am_pp_db                    0.03      0.02    -0.01  ok
am_rate_hz                 51.37     13.95           .
lsd_db                      1.80      3.68    +1.88  ok
pre_echo_db              -148.03    -18.84  +129.19  ok
flam_db                     1.20      1.24    +0.04  ok
attack_smear                0.88      1.80    +0.92  ok
disc_db                     0.09      0.08    -0.01  ok
level_db                    0.26      3.16    +2.90  ok
silence_dbfs             -200.00   -200.00    -0.00  ok
nonfinite                   0.00      0.00    -0.00  ok
cpu_ns_per_sample         304.58         -           .
cpu_worst_block_pct        75.48         -           .
-- 3 metric(s) worse than ref beyond tolerance

run: results\shift-down_margin\20260915-135437-full-E2  jobs=313  render 0s  process 35s  measure 45s
```

## [default] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 15.85 | 35.95 | 0.20 | 0.29 | 12.54 | 74.45 | 26.11 | 0.01 | 2.47 | 0.76 | 1.09 | -124.31 | 0.25 |
| -7 | 11.88 | 30.25 | 0.14 | 0.60 | 5.16 | 60.15 | 25.82 | 0.03 | 1.71 | 0.78 | 1.03 | -153.01 | 0.15 |
| -5 | 10.46 | 24.34 | 0.13 | 0.47 | 6.02 | 60.55 | 25.55 | 0.04 | 1.46 | 0.81 | 0.79 | -149.22 | 0.18 |
| -1 | 8.39 | 22.18 | 0.89 | 0.95 | 2.27 | 65.27 | 33.19 | 0.05 | 0.93 | 0.66 | 0.89 | -149.95 | 0.17 |
| 1 | 7.12 | 35.70 | 0.01 | 0.12 | 1.51 | 64.61 | 32.96 | 0.05 | 0.77 | 1.01 | 0.59 | -145.73 | 0.21 |
| 5 | 10.44 | 34.37 | 0.05 | 0.26 | 2.97 | 60.45 | 25.32 | 0.04 | 1.67 | 2.31 | 0.86 | -156.35 | 0.32 |
| 7 | 11.89 | 17.90 | 0.11 | 0.36 | 4.47 | 61.53 | 21.90 | 0.04 | 2.24 | 1.15 | 0.77 | -152.85 | 0.37 |
| 12 | 10.20 | 32.89 | 0.07 | 0.35 | 3.45 | 64.95 | 22.93 | 0.02 | 3.14 | 2.10 | 0.99 | -152.84 | 0.41 |

## [default] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 11.15 | 19.10 | - | - | 0.18 | - | - | - | 0.63 | - | - | - | 0.02 |
| burst | 11.00 | 16.93 | - | - | - | - | - | - | 2.38 | 0.11 | 1.03 | -209.70 | 0.18 |
| chord | 6.60 | 35.95 | - | - | - | - | 21.74 | - | 2.49 | 3.93 | 1.22 | -61.20 | 0.11 |
| click | 11.06 | 15.63 | - | - | - | - | - | - | 3.42 | 0.00 | 0.00 | -179.92 | 3.74 |
| decay | 10.74 | 15.60 | 0.15 | 0.35 | - | 55.45 | - | 0.04 | 0.40 | - | - | - | 0.06 |
| dyad | 10.99 | 21.72 | - | - | - | - | 30.71 | - | 3.48 | - | - | - | 0.10 |
| harmonic | 10.29 | 19.23 | 0.15 | 0.18 | - | 71.79 | - | 0.00 | 0.48 | - | - | - | 0.01 |
| noise | 9.28 | 18.50 | - | - | - | - | - | - | 4.93 | - | - | - | 1.90 |
| pluck | 11.20 | 19.59 | 0.29 | 0.88 | - | 49.49 | - | 0.09 | 0.60 | 0.39 | 0.72 | -206.25 | 0.07 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 10.67 | 19.76 | 0.15 | 0.18 | - | 73.43 | - | 0.00 | 0.26 | - | - | - | 0.02 |
| staccato | 6.86 | 16.51 | - | - | - | - | - | - | 2.93 | 0.40 | 0.85 | -38.60 | 0.14 |
| sweep | 10.83 | 25.90 | - | - | 14.23 | - | - | - | 5.75 | - | - | - | 0.07 |
| vibrato | 9.96 | 19.25 | - | - | 2.39 | - | - | - | 1.07 | - | - | - | 0.01 |

## [default] worst cases

- **sinad_db**: pluck_E2 +12 (41.9), pluck_E2 -7 (42.1), pluck_E2 +5 (42.7), pluck_E2 -5 (42.7), pluck_E2 +1 (43.9)
- **sinad_poly_db**: dyad_min2_E4 +7 (-12.2), dyad_min2_E4 +12 (-11.7), dyad_min2_E4 -12 (-6.2), dyad_min2_E4 +5 (-1.5), dyad_min2_E4 -7 (-0.9)
- **if_dev_cents**: pluck_E2 -7 (3.5), pluck_E2 +12 (2.9), pluck_A2 -7 (2.1), pluck_E2 -5 (1.9), pluck_E2 -12 (1.8)
- **am_pp_db**: pluck_E2 +1 (0.2), pluck_E2 -1 (0.2), pluck_A2 -1 (0.2), pluck_A2 +1 (0.2), pluck_E4 -1 (0.2)
- **flam_db**: chord_E5power +5 (15.9), chord_Amin +12 (10.3), chord_Cmaj7 +5 (9.1), chord_Cmaj7 +7 (8.7), chord_Amin -1 (7.7)
- **lat_ms_max**: chord_Amin -12 (36.0), chord_Amin +1 (35.7), chord_Amin +5 (34.4), chord_Cmaj7 +1 (33.1), chord_Emaj +12 (32.9)
- **lsd_db**: dyad_min2_E4 +12 (17.0), dyad_min2_E4 +7 (13.7), sweep_80_2k +12 (10.9), noise_white -12 (10.3), click -12 (9.6)
- **track_err_cents**: sweep_80_2k -12 (38.6), sweep_80_2k -5 (20.5), sweep_80_2k -7 (14.7), sweep_80_2k +7 (12.4), sweep_80_2k +12 (10.5)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
