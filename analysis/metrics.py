"""Objective metrics for a pitch-shifted output against the analytic ideal (see siggen.py).

measure(sig, semis, y, reported_latency) -> flat dict of scalars (None where not applicable).
Conventions: *_ms latency, *_cents pitch, *_db levels/ratios. Direction and tolerance for
each scorecard metric live in SCORECARD (used by run_suite/report).
"""
from __future__ import annotations

import numpy as np
from scipy import signal as sps

from siggen import SR, Signal

EPS = 1e-20
MONO_KINDS = {"sine", "harmonic", "pluck", "decay", "guitar"}
POLY_KINDS = {"dyad", "chord", "gchord"}
TRACK_KINDS = {"vibrato", "bend", "sweep"}
ONSET_KINDS = {"pluck", "chord", "burst", "staccato", "click", "guitar", "gchord"}

# metric -> (aggregate, direction (+1 higher better / -1 lower better), tolerance, description)
SCORECARD: dict[str, tuple[str, int, float, str]] = {
    "lat_ms":             ("median", -1, 0.5,  "measured latency (onset), median"),
    "lat_ms_max":         ("max",    -1, 1.0,  "measured latency, worst case"),
    "lat_jitter_ms":      ("mean",   -1, 0.3,  "per-onset latency std"),
    "lat_reported_ms":    ("max",    -1, 0.5,  "reported latency"),
    "pitch_err_cents":    ("mean",   -1, 0.5,  "steady f0 |error|, median per job"),
    "pitch_err_p95_cents":("mean",   -1, 1.0,  "steady f0 |error| p95 per job"),
    "if_dev_cents":       ("mean",   -1, 1.0,  "instantaneous-freq deviation p95 (warble)"),
    "track_err_cents":    ("mean",   -1, 2.0,  "vibrato/bend/sweep tracking |error| median"),
    "poly_pitch_err_cents":("mean",  -1, 1.0,  "dyad/chord per-note f0 |error| max"),
    "sinad_db":           ("mean",   +1, 1.0,  "mono: expected-partial energy / rest"),
    "sinad_poly_db":      ("mean",   +1, 1.0,  "poly: expected-partial energy / rest"),
    "subharm_db":         ("mean",   -1, 2.0,  "energy at f0/2 rel. fundamental"),
    "hf_junk_db":         ("mean",   -1, 2.0,  "energy above top expected partial rel. signal"),
    "am_pp_db":           ("mean",   -1, 0.3,  "steady envelope modulation p-p (splice warble)"),
    "am_rate_hz":         ("median",  0, 0.0,  "dominant envelope modulation rate (info)"),
    "lsd_db":             ("mean",   -1, 0.5,  "1/6-oct log-spectral distance to ideal"),
    "pre_echo_db":        ("mean",   -1, 2.0,  "energy before onset rel. after"),
    "flam_db":            ("mean",   -1, 1.0,  "excess HF re-attack rise 12-80 ms after onset vs ideal"),
    "attack_smear":       ("mean",   -1, 0.2,  "|log2| 10-90% rise time ratio vs ideal"),
    "disc_db":            ("mean",   -1, 2.0,  "discontinuity peak/RMS excess vs ideal"),
    "fm_rough_cents":     ("mean",   -1, 0.3,  "partial FM 3-70 Hz rms excess vs ideal (granular warble)"),
    "am_rough_db":        ("mean",   -1, 0.1,  "partial AM 3-70 Hz rms excess vs ideal (dB)"),
    "env_mod_db":         ("mean",   -1, 0.1,  "1/3-oct band envelope modulation 3-64 Hz excess (dB rms)"),
    "env_mod_note_db":    ("mean",   -1, 0.1,  "same, over the whole note (attack settle + decay), not just steady"),
    "env_mod_hi_db":      ("mean",   -1, 0.1,  "band envelope modulation 64-300 Hz excess (granular buzz), whole note"),
    "grain_noise_p90_db": ("mean",   -1, 1.0,  "short-time (150 ms) noise/signal excess vs ideal, p90 over the note"),
    "level_db":           ("meanabs",-1, 0.5,  "output level vs ideal"),
    "silence_dbfs":       ("max",    -1, 3.0,  "output RMS for silent input"),
    "nonfinite":          ("sum",    -1, 0.0,  "NaN/Inf samples"),
    "cpu_ns_per_sample":  ("median", -1, 0.0,  "process() cost (shiftbench only)"),
    "cpu_worst_block_pct":("max",    -1, 0.0,  "worst block time / block period (shiftbench)"),
}

# Inaudible floors: when candidate and reference are both at or beyond the floor, the scorecard counts the
# metric as ok regardless of the delta (a -91 vs -127 dB subharmonic is not a regression).
FLOORS: dict[str, float] = {"subharm_db": -60.0, "hf_junk_db": -60.0, "silence_dbfs": -90.0, "disc_db": 1.0}


# ---------------------------------------------------------------------------- basics

def rms_env(x: np.ndarray, frame: int, hop: int, hann: bool = False) -> np.ndarray:
    if len(x) < frame:
        return np.array([np.sqrt(np.mean(x**2) + EPS)])
    n = 1 + (len(x) - frame) // hop
    idx = np.arange(frame)[None, :] + hop * np.arange(n)[:, None]
    if hann:
        w = np.hanning(frame)
        return np.sqrt((x[idx] ** 2) @ w / w.sum() + EPS)
    return np.sqrt(np.mean(x[idx] ** 2, axis=1) + EPS)


def db(x):
    return 10.0 * np.log10(np.asarray(x, dtype=float) + EPS)


def _peak_interp(mag: np.ndarray, i: int) -> float:
    """Gaussian (log-parabolic) peak interpolation, returns fractional bin."""
    if i <= 0 or i >= len(mag) - 1:
        return float(i)
    a, b, c = np.log(mag[i - 1] + EPS), np.log(mag[i] + EPS), np.log(mag[i + 1] + EPS)
    d = a - 2 * b + c
    return float(i) + (0.5 * (a - c) / d if d < 0 else 0.0)


def spectrum(x: np.ndarray, pad: int = 1 << 17, bh: bool = False) -> tuple[np.ndarray, float]:
    w = sps.windows.blackmanharris(len(x)) if bh else np.hanning(len(x))
    nfft = max(pad, 1 << int(np.ceil(np.log2(len(x) * 4))))
    return np.abs(np.fft.rfft(x * w, nfft)), SR / nfft


def find_peak_hz(mag: np.ndarray, hz_per_bin: float, f_expect: float, cents: float = 80.0) -> tuple[float, float]:
    lo = int(f_expect * 2 ** (-cents / 1200) / hz_per_bin)
    hi = int(np.ceil(f_expect * 2 ** (cents / 1200) / hz_per_bin)) + 1
    lo, hi = max(lo, 1), min(hi, len(mag) - 1)
    if hi <= lo:
        return f_expect, 0.0
    i = lo + int(np.argmax(mag[lo:hi]))
    return _peak_interp(mag, i) * hz_per_bin, float(mag[i])


def cents(f, ref):
    return 1200.0 * np.log2(np.asarray(f) / ref)


# ---------------------------------------------------------------------------- latency

def _parabolic(seg: np.ndarray, k: int) -> float:
    if 0 < k < len(seg) - 1:
        d = seg[k - 1] - 2 * seg[k] + seg[k + 1]
        if d < 0:
            return k + 0.5 * (seg[k - 1] - seg[k + 1]) / d
    return float(k)


def onset_lag(si: np.ndarray, so: np.ndarray, i0: int, i1: int, lag_lo: int, lag_hi: int) -> float | None:
    """Lag (frames) maximising normalised correlation of onset-strength windows si[i0:i1] vs so[i0+lag:i1+lag]."""
    ref = si[i0:i1] - si[i0:i1].mean()
    nr = np.linalg.norm(ref)
    if nr <= 0 or i0 + lag_lo < 0:
        return None
    lags = np.arange(lag_lo, lag_hi + 1)
    lags = lags[i1 + lags <= len(so)]
    if len(lags) < 3:
        return None
    idx = i0 + lags[:, None] + np.arange(i1 - i0)[None, :]
    cand = so[idx]
    cand = cand - cand.mean(axis=1, keepdims=True)        # Pearson: shape of the rise, not level
    score = (cand @ ref) / (np.linalg.norm(cand, axis=1) * nr + EPS)
    k = int(np.argmax(score))
    return float(lags[0] + _parabolic(score, k)) if score[k] > 0.3 else None


def measure_latency(sig: Signal, y: np.ndarray, ideal: np.ndarray, max_lat_s: float = 0.25) -> dict:
    """Global envelope cross-correlation first (robust to frame-rate ripple and smearing), then
    each onset refined within +-25 ms of it -- that spread is what lat_jitter_ms reports."""
    hop, frame = 24, 240                                 # 0.5 ms hop, 5 ms Hann frame
    hop_s = hop / SR
    ey, ei = rms_env(y, frame, hop, hann=True), rms_env(ideal, frame, hop, hann=True)
    res: dict = {}
    lag0, max_lag = int(0.01 / hop_s), int(max_lat_s / hop_s)
    glob = None
    a, b = ey - ey.mean(), ei - ei.mean()
    if np.std(a) > 0 and np.std(b) > 0:
        xc = sps.correlate(a, b, mode="full", method="fft")
        mid = len(b) - 1
        seg = xc[mid - lag0: mid + max_lag]
        glob = (_parabolic(seg, int(np.argmax(seg))) - lag0) * hop_s * 1000.0
        res["lat_xcorr_ms"] = float(glob)
    if sig.onsets:
        cy, ci = np.sqrt(ey), np.sqrt(ei)
        centre, span = int(round((glob or 0.0) / 1000.0 / hop_s)), int(0.025 / hop_s)
        lats = []
        for j, o in enumerate(sig.onsets):
            gap = sig.onsets[j + 1] - o if j + 1 < len(sig.onsets) else 10.0
            i0, i1 = int((o - 0.015) / hop_s), int((o + min(0.04, gap * 0.5)) / hop_s)
            lo = max(centre - span, int(-0.005 / hop_s))
            hi = min(centre + span, max_lag, int(gap * 0.8 / hop_s))
            lag = onset_lag(ci, cy, max(i0, 0), i1, lo, hi) if hi > lo else None
            if lag is not None:
                lats.append(lag * hop_s * 1000.0)
        if lats:
            res["lat_ms"] = float(np.median(lats))
            res["lat_ms_max"] = float(np.max(lats))
            res["lat_jitter_ms"] = float(np.std(lats)) if len(lats) > 1 else None
    return res


def align(y: np.ndarray, lat_ms: float, n: int) -> np.ndarray:
    d = int(round(lat_ms * SR / 1000.0))
    out = np.zeros(n, dtype=np.float64)
    if d >= 0:
        seg = y[d:d + n]
    else:
        seg = np.concatenate([np.zeros(-d), y[: n + d]])
    out[: len(seg)] = seg
    return out


# ---------------------------------------------------------------------------- pitch

def steady_f0_track(x: np.ndarray, t0: float, t1: float, f_expect: float, harmonics: int = 4) -> np.ndarray:
    """Per-frame f0 estimates (Hz) in [t0,t1] from amplitude-weighted partial peaks."""
    frame = int(max(0.2, 8.0 / f_expect) * SR)
    hop = frame // 4
    i0, i1 = int(t0 * SR), int(t1 * SR)
    f0s = []
    for s in range(i0, max(i0 + 1, i1 - frame), hop):
        seg = x[s:s + frame]
        if len(seg) < frame or np.sqrt(np.mean(seg**2)) < 1e-5:
            continue
        mag, hpb = spectrum(seg, 1 << 16)
        est, wsum = 0.0, 0.0
        for k in range(1, harmonics + 1):
            if k * f_expect > 8000:
                break
            f, amp = find_peak_hz(mag, hpb, k * f_expect, 60.0)
            est += (f / k) * amp**2
            wsum += amp**2
        if wsum > 0:
            f0s.append(est / wsum)
    return np.array(f0s)


def inst_freq(x: np.ndarray, f_lo: float, f_hi: float) -> tuple[np.ndarray, np.ndarray]:
    """Band-limited instantaneous frequency (Hz) and envelope, FFT-mask bandpass + Hilbert."""
    X = np.fft.rfft(x * sps.windows.tukey(len(x), 0.1))
    f = np.fft.rfftfreq(len(x), 1 / SR)
    lo0, hi1 = f_lo * 0.7, f_hi * 1.4                   # wide raised-cosine skirts: short ringing
    mask = np.clip(np.minimum((f - lo0) / (f_lo - lo0), (hi1 - f) / (hi1 - f_hi)), 0, 1)
    mask = 0.5 - 0.5 * np.cos(np.pi * mask)
    xb = np.fft.irfft(X * mask, len(x))
    an = sps.hilbert(xb)
    phase = np.unwrap(np.angle(an))
    fi = np.diff(phase) * SR / (2 * np.pi)
    k = int(0.005 * SR)
    fi = np.convolve(fi, np.ones(k) / k, mode="same")
    return fi, np.abs(an[1:])


# ---------------------------------------------------------------------------- spectral

def partial_energy_split(x: np.ndarray, partials: list[float], f_floor: float = 20.0,
                         pad: int = 1 << 18) -> tuple[float, float, float]:
    """(energy near expected partials, energy elsewhere 20Hz-20k, energy above top partial)."""
    mag, hpb = spectrum(x, pad, bh=True)
    p = mag**2
    f = np.arange(len(p)) * hpb
    band = (f >= f_floor) & (f <= 20000)
    near = np.zeros_like(band)
    lobe = 5.0 * SR / len(x)             # Blackman-Harris main lobe half-width (4 bins) + margin, Hz
    for fp in partials:
        tol = max(lobe, fp * 0.006)       # ~10 cents
        near |= (f > fp - tol) & (f < fp + tol)
    top = (max(partials) * 1.06 + lobe) if partials else 20000
    s = float(p[band & near].sum())
    n = float(p[band & ~near].sum())
    hf = float(p[band & ~near & (f > top)].sum())
    return s, n, hf


def band_energies(x: np.ndarray, nfft: int = 2048, hop: int = 512) -> np.ndarray:
    """Frames x 1/6-octave band energies (50 Hz..16 kHz)."""
    f, _, Z = sps.stft(x, SR, nperseg=nfft, noverlap=nfft - hop, boundary=None, padded=False)
    P = np.abs(Z) ** 2
    edges = 50.0 * 2 ** (np.arange(0, np.log2(16000 / 50) * 6 + 1) / 6)
    bands = np.stack([P[(f >= lo) & (f < hi)].sum(axis=0) for lo, hi in zip(edges[:-1], edges[1:])], axis=0)
    return bands.T


def log_spectral_distance(y: np.ndarray, ideal: np.ndarray) -> float | None:
    By, Bi = band_energies(y), band_energies(ideal)
    n = min(len(By), len(Bi))
    By, Bi = By[:n], Bi[:n]
    frame_e = Bi.sum(axis=1)
    if n == 0 or frame_e.max() <= 0:
        return None
    active = frame_e > frame_e.max() * 1e-3            # within 30 dB of loudest frame
    floor = frame_e.max() * 1e-7
    d = db(By[active] + floor) - db(Bi[active] + floor)
    return float(np.mean(np.sqrt(np.mean(d**2, axis=1))))


# ---------------------------------------------------------------------------- transients

_HF_SOS = sps.butter(4, [2000, 12000], btype="bandpass", fs=SR, output="sos")


def hf_env_db(x: np.ndarray, frame: int = 144, hop: int = 24) -> np.ndarray:
    """2-12 kHz band envelope in dB (3 ms frames, 0.5 ms hop): where pick attacks live."""
    return db(rms_env(sps.sosfilt(_HF_SOS, x), frame, hop) ** 2)


def reattack_rise_db(env_db: np.ndarray, hop_s: float, onset: float, win: float = 0.08,
                     skip: float = 0.012, look: float = 0.008) -> float | None:
    """Largest HF-envelope rise (dB over the preceding `look`) between onset+skip and onset+win.
    Absolute values overlap between real flams, vocoder ripple and low notes' natural period
    structure, so callers report output minus ideal (flam_db), never a thresholded count."""
    i0, i1 = int((onset + skip) / hop_s), min(int((onset + win) / hop_s), len(env_db))
    lb = max(int(look / hop_s), 1)
    if i1 - i0 < 3 or i0 - lb < 0:
        return None
    floor = env_db[max(i0 - int(0.02 / hop_s), 0):i1].max() - 40.0
    e = np.maximum(env_db, floor)
    return float(max(e[i] - e[i - lb:i].min() for i in range(i0, i1)))


def rise_time(env: np.ndarray, hop_s: float, onset: float, win: float = 0.08) -> float | None:
    i0, i1 = max(int((onset - 0.005) / hop_s), 0), int((onset + win) / hop_s)
    seg = env[i0:i1]
    if len(seg) < 4 or seg.max() <= 1e-5:
        return None
    pk = int(np.argmax(seg))
    lo = np.argmax(seg[: pk + 1] >= 0.1 * seg[pk])
    hi = np.argmax(seg[: pk + 1] >= 0.9 * seg[pk])
    return max(hi - lo, 1) * hop_s


def discontinuity_db(x: np.ndarray, t0: float, t1: float) -> float | None:
    i0, i1 = int(t0 * SR), int(t1 * SR)
    seg = x[i0:i1]
    if len(seg) < 4800:
        return None
    d2 = np.abs(np.diff(seg, 2))
    loc = np.sqrt(np.convolve(d2**2, np.ones(960) / 960, mode="same")) + 1e-9
    return float(db(np.max(d2 / loc) ** 2))


# ---------------------------------------------------------------------------- modulation (granular warble)
#
# Grain/splice shifters leave slow periodic FM and AM on every partial (sidebands a few to a few tens of Hz from
# each partial). SINAD's +-10 cent partial tolerance counts those as signal and the fundamental-only IF/AM metrics
# miss them on upper partials, so they are measured directly here, in the 3-70 Hz fluctuation/roughness range,
# always as EXCESS over the ideal render (which carries any natural decay, drift or vibrato).

MOD_LO, MOD_HI = 3.0, 70.0
PITCHED_MOD_KINDS = {"sine", "harmonic", "pluck", "decay", "dyad", "chord", "vibrato", "bend", "sweep", "formant",
                     "guitar", "gchord"}


def band_rms(x: np.ndarray, fs: float, lo: float, hi: float) -> float:
    """RMS of x restricted to [lo, hi] Hz after removing its linear trend (Parseval on a Tukey-windowed FFT)."""
    n = len(x)
    if n < 32 or hi <= lo:
        return 0.0
    t = np.arange(n)
    x = x - np.polyval(np.polyfit(t, x, 1), t)
    w = sps.windows.tukey(n, 0.2)
    X = np.fft.rfft(x * w)
    f = np.fft.rfftfreq(n, 1.0 / fs)
    band = (f >= lo) & (f <= hi)
    return float(np.sqrt(2.0 * np.sum(np.abs(X[band]) ** 2) / (n * np.sum(w**2))))


def _demodulate(x: np.ndarray, fk: float, cutoff: float, dec: int = 24):
    """Complex demodulation of the partial at fk: (cents deviation, amplitude dB, mean power, decimated rate)."""
    t = np.arange(len(x)) / SR
    base = x * np.exp(-2j * np.pi * fk * t)
    sos = sps.butter(4, cutoff, fs=SR, output="sos")
    z = sps.sosfiltfilt(sos, base.real)[::dec] + 1j * sps.sosfiltfilt(sos, base.imag)[::dec]
    fs2 = SR / dec
    trim = max(int(0.05 * len(z)), int(0.03 * fs2))
    z = z[trim:len(z) - trim]
    if len(z) < 64:
        return None
    amp = np.abs(z)
    dev_hz = np.gradient(np.unwrap(np.angle(z))) * fs2 / (2 * np.pi)
    cents_dev = 1200.0 * np.log2(np.maximum(fk + dev_hz, 1e-3) / fk)
    return cents_dev, 20.0 * np.log10(amp + 1e-12), float(np.mean(amp**2)), fs2


def partial_modulation(seg: np.ndarray, iseg: np.ndarray, partials: list[float], max_partials: int = 8) -> dict:
    """Energy-weighted FM (cents rms) and AM (dB rms) excess over the ideal, 3-70 Hz, across isolated partials."""
    ps = sorted(p for p in partials if 40.0 < p < 6000.0)
    picked = []
    for i, fk in enumerate(ps):
        neighbours = [abs(fk - q) for j, q in enumerate(ps) if j != i]
        spacing = min(neighbours) if neighbours else fk
        cutoff = min(80.0, 0.4 * spacing)
        if cutoff >= 8.0:
            picked.append((fk, cutoff))
    if not picked:
        return {}
    fm_num = am_num = wsum = 0.0
    for fk, cutoff in picked[: max_partials * 3]:
        do, di = _demodulate(seg, fk, cutoff), _demodulate(iseg, fk, cutoff)
        if do is None or di is None:
            continue
        c_o, a_o, _, fs2 = do
        c_i, a_i, p_i, _ = di
        hi = min(MOD_HI, 0.9 * cutoff)
        fm = max(band_rms(c_o, fs2, MOD_LO, hi) ** 2 - band_rms(c_i, fs2, MOD_LO, hi) ** 2, 0.0)
        am = max(band_rms(a_o, fs2, MOD_LO, hi) ** 2 - band_rms(a_i, fs2, MOD_LO, hi) ** 2, 0.0)
        fm_num += p_i * fm
        am_num += p_i * am
        wsum += p_i
    if wsum <= 0:
        return {}
    return {"fm_rough_cents": float(np.sqrt(fm_num / wsum)), "am_rough_db": float(np.sqrt(am_num / wsum))}


def envelope_modulation(seg: np.ndarray, iseg: np.ndarray, nfft: int = 512, hop: int = 96, lo_hz: float = 200.0,
                        mod_lo: float = MOD_LO, mod_hi: float = 64.0, weight_pow: float = 1.0) -> float | None:
    """Weighted excess of 1/3-octave band log-envelope fluctuation (mod_lo..mod_hi Hz, dB rms) over the ideal.
    Catches grain AM on anything, including chords and partials too close together to demodulate.
    weight_pow < 1 compresses band energies so quieter mid/high bands (where roughness is heard) still count."""
    if len(seg) < 8 * nfft:
        return None
    fsm = SR / hop
    edges = lo_hz * 2 ** (np.arange(0, np.log2(8000 / lo_hz) * 3 + 1) / 3)

    def bands(x):
        f, _, Z = sps.stft(x, SR, nperseg=nfft, noverlap=nfft - hop, boundary=None, padded=False)
        P = np.abs(Z) ** 2
        return np.stack([P[(f >= lo) & (f < hi)].sum(axis=0) for lo, hi in zip(edges[:-1], edges[1:])], axis=1)

    Bo, Bi = bands(seg), bands(iseg)
    k = min(len(Bo), len(Bi))
    Bo, Bi = Bo[:k], Bi[:k]
    energy = Bi.mean(axis=0)
    if energy.max() <= 0:
        return None
    active = energy > energy.max() * 1e-3
    weight = energy ** weight_pow
    floor = Bi.max() * 1e-6
    num = 0.0
    for b in np.nonzero(active)[0]:
        ro = band_rms(10 * np.log10(Bo[:, b] + floor), fsm, mod_lo, mod_hi)
        ri = band_rms(10 * np.log10(Bi[:, b] + floor), fsm, mod_lo, mod_hi)
        num += weight[b] * max(ro**2 - ri**2, 0.0)
    return float(np.sqrt(num / weight[active].sum()))


def note_span(ideal: np.ndarray, onsets: list[float]) -> tuple[int, int]:
    """Whole-note analysis span: 50 ms after the first onset until the ideal's envelope falls 40 dB below its peak."""
    env = rms_env(ideal, 2400, 480)
    if env.max() <= 0:
        return 0, 0
    live = np.nonzero(env > env.max() * 1e-2)[0]
    start = int(((onsets[0] if onsets else 0.0) + 0.05) * SR)
    end = int(live[-1] * 480 + 2400) if len(live) else 0
    return max(start, int(live[0] * 480)) if len(live) else start, min(end, len(ideal))


def grain_noise_p90(ya: np.ndarray, ideal: np.ndarray, sig: Signal, semis: float, a: int, b: int,
                    frame_s: float = 0.15, hop_s: float = 0.05) -> float | None:
    """Short-time inharmonic residual excess over the ideal, frame by frame through the note (skipping 80 ms after
    each onset), reported at the 90th percentile: intermittent granular distortion that averages away elsewhere."""
    fl, hp = int(frame_s * SR), int(hop_s * SR)
    starts = list(range(a, max(a, b - fl), hp))
    if len(starts) < 3:
        return None
    energies = [float(np.sum(ideal[i:i + fl] ** 2)) for i in starts]
    emax = max(energies)
    vals = []
    for i, e in zip(starts, energies):
        if e < emax * 1e-3:
            continue
        t0, t1 = i / SR, (i + fl) / SR
        if any(o - 0.01 <= t1 and t0 <= o + 0.08 for o in sig.onsets):
            continue
        partials = sig.expected_partials(semis, t0, t1)
        if not partials:
            continue
        s, n, _ = partial_energy_split(ya[i:i + fl], partials, pad=1 << 15)
        si, ni, _ = partial_energy_split(ideal[i:i + fl], partials, pad=1 << 15)
        vals.append(float(db(max(n / (s + EPS) - ni / (si + EPS), 1e-10))))
    return float(np.percentile(vals, 90)) if len(vals) >= 3 else None


# ---------------------------------------------------------------------------- main entry

def measure(sig: Signal, semis: float, y: np.ndarray, reported_latency: int | None = None, extra: dict | None = None) -> dict:
    y = np.asarray(y, dtype=np.float64)
    kind = sig.spec["kind"]
    ratio = 2.0 ** (semis / 12.0)
    ideal = sig.render(semis).astype(np.float64)
    m: dict = {"nonfinite": int(np.count_nonzero(~np.isfinite(y)))}
    y = np.nan_to_num(y)
    if reported_latency is not None:
        m["lat_reported_ms"] = reported_latency * 1000.0 / SR
    if extra:
        m.update(extra)

    if kind == "silence":
        m["silence_dbfs"] = float(db(np.mean(y**2)))
        return m

    lat = measure_latency(sig, y, ideal)
    m.update(lat)
    lat_ms = lat.get("lat_ms", lat.get("lat_xcorr_ms", 0.0))
    ya = align(y, lat_ms, sig.n)

    # level over the part of the ideal with energy
    act = np.abs(ideal) > 0
    if act.any():
        ei, ey = rms_env(ideal, 2400, 1200), rms_env(ya, 2400, 1200)
        on = ei > ei.max() * 0.1
        m["level_db"] = float(db(np.sum(ey[on] ** 2)) - db(np.sum(ei[on] ** 2)))

    m["lsd_db"] = log_spectral_distance(ya, ideal)

    # modulation artefacts (granular warble): steady window when there is one, else the active span of the ideal
    if kind in PITCHED_MOD_KINDS:
        if sig.steady:
            s0, s1 = int(sig.steady[0] * SR), int(sig.steady[1] * SR)
        else:
            on_idx = np.nonzero(np.abs(ideal) > np.max(np.abs(ideal)) * 1e-2)[0]
            s0, s1 = (int(on_idx[0] + 0.05 * SR), int(on_idx[-1] - 0.05 * SR)) if len(on_idx) else (0, 0)
        if s1 - s0 > int(0.3 * SR):
            seg_m, iseg_m = ya[s0:s1], ideal[s0:s1]
            m["env_mod_db"] = envelope_modulation(seg_m, iseg_m)
            if sig.steady:
                m.update(partial_modulation(seg_m, iseg_m, sig.expected_partials(semis, *sig.steady)))
        # whole note: attack settle and decay are where the tracker locks/unlocks and grain lengths jump
        a, b = note_span(ideal, sig.onsets)
        if b - a > int(0.3 * SR):
            m["env_mod_note_db"] = envelope_modulation(ya[a:b], ideal[a:b])
            m["env_mod_hi_db"] = envelope_modulation(ya[a:b], ideal[a:b], nfft=128, hop=24, lo_hz=500.0,
                                                     mod_lo=64.0, mod_hi=300.0, weight_pow=0.3)
            if kind in MONO_KINDS | POLY_KINDS:
                m["grain_noise_p90_db"] = grain_noise_p90(ya, ideal, sig, semis, a, b)

    if sig.steady and kind in MONO_KINDS | POLY_KINDS:
        t0, t1 = sig.steady
        seg = ya[int(t0 * SR):int(t1 * SR)]
        iseg = ideal[int(t0 * SR):int(t1 * SR)]
        partials = sig.expected_partials(semis, t0, t1)
        s, n, hf = partial_energy_split(seg, partials)
        si_, ni_, hfi = partial_energy_split(iseg, partials)
        # Excess over the ideal's own leakage (decaying partials, window), so only processor damage counts.
        key = "sinad_db" if kind in MONO_KINDS else "sinad_poly_db"
        m[key] = float(-db(max(n / (s + EPS) - ni_ / (si_ + EPS), 1e-10)))
        m["hf_junk_db"] = float(db(max(hf / (s + EPS) - hfi / (si_ + EPS), 1e-12)))

        if kind in MONO_KINDS:
            f_exp = float(np.median(sig.notes[0].f_curve)) * ratio
            f0s = steady_f0_track(ya, t0, t1, f_exp, 1 if kind == "sine" else 4)
            if len(f0s):
                err = np.abs(cents(f0s, f_exp))
                m["pitch_err_cents"] = float(np.median(err))
                m["pitch_err_p95_cents"] = float(np.percentile(err, 95))
            fi, env = inst_freq(seg, f_exp * 0.8, f_exp * 1.25)
            keep = env > env.max() * 0.3
            trim = int(max(0.07 * len(seg), 10.0 / f_exp * SR))
            if keep[trim:-trim].sum() > SR * 0.1:
                dev = np.abs(cents(np.maximum(fi[trim:-trim][keep[trim:-trim]], 1.0), f_exp))
                m["if_dev_cents"] = float(np.percentile(dev, 95))
            mag, hpb = spectrum(seg, 1 << 18)
            _, a1 = find_peak_hz(mag, hpb, f_exp, 30)
            _, ah = find_peak_hz(mag, hpb, f_exp / 2, 30)
            m["subharm_db"] = float(db(ah**2) - db(a1**2))
            # envelope modulation of output relative to ideal (removes natural decay)
            # whole periods + Hann weighting: no waveform ripple in the envelope to misalign
            frame = int(round(max(2, np.ceil(0.005 * f_exp)) * SR / f_exp))
            ry = rms_env(seg, frame, frame // 4, hann=True)
            ri = rms_env(iseg, frame, frame // 4, hann=True)
            k = min(len(ry), len(ri))
            if k > 16:
                r = db(ry[:k] ** 2) - db(ri[:k] ** 2)
                r = r - np.polyval(np.polyfit(np.arange(k), r, 1), np.arange(k))
                m["am_pp_db"] = float(np.percentile(r, 98) - np.percentile(r, 2))
                spec_r = np.abs(np.fft.rfft(r * np.hanning(k), 8192))
                fr = np.fft.rfftfreq(8192, (frame // 4) / SR)
                band = (fr >= 1.0) & (fr <= 200.0)
                m["am_rate_hz"] = float(fr[band][np.argmax(spec_r[band])])
        else:
            errs = []
            all_f0 = [float(np.median(nt.f_curve)) * ratio for nt in sig.notes]
            mag, hpb = spectrum(seg, 1 << 18)
            for j, f in enumerate(all_f0):
                others = [k * g for i, g in enumerate(all_f0) if i != j for k in range(1, 12)]
                if any(abs(cents(o, f)) < 60 for o in others):
                    continue   # fundamental overlaps another note's partial
                fe, _ = find_peak_hz(mag, hpb, f, 40)
                errs.append(abs(float(cents(fe, f))))
            if errs:
                m["poly_pitch_err_cents"] = float(np.max(errs))
        m["disc_db"] = (discontinuity_db(ya, t0, t1) or 0.0) - (discontinuity_db(ideal, t0, t1) or 0.0)

    if kind in TRACK_KINDS:
        nt = sig.notes[0]
        fc = nt.f_curve * ratio
        on = nt.env > nt.env.max() * 0.5
        lo, hi = float(fc[on].min()), float(fc[on].max())
        fi, env = inst_freq(ya, lo * 0.8, hi * 1.25) if kind != "sweep" else inst_freq(ya, lo * 0.7, hi * 1.4)
        valid = on[1:] & (env > env.max() * 0.2)
        trim = int(0.05 * SR)
        valid[:trim] = False
        valid[-trim:] = False
        if valid.sum() > SR * 0.1:
            m["track_err_cents"] = float(np.median(np.abs(cents(np.maximum(fi[valid], 1.0), fc[1:][valid]))))

    if kind in ONSET_KINDS and sig.onsets:
        hop_s = 24 / SR
        hy, hi_ = hf_env_db(ya), hf_env_db(ideal)
        ey, ei = rms_env(ya, 48, 24), rms_env(ideal, 48, 24)
        flams, smear, pre = [], [], []
        for j, o in enumerate(sig.onsets):
            win = min(0.08, (sig.onsets[j + 1] - o) * 0.9) if j + 1 < len(sig.onsets) else 0.08
            ry_, ri_ = reattack_rise_db(hy, hop_s, o, win), reattack_rise_db(hi_, hop_s, o, win)
            if ry_ is not None and ri_ is not None:
                flams.append(max(ry_ - ri_, 0.0))
            ry, ri = rise_time(ey, 24 / SR, o, win), rise_time(ei, 24 / SR, o, win)
            if ry and ri:
                smear.append(abs(np.log2(ry / ri)))
            i = int(o * SR)
            pre_len = int(min(0.05, (o - sig.onsets[j - 1]) * 0.5 if j else 0.05) * SR)
            e_pre = np.sum(ya[max(i - pre_len, 0):i - int(0.003 * SR)] ** 2)
            e_post = np.sum(ya[i:i + int(0.05 * SR)] ** 2)
            if e_post > 0:
                pre.append(float(db(e_pre) - db(e_post)))
        m["flam_db"] = float(np.mean(flams)) if flams else None
        m["attack_smear"] = float(np.mean(smear)) if smear else None
        m["pre_echo_db"] = float(np.mean(pre)) if pre else None

    return {k: v for k, v in m.items() if v is not None}
