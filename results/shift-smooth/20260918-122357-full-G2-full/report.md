# Report: shift:smooth / full

Settings: exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85  
Shifts: [-12, -7, -5, -1, 1, 5, 7, 12]

## Scorecard
```
== shift:smooth [exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85]  (313 cases)  vs vst:Archetype Misha Mansoor X:Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0
metric                      cand       ref   delta+  ok
lat_ms                      2.53      8.16    +5.63  ok
lat_ms_max                 30.70     45.15   +14.45  ok
lat_jitter_ms               2.58      4.31    +1.73  ok
lat_reported_ms                -      1.75           .
pitch_err_cents             0.02      0.27    +0.26  ok
pitch_err_p95_cents         0.04      0.48    +0.44  ok
if_dev_cents                0.31      1.83    +1.51  ok
track_err_cents             3.06      6.29    +3.23  ok
poly_pitch_err_cents        9.31      8.06    -1.24  XX
sinad_db                   60.70     42.98   +17.72  ok
sinad_poly_db              32.32     26.58    +5.74  ok
subharm_db                -95.98    -77.19   +18.79  ok
hf_junk_db               -108.23    -79.57   +28.66  ok
am_pp_db                    0.07      0.28    +0.21  ok
am_rate_hz                 36.70     12.58           .
lsd_db                      2.53      2.64    +0.11  ok
pre_echo_db              -145.30   -124.54   +20.76  ok
flam_db                     1.42      2.46    +1.03  ok
attack_smear                0.96      0.88    -0.08  ok
disc_db                     0.04      1.06    +1.01  ok
fm_rough_cents              1.11         -           .
am_rough_db                 0.18         -           .
env_mod_db                  0.24         -           .
env_mod_note_db             0.38         -           .
env_mod_hi_db               0.43         -           .
grain_noise_p90_db        -53.87         -           .
level_db                    0.54      0.35    -0.20  ok
silence_dbfs             -200.00   -200.00    -0.00  ok
nonfinite                   0.00      0.00    -0.00  ok
cpu_ns_per_sample         440.04         -           .
cpu_worst_block_pct       104.72         -           .
-- 1 metric(s) worse than ref beyond tolerance

run: results\shift-smooth\20260918-122357-full-G2-full  jobs=313  render 0s  process 53s  measure 95s
```

## [exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 2.14 | 13.01 | 0.03 | 0.38 | 3.54 | 60.71 | 30.11 | 0.07 | 2.68 | 1.13 | 1.30 | -137.64 | 0.42 |
| -7 | 1.61 | 29.60 | 0.02 | 0.39 | 2.20 | 58.42 | 31.60 | 0.06 | 2.14 | 1.30 | 1.04 | -145.72 | 0.60 |
| -5 | 1.22 | 30.70 | 0.04 | 0.82 | 3.13 | 57.93 | 31.90 | 0.06 | 2.20 | 1.12 | 0.75 | -147.39 | 0.75 |
| -1 | 1.23 | 15.50 | 0.01 | 0.20 | 2.06 | 57.02 | 38.46 | 0.08 | 2.05 | 1.72 | 0.76 | -135.17 | 0.73 |
| 1 | 1.60 | 19.25 | 0.01 | 0.15 | 2.17 | 63.20 | 38.22 | 0.08 | 1.68 | 1.66 | 0.71 | -147.68 | 0.23 |
| 5 | 3.28 | 16.62 | 0.01 | 0.20 | 2.78 | 63.99 | 31.27 | 0.07 | 2.57 | 1.52 | 0.90 | -149.67 | 0.48 |
| 7 | 3.84 | 23.71 | 0.01 | 0.21 | 2.91 | 64.79 | 28.69 | 0.06 | 3.08 | 1.28 | 0.91 | -149.49 | 0.53 |
| 12 | 4.98 | 22.30 | 0.01 | 0.14 | 5.70 | 59.57 | 28.35 | 0.08 | 3.85 | 1.64 | 1.32 | -149.65 | 0.59 |

## [exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bend | 2.29 | 4.59 | - | - | 0.03 | - | - | - | 2.15 | - | - | - | 0.04 |
| burst | 2.01 | 5.26 | - | - | - | - | - | - | 3.10 | 1.29 | 1.29 | -210.26 | 0.09 |
| chord | 1.93 | 30.70 | - | - | - | - | 24.11 | - | 2.86 | 1.83 | 1.26 | -52.64 | 0.62 |
| click | 1.65 | 5.71 | - | - | - | - | - | - | 7.14 | 0.00 | 0.00 | -174.86 | 9.32 |
| decay | 3.07 | 4.93 | 0.00 | 0.34 | - | 46.04 | - | 0.31 | 1.99 | - | - | - | 0.13 |
| dyad | 2.67 | 17.03 | - | - | - | - | 38.90 | - | 3.54 | - | - | - | 0.07 |
| harmonic | 3.14 | 5.32 | 0.01 | 0.04 | - | 71.20 | - | 0.01 | 1.39 | - | - | - | 0.06 |
| noise | 2.00 | 5.25 | - | - | - | - | - | - | 4.89 | - | - | - | 2.59 |
| pluck | 2.84 | 6.89 | 0.02 | 0.63 | - | 45.75 | - | 0.15 | 1.81 | 1.60 | 0.76 | -206.31 | 0.35 |
| silence | - | - | - | - | - | - | - | - | - | - | - | - | - |
| sine | 3.11 | 5.06 | 0.02 | 0.22 | - | 69.36 | - | 0.00 | 0.84 | - | - | - | 0.04 |
| staccato | 2.22 | 13.01 | - | - | - | - | - | - | 3.82 | 0.97 | 0.94 | -35.38 | 0.21 |
| sweep | 2.66 | 5.91 | - | - | 5.41 | - | - | - | 4.66 | - | - | - | 0.15 |
| vibrato | 2.48 | 5.28 | - | - | 3.40 | - | - | - | 2.02 | - | - | - | 0.05 |

## [exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85] worst cases

- **sinad_db**: sine_E5 -5 (32.9), sine_E5 -1 (34.3), sine_E5 -7 (36.9), sine_E5 -12 (40.0), sine_E6 -7 (41.8)
- **sinad_poly_db**: dyad_min2_E4 +12 (-23.7), dyad_min2_E4 +7 (-18.6), dyad_min2_E4 -12 (-18.2), dyad_min2_E4 +5 (-2.7), dyad_min2_E4 -7 (-2.7)
- **if_dev_cents**: sine_E5 -5 (8.0), pluck_E2 -5 (1.9), pluck_E2 +7 (1.5), pluck_E2 -7 (1.5), pluck_A2 -5 (1.4)
- **am_pp_db**: decay_A3 +12 (0.5), decay_A3 +1 (0.4), decay_A3 -1 (0.4), decay_A3 -12 (0.3), pluck_E2 +1 (0.2)
- **flam_db**: burst_E3 +12 (7.0), burst_E3 -12 (5.4), pluck_G3 +7 (5.2), pluck_G3 -1 (4.9), chord_Emaj -1 (4.7)
- **lat_ms_max**: chord_Emaj -5 (30.7), chord_Amin -7 (29.6), chord_Cmaj7 -7 (26.7), chord_Cmaj7 +7 (23.7), chord_Emaj +12 (22.3)
- **lsd_db**: click -1 (18.5), click -5 (13.1), burst_E3 +12 (12.6), dyad_min2_E4 +12 (11.6), dyad_min2_E4 +7 (10.8)
- **track_err_cents**: sweep_80_2k +12 (12.0), vibrato_A2 +12 (8.6), sweep_80_2k -12 (8.2), vibrato_A2 +7 (6.8), sweep_80_2k -5 (6.8)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
