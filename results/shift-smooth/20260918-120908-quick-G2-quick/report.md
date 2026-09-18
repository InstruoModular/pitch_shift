# Report: shift:smooth / quick

Settings: exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85  
Shifts: [-12, -7, 7, 12]

## Scorecard
```
== shift:smooth [exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85]  (32 cases)  vs vst:Archetype Misha Mansoor X:Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0
metric                      cand       ref   delta+  ok
lat_ms                      3.34      8.78    +5.44  ok
lat_ms_max                 22.30     37.37   +15.06  ok
lat_jitter_ms               3.14      5.76    +2.62  ok
lat_reported_ms                -      1.75           .
pitch_err_cents             0.01      0.59    +0.58  ok
pitch_err_p95_cents         0.04      0.82    +0.78  ok
if_dev_cents                0.34      2.72    +2.38  ok
track_err_cents             1.82      4.84    +3.02  ok
poly_pitch_err_cents        7.14      6.68    -0.46  ok
sinad_db                   64.24     42.18   +22.06  ok
sinad_poly_db              30.31     29.61    +0.70  ok
subharm_db                -89.69    -70.88   +18.81  ok
hf_junk_db               -108.89    -87.69   +21.20  ok
am_pp_db                    0.06      0.29    +0.23  ok
am_rate_hz                 28.31     11.75           .
lsd_db                      2.25      2.06    -0.19  ok
pre_echo_db              -160.01   -127.47   +32.54  ok
flam_db                     1.26      1.52    +0.26  ok
attack_smear                1.05      1.19    +0.14  ok
disc_db                    -0.09      1.18    +1.27  ok
fm_rough_cents              0.70         -           .
am_rough_db                 0.11         -           .
env_mod_db                  0.20         -           .
env_mod_note_db             0.47         -           .
env_mod_hi_db               0.46         -           .
grain_noise_p90_db        -54.79         -           .
level_db                    0.30      0.07    -0.23  ok
nonfinite                   0.00      0.00    -0.00  ok
cpu_ns_per_sample         516.74         -           .
cpu_worst_block_pct        95.50         -           .
-- 0 metric(s) worse than ref beyond tolerance

run: results\shift-smooth\20260918-120908-quick-G2-quick  jobs=40  render 0s  process 7s  measure 17s
```

## [exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 2.04 | 6.89 | 0.02 | 0.32 | 1.64 | 67.28 | 39.64 | 0.04 | 2.19 | 0.72 | 1.32 | -105.52 | 0.31 |
| -7 | 1.56 | 12.00 | 0.01 | 0.42 | 1.29 | 64.26 | 38.78 | 0.06 | 1.97 | 1.07 | 1.01 | -144.28 | 0.31 |
| 7 | 3.76 | 19.40 | 0.01 | 0.37 | 1.87 | 65.00 | 38.40 | 0.06 | 2.92 | 2.00 | 0.97 | -144.82 | 0.21 |
| 12 | 5.04 | 22.30 | 0.01 | 0.24 | 2.48 | 60.40 | 38.23 | 0.07 | 3.84 | 1.30 | 0.97 | -144.44 | 0.32 |

## [exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| burst | 2.62 | 4.50 | - | - | - | - | - | - | 2.45 | 0.17 | 1.44 | -215.81 | 0.06 |
| chord | 2.07 | 22.30 | - | - | - | - | 30.31 | - | 3.11 | 1.82 | 1.22 | -46.17 | 0.91 |
| dyad | 1.41 | 4.10 | - | - | - | - | 47.21 | - | 3.71 | - | - | - | 0.18 |
| harmonic | 4.28 | 5.32 | 0.01 | 0.03 | - | 76.99 | - | 0.01 | 2.42 | - | - | - | 0.17 |
| pluck | 3.80 | 6.89 | 0.02 | 0.79 | - | 47.22 | - | 0.14 | 2.39 | 1.52 | 0.77 | -189.03 | 0.52 |
| sine | 3.95 | 5.06 | 0.01 | 0.04 | - | 74.87 | - | 0.00 | 1.24 | - | - | - | 0.10 |
| staccato | 3.21 | 7.06 | - | - | - | - | - | - | 5.60 | 1.32 | 1.14 | -33.78 | 0.30 |
| vibrato | 4.38 | 5.28 | - | - | 1.82 | - | - | - | 2.76 | - | - | - | 0.01 |

## [exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85] worst cases

- **sinad_db**: pluck_E2 +12 (42.9), pluck_E2 +7 (43.1), pluck_E2 -7 (43.4), pluck_G3 -7 (48.1), pluck_G3 +12 (48.2)
- **sinad_poly_db**: chord_Emaj +12 (28.8), chord_Emaj +7 (29.2), chord_Emaj -7 (30.1), chord_Emaj -12 (33.1), dyad_fifth_A2 -12 (46.2)
- **if_dev_cents**: pluck_E2 +7 (1.5), pluck_E2 -7 (1.5), pluck_E2 -12 (1.2), pluck_E2 +12 (0.9), pluck_G3 -7 (0.5)
- **am_pp_db**: pluck_E2 +7 (0.2), pluck_E2 +12 (0.2), pluck_E2 -7 (0.2), pluck_G3 -7 (0.1), pluck_G3 -12 (0.1)
- **flam_db**: pluck_G3 +7 (5.2), staccato_E3 +12 (2.8), chord_Emaj -12 (2.5), pluck_G3 -7 (2.4), pluck_G3 +12 (2.3)
- **lat_ms_max**: chord_Emaj +12 (22.3), chord_Emaj +7 (19.4), chord_Emaj -7 (12.0), staccato_E3 -7 (7.1), pluck_G3 -12 (6.9)
- **lsd_db**: staccato_E3 +12 (9.0), staccato_E3 +7 (6.1), vibrato_D4 +12 (4.3), dyad_fifth_A2 +7 (4.3), pluck_G3 +12 (4.2)
- **track_err_cents**: vibrato_D4 +12 (2.5), vibrato_D4 +7 (1.9), vibrato_D4 -12 (1.6), vibrato_D4 -7 (1.3)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
