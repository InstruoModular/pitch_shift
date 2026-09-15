# Report: vst / full

Settings: TransBoost=0,Quality=0, TransBoost=0,Quality=0.333333, TransBoost=0,Quality=0.666667, TransBoost=0,Quality=1  
Shifts: [-12, -7, -5, -1, 1, 5, 7, 12]

## Scorecard
```
== vst [TransBoost=0,Quality=0]  (313 cases)
metric                      cand
lat_ms                     39.88
lat_ms_max                 73.17
lat_jitter_ms               6.27
lat_reported_ms            42.67
pitch_err_cents             0.02
pitch_err_p95_cents         0.03
if_dev_cents               12.89
track_err_cents            10.35
poly_pitch_err_cents        1.88
sinad_db                   31.67
sinad_poly_db              21.40
subharm_db                -97.84
hf_junk_db                -97.76
am_pp_db                    0.43
am_rate_hz                 70.68
lsd_db                      4.51
pre_echo_db               -18.51
flam_db                     1.58
attack_smear                1.92
disc_db                     0.13
level_db                    3.73
silence_dbfs             -200.00
nonfinite                   0.00

== vst [TransBoost=0,Quality=0.333333]  (313 cases)
metric                      cand
lat_ms                     41.35
lat_ms_max                 77.00
lat_jitter_ms               6.58
lat_reported_ms            42.67
pitch_err_cents             0.00
pitch_err_p95_cents         0.01
if_dev_cents                0.63
track_err_cents             6.69
poly_pitch_err_cents        1.41
sinad_db                   54.31
sinad_poly_db              26.23
subharm_db               -114.38
hf_junk_db               -103.71
am_pp_db                    0.04
am_rate_hz                 32.50
lsd_db                      3.95
pre_echo_db               -18.34
flam_db                     1.23
attack_smear                1.91
disc_db                     0.26
level_db                    3.66
silence_dbfs             -200.00
nonfinite                   0.00

== vst [TransBoost=0,Quality=0.666667]  (313 cases)
metric                      cand
lat_ms                     40.99
lat_ms_max                 67.71
lat_jitter_ms               6.20
lat_reported_ms            42.67
pitch_err_cents             0.00
pitch_err_p95_cents         0.00
if_dev_cents                0.34
track_err_cents             4.68
poly_pitch_err_cents        1.64
sinad_db                   69.94
sinad_poly_db              27.35
subharm_db               -122.89
hf_junk_db               -109.01
am_pp_db                    0.03
am_rate_hz                 32.61
lsd_db                      3.69
pre_echo_db               -17.62
flam_db                     1.34
attack_smear                1.93
disc_db                     0.11
level_db                    3.30
silence_dbfs             -200.00
nonfinite                   0.00

== vst [TransBoost=0,Quality=1]  (313 cases)
metric                      cand
lat_ms                     41.06
lat_ms_max                 68.04
lat_jitter_ms               6.04
lat_reported_ms            42.67
pitch_err_cents             0.00
pitch_err_p95_cents         0.00
if_dev_cents                0.31
track_err_cents             4.72
poly_pitch_err_cents        1.43
sinad_db                   81.04
sinad_poly_db              27.86
subharm_db               -126.94
hf_junk_db               -114.47
am_pp_db                    0.02
am_rate_hz                 13.95
lsd_db                      3.68
pre_echo_db               -18.84
flam_db                     1.24
attack_smear                1.80
disc_db                     0.08
level_db                    3.16
silence_dbfs             -200.00
nonfinite                   0.00

run: results\vst\20260915-133134-full-qsweep  jobs=1252  render 0s  process 163s  measure 352s
```

## Settings comparison

| setting | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| TransBoost=0,Quality=0 | 39.88 | 73.17 | 0.02 | 12.89 | 10.35 | 31.67 | 21.40 | 0.43 | 4.51 | 1.58 | 1.92 | -18.51 | 3.73 |
| TransBoost=0,Quality=0.333333 | 41.35 | 77.00 | 0.00 | 0.63 | 6.69 | 54.31 | 26.23 | 0.04 | 3.95 | 1.23 | 1.91 | -18.34 | 3.66 |
| TransBoost=0,Quality=0.666667 | 40.99 | 67.71 | 0.00 | 0.34 | 4.68 | 69.94 | 27.35 | 0.03 | 3.69 | 1.34 | 1.93 | -17.62 | 3.30 |
| TransBoost=0,Quality=1 | 41.06 | 68.04 | 0.00 | 0.31 | 4.72 | 81.04 | 27.86 | 0.02 | 3.68 | 1.24 | 1.80 | -18.84 | 3.16 |

## [TransBoost=0,Quality=0] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 35.55 | 72.10 | 0.05 | 19.80 | 12.73 | 30.60 | 22.00 | 0.29 | 3.96 | 1.92 | 1.99 | -22.22 | 1.28 |
| -7 | 40.22 | 73.17 | 0.03 | 6.01 | 7.00 | 34.53 | 19.16 | 0.22 | 3.36 | 1.45 | 1.80 | -18.10 | 1.93 |
| -5 | 33.48 | 61.56 | 0.02 | 4.57 | 11.15 | 33.43 | 17.13 | 0.24 | 3.72 | 1.30 | 1.89 | -20.55 | 3.40 |
| -1 | 46.91 | 65.34 | 0.01 | 3.70 | 4.87 | 36.61 | 35.91 | 0.23 | 4.14 | 0.85 | 1.83 | -13.27 | 4.25 |
| 1 | 47.20 | 62.03 | 0.02 | 1.82 | 5.37 | 42.85 | 36.87 | 0.10 | 3.40 | 1.89 | 1.90 | -17.42 | 2.92 |
| 5 | 38.80 | 61.23 | 0.00 | 10.37 | 8.40 | 31.74 | 17.04 | 0.35 | 4.90 | 1.82 | 2.09 | -20.87 | 5.04 |
| 7 | 39.74 | 61.37 | 0.00 | 13.68 | 14.05 | 24.82 | 13.02 | 0.76 | 5.94 | 1.95 | 1.96 | -20.55 | 5.39 |
| 12 | 40.64 | 57.46 | 0.00 | 43.19 | 19.25 | 18.75 | 10.05 | 1.23 | 6.68 | 1.47 | 1.88 | -15.12 | 5.60 |

## [TransBoost=0,Quality=0] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 39.49 | 50.54 | - | - | 6.67 | - | - | - | 5.34 | - | - | - | 3.83 |
| burst | 44.79 | 64.05 | - | - | - | - | - | - | 4.92 | 1.99 | 3.20 | -15.70 | 3.33 |
| chord | 37.94 | 72.10 | - | - | - | - | 20.69 | - | 4.66 | 2.38 | 1.02 | -25.29 | 3.22 |
| click | 45.63 | 56.81 | - | - | - | - | - | - | 7.30 | 0.94 | 1.68 | 1.17 | 5.61 |
| decay | 45.02 | 51.55 | 0.00 | 46.01 | - | 27.55 | - | 0.67 | 5.24 | - | - | - | 4.79 |
| dyad | 37.73 | 73.17 | - | - | - | - | 21.97 | - | 4.90 | - | - | - | 3.39 |
| harmonic | 38.05 | 60.90 | 0.03 | 13.10 | - | 30.28 | - | 0.29 | 4.88 | - | - | - | 3.34 |
| noise | 39.34 | 45.71 | - | - | - | - | - | - | 6.73 | - | - | - | 4.10 |
| pluck | 39.03 | 57.15 | 0.02 | 13.35 | - | 31.89 | - | 0.41 | 3.71 | 1.02 | 1.91 | -20.96 | 3.65 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 39.78 | 65.34 | 0.01 | 6.75 | - | 33.28 | - | 0.51 | 2.51 | - | - | - | 4.39 |
| staccato | 38.07 | 58.00 | - | - | - | - | - | - | 6.21 | 1.35 | 1.93 | -11.66 | 3.22 |
| sweep | 43.20 | 55.54 | - | - | 17.19 | - | - | - | 2.61 | - | - | - | 3.52 |
| vibrato | 44.95 | 54.57 | - | - | 8.77 | - | - | - | 5.03 | - | - | - | 4.07 |

## [TransBoost=0,Quality=0] worst cases

- **sinad_db**: sine_E2 +12 (2.1), pluck_E2 +12 (11.8), pluck_B3 +12 (13.5), pluck_G3 +12 (13.9), harm_E2 +12 (14.8)
- **sinad_poly_db**: chord_Emaj +12 (4.2), chord_Cmaj7 +12 (4.7), dyad_min2_E4 +7 (5.1), chord_E5power +12 (5.2), chord_Cmaj7 +7 (6.0)
- **if_dev_cents**: decay_A3 -12 (229.0), sine_E2 +12 (124.1), harm_E2 +12 (122.3), pluck_E2 +12 (97.7), pluck_G3 +12 (86.7)
- **am_pp_db**: sine_E2 +12 (5.4), pluck_E2 +12 (2.8), sine_E3 +7 (2.1), decay_A3 +12 (2.1), decay_A3 +7 (1.8)
- **flam_db**: chord_Amin +1 (8.6), burst_E3 -12 (8.3), chord_Amin +7 (6.8), chord_E5power +7 (6.4), pluck_G3 +12 (5.4)
- **lat_ms_max**: dyad_min2_E4 -7 (73.2), chord_Emaj -12 (72.1), chord_E5power -12 (65.4), sine_E6 -1 (65.3), burst_A4 -1 (64.0)
- **lsd_db**: click -12 (13.4), noise_white -12 (12.7), sine_E2 +12 (11.5), noise_pink -12 (11.1), harm_E2 +12 (10.5)
- **track_err_cents**: sweep_80_2k +12 (33.4), sweep_80_2k -12 (30.0), vibrato_A2 -5 (22.2), vibrato_A2 +12 (20.5), vibrato_A2 +7 (19.5)

## [TransBoost=0,Quality=0.333333] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 35.46 | 62.62 | 0.01 | 0.40 | 6.49 | 53.99 | 26.32 | 0.03 | 3.49 | 1.41 | 1.73 | -22.72 | 1.05 |
| -7 | 38.16 | 57.49 | 0.00 | 0.68 | 9.11 | 52.52 | 24.20 | 0.06 | 3.32 | 1.02 | 2.14 | -20.07 | 2.52 |
| -5 | 42.05 | 77.00 | 0.01 | 0.32 | 7.12 | 53.71 | 22.42 | 0.05 | 4.32 | 0.92 | 1.54 | -16.02 | 3.57 |
| -1 | 43.10 | 59.09 | 0.00 | 0.13 | 2.80 | 60.27 | 45.57 | 0.02 | 4.11 | 1.61 | 2.08 | -20.88 | 3.86 |
| 1 | 42.81 | 60.16 | 0.00 | 0.22 | 6.62 | 61.11 | 42.46 | 0.03 | 3.30 | 1.60 | 2.02 | -20.78 | 3.03 |
| 5 | 41.83 | 61.85 | 0.00 | 0.24 | 2.78 | 56.43 | 19.50 | 0.02 | 4.16 | 0.91 | 2.09 | -15.63 | 4.40 |
| 7 | 37.27 | 63.60 | 0.00 | 1.92 | 12.88 | 52.06 | 16.78 | 0.05 | 4.57 | 1.11 | 1.54 | -17.23 | 5.57 |
| 12 | 47.24 | 57.43 | 0.00 | 1.18 | 5.76 | 44.38 | 12.62 | 0.06 | 4.34 | 1.25 | 2.15 | -13.39 | 5.31 |

## [TransBoost=0,Quality=0.333333] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 37.46 | 49.68 | - | - | 0.16 | - | - | - | 4.51 | - | - | - | 3.71 |
| burst | 45.34 | 62.85 | - | - | - | - | - | - | 4.24 | 1.15 | 3.04 | -14.05 | 3.03 |
| chord | 25.56 | 77.00 | - | - | - | - | 24.26 | - | 4.48 | 2.10 | 1.13 | -25.96 | 3.16 |
| click | 44.90 | 52.98 | - | - | - | - | - | - | 7.27 | 0.29 | 1.72 | 0.27 | 5.44 |
| decay | 51.54 | 56.32 | 0.00 | 0.28 | - | 54.97 | - | 0.02 | 3.34 | - | - | - | 4.21 |
| dyad | 39.53 | 59.09 | - | - | - | - | 27.81 | - | 4.42 | - | - | - | 3.44 |
| harmonic | 40.87 | 55.17 | 0.00 | 1.04 | - | 50.74 | - | 0.03 | 4.20 | - | - | - | 3.28 |
| noise | 37.40 | 44.27 | - | - | - | - | - | - | 6.75 | - | - | - | 3.92 |
| pluck | 41.07 | 55.38 | 0.00 | 0.81 | - | 54.33 | - | 0.08 | 3.25 | 0.80 | 1.83 | -20.39 | 3.51 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 43.08 | 61.79 | 0.00 | 0.19 | - | 57.14 | - | 0.02 | 1.71 | - | - | - | 4.74 |
| staccato | 36.39 | 57.23 | - | - | - | - | - | - | 5.94 | 1.36 | 2.11 | -12.68 | 3.36 |
| sweep | 42.64 | 63.60 | - | - | 17.07 | - | - | - | 1.96 | - | - | - | 3.29 |
| vibrato | 45.87 | 60.21 | - | - | 4.78 | - | - | - | 4.02 | - | - | - | 3.41 |

## [TransBoost=0,Quality=0.333333] worst cases

- **sinad_db**: harm_A2 +12 (32.2), harm_A2 -12 (32.9), pluck_E2 +7 (33.6), harm_E2 +7 (33.8), pluck_A2 +12 (33.9)
- **sinad_poly_db**: dyad_min2_E4 +7 (3.9), dyad_min2_E4 +12 (4.6), chord_Cmaj7 +12 (4.8), chord_Emaj +12 (5.5), chord_E5power +12 (6.5)
- **if_dev_cents**: harm_E2 +7 (13.6), pluck_E2 +7 (13.3), harm_A2 +12 (7.1), pluck_A2 +12 (6.1), harm_A2 -7 (4.6)
- **am_pp_db**: pluck_E2 +7 (0.3), pluck_G3 -5 (0.2), pluck_G3 +12 (0.2), pluck_G3 -7 (0.2), pluck_E2 +1 (0.2)
- **flam_db**: chord_Cmaj7 +1 (6.4), chord_Emaj -5 (5.9), chord_E5power +1 (5.7), chord_E5power -1 (4.9), burst_E3 -12 (4.6)
- **lat_ms_max**: chord_Amin -5 (77.0), chord_Cmaj7 -5 (65.2), sweep_80_2k +7 (63.6), burst_A4 +7 (62.8), burst_A4 -12 (62.6)
- **lsd_db**: click -12 (13.2), noise_white -12 (12.7), noise_pink -12 (11.1), noise_white +12 (8.5), noise_white +7 (8.0)
- **track_err_cents**: sweep_80_2k +7 (38.4), sweep_80_2k +1 (22.1), vibrato_A2 -7 (21.5), sweep_80_2k -5 (17.5), sweep_80_2k +12 (17.1)

## [TransBoost=0,Quality=0.666667] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 39.18 | 63.49 | 0.00 | 0.11 | 9.33 | 70.94 | 26.96 | 0.02 | 3.18 | 0.68 | 1.91 | -15.96 | 0.76 |
| -7 | 41.40 | 54.99 | 0.00 | 0.10 | 3.04 | 71.27 | 25.50 | 0.03 | 3.05 | 1.01 | 1.83 | -15.91 | 1.77 |
| -5 | 40.85 | 58.86 | 0.00 | 0.06 | 4.32 | 70.93 | 24.53 | 0.02 | 3.33 | 0.93 | 1.73 | -15.44 | 2.84 |
| -1 | 43.61 | 65.91 | 0.00 | 0.03 | 2.44 | 74.62 | 47.97 | 0.03 | 4.15 | 2.03 | 1.96 | -23.57 | 3.88 |
| 1 | 42.80 | 64.03 | 0.00 | 0.02 | 3.62 | 80.84 | 43.78 | 0.02 | 2.85 | 1.08 | 1.82 | -18.40 | 2.41 |
| 5 | 39.46 | 67.00 | 0.00 | 0.05 | 3.36 | 70.57 | 19.63 | 0.03 | 3.88 | 1.31 | 2.13 | -17.00 | 3.83 |
| 7 | 40.42 | 59.41 | 0.00 | 1.28 | 7.20 | 64.63 | 17.21 | 0.04 | 4.57 | 1.34 | 2.01 | -16.86 | 5.29 |
| 12 | 35.88 | 67.71 | 0.00 | 1.03 | 4.16 | 55.73 | 13.21 | 0.04 | 4.48 | 2.29 | 2.00 | -17.79 | 5.65 |

## [TransBoost=0,Quality=0.666667] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 45.95 | 53.20 | - | - | 0.04 | - | - | - | 3.98 | - | - | - | 2.64 |
| burst | 45.35 | 67.00 | - | - | - | - | - | - | 4.41 | 1.31 | 2.99 | -15.06 | 3.35 |
| chord | 38.68 | 67.71 | - | - | - | - | 24.77 | - | 4.18 | 1.98 | 1.22 | -22.70 | 2.94 |
| click | 44.23 | 58.75 | - | - | - | - | - | - | 7.23 | 0.17 | 1.66 | -0.21 | 5.34 |
| decay | 43.69 | 51.41 | 0.00 | 0.08 | - | 69.91 | - | 0.01 | 3.38 | - | - | - | 2.77 |
| dyad | 38.98 | 63.49 | - | - | - | - | 29.41 | - | 4.14 | - | - | - | 3.14 |
| harmonic | 40.33 | 55.05 | 0.00 | 0.58 | - | 62.88 | - | 0.01 | 3.82 | - | - | - | 3.28 |
| noise | 38.36 | 44.04 | - | - | - | - | - | - | 6.79 | - | - | - | 3.90 |
| pluck | 40.94 | 54.22 | 0.00 | 0.49 | - | 69.60 | - | 0.07 | 2.77 | 1.02 | 1.87 | -19.98 | 3.24 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 44.99 | 64.29 | 0.00 | 0.01 | - | 76.17 | - | 0.00 | 1.30 | - | - | - | 3.47 |
| staccato | 36.84 | 52.51 | - | - | - | - | - | - | 6.10 | 1.60 | 2.03 | -12.90 | 3.31 |
| sweep | 40.10 | 47.65 | - | - | 11.90 | - | - | - | 1.76 | - | - | - | 3.18 |
| vibrato | 41.42 | 54.97 | - | - | 3.39 | - | - | - | 3.71 | - | - | - | 3.16 |

## [TransBoost=0,Quality=0.666667] worst cases

- **sinad_db**: harm_A2 +12 (31.8), pluck_A2 +12 (33.0), harm_E2 +7 (35.6), pluck_E2 +7 (35.8), harm_E2 +12 (36.3)
- **sinad_poly_db**: chord_Cmaj7 +12 (5.1), chord_Emaj +12 (5.8), chord_Cmaj7 +7 (6.0), dyad_min2_E4 +7 (6.0), chord_E5power +12 (6.8)
- **if_dev_cents**: pluck_E2 +7 (9.4), harm_E2 +7 (9.2), harm_A2 +12 (7.0), pluck_A2 +12 (6.6), pluck_E2 +12 (2.0)
- **am_pp_db**: pluck_A2 +12 (0.3), pluck_G3 +5 (0.2), pluck_E2 +7 (0.2), pluck_D3 +7 (0.2), pluck_E4 -7 (0.2)
- **flam_db**: pluck_E4 +12 (6.7), staccato_E3_16 +12 (6.2), chord_Cmaj7 +12 (6.2), chord_Cmaj7 +1 (5.0), burst_E5_saw -1 (5.0)
- **lat_ms_max**: chord_Emaj +12 (67.7), burst_E5_saw +5 (67.0), burst_E5_saw -1 (65.9), sine_E5 -1 (64.3), chord_Emaj +1 (64.0)
- **lsd_db**: click -12 (13.2), noise_white -12 (12.7), noise_pink -12 (11.2), noise_white +12 (8.5), burst_E5_saw -1 (8.2)
- **track_err_cents**: sweep_80_2k -12 (21.0), sweep_80_2k +7 (19.2), vibrato_A2 -12 (14.2), sweep_80_2k +12 (10.5), sweep_80_2k +1 (10.0)

## [TransBoost=0,Quality=1] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 39.82 | 67.00 | 0.00 | 0.04 | 3.94 | 85.99 | 29.75 | 0.01 | 3.48 | 0.81 | 1.61 | -18.67 | 0.87 |
| -7 | 40.98 | 65.57 | 0.00 | 0.03 | 4.69 | 84.90 | 27.59 | 0.02 | 3.12 | 0.78 | 1.80 | -15.60 | 1.96 |
| -5 | 40.15 | 56.83 | 0.00 | 0.03 | 6.27 | 82.63 | 25.26 | 0.02 | 3.53 | 1.08 | 1.74 | -19.65 | 2.96 |
| -1 | 43.00 | 65.59 | 0.00 | 0.02 | 2.80 | 89.67 | 48.40 | 0.01 | 3.46 | 1.49 | 1.78 | -21.92 | 2.73 |
| 1 | 41.81 | 54.10 | 0.00 | 0.01 | 2.18 | 90.46 | 43.99 | 0.02 | 2.74 | 1.82 | 1.76 | -23.46 | 2.09 |
| 5 | 34.35 | 61.13 | 0.00 | 0.03 | 3.90 | 81.17 | 19.38 | 0.02 | 4.30 | 1.12 | 1.87 | -18.37 | 4.29 |
| 7 | 37.39 | 60.52 | 0.00 | 1.19 | 7.32 | 71.42 | 16.09 | 0.05 | 4.39 | 1.01 | 2.01 | -17.50 | 5.41 |
| 12 | 45.28 | 68.04 | 0.00 | 1.11 | 6.65 | 62.05 | 12.39 | 0.05 | 4.38 | 1.80 | 1.80 | -15.55 | 4.97 |

## [TransBoost=0,Quality=1] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 40.24 | 45.78 | - | - | 0.02 | - | - | - | 4.19 | - | - | - | 2.74 |
| burst | 44.09 | 67.00 | - | - | - | - | - | - | 4.27 | 1.55 | 2.83 | -16.75 | 3.00 |
| chord | 34.09 | 68.04 | - | - | - | - | 24.64 | - | 4.17 | 1.86 | 1.31 | -23.71 | 2.90 |
| click | 45.56 | 52.02 | - | - | - | - | - | - | 7.36 | 0.18 | 1.66 | 1.02 | 5.59 |
| decay | 42.33 | 46.01 | 0.00 | 0.01 | - | 83.23 | - | 0.01 | 3.28 | - | - | - | 2.17 |
| dyad | 36.33 | 63.50 | - | - | - | - | 30.43 | - | 4.17 | - | - | - | 2.98 |
| harmonic | 41.18 | 50.28 | 0.00 | 0.54 | - | 71.76 | - | 0.02 | 4.02 | - | - | - | 3.01 |
| noise | 39.47 | 54.38 | - | - | - | - | - | - | 6.82 | - | - | - | 3.91 |
| pluck | 41.68 | 52.93 | 0.00 | 0.46 | - | 76.87 | - | 0.06 | 2.71 | 0.84 | 1.57 | -21.85 | 2.97 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 42.45 | 65.59 | 0.00 | 0.01 | - | 92.57 | - | 0.00 | 1.16 | - | - | - | 3.29 |
| staccato | 36.20 | 56.37 | - | - | - | - | - | - | 6.00 | 1.28 | 1.98 | -13.15 | 3.34 |
| sweep | 40.08 | 47.98 | - | - | 10.86 | - | - | - | 1.68 | - | - | - | 3.08 |
| vibrato | 37.62 | 60.05 | - | - | 4.00 | - | - | - | 3.80 | - | - | - | 3.56 |

## [TransBoost=0,Quality=1] worst cases

- **sinad_db**: harm_A2 +12 (33.9), harm_E2 +7 (34.9), pluck_E2 +7 (35.2), harm_E2 +12 (35.3), pluck_A2 +12 (35.9)
- **sinad_poly_db**: dyad_min2_E4 +7 (3.3), chord_Cmaj7 +12 (4.8), dyad_min2_E4 +12 (5.1), chord_Emaj +12 (5.1), chord_E5power +12 (6.4)
- **if_dev_cents**: pluck_E2 +7 (9.5), harm_E2 +7 (9.2), harm_A2 +12 (5.3), pluck_A2 +12 (4.9), harm_E2 +12 (3.8)
- **am_pp_db**: pluck_E2 +7 (0.2), pluck_A2 +12 (0.2), harm_E2 +7 (0.2), harm_A2 +12 (0.2), pluck_D3 +7 (0.1)
- **flam_db**: chord_E5power +1 (7.5), burst_E3 +12 (6.4), chord_E5power -1 (5.4), pluck_G3 +7 (4.8), pluck_E4 -1 (4.6)
- **lat_ms_max**: chord_Amin +12 (68.0), burst_A4 -12 (67.0), sine_E6 -1 (65.6), sine_E6 -7 (65.6), dyad_fourth_A2 -12 (63.5)
- **lsd_db**: click -12 (14.1), noise_white -12 (12.7), noise_pink -12 (11.2), noise_white +12 (8.5), noise_white +7 (8.2)
- **track_err_cents**: sweep_80_2k +7 (21.8), sweep_80_2k -12 (14.3), vibrato_D4 +12 (13.7), sweep_80_2k -5 (11.6), vibrato_A2 -5 (10.1)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
