# Report: shift:smooth / real

Settings: exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85  
Shifts: [-12, -7, -5, -1, 1, 5, 7, 12]

## Scorecard
```
== shift:smooth [exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85]  (0 cases)  vs vst:Archetype Misha Mansoor X:Input Gain=0.5,Output Gain=0.5,Gate Active=0,Doubler Active=0,Special FX Section Active=0,Pre FX Section Active=0,Amp Section Active=0,Cab Section Active=0,EQ Section Active=0,Post FX Section Active=0
metric                      cand       ref   delta+  ok
-- 0 metric(s) worse than ref beyond tolerance

run: results\shift-smooth\20260918-122014-real-G2-real  jobs=104  render 0s  process 19s  measure 64s
```

## [exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85] by shift

| shift | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| -12 | 1.46 | 15.50 | 1.55 | 3.47 | - | 43.88 | 24.36 | 0.14 | 2.21 | 2.02 | 0.90 | -146.93 | 0.69 |
| -7 | 1.43 | 11.61 | 1.44 | 3.55 | - | 44.24 | 24.22 | 0.14 | 1.86 | 0.88 | 0.95 | -142.31 | 0.50 |
| -5 | 1.88 | 30.50 | 1.47 | 4.18 | - | 46.36 | 24.52 | 0.13 | 1.61 | 1.19 | 1.07 | -144.25 | 0.44 |
| -1 | 2.12 | 23.91 | 1.42 | 3.17 | - | 43.55 | 31.06 | 0.17 | 1.77 | 1.19 | 0.66 | -137.15 | 0.17 |
| 1 | 2.36 | 21.50 | 1.43 | 3.18 | - | 44.47 | 29.81 | 0.16 | 1.72 | 1.41 | 0.99 | -145.19 | 0.17 |
| 5 | 3.60 | 27.53 | 1.42 | 3.34 | - | 53.09 | 23.47 | 0.15 | 2.20 | 2.00 | 1.26 | -147.98 | 0.48 |
| 7 | 4.41 | 26.17 | 1.43 | 3.45 | - | 54.51 | 22.54 | 0.16 | 2.54 | 1.82 | 0.89 | -150.62 | 0.41 |
| 12 | 5.16 | 35.00 | 1.47 | 3.38 | - | 52.17 | 21.87 | 0.17 | 3.28 | 2.17 | 1.39 | -139.34 | 0.55 |

## [exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85] by signal kind

| kind | lat_ms | lat_ms_max | pitch_err_cents | if_dev_cents | track_err_cents | sinad_db | sinad_poly_db | am_pp_db | lsd_db | flam_db | attack_smear | pre_echo_db | level_db |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| gchord | 5.37 | 35.00 | - | - | - | - | 25.23 | - | 2.77 | 1.32 | 1.38 | -35.01 | 0.60 |
| guitar | 2.30 | 5.27 | 1.45 | 3.47 | - | 47.78 | - | 0.15 | 1.76 | 1.75 | 0.79 | -212.48 | 0.32 |

## [exact_ratio=1,onset_runway=150,fallback_corr_window=256,onset_grain=768,causal_corr=1,guard_samples=24,xfade_frac=0.125,blind_span_cap=538,ola_mode=1,ola_periods=3,ola_min_len=256,ola_window=512,ola_reach=48,ola_kill_len=64,ola_onsets=1,ola_coarse=1,ola_coarse_cands=2,ola_fine_window=256,yin_weak_threshold=0.5,yin_weak_stable=2,yin_weak_hold=4,yin_max_lag=450,yin_octave_tol=0.85] worst cases

- **sinad_db**: gtr_G3_vib -1 (34.5), gtr_G3_vib +1 (35.1), gtr_E5 -7 (35.3), gtr_G3_vib -12 (38.4), gtr_E5 -12 (40.4)
- **sinad_poly_db**: gch_Amin +12 (7.3), gch_Cmaj7 +12 (8.5), gch_Cmaj7 +7 (8.6), gch_Amin -12 (8.8), gch_Amin +7 (9.2)
- **if_dev_cents**: gtr_E5 -5 (7.6), gtr_G3_vib +7 (6.0), gtr_G3_vib -5 (6.0), gtr_G3_vib +5 (6.0), gtr_G3_vib -7 (6.0)
- **am_pp_db**: gtr_G3_vib +7 (0.3), gtr_G3_vib +12 (0.3), gtr_G3_vib -1 (0.2), gtr_G3_vib +5 (0.2), gtr_E2 -1 (0.2)
- **flam_db**: gtr_G3_vib -12 (7.1), gtr_G3 -12 (6.6), gtr_G3 +7 (5.9), gtr_G3_vib +5 (5.9), gch_E5power -12 (5.6)
- **lat_ms_max**: gch_Emaj +12 (35.0), gch_Amin +12 (31.2), gch_Dmaj -5 (30.5), gch_Amin +5 (27.5), gch_E5power +12 (26.4)
- **lsd_db**: gch_Cmaj7 +12 (5.4), gtr_G3_vib +12 (4.4), gch_Dmaj +12 (4.3), gtr_G3 +12 (4.3), gch_Cmaj7 +7 (4.1)

## Plots

![by_shift.png](plots/by_shift.png)
![latency_by_signal.png](plots/latency_by_signal.png)
