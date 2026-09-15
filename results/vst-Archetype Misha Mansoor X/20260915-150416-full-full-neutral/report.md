# Report: vst:Archetype Misha Mansoor X / full

Settings: Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0  
Shifts: [-12, -7, -5, -1, 1, 5, 7, 12]

## Scorecard
```
== vst:Archetype Misha Mansoor X [Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0]  (313 cases)
metric                      cand
lat_ms                      8.16
lat_ms_max                 45.15
lat_jitter_ms               4.31
lat_reported_ms             1.75
pitch_err_cents             0.27
pitch_err_p95_cents         0.48
if_dev_cents                1.83
track_err_cents             6.29
poly_pitch_err_cents        8.06
sinad_db                   42.98
sinad_poly_db              26.58
subharm_db                -77.19
hf_junk_db                -79.57
am_pp_db                    0.28
am_rate_hz                 12.58
lsd_db                      2.64
pre_echo_db              -124.54
flam_db                     2.46
attack_smear                0.88
disc_db                     1.06
level_db                    0.35
silence_dbfs             -200.00
nonfinite                   0.00

run: results\vst-Archetype Misha Mansoor X\20260915-150416-full-full-neutral  jobs=313  render 0s  process 138s  measure 41s
```

## [Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 5.83 | 30.99 | 0.77 | 2.81 | 20.51 | 42.45 | 24.32 | 0.26 | 2.67 | 2.90 | 1.07 | -119.05 | 0.20 |
| -7 | 8.37 | 15.55 | 0.42 | 2.03 | 6.30 | 42.95 | 25.45 | 0.26 | 2.82 | 1.78 | 1.05 | -133.73 | 0.66 |
| -5 | 4.79 | 20.90 | 0.29 | 1.91 | 3.34 | 43.85 | 26.73 | 0.24 | 2.91 | 2.07 | 0.66 | -133.32 | 0.35 |
| -1 | 9.20 | 21.03 | 0.04 | 0.59 | 5.04 | 49.57 | 32.23 | 0.18 | 1.73 | 1.47 | 0.83 | -153.93 | 0.26 |
| 1 | 9.00 | 45.15 | 0.11 | 1.39 | 2.99 | 44.50 | 31.91 | 0.32 | 1.94 | 1.75 | 0.78 | -140.29 | 0.23 |
| 5 | 9.39 | 41.28 | 0.18 | 2.02 | 4.14 | 40.71 | 25.01 | 0.33 | 2.51 | 1.67 | 0.86 | -135.28 | 0.28 |
| 7 | 4.46 | 37.37 | 0.19 | 2.08 | 3.69 | 40.72 | 23.10 | 0.32 | 3.13 | 5.92 | 0.78 | -135.22 | 0.46 |
| 12 | 10.17 | 27.97 | 0.19 | 1.77 | 4.31 | 39.11 | 23.91 | 0.34 | 3.40 | 2.09 | 1.04 | -45.47 | 0.33 |

## [Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 7.68 | 10.53 | - | - | 0.16 | - | - | - | 1.57 | - | - | - | 0.02 |
| burst | 8.51 | 22.24 | - | - | - | - | - | - | 3.28 | 2.95 | 0.92 | -192.45 | 0.13 |
| chord | 6.37 | 45.15 | - | - | - | - | 22.21 | - | 2.42 | 3.24 | 1.05 | -49.41 | 0.11 |
| click | 7.46 | 16.55 | - | - | - | - | - | - | 15.04 | 0.00 | 0.00 | -155.56 | 8.55 |
| decay | 8.87 | 12.95 | 0.26 | 2.46 | - | 36.25 | - | 0.45 | 0.89 | - | - | - | 0.07 |
| dyad | 7.20 | 18.45 | - | - | - | - | 30.08 | - | 3.26 | - | - | - | 0.06 |
| harmonic | 8.73 | 17.07 | 0.22 | 1.08 | - | 42.98 | - | 0.01 | 1.67 | - | - | - | 0.02 |
| noise | 7.32 | 13.01 | - | - | - | - | - | - | 4.26 | - | - | - | 1.22 |
| pluck | 8.75 | 14.56 | 0.41 | 3.28 | - | 33.56 | - | 0.76 | 1.10 | 1.24 | 0.89 | -167.35 | 0.12 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 8.34 | 17.05 | 0.19 | 0.89 | - | 53.52 | - | 0.00 | 0.61 | - | - | - | 0.02 |
| staccato | 6.89 | 32.50 | - | - | - | - | - | - | 3.92 | 5.02 | 0.93 | -28.99 | 0.16 |
| sweep | 9.77 | 15.50 | - | - | 15.56 | - | - | - | 7.78 | - | - | - | 0.04 |
| vibrato | 9.23 | 15.11 | - | - | 4.72 | - | - | - | 3.41 | - | - | - | 0.01 |

## [Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0] worst cases

- **sinad_db**: pluck_D3 +5 (26.5), pluck_D3 +1 (26.9), pluck_D3 +7 (27.2), pluck_D3 +12 (27.4), pluck_E2 +5 (28.3)
- **sinad_poly_db**: dyad_min2_E4 -12 (-7.9), dyad_min2_E4 +7 (-5.1), dyad_min2_E4 -7 (-1.5), dyad_min2_E4 +5 (-1.2), dyad_min2_E4 -5 (1.8)
- **if_dev_cents**: pluck_D3 -12 (10.7), pluck_D3 +1 (8.9), pluck_D3 -7 (8.6), pluck_D3 -5 (8.6), pluck_E2 +7 (7.9)
- **am_pp_db**: pluck_B3 +1 (1.2), pluck_D3 +1 (1.2), pluck_D3 +7 (1.1), pluck_D3 +5 (1.1), pluck_B3 +5 (1.1)
- **flam_db**: pluck_D3 +7 (36.3), staccato_E3_16 +12 (20.2), staccato_E3_16 +7 (16.2), burst_E5_saw -12 (16.0), staccato_E3_16 +5 (15.1)
- **lat_ms_max**: chord_E5power +1 (45.2), chord_Emaj +5 (41.3), chord_Emaj +7 (37.4), staccato_E3_16 +5 (32.5), chord_Emaj -12 (31.0)
- **lsd_db**: click -5 (29.6), click -7 (29.2), click -12 (15.7), click +1 (11.8), dyad_min2_E4 +7 (11.2)
- **track_err_cents**: sweep_80_2k -12 (60.2), sweep_80_2k -7 (15.2), sweep_80_2k -1 (12.6), vibrato_A2 -12 (11.6), sweep_80_2k +5 (10.9)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
