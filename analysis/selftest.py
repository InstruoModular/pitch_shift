"""Metric self-validation: ideal signals must score perfectly, and injected damage of a known
size must be recovered. Prints one PASS/FAIL line per check; exits 1 on any failure.

    python analysis/selftest.py [-v]
"""
from __future__ import annotations

import sys

import numpy as np

import metrics
import siggen
from siggen import SR

SPECS = {
    "sine":  {"id": "st_sine_A4",  "kind": "sine",     "note": "A4", "dur": 1.5},
    "harm":  {"id": "st_harm_A2",  "kind": "harmonic", "note": "A2", "dur": 1.5},
    "pluck": {"id": "st_pluck_G3", "kind": "pluck",    "note": "G3", "dur": 1.5},
    "chord": {"id": "st_chord",    "kind": "chord",    "notes": ["E2", "B2", "E3", "G#3"], "dur": 2.0},
    "vib":   {"id": "st_vib_D4",   "kind": "vibrato",  "note": "D4", "dur": 1.5},
    "burst": {"id": "st_burst_A4", "kind": "burst",    "note": "A4", "count": 4},
    "stacc": {"id": "st_stacc_E3", "kind": "staccato", "note": "E3", "count": 8, "dur": 1.0},
    "sil":   {"id": "st_silence",  "kind": "silence",  "dur": 0.5},
}
SIG = {k: siggen.build(v) for k, v in SPECS.items()}
VERBOSE = "-v" in sys.argv
failures = 0


def check(name: str, got, lo: float, hi: float) -> None:
    global failures
    ok = got is not None and lo <= got <= hi
    failures += not ok
    val = "None" if got is None else f"{got:.3f}"
    print(f"{'PASS' if ok else 'FAIL'}  {name:<42} {val:>9}  in [{lo:g}, {hi:g}]")


def run(key: str, semis: float, y: np.ndarray) -> dict:
    m = metrics.measure(SIG[key], semis, y)
    if VERBOSE:
        print("   ", key, semis, {k: round(v, 3) if isinstance(v, float) else v for k, v in m.items()})
    return m


def delayed(x: np.ndarray, ms: float, extra: int = SR // 2) -> np.ndarray:
    d = int(round(ms * SR / 1000))
    return np.concatenate([np.zeros(d), x, np.zeros(extra)])


# ---- (a) ideal renders score perfectly -------------------------------------------------------
for key, semis in [("sine", 7), ("harm", -12), ("pluck", 12), ("chord", 7), ("vib", 7), ("burst", 5), ("stacc", -7)]:
    s = SIG[key]
    m = run(key, semis, s.render(semis))
    check(f"ideal {key}{semis:+d} lat_ms", m.get("lat_ms", m.get("lat_xcorr_ms")), -0.3, 0.3)
    check(f"ideal {key}{semis:+d} lsd_db", m.get("lsd_db"), 0.0, 0.3)
    check(f"ideal {key}{semis:+d} level_db", m.get("level_db"), -0.1, 0.1)
    if key in ("sine", "harm", "pluck"):
        check(f"ideal {key}{semis:+d} pitch_err_cents", m.get("pitch_err_cents"), 0.0, 0.3)
        check(f"ideal {key}{semis:+d} if_dev_cents", m.get("if_dev_cents"), 0.0, 1.5)
        check(f"ideal {key}{semis:+d} sinad_db", m.get("sinad_db"), 45.0, 400.0)
        check(f"ideal {key}{semis:+d} am_pp_db", m.get("am_pp_db"), 0.0, 0.3)
    if key == "chord":
        check(f"ideal {key}{semis:+d} sinad_poly_db", m.get("sinad_poly_db"), 40.0, 400.0)
        check(f"ideal {key}{semis:+d} poly_pitch_err_cents", m.get("poly_pitch_err_cents"), 0.0, 0.5)
    if key == "vib":
        check(f"ideal {key}{semis:+d} track_err_cents", m.get("track_err_cents"), 0.0, 1.0)
    if key in ("pluck", "burst", "stacc"):
        check(f"ideal {key}{semis:+d} flam_db", m.get("flam_db"), 0.0, 0.1)
        check(f"ideal {key}{semis:+d} attack_smear", m.get("attack_smear"), 0.0, 0.15)

# ---- (b) injected damage is recovered --------------------------------------------------------
# delay
for key in ("burst", "pluck", "stacc"):
    m = run(key, 7, delayed(SIG[key].render(7), 12.0))
    check(f"delay12ms {key} lat_ms", m.get("lat_ms"), 11.5, 12.5)
    check(f"delay12ms {key} lat_jitter_ms", m.get("lat_jitter_ms", 0.0), 0.0, 0.3)

# detune +7 cents
for key in ("sine", "harm", "pluck"):
    m = run(key, 7, SIG[key].render(7.07))
    check(f"detune+7c {key} pitch_err_cents", m.get("pitch_err_cents"), 6.3, 7.7)
m = run("chord", 7, SIG["chord"].render(7.07))
check("detune+7c chord poly_pitch_err_cents", m.get("poly_pitch_err_cents"), 6.0, 8.0)
m = run("vib", 7, SIG["vib"].render(7.07))
check("detune+7c vib track_err_cents", m.get("track_err_cents"), 6.0, 8.0)

# 3 dB p-p amplitude modulation at 23 Hz
for key in ("sine", "harm"):
    x = SIG[key].render(7).astype(np.float64)
    t = np.arange(len(x)) / SR
    m = run(key, 7, x * 10 ** ((1.5 * np.sin(2 * np.pi * 23 * t)) / 20))
    check(f"AM3dB@23Hz {key} am_pp_db", m.get("am_pp_db"), 2.6, 3.4)
    check(f"AM3dB@23Hz {key} am_rate_hz", m.get("am_rate_hz"), 22.0, 24.0)

# white noise 40 dB below the signal (steady window power)
for key in ("sine", "harm"):
    s = SIG[key]
    x = s.render(7).astype(np.float64)
    t0, t1 = s.steady
    p = np.mean(x[int(t0 * SR):int(t1 * SR)] ** 2)
    rng = np.random.default_rng(1)
    m = run(key, 7, x + rng.standard_normal(len(x)) * np.sqrt(p * 1e-4))
    check(f"noise-40dB {key} sinad_db", m.get("sinad_db"), 38.5, 42.0)

# duplicated attack 30 ms after each onset (flam)
for key in ("pluck", "stacc"):
    s = SIG[key]
    x = s.render(12).astype(np.float64)
    y = x.copy()
    d, L = int(0.03 * SR), int(0.02 * SR)
    for o in s.onsets:
        i = int(o * SR)
        y[i + d:i + d + L] += 0.8 * x[i:i + L]
    m = run(key, 12, y)
    check(f"flam30ms {key} flam_db", m.get("flam_db"), 2.5, 40.0)

# discontinuity: sign flip of the rest of the signal mid-steady (sine)
s = SIG["sine"]
x = s.render(7).astype(np.float64)
mid = int(np.mean(s.steady) * SR)
x[mid:] *= -1
m = run("sine", 7, x)
check("signflip sine disc_db", m.get("disc_db"), 10.0, 400.0)

# gain -6 dB
m = run("harm", -12, 0.5 * SIG["harm"].render(-12))
check("gain0.5 harm level_db", m.get("level_db"), -6.2, -5.8)

# subharmonic at -12 dB
s = SIG["sine"]
x = s.render(7).astype(np.float64)
x += 0.25 * s.render(-5).astype(np.float64)           # f/2 component at 1/4 amplitude
m = run("sine", 7, x)
check("subharm-12dB sine subharm_db", m.get("subharm_db"), -13.0, -11.0)

# ---- modulation artefacts (granular warble) ---------------------------------------------------------------
for key, semis in [("sine", 7), ("harm", -12), ("pluck", 12), ("chord", 7)]:
    m = run(key, semis, SIG[key].render(semis))
    check(f"ideal {key}{semis:+d} env_mod_db", m.get("env_mod_db"), 0.0, 0.05)
    if key != "chord":
        check(f"ideal {key}{semis:+d} fm_rough_cents", m.get("fm_rough_cents"), 0.0, 0.3)
        check(f"ideal {key}{semis:+d} am_rough_db", m.get("am_rough_db"), 0.0, 0.05)

# FM: +-5 cents at 12 Hz on every partial -> rms 5/sqrt(2) = 3.54 c
for key in ("harm", "pluck"):
    s = SIG[key]
    note = s.notes[0]
    orig = note.f_curve.copy()
    t = np.arange(s.n) / SR
    note.f_curve = orig * 2 ** (5.0 * np.sin(2 * np.pi * 12.0 * t) / 1200.0)
    y = s.render(7)
    note.f_curve = orig
    m = run(key, 7, y)
    check(f"FM5c@12Hz {key} fm_rough_cents", m.get("fm_rough_cents"), 3.0, 4.1)

# AM: +-0.5 dB at 20 Hz -> rms 0.354 dB (partials and band envelopes)
for key in ("harm", "chord"):
    x = SIG[key].render(7).astype(np.float64)
    t = np.arange(len(x)) / SR
    m = run(key, 7, x * 10 ** (0.5 * np.sin(2 * np.pi * 20.0 * t) / 20))
    check(f"AM0.5dB@20Hz {key} env_mod_db", m.get("env_mod_db"), 0.25, 0.45)
    if key == "harm":
        check(f"AM0.5dB@20Hz {key} am_rough_db", m.get("am_rough_db"), 0.28, 0.42)

# granular buzz: +-0.5 dB AM at 150 Hz must show in the 64-300 Hz band metric (whole note), ideals must not
for key in ("harm", "pluck"):
    m = run(key, 7, SIG[key].render(7))
    check(f"ideal {key}+7 env_mod_hi_db", m.get("env_mod_hi_db"), 0.0, 0.05)
    check(f"ideal {key}+7 env_mod_note_db", m.get("env_mod_note_db"), 0.0, 0.05)
    check(f"ideal {key}+7 grain_noise_p90_db", m.get("grain_noise_p90_db"), -101.0, -60.0)
    x = SIG[key].render(7).astype(np.float64)
    t = np.arange(len(x)) / SR
    m = run(key, 7, x * 10 ** (0.5 * np.sin(2 * np.pi * 150.0 * t) / 20))
    check(f"AM0.5dB@150Hz {key} env_mod_hi_db", m.get("env_mod_hi_db"), 0.2, 0.45)
# On a pure sine the 150 Hz AM must stay out of the 3-64 Hz band (on harmonic tones its sidebands beat against
# neighbouring partials at a few Hz, so slow modulation legitimately rises there).
x = SIG["sine"].render(7).astype(np.float64)
t = np.arange(len(x)) / SR
m = run("sine", 7, x * 10 ** (0.5 * np.sin(2 * np.pi * 150.0 * t) / 20))
check("AM0.5dB@150Hz sine env_mod_db (3-64 Hz, must stay low)", m.get("env_mod_db", 0.0), 0.0, 0.1)

# -40 dB white noise must not read as warble
s = SIG["harm"]
x = s.render(7).astype(np.float64)
p = np.mean(x[int(s.steady[0] * SR):int(s.steady[1] * SR)] ** 2)
m = run("harm", 7, x + np.random.default_rng(3).standard_normal(len(x)) * np.sqrt(p * 1e-4))
check("noise-40dB harm fm_rough_cents", m.get("fm_rough_cents"), 0.0, 1.0)
check("noise-40dB harm env_mod_db", m.get("env_mod_db"), 0.0, 0.15)

# silence
m = run("sil", 7, np.full(SIG["sil"].n, 1e-4))
check("silence 1e-4 silence_dbfs", m.get("silence_dbfs"), -81.0, -79.0)

print(f"\nselftest: {'OK' if failures == 0 else f'{failures} FAILED'}")
sys.exit(1 if failures else 0)
