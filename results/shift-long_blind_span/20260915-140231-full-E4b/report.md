# Report: shift:long_blind_span / full

Settings: default  
Shifts: [-12, -7, -5, -1, 1, 5, 7, 12]

## Scorecard
```
== shift:long_blind_span [default]  (313 cases)  vs vst:TransBoost=0,Quality=1
metric                      cand       ref   delta+  ok
lat_ms                     11.53     41.06   +29.53  ok
lat_ms_max                 49.35     68.04   +18.69  ok
lat_jitter_ms               3.95      6.04    +2.09  ok
lat_reported_ms                -     42.67           .
pitch_err_cents             0.19      0.00    -0.19  ok
pitch_err_p95_cents         0.25      0.00    -0.25  ok
if_dev_cents                0.42      0.31    -0.11  ok
track_err_cents             5.49      4.72    -0.77  ok
poly_pitch_err_cents        9.81      1.43    -8.38  XX
sinad_db                   63.95     81.04   -17.09  XX
sinad_poly_db              32.54     27.86    +4.68  ok
subharm_db                -94.67   -126.94   -32.27  ok
hf_junk_db               -108.95   -114.47    -5.53  ok
am_pp_db                    0.04      0.02    -0.02  ok
am_rate_hz                 49.05     13.95           .
lsd_db                      2.00      3.68    +1.68  ok
pre_echo_db              -140.68    -18.84  +121.84  ok
flam_db                     3.36      1.24    -2.12  XX
attack_smear                0.87      1.80    +0.93  ok
disc_db                     0.06      0.08    +0.02  ok
level_db                    0.31      3.16    +2.85  ok
silence_dbfs             -200.00   -200.00    -0.00  ok
nonfinite                   0.00      0.00    -0.00  ok
cpu_ns_per_sample         296.45         -           .
cpu_worst_block_pct        68.97         -           .
-- 3 metric(s) worse than ref beyond tolerance

run: results\shift-long_blind_span\20260915-140231-full-E4b  jobs=313  render 0s  process 35s  measure 47s
```

## [default] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 15.73 | 44.89 | 0.15 | 0.30 | 12.65 | 74.45 | 30.79 | 0.01 | 2.53 | 0.66 | 1.03 | -125.95 | 0.22 |
| -7 | 11.80 | 28.33 | 0.13 | 0.61 | 5.21 | 60.26 | 31.23 | 0.03 | 1.76 | 0.91 | 0.96 | -149.11 | 0.17 |
| -5 | 10.46 | 40.53 | 0.13 | 0.50 | 6.09 | 60.31 | 31.84 | 0.04 | 1.47 | 0.66 | 0.79 | -148.69 | 0.17 |
| -1 | 8.39 | 22.18 | 0.89 | 0.95 | 2.27 | 65.27 | 38.38 | 0.05 | 0.95 | 0.66 | 0.89 | -149.95 | 0.16 |
| 1 | 7.02 | 49.35 | 0.01 | 0.06 | 1.79 | 64.67 | 40.27 | 0.12 | 1.61 | 4.94 | 0.79 | -131.55 | 0.51 |
| 5 | 11.50 | 31.87 | 0.04 | 0.26 | 3.98 | 60.45 | 30.86 | 0.04 | 2.09 | 4.17 | 0.79 | -138.11 | 0.37 |
| 7 | 13.45 | 41.29 | 0.13 | 0.44 | 4.61 | 61.45 | 28.67 | 0.04 | 2.43 | 6.70 | 0.71 | -139.96 | 0.38 |
| 12 | 15.93 | 33.24 | 0.06 | 0.23 | 7.35 | 64.72 | 28.28 | 0.02 | 3.16 | 8.16 | 0.96 | -142.14 | 0.49 |

## [default] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 11.83 | 19.10 | - | - | 0.18 | - | - | - | 0.75 | - | - | - | 0.03 |
| burst | 12.07 | 16.92 | - | - | - | - | - | - | 3.47 | 9.87 | 0.96 | -206.92 | 0.25 |
| chord | 8.54 | 44.89 | - | - | - | - | 24.51 | - | 2.40 | 3.34 | 1.26 | -58.85 | 0.15 |
| click | 23.09 | 32.50 | - | - | - | - | - | - | 7.63 | 0.00 | 0.00 | -80.90 | 5.24 |
| decay | 12.90 | 16.61 | 0.15 | 0.36 | - | 55.68 | - | 0.07 | 0.46 | - | - | - | 0.09 |
| dyad | 12.15 | 43.31 | - | - | - | - | 38.96 | - | 3.12 | - | - | - | 0.05 |
| harmonic | 11.92 | 19.23 | 0.15 | 0.18 | - | 71.69 | - | 0.00 | 0.62 | - | - | - | 0.02 |
| noise | 12.20 | 49.35 | - | - | - | - | - | - | 5.07 | - | - | - | 1.81 |
| pluck | 11.65 | 19.60 | 0.27 | 0.86 | - | 49.57 | - | 0.11 | 0.72 | 1.51 | 0.71 | -206.14 | 0.13 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 12.13 | 20.19 | 0.15 | 0.19 | - | 73.25 | - | 0.00 | 0.32 | - | - | - | 0.02 |
| staccato | 6.87 | 16.94 | - | - | - | - | - | - | 3.02 | 0.87 | 0.83 | -38.50 | 0.16 |
| sweep | 12.35 | 25.77 | - | - | 15.43 | - | - | - | 5.72 | - | - | - | 0.07 |
| vibrato | 11.52 | 19.31 | - | - | 3.18 | - | - | - | 1.16 | - | - | - | 0.02 |

## [default] worst cases

- **sinad_db**: pluck_E2 +7 (41.8), pluck_E2 -7 (42.1), pluck_E2 +5 (42.6), pluck_E2 -5 (42.7), pluck_A2 +1 (44.2)
- **sinad_poly_db**: dyad_min2_E4 +12 (-14.4), dyad_min2_E4 +7 (-7.5), dyad_min2_E4 -12 (-6.2), dyad_min2_E4 +5 (-1.5), dyad_min2_E4 -7 (-0.7)
- **if_dev_cents**: pluck_E2 -7 (3.4), pluck_A2 -7 (2.5), pluck_A2 -12 (2.3), pluck_E2 +7 (2.2), pluck_E2 -5 (1.9)
- **am_pp_db**: pluck_E4 +1 (0.4), pluck_B3 +1 (0.3), pluck_G3 +1 (0.3), pluck_D3 +1 (0.3), decay_A3 +1 (0.3)
- **flam_db**: burst_E3 +12 (37.4), burst_E3 +7 (33.2), burst_E3 +1 (30.5), burst_E3 +5 (29.6), burst_E5_saw +12 (25.9)
- **lat_ms_max**: noise_pink +1 (49.4), chord_Amin -12 (44.9), dyad_fifth_E2 +1 (43.3), chord_Cmaj7 +1 (43.2), chord_Amin +1 (43.1)
- **lsd_db**: dyad_min2_E4 +12 (16.8), dyad_min2_E4 +7 (13.4), click +5 (13.1), click +1 (11.9), click +7 (11.1)
- **track_err_cents**: sweep_80_2k -12 (38.8), sweep_80_2k -5 (20.7), sweep_80_2k +12 (17.4), sweep_80_2k -7 (14.8), sweep_80_2k +5 (11.6)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
