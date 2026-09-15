# Report: shift:current / full

Settings: default  
Shifts: [-12, -7, -5, -1, 1, 5, 7, 12]

## Scorecard
```
== shift:current [default]  (313 cases)  vs vst:Archetype Misha Mansoor X:Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0
metric                      cand       ref   delta+  ok
lat_ms                      3.66      8.16    +4.50  ok
lat_ms_max                 46.31     45.15    -1.16  XX
lat_jitter_ms               3.96      4.31    +0.35  ok
lat_reported_ms                -      1.75           .
pitch_err_cents             0.06      0.27    +0.22  ok
pitch_err_p95_cents         0.09      0.48    +0.39  ok
if_dev_cents                0.17      1.83    +1.65  ok
track_err_cents             3.85      6.29    +2.44  ok
poly_pitch_err_cents        8.67      8.06    -0.60  ok
sinad_db                   68.13     42.98   +25.15  ok
sinad_poly_db              32.83     26.58    +6.25  ok
subharm_db                -96.52    -77.19   +19.33  ok
hf_junk_db               -106.87    -79.57   +27.30  ok
am_pp_db                    0.08      0.28    +0.20  ok
am_rate_hz                 11.66     12.58           .
lsd_db                      2.02      2.64    +0.62  ok
pre_echo_db              -135.26   -124.54   +10.72  ok
flam_db                     2.07      2.46    +0.39  ok
attack_smear                0.97      0.88    -0.09  ok
disc_db                     0.04      1.06    +1.01  ok
level_db                    0.35      0.35    +0.00  ok
silence_dbfs             -200.00   -200.00    -0.00  ok
nonfinite                   0.00      0.00    -0.00  ok
cpu_ns_per_sample         264.50         -           .
cpu_worst_block_pct       226.86         -           .
-- 1 metric(s) worse than ref beyond tolerance

run: results\shift-current\20260915-192359-full-folded-final  jobs=313  render 0s  process 31s  measure 58s
```

## [default] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 4.71 | 46.31 | 0.13 | 0.19 | 8.43 | 75.19 | 31.03 | 0.05 | 2.63 | 0.97 | 1.02 | -124.87 | 0.42 |
| -7 | 3.38 | 25.32 | 0.10 | 0.27 | 3.53 | 65.98 | 31.28 | 0.03 | 1.94 | 0.65 | 0.99 | -146.05 | 0.29 |
| -5 | 2.93 | 33.73 | 0.04 | 0.19 | 3.82 | 65.33 | 32.08 | 0.02 | 1.69 | 0.86 | 0.74 | -147.34 | 0.34 |
| -1 | 2.61 | 16.13 | 0.01 | 0.10 | 2.40 | 67.67 | 39.39 | 0.03 | 0.97 | 0.52 | 0.80 | -142.82 | 0.22 |
| 1 | 1.62 | 24.45 | 0.01 | 0.06 | 2.18 | 68.26 | 39.56 | 0.10 | 1.14 | 3.55 | 0.94 | -135.15 | 0.25 |
| 5 | 4.01 | 22.91 | 0.03 | 0.19 | 2.19 | 65.96 | 31.28 | 0.10 | 2.17 | 2.57 | 0.96 | -117.07 | 0.26 |
| 7 | 4.01 | 22.97 | 0.05 | 0.18 | 2.17 | 67.25 | 28.77 | 0.11 | 2.44 | 3.79 | 0.99 | -135.48 | 0.37 |
| 12 | 3.87 | 34.16 | 0.10 | 0.20 | 6.06 | 69.43 | 29.30 | 0.22 | 3.16 | 3.63 | 1.32 | -133.28 | 0.63 |

## [default] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 3.27 | 7.78 | - | - | 0.04 | - | - | - | 0.51 | - | - | - | 0.01 |
| burst | 3.38 | 24.45 | - | - | - | - | - | - | 2.50 | 5.36 | 1.25 | -199.04 | 0.28 |
| chord | 4.07 | 46.31 | - | - | - | - | 26.34 | - | 2.47 | 2.05 | 1.13 | -44.07 | 0.20 |
| click | 3.77 | 19.79 | - | - | - | - | - | - | 5.07 | 0.00 | 0.28 | -106.82 | 2.64 |
| decay | 3.92 | 5.36 | 0.01 | 0.06 | - | 67.94 | - | 0.33 | 0.48 | - | - | - | 0.14 |
| dyad | 3.64 | 22.48 | - | - | - | - | 38.03 | - | 3.59 | - | - | - | 0.21 |
| harmonic | 3.67 | 8.06 | 0.01 | 0.06 | - | 70.86 | - | 0.00 | 0.51 | - | - | - | 0.01 |
| noise | 3.24 | 7.38 | - | - | - | - | - | - | 5.06 | - | - | - | 1.72 |
| pluck | 4.07 | 8.27 | 0.15 | 0.42 | - | 61.16 | - | 0.18 | 0.71 | 1.07 | 0.81 | -201.47 | 0.15 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 3.53 | 8.90 | 0.01 | 0.05 | - | 72.87 | - | 0.00 | 0.31 | - | - | - | 0.01 |
| staccato | 2.73 | 15.92 | - | - | - | - | - | - | 3.60 | 1.19 | 1.05 | -37.55 | 1.79 |
| sweep | 5.88 | 14.61 | - | - | 11.18 | - | - | - | 9.01 | - | - | - | 0.04 |
| vibrato | 3.45 | 7.93 | - | - | 2.09 | - | - | - | 1.18 | - | - | - | 0.01 |

## [default] worst cases

- **sinad_db**: pluck_A2 -1 (49.9), pluck_E2 +12 (50.2), pluck_E2 +1 (50.3), pluck_E2 -1 (50.7), pluck_E2 -7 (50.8)
- **sinad_poly_db**: dyad_min2_E4 +7 (-10.3), dyad_min2_E4 +12 (-9.8), dyad_min2_E4 -12 (-9.2), dyad_min2_E4 -7 (-2.7), dyad_min2_E4 +5 (-2.5)
- **if_dev_cents**: pluck_E2 -7 (2.1), pluck_E2 +12 (2.0), pluck_E2 +5 (1.7), pluck_E2 +7 (1.6), pluck_E2 -5 (1.0)
- **am_pp_db**: decay_A3 +12 (1.0), pluck_E4 +7 (0.7), decay_A3 +5 (0.7), pluck_G3 +12 (0.7), pluck_E4 +12 (0.6)
- **flam_db**: burst_E5_saw +5 (23.4), burst_E5_saw +12 (16.9), burst_A4 +7 (16.6), burst_E3 +1 (15.9), burst_A4 +1 (14.4)
- **lat_ms_max**: chord_Emaj -12 (46.3), chord_Cmaj7 +12 (34.2), chord_Emaj -5 (33.7), chord_Amin -12 (27.9), chord_Emaj -7 (25.3)
- **lsd_db**: dyad_min2_E4 +12 (18.6), dyad_min2_E4 +7 (15.4), sweep_80_2k +12 (15.1), sweep_80_2k +7 (12.5), sweep_80_2k -12 (12.3)
- **track_err_cents**: sweep_80_2k -12 (29.0), sweep_80_2k +12 (22.1), sweep_80_2k -5 (11.5), sweep_80_2k -7 (7.8), sweep_80_2k +7 (6.6)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
