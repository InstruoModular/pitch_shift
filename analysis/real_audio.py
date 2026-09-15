"""Reference-free analysis of REAL audio (sax, DI, riffs) through Archetype and Shift variants.

    python analysis/real_audio.py <file.wav> [--shifts -12,-7,7,12] [--targets archetype,shift:current,shift:smooth]
                                  [--params-smooth "k=v;..."] [--mono-f0]

There is no analytic ideal for real audio, so every measure compares the OUTPUT with the INPUT's own behaviour:
  lat_ms          output-vs-input broadband envelope cross-correlation (envelopes don't change with pitch)
  fm_rough_c      (--mono-f0 material) partials demodulated along k*r*f0(t) in the output vs k*f0(t) in the input;
                  energy-weighted FM rms excess over the input in 3-70 Hz (phase-insensitive)
  am_rough_db     same, amplitude modulation
  harm_drop_p90   short-time (80 ms) harmonic-to-residual ratio of the input minus that of the output, p90 (dB, higher = worse)
  mod_lines_db    energy of NEW band-envelope modulation lines in 8-120 Hz: output 1-4 kHz band modulation spectrum minus
                  the input's (mapped band), peaks only (granular splice-rate lines), dB above input
Renders are kept in build/real_audio/<stem>/ for listening.
"""
from __future__ import annotations

import argparse
import os
import subprocess
import sys
import warnings
from pathlib import Path

import numpy as np
from scipy import signal as sps
from scipy.io import wavfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import metrics  # noqa: E402
import run_suite  # noqa: E402

SR = 48000
ROOT = Path(__file__).resolve().parent.parent
ARCHETYPE = "Archetype Misha Mansoor X.dll"
ARCH_NEUTRAL = ("Input Gain=0.5;Output Gain=0.5;Gate Active=0;Doubler Active=0;Special FX Section Active=0;"
                "Pre FX Section Active=0;Amp Section Active=0;Cab Section Active=0;EQ Section Active=0;Post FX Section Active=0")


def load_48k_mono(path: Path) -> np.ndarray:
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        sr, x = wavfile.read(path)
    x = np.asarray(x)
    if np.issubdtype(x.dtype, np.integer):
        x = x.astype(np.float64) / np.iinfo(x.dtype).max
    x = x.astype(np.float64)
    if x.ndim > 1:
        x = x.mean(axis=1)
    if sr != SR:
        g = np.gcd(sr, SR)
        x = sps.resample_poly(x, SR // g, sr // g)
    return x


def render(target: str, inp: Path, out_dir: Path, shifts: list[int], params_smooth: str) -> dict[int, Path]:
    out_dir.mkdir(parents=True, exist_ok=True)
    name = target.replace(":", "-")
    outs = {s: out_dir / f"{name}_{s:+d}.wav" for s in shifts}
    man = out_dir / f"{name}.tsv"
    if target == "archetype":
        info = run_suite.plugin_info(Path(ARCHETYPE), probe=["Transpose"])
        to_norm = run_suite.shift_normaliser(info, "Transpose")
        man.write_text("".join(f"{inp}\t{outs[s]}\t{ARCH_NEUTRAL};Transpose={to_norm(s):.6f}\n" for s in shifts))
        cmd = [str(run_suite.VST2HOST), "run", str(ROOT / ARCHETYPE), "--manifest", str(man), "--block", "32"]
        env = dict(os.environ)
    else:
        variant = target.split(":", 1)[1]
        extra = f";{params_smooth}" if (variant != "current" and params_smooth) else ""
        man.write_text("".join(f"{inp}\t{outs[s]}\tshift={s}{extra}\n" for s in shifts))
        cmd = [str(run_suite.SHIFTBENCH), "run", variant, "--manifest", str(man)]
        env = dict(os.environ, PATH=run_suite.MSYS_BIN + os.pathsep + os.environ["PATH"])
    r = subprocess.run(cmd, cwd=ROOT, env=env, capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"{target} render failed:\n{r.stdout[-1500:]}\n{r.stderr[-1500:]}")
    return outs


def f0_track(x: np.ndarray, hop: int = 240, fmin: float = 60.0, fmax: float = 1200.0, thresh: float = 0.15):
    """Plain YIN on 40 ms frames: (times s, f0 Hz or nan)."""
    frame = int(0.04 * SR)
    taus = np.arange(int(SR / fmax), int(SR / fmin) + 1)
    times, f0s = [], []
    for start in range(0, len(x) - frame - taus[-1], hop):
        seg = x[start:start + frame + taus[-1]]
        if np.sqrt(np.mean(seg[:frame] ** 2)) < 1e-3:
            times.append((start + frame / 2) / SR)
            f0s.append(np.nan)
            continue
        d = np.array([np.sum((seg[:frame] - seg[t:t + frame]) ** 2) for t in taus])
        cmnd = d * np.arange(1, len(d) + 1) / (np.cumsum(d) + 1e-20)
        below = np.nonzero(cmnd < thresh)[0]
        if len(below) == 0:
            f0s.append(np.nan)
        else:
            i = below[0]
            while i + 1 < len(cmnd) and cmnd[i + 1] < cmnd[i]:
                i += 1
            if 0 < i < len(cmnd) - 1:
                a, b, c = cmnd[i - 1], cmnd[i], cmnd[i + 1]
                den = a - 2 * b + c
                i = i + (0.5 * (a - c) / den if den > 0 else 0.0)
            f0s.append(SR / (taus[0] + i))
        times.append((start + frame / 2) / SR)
    return np.array(times), np.array(f0s)


def demod_along(x: np.ndarray, f_curve: np.ndarray, cutoff: float = 60.0, dec: int = 24):
    phase = 2 * np.pi * np.cumsum(f_curve) / SR
    base = x * np.exp(-1j * phase)
    sos = sps.butter(4, cutoff, fs=SR, output="sos")
    z = sps.sosfiltfilt(sos, base.real)[::dec] + 1j * sps.sosfiltfilt(sos, base.imag)[::dec]
    fs2 = SR / dec
    dev = np.gradient(np.unwrap(np.angle(z))) * fs2 / (2 * np.pi)
    return z, dev, fs2


def partial_roughness(x: np.ndarray, y: np.ndarray, f0_in: np.ndarray, ratio: float, partials: int = 6):
    """FM (cents) and AM (dB) rms excess of output over input, 3-70 Hz, along the tracked partials, voiced runs only."""
    voiced = np.isfinite(f0_in)
    runs, start = [], None
    for i, v in enumerate(np.append(voiced, False)):
        if v and start is None:
            start = i
        elif not v and start is not None:
            if i - start >= int(0.4 * SR):
                runs.append((start, i))
            start = None
    fm_num = am_num = w = 0.0
    for a, b in runs:
        a2, b2 = a + int(0.05 * SR), b - int(0.05 * SR)
        f_in = f0_in[a2:b2]
        for k in range(1, partials + 1):
            fk_in, fk_out = k * f_in, k * f_in * ratio
            if fk_out.max() > 6000:
                break
            cut = min(60.0, 0.4 * np.min(f_in) * min(ratio, 1.0))
            zi, di, fs2 = demod_along(x[a2:b2], fk_in, cut)
            zo, do, _ = demod_along(y[a2:b2], fk_out, cut)
            ci = 1200 * np.log2(np.maximum(fk_in[::24][:len(di)] + di, 1e-3) / fk_in[::24][:len(di)])
            co = 1200 * np.log2(np.maximum(fk_out[::24][:len(do)] + do, 1e-3) / fk_out[::24][:len(do)])
            fm = max(metrics.band_rms(co, fs2, 3, 0.9 * cut) ** 2 - metrics.band_rms(ci, fs2, 3, 0.9 * cut) ** 2, 0.0)
            am = max(metrics.band_rms(20 * np.log10(np.abs(zo) + 1e-9), fs2, 3, 0.9 * cut) ** 2
                     - metrics.band_rms(20 * np.log10(np.abs(zi) + 1e-9), fs2, 3, 0.9 * cut) ** 2, 0.0)
            p = float(np.mean(np.abs(zi) ** 2))
            fm_num += p * fm
            am_num += p * am
            w += p
    if w <= 0:
        return None, None
    return float(np.sqrt(fm_num / w)), float(np.sqrt(am_num / w))


def harmonic_ratio_db(x: np.ndarray, f0: float, frame: np.ndarray) -> float:
    mag, hpb = metrics.spectrum(frame, 1 << 15, bh=True)
    f = np.arange(len(mag)) * hpb
    band = (f > 50) & (f < 8000)
    near = np.zeros_like(band)
    k = 1
    while k * f0 < 8000:
        near |= np.abs(f - k * f0) < max(0.03 * k * f0, 6.0)
        k += 1
    p = mag**2
    return float(10 * np.log10((p[band & near].sum() + 1e-20) / (p[band & ~near].sum() + 1e-20)))


def harm_drop(x: np.ndarray, y: np.ndarray, times: np.ndarray, f0_in: np.ndarray, ratio: float) -> float | None:
    fl = int(0.08 * SR)
    drops = []
    for t, f in zip(times[::8], f0_in[::8]):
        if not np.isfinite(f):
            continue
        i = int(t * SR) - fl // 2
        if i < 0 or i + fl > min(len(x), len(y)):
            continue
        drops.append(harmonic_ratio_db(x, f, x[i:i + fl]) - harmonic_ratio_db(y, f * ratio, y[i:i + fl]))
    return float(np.percentile(drops, 90)) if len(drops) >= 5 else None


def mod_lines(x: np.ndarray, y: np.ndarray, ratio: float) -> float:
    """dB of the strongest NEW modulation line (8-120 Hz) in the output's 1-4 kHz envelope vs the input's mapped band."""
    def spec(sig, lo, hi):
        f, _, Z = sps.stft(sig, SR, nperseg=256, noverlap=256 - 48, boundary=None, padded=False)
        e = (np.abs(Z) ** 2)[(f >= lo) & (f < hi)].sum(axis=0)
        L = 10 * np.log10(e + e.max() * 1e-6)
        L = L - np.polyval(np.polyfit(np.arange(len(L)), L, 1), np.arange(len(L)))
        S = np.abs(np.fft.rfft(L * np.hanning(len(L)), 1 << 16)) / len(L)
        fm = np.fft.rfftfreq(1 << 16, 48 / SR)
        m = (fm >= 8) & (fm <= 120)
        return S[m]
    so = spec(y, 1000, 4000)
    si = spec(x, 1000 / ratio, 4000 / ratio)
    n = min(len(so), len(si))
    excess = 20 * np.log10((so[:n] + 1e-9) / (sps.medfilt(si[:n], 9) + 1e-9))
    return float(np.max(excess))


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("wav")
    ap.add_argument("--shifts", default="-12,-7,7,12")
    ap.add_argument("--targets", default="archetype,shift:current")
    ap.add_argument("--params-smooth", default="")
    ap.add_argument("--mono-f0", action="store_true", help="material is monophonic: run the partial FM/AM analysis")
    a = ap.parse_args()

    src = Path(a.wav)
    stem = src.stem[:40]
    out_dir = ROOT / "build/real_audio" / stem
    out_dir.mkdir(parents=True, exist_ok=True)
    x = load_48k_mono(src)
    inp = out_dir / "input_48k.wav"
    wavfile.write(str(inp), SR, x.astype(np.float32))
    shifts = [int(s) for s in a.shifts.split(",")]
    targets = [t.strip() for t in a.targets.split(",") if t.strip()]

    times = f0_in = None
    if a.mono_f0:
        times, f0_frames = f0_track(x)
        f0_in = np.interp(np.arange(len(x)) / SR, times, np.nan_to_num(f0_frames, nan=0.0))
        f0_in[f0_in < 50] = np.nan
        print(f"input: {len(x) / SR:.2f} s, voiced {100 * np.mean(np.isfinite(f0_frames)):.0f} % of frames, "
              f"median f0 {np.nanmedian(f0_frames):.1f} Hz")

    print(f"{'target':<16}{'shift':>6}{'lat_ms':>8}{'fm_rough_c':>12}{'am_rough_db':>12}{'harm_drop_p90':>15}{'mod_lines_db':>14}")
    for target in targets:
        outs = render(target, inp, out_dir, shifts, a.params_smooth)
        for s in shifts:
            with warnings.catch_warnings():
                warnings.simplefilter("ignore")
                _, y = wavfile.read(outs[s])
            y = y.astype(np.float64)
            ex = metrics.rms_env(x, 240, 48)
            ey = metrics.rms_env(y, 240, 48)
            n = min(len(ex), len(ey))
            xc = sps.correlate(ey[:n] - ey[:n].mean(), ex[:n] - ex[:n].mean(), mode="full", method="fft")
            mid = n - 1
            lag_frames = int(np.argmax(xc[mid: mid + int(0.25 * SR / 48)]))
            lat = lag_frames * 48 / SR * 1000
            ya = np.zeros(len(x))
            d = lag_frames * 48
            seg = y[d:d + len(x)]
            ya[:len(seg)] = seg
            ratio = 2 ** (s / 12)
            fm = am = hd = None
            if a.mono_f0:
                fm, am = partial_roughness(x, ya, f0_in, ratio)
                hd = harm_drop(x, ya, times, f0_frames, ratio)
            ml = mod_lines(x, ya, ratio)
            fmt = lambda v, p=2: f"{v:.{p}f}" if v is not None else "-"  # noqa: E731
            print(f"{target:<16}{s:>+6d}{lat:>8.1f}{fmt(fm):>12}{fmt(am):>12}{fmt(hd, 1):>15}{fmt(ml, 1):>14}")
    print(f"renders kept in {out_dir}")


if __name__ == "__main__":
    main()
