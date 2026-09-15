# Report: shift:env_match2 / full

Settings: default  
Shifts: [-12, -7, -5, -1, 1, 5, 7, 12]

## Scorecard
```
== shift:env_match2 [default]  (313 cases)  vs vst:Archetype Misha Mansoor X:Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0
metric                      cand       ref   delta+  ok
lat_ms                     11.70      8.16    -3.54  XX
lat_ms_max                 60.96     45.15   -15.80  XX
lat_jitter_ms               4.07      4.31    +0.24  ok
lat_reported_ms                -      1.75           .
pitch_err_cents             0.20      0.27    +0.08  ok
pitch_err_p95_cents         0.25      0.48    +0.23  ok
if_dev_cents                0.30      1.83    +1.52  ok
track_err_cents             6.05      6.29    +0.24  ok
poly_pitch_err_cents        8.38      8.06    -0.32  ok
sinad_db                   68.29     42.98   +25.31  ok
sinad_poly_db              32.86     26.58    +6.28  ok
subharm_db                -98.08    -77.19   +20.88  ok
hf_junk_db               -109.27    -79.57   +29.70  ok
am_pp_db                    0.05      0.28    +0.23  ok
am_rate_hz                 11.69     12.58           .
lsd_db                      1.80      2.64    +0.84  ok
pre_echo_db              -140.20   -124.54   +15.67  ok
flam_db                     1.14      2.46    +1.32  ok
attack_smear                0.93      0.88    -0.05  ok
disc_db                     0.07      1.06    +0.98  ok
level_db                    0.38      0.35    -0.04  ok
silence_dbfs             -200.00   -200.00    -0.00  ok
nonfinite                   0.00      0.00    -0.00  ok
cpu_ns_per_sample         275.92         -           .
cpu_worst_block_pct        43.71         -           .
-- 2 metric(s) worse than ref beyond tolerance

run: results\shift-env_match2\20260915-150812-full-vsArchetype  jobs=313  render 0s  process 32s  measure 55s
```

## [default] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 15.81 | 60.96 | 0.17 | 0.25 | 13.58 | 75.99 | 32.72 | 0.03 | 2.56 | 0.75 | 1.18 | -112.21 | 0.36 |
| -7 | 11.81 | 30.25 | 0.14 | 0.31 | 5.53 | 65.69 | 30.51 | 0.02 | 1.85 | 0.75 | 1.03 | -143.42 | 0.31 |
| -5 | 10.46 | 40.53 | 0.13 | 0.26 | 6.20 | 65.75 | 31.95 | 0.01 | 1.57 | 0.77 | 0.80 | -142.34 | 0.32 |
| -1 | 8.39 | 22.18 | 0.89 | 0.93 | 2.41 | 68.33 | 39.01 | 0.04 | 1.04 | 0.66 | 0.89 | -150.03 | 0.26 |
| 1 | 7.12 | 36.86 | 0.01 | 0.11 | 1.54 | 67.74 | 37.89 | 0.03 | 0.78 | 0.87 | 0.71 | -145.72 | 0.20 |
| 5 | 11.80 | 46.50 | 0.04 | 0.17 | 4.03 | 65.55 | 31.36 | 0.07 | 1.60 | 1.83 | 0.90 | -147.12 | 0.41 |
| 7 | 13.69 | 28.19 | 0.12 | 0.25 | 6.26 | 66.51 | 29.85 | 0.06 | 2.08 | 0.91 | 0.90 | -140.02 | 0.47 |
| 12 | 16.09 | 22.36 | 0.09 | 0.15 | 8.87 | 70.79 | 29.61 | 0.16 | 2.87 | 2.56 | 1.06 | -140.79 | 0.72 |

## [default] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 11.86 | 19.10 | - | - | 0.18 | - | - | - | 0.67 | - | - | - | 0.02 |
| burst | 11.96 | 17.11 | - | - | - | - | - | - | 2.06 | 0.10 | 1.17 | -167.65 | 0.23 |
| chord | 7.22 | 60.96 | - | - | - | - | 26.13 | - | 2.45 | 3.67 | 1.30 | -61.91 | 0.18 |
| click | 12.01 | 17.01 | - | - | - | - | - | - | 3.47 | 0.00 | 0.00 | -179.46 | 3.97 |
| decay | 12.92 | 16.69 | 0.15 | 0.19 | - | 67.72 | - | 0.23 | 0.38 | - | - | - | 0.16 |
| dyad | 12.93 | 21.72 | - | - | - | - | 38.25 | - | 3.14 | - | - | - | 0.22 |
| harmonic | 11.96 | 19.23 | 0.15 | 0.18 | - | 71.78 | - | 0.00 | 0.49 | - | - | - | 0.02 |
| noise | 11.52 | 18.50 | - | - | - | - | - | - | 5.03 | - | - | - | 1.81 |
| pluck | 11.75 | 19.59 | 0.29 | 0.54 | - | 60.08 | - | 0.12 | 0.60 | 0.30 | 0.73 | -206.23 | 0.17 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 11.98 | 19.76 | 0.15 | 0.18 | - | 73.70 | - | 0.00 | 0.27 | - | - | - | 0.02 |
| staccato | 6.81 | 18.12 | - | - | - | - | - | - | 3.47 | 0.68 | 0.94 | -37.92 | 1.63 |
| sweep | 13.66 | 25.90 | - | - | 17.79 | - | - | - | 6.73 | - | - | - | 0.08 |
| vibrato | 11.63 | 19.25 | - | - | 3.12 | - | - | - | 1.22 | - | - | - | 0.02 |

## [default] worst cases

- **sinad_db**: pluck_E2 -7 (47.3), pluck_E2 +1 (49.2), pluck_E2 -1 (49.7), pluck_A2 +1 (51.0), pluck_A2 -1 (51.0)
- **sinad_poly_db**: dyad_min2_E4 +12 (-13.1), dyad_min2_E4 +7 (-10.8), dyad_min2_E4 -12 (-6.4), dyad_min2_E4 +5 (-2.2), dyad_min2_E4 -7 (-1.0)
- **if_dev_cents**: pluck_E2 -7 (2.7), pluck_E2 -12 (1.2), pluck_A2 -5 (1.1), pluck_A2 -1 (1.1), pluck_E2 -1 (1.0)
- **am_pp_db**: decay_A3 +12 (0.8), pluck_E4 +12 (0.5), pluck_A2 +12 (0.4), pluck_E2 +12 (0.3), decay_A3 +7 (0.3)
- **flam_db**: chord_E5power +5 (15.9), chord_E5power +12 (12.6), chord_Cmaj7 +12 (10.1), chord_Emaj +5 (8.3), chord_Emaj +12 (7.8)
- **lat_ms_max**: chord_Emaj -12 (61.0), chord_Cmaj7 +5 (46.5), chord_Emaj -5 (40.5), chord_Amin +1 (36.9), chord_Amin +5 (36.0)
- **lsd_db**: dyad_min2_E4 +12 (16.8), dyad_min2_E4 +7 (12.9), sweep_80_2k +12 (12.7), noise_white -12 (10.4), click -12 (9.6)
- **track_err_cents**: sweep_80_2k -12 (42.9), sweep_80_2k +12 (23.9), sweep_80_2k -5 (21.1), sweep_80_2k +7 (17.2), sweep_80_2k -7 (16.4)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
