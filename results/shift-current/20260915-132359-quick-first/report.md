# Report: shift:current / quick

Settings: default  
Shifts: [-12, -7, 7, 12]

## Scorecard
```
== shift:current [default]  (40 cases)
metric                      cand
lat_ms                     10.38
lat_ms_max                 32.89
lat_jitter_ms               3.20
pitch_err_cents             0.19
pitch_err_p95_cents         0.37
if_dev_cents                0.72
track_err_cents             4.53
poly_pitch_err_cents        3.54
sinad_db                   62.50
sinad_poly_db              38.52
subharm_db                -84.63
hf_junk_db               -106.44
am_pp_db                    0.03
am_rate_hz                 48.14
lsd_db                      1.92
pre_echo_db              -135.25
flam_db                     1.34
attack_smear                0.90
disc_db                    -0.05
level_db                    0.10
nonfinite                   0.00
cpu_ns_per_sample         390.44
cpu_worst_block_pct        84.51

run: results\shift-current\20260915-132359-quick-first  jobs=40  render 0s  process 2s  measure 7s
```

## [default] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 12.05 | 17.91 | 0.17 | 0.68 | 8.37 | 61.48 | 42.64 | 0.01 | 2.00 | 0.99 | 1.03 | -106.76 | 0.18 |
| -7 | 9.14 | 17.26 | 0.38 | 1.08 | 3.87 | 63.14 | 38.25 | 0.03 | 1.61 | 0.75 | 1.06 | -144.28 | 0.08 |
| 7 | 12.09 | 13.97 | 0.13 | 0.49 | 3.26 | 61.55 | 37.18 | 0.04 | 1.67 | 0.72 | 0.59 | -144.85 | 0.08 |
| 12 | 10.38 | 32.89 | 0.08 | 0.64 | 2.61 | 63.84 | 36.00 | 0.03 | 2.40 | 2.91 | 0.93 | -145.10 | 0.06 |

## [default] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| burst | 11.63 | 12.73 | - | - | - | - | - | - | 3.18 | 0.13 | 0.88 | -214.55 | 0.22 |
| chord | 3.24 | 32.89 | - | - | - | - | 28.76 | - | 2.59 | 4.05 | 0.81 | -46.29 | 0.31 |
| dyad | 8.63 | 9.80 | - | - | - | - | 48.27 | - | 2.45 | - | - | - | 0.15 |
| harmonic | 11.43 | 12.40 | 0.04 | 0.10 | - | 72.25 | - | 0.00 | 0.71 | - | - | - | 0.02 |
| pluck | 12.27 | 16.18 | 0.40 | 1.45 | - | 49.25 | - | 0.07 | 0.91 | 0.39 | 0.93 | -190.44 | 0.07 |
| sine | 11.17 | 16.60 | 0.05 | 0.30 | - | 70.88 | - | 0.00 | 0.52 | - | - | - | 0.03 |
| staccato | 5.95 | 17.91 | - | - | - | - | - | - | 4.63 | 1.74 | 0.96 | -34.52 | 0.10 |
| vibrato | 11.49 | 15.92 | - | - | 4.53 | - | - | - | 2.80 | - | - | - | 0.02 |

## [default] worst cases

- **sinad_db**: pluck_E2 +12 (41.9), pluck_E2 -7 (42.0), pluck_E2 +7 (44.2), pluck_G3 +7 (48.2), pluck_G3 -7 (48.3)
- **sinad_poly_db**: chord_Emaj +12 (24.1), chord_Emaj +7 (27.1), chord_Emaj -7 (28.8), chord_Emaj -12 (35.1), dyad_fifth_A2 +7 (47.3)
- **if_dev_cents**: pluck_E2 -7 (4.0), pluck_E2 +12 (2.9), sine_A4 -12 (1.8), pluck_E2 +7 (1.5), pluck_G3 -7 (1.2)
- **am_pp_db**: pluck_E2 +7 (0.1), pluck_E2 -7 (0.1), pluck_E2 +12 (0.1), pluck_G3 +7 (0.1), pluck_G3 +12 (0.1)
- **flam_db**: chord_Emaj +12 (7.4), staccato_E3 +12 (5.9), chord_Emaj -12 (4.0), chord_Emaj -7 (3.1), chord_Emaj +7 (1.7)
- **lat_ms_max**: chord_Emaj +12 (32.9), staccato_E3 -12 (17.9), chord_Emaj -7 (17.3), sine_E2 -12 (16.6), pluck_G3 -12 (16.2)
- **lsd_db**: staccato_E3 +12 (7.3), staccato_E3 +7 (4.6), burst_A4 +12 (4.2), staccato_E3 -12 (3.9), chord_Emaj -12 (3.4)
- **track_err_cents**: vibrato_D4 -12 (8.4), vibrato_D4 -7 (3.9), vibrato_D4 +7 (3.3), vibrato_D4 +12 (2.6)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
