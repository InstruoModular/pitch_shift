# Report: vst / quick

Settings: default  
Shifts: [-12, -7, 7, 12]

## Scorecard
```
== vst [default]  (40 cases)
metric                      cand
lat_ms                     40.66
lat_ms_max                 72.10
lat_jitter_ms               5.53
lat_reported_ms            42.67
pitch_err_cents             0.03
pitch_err_p95_cents         0.05
if_dev_cents               23.21
track_err_cents             9.92
poly_pitch_err_cents        5.87
sinad_db                   24.43
sinad_poly_db              16.76
subharm_db                -84.02
hf_junk_db                -89.08
am_pp_db                    0.76
am_rate_hz                 62.57
lsd_db                      4.81
pre_echo_db               -15.04
flam_db                     1.30
attack_smear                2.10
disc_db                     0.44
level_db                    3.45
nonfinite                   0.00

run: results\vst\20260915-131904-quick-q0  jobs=40  render 0s  process 1s  measure 8s
```

## [default] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 36.72 | 72.10 | 0.07 | 11.77 | 13.93 | 26.65 | 27.82 | 0.44 | 3.44 | 1.27 | 1.61 | -16.05 | 1.99 |
| -7 | 40.96 | 52.77 | 0.04 | 4.14 | 6.25 | 33.90 | 18.87 | 0.21 | 3.41 | 1.24 | 2.30 | -15.27 | 1.19 |
| 7 | 44.67 | 58.78 | 0.00 | 19.71 | 10.29 | 21.39 | 12.75 | 0.79 | 5.44 | 0.92 | 2.18 | -12.97 | 4.90 |
| 12 | 41.40 | 57.40 | 0.00 | 57.21 | 9.21 | 15.77 | 7.59 | 1.60 | 6.97 | 1.78 | 2.29 | -15.86 | 5.71 |

## [default] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| burst | 44.63 | 58.59 | - | - | - | - | - | - | 4.36 | 1.40 | 3.45 | -13.77 | 2.18 |
| chord | 48.35 | 72.10 | - | - | - | - | 15.34 | - | 5.25 | 1.16 | 1.35 | -12.91 | 2.80 |
| dyad | 25.75 | 38.00 | - | - | - | - | 18.17 | - | 5.25 | - | - | - | 2.92 |
| harmonic | 35.61 | 53.03 | 0.06 | 25.27 | - | 20.91 | - | 0.39 | 5.72 | - | - | - | 2.96 |
| pluck | 37.50 | 46.16 | 0.02 | 32.59 | - | 25.75 | - | 0.93 | 4.16 | 1.23 | 1.86 | -20.26 | 3.93 |
| sine | 41.57 | 58.78 | 0.02 | 12.79 | - | 24.86 | - | 0.78 | 3.43 | - | - | - | 4.93 |
| staccato | 40.10 | 54.48 | - | - | - | - | - | - | 7.19 | 1.48 | 1.96 | -8.00 | 2.90 |
| vibrato | 48.31 | 57.40 | - | - | 9.92 | - | - | - | 5.18 | - | - | - | 3.01 |

## [default] worst cases

- **sinad_db**: sine_E2 +12 (10.7), pluck_E2 +12 (11.1), sine_E2 +7 (13.6), pluck_G3 +12 (14.0), harm_A2 +12 (17.6)
- **sinad_poly_db**: chord_Emaj +12 (4.7), dyad_fifth_A2 +12 (10.5), dyad_fifth_A2 +7 (12.4), chord_Emaj +7 (13.1), chord_Emaj -7 (18.6)
- **if_dev_cents**: pluck_E2 +12 (120.8), pluck_G3 +12 (78.0), harm_A2 +12 (42.7), harm_A2 +7 (41.7), sine_E2 +12 (35.4)
- **am_pp_db**: pluck_E2 +12 (3.2), sine_E2 +12 (2.7), pluck_G3 +12 (1.6), sine_E2 +7 (0.9), sine_A4 -12 (0.9)
- **flam_db**: staccato_E3 +12 (3.5), pluck_E2 -7 (3.4), pluck_G3 +12 (3.3), chord_Emaj -12 (2.6), burst_A4 -12 (2.0)
- **lat_ms_max**: chord_Emaj -12 (72.1), sine_E2 +7 (58.8), burst_A4 -12 (58.6), vibrato_D4 +12 (57.4), sine_A4 +7 (55.1)
- **lsd_db**: sine_E2 +12 (10.3), harm_A2 +7 (8.4), pluck_E2 +12 (8.4), staccato_E3 +7 (7.9), harm_A2 +12 (7.8)
- **track_err_cents**: vibrato_D4 -12 (13.9), vibrato_D4 +7 (10.3), vibrato_D4 +12 (9.2), vibrato_D4 -7 (6.3)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
