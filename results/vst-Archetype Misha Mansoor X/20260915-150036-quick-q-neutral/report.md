# Report: vst:Archetype Misha Mansoor X / quick

Settings: Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0  
Shifts: [-12, -7, 7, 12]

## Scorecard
```
== vst:Archetype Misha Mansoor X [Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0]  (40 cases)
metric                      cand
lat_ms                      8.45
lat_ms_max                 37.37
lat_jitter_ms               5.77
lat_reported_ms             1.75
pitch_err_cents             0.58
pitch_err_p95_cents         0.81
if_dev_cents                2.70
track_err_cents             4.99
poly_pitch_err_cents        3.88
sinad_db                   43.11
sinad_poly_db              34.23
subharm_db                -70.69
hf_junk_db                -86.18
am_pp_db                    0.29
am_rate_hz                 11.75
lsd_db                      2.51
pre_echo_db              -103.15
flam_db                     3.26
attack_smear                1.13
disc_db                     1.38
level_db                    0.09
nonfinite                   0.00

run: results\vst-Archetype Misha Mansoor X\20260915-150036-quick-q-neutral  jobs=40  render 0s  process 19s  measure 7s
```

## [Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 4.72 | 30.99 | 1.21 | 2.87 | 10.12 | 45.43 | 35.55 | 0.22 | 2.04 | 3.51 | 1.22 | -129.50 | 0.12 |
| -7 | 8.80 | 14.96 | 0.55 | 1.86 | 4.46 | 45.99 | 34.12 | 0.21 | 1.83 | 1.96 | 1.47 | -140.80 | 0.08 |
| 7 | 6.63 | 37.37 | 0.26 | 3.20 | 2.80 | 41.37 | 34.01 | 0.37 | 2.68 | 3.23 | 0.71 | -92.98 | 0.08 |
| 12 | 10.40 | 27.65 | 0.32 | 2.85 | 2.58 | 39.67 | 33.24 | 0.36 | 3.48 | 4.32 | 1.11 | -49.33 | 0.09 |

## [Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| burst | 8.72 | 22.24 | - | - | - | - | - | - | 2.91 | 1.77 | 0.90 | -172.37 | 0.11 |
| chord | 8.16 | 37.37 | - | - | - | - | 29.61 | - | 2.26 | 2.53 | 0.81 | -33.49 | 0.09 |
| dyad | 7.12 | 9.75 | - | - | - | - | 38.85 | - | 2.40 | - | - | - | 0.12 |
| harmonic | 5.86 | 12.87 | 0.49 | 1.65 | - | 41.25 | - | 0.01 | 0.93 | - | - | - | 0.01 |
| pluck | 8.96 | 13.03 | 0.64 | 4.04 | - | 32.67 | - | 0.72 | 1.44 | 1.05 | 1.56 | -144.09 | 0.16 |
| sine | 9.45 | 15.81 | 0.57 | 1.88 | - | 54.49 | - | 0.00 | 0.88 | - | - | - | 0.02 |
| staccato | 8.28 | 27.65 | - | - | - | - | - | - | 6.10 | 9.89 | 0.80 | -21.73 | 0.24 |
| vibrato | 8.38 | 13.66 | - | - | 4.99 | - | - | - | 5.86 | - | - | - | 0.03 |

## [Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0] worst cases

- **sinad_db**: pluck_E2 +12 (28.3), pluck_G3 -12 (29.4), pluck_E2 +7 (29.6), pluck_G3 +12 (30.5), pluck_G3 +7 (30.7)
- **sinad_poly_db**: chord_Emaj +7 (28.5), chord_Emaj +12 (28.8), chord_Emaj -7 (29.1), chord_Emaj -12 (32.0), dyad_fifth_A2 +12 (37.7)
- **if_dev_cents**: pluck_E2 +7 (7.9), pluck_E2 +12 (6.2), pluck_E2 -12 (5.4), sine_E2 +7 (4.9), sine_E2 +12 (4.4)
- **am_pp_db**: pluck_G3 +7 (1.0), pluck_G3 +12 (0.9), pluck_E2 +7 (0.9), pluck_G3 -12 (0.9), pluck_E2 +12 (0.8)
- **flam_db**: staccato_E3 +12 (17.7), staccato_E3 +7 (13.0), staccato_E3 -12 (8.7), burst_A4 -7 (6.3), pluck_E2 -12 (4.4)
- **lat_ms_max**: chord_Emaj +7 (37.4), chord_Emaj -12 (31.0), staccato_E3 +12 (27.7), staccato_E3 -12 (27.3), chord_Emaj +12 (22.3)
- **lsd_db**: staccato_E3 +12 (9.4), vibrato_D4 +12 (9.3), vibrato_D4 +7 (8.3), staccato_E3 +7 (6.6), staccato_E3 -12 (4.8)
- **track_err_cents**: vibrato_D4 -12 (10.1), vibrato_D4 -7 (4.5), vibrato_D4 +7 (2.8), vibrato_D4 +12 (2.6)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
