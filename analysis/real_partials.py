"""Per-window partial tracking on REAL audio: how steady is each partial in the OUTPUT vs the INPUT?

    python analysis/real_partials.py <file.wav> [--shifts=-12,-7,7,12] [--targets archetype,shift:current]
                                     [--params-smooth "k=v;..."] [--per-window]

real_audio.py needs one f0 contour for the whole file, so it breaks on material that changes octave and it
cannot read chords at all. This tool needs neither: in each 0.5 s window it picks the input's strongest
spectral peaks, finds the matching output peak near f*ratio, and demodulates BOTH at their own measured
frequency (fixed heterodyne, so nothing can lose lock). Works on chords, riffs and note changes.

  warble_c  rms excess of output partial FM over the input's, 3-20 Hz, cents, power-weighted over partials
  rough_c   the same in 20-70 Hz (the roughness band)
  am_db     amplitude-modulation excess, 3-70 Hz, dB
  lost_%    input partials with no output peak within +-60 cents
  p10_dB /  partial LEVEL transfer: output energy at f*ratio vs input energy at f, each normalised by its
  <-6dB%    window's energy. Median is ~0 for any working shifter; the low tail is what grain cancellation
            does. This is the measure that matches what the ear calls "granular" / "scrambled" on guitar.

Both signals are time-aligned by broadband envelope cross-correlation first (lat_ms is that lag).
Floor check: any target at 0 semitones reads ~0 on every column.
"""
from __future__ import annotations
import argparse, glob, os, subprocess, sys, warnings
from pathlib import Path
import numpy as np
from scipy import signal as sps
from scipy.io import wavfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import metrics  # noqa: E402
import real_audio as ra  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
SR = 48000
BANDS = [(0.5, 3.0, "drift"), (3.0, 20.0, "warble"), (20.0, 70.0, "rough")]


def peaks(seg, fmin=70.0, fmax=4000.0, top=8, floor_db=32.0):
    nfft = 1 << 16
    w = sps.windows.blackmanharris(len(seg))
    S = np.abs(np.fft.rfft(seg*w, nfft))
    hz = SR/nfft
    f = np.arange(len(S))*hz
    sel = (f >= fmin) & (f <= fmax)
    if S[sel].max() <= 0: return []
    thr = S[sel].max() * 10**(-floor_db/20)
    idx = sps.argrelmax(S, order=5)[0]
    idx = idx[sel[idx] & (S[idx] > thr)]
    if not len(idx): return []
    idx = idx[np.argsort(-S[idx])][:top]
    out = []
    for i in idx:
        a,b,c = np.log(S[i-1]+1e-30), np.log(S[i]+1e-30), np.log(S[i+1]+1e-30)
        den = a-2*b+c
        d = 0.5*(a-c)/den if den != 0 else 0.0
        out.append(((i+d)*hz, float(S[i])))
    return sorted(out)


def demod_fixed(x, fc, cutoff, dec=24):
    n = np.arange(len(x))
    z = x*np.exp(-2j*np.pi*fc*n/SR)
    sos = sps.butter(4, cutoff, fs=SR, output="sos")
    z = sps.sosfiltfilt(sos, z.real)[::dec] + 1j*sps.sosfiltfilt(sos, z.imag)[::dec]
    fs2 = SR/dec
    dev = np.gradient(np.unwrap(np.angle(z)))*fs2/(2*np.pi)     # Hz offset from fc
    return z, dev, fs2


def analyse(x, y, ratio, win_s=0.5, hop_s=0.25, max_partials=6):
    W, H = int(win_s*SR), int(hop_s*SR)
    acc = {n: [0.0,0.0] for _,_,n in BANDS}; am=[0.0,0.0]; devs=[]; lost=0; tot=0
    n = min(len(x), len(y))
    rms_all = np.array([np.sqrt(np.mean(x[i:i+W]**2)) for i in range(0, n-W, H)])
    if rms_all.size == 0: return None
    loud = rms_all > 0.15*rms_all.max()
    for j, i in enumerate(range(0, n-W, H)):
        if not loud[j]: continue
        xs, ys = x[i:i+W], y[i:i+W]
        pin = peaks(xs)
        if not pin: continue
        pout = peaks(ys, fmin=70.0*min(ratio,1.0)*0.9, fmax=4000.0*ratio*1.1)
        fo_arr = np.array([p[0] for p in pout]) if pout else np.array([])
        # isolation: nearest other input peak
        fin = np.array([p[0] for p in pin]); ain = np.array([p[1] for p in pin])
        order = np.argsort(-ain)[:max_partials]
        for oi in order:
            f_i, a_i = fin[oi], ain[oi]
            if f_i*ratio > 0.42*SR: continue          # partial would land above the analysis range
            others = np.delete(fin, oi)
            gap_in = np.min(np.abs(others - f_i)) if len(others) else 1e9
            f_e = f_i*ratio
            tot += 1
            if len(fo_arr):
                k = int(np.argmin(np.abs(fo_arr - f_e)))
                f_o, a_o = fo_arr[k], pout[k][1]
                cents_off = 1200*np.log2(f_o/f_e)
            else:
                f_o, a_o, cents_off = f_e, 0.0, 1e9
            if abs(cents_off) > 60:
                lost += 1
                continue
            others_o = np.delete(fo_arr, k)
            gap_out = np.min(np.abs(others_o - f_o)) if len(others_o) else 1e9
            cut = float(np.clip(0.35*min(gap_in, gap_out/max(ratio,1e-9)*ratio), 8.0, 60.0))
            zi, di, fs2 = demod_fixed(xs, f_i, cut)
            zo, do, _   = demod_fixed(ys, f_o, cut)
            m = min(len(di), len(do))
            ci = 1200*np.log2(np.maximum(f_i+di[:m], 1.0)/f_i)
            co = 1200*np.log2(np.maximum(f_o+do[:m], 1.0)/f_o)
            p = float(a_i**2)
            for lo,hi,name in BANDS:
                hi2 = min(hi, 0.9*cut)
                if hi2 <= lo: continue
                e = max(metrics.band_rms(co,fs2,lo,hi2)**2 - metrics.band_rms(ci,fs2,lo,hi2)**2, 0.0)
                acc[name][0] += p*e; acc[name][1] += p
            ai = 20*np.log10(np.abs(zi[:m])+1e-9); ao = 20*np.log10(np.abs(zo[:m])+1e-9)
            e = max(metrics.band_rms(ao,fs2,3,0.9*cut)**2 - metrics.band_rms(ai,fs2,3,0.9*cut)**2, 0.0)
            am[0]+=p*e; am[1]+=p
            if oi == order[0]:
                sos = sps.butter(2, [0.5, min(70,0.9*cut)], btype='band', fs=fs2, output='sos')
                devs.append(sps.sosfiltfilt(sos, co-ci))
    r = {n: (float(np.sqrt(acc[n][0]/acc[n][1])) if acc[n][1]>0 else float('nan')) for _,_,n in BANDS}
    r['am'] = float(np.sqrt(am[0]/am[1])) if am[1]>0 else float('nan')
    r['p95'] = float(np.percentile(np.abs(np.concatenate(devs)), 95)) if devs else float('nan')
    r['lost'] = 100.0*lost/max(tot,1)
    return r


def align(x, y):
    ex, ey = metrics.rms_env(x,240,48), metrics.rms_env(y,240,48)
    n = min(len(ex),len(ey))
    xc = sps.correlate(ey[:n]-ey[:n].mean(), ex[:n]-ex[:n].mean(), mode="full", method="fft")
    lagf = int(np.argmax(xc[n-1:n-1+int(0.25*SR/48)]))
    ya = np.zeros(len(x)); seg = y[lagf*48:lagf*48+len(x)]; ya[:len(seg)] = seg
    return ya, lagf*48/SR*1000


def load(p):
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        sr, v = wavfile.read(str(p))
    v = np.asarray(v)
    if np.issubdtype(v.dtype, np.integer): v = v.astype(np.float64)/np.iinfo(v.dtype).max
    return v.astype(np.float64)


def spec(seg, nfft=1<<16):
    w = sps.windows.blackmanharris(len(seg))
    S = np.abs(np.fft.rfft(seg*w, nfft))**2
    return S, SR/nfft

def level_err(x, y, ratio):
    W, H = int(0.5*SR), int(0.25*SR)
    n = min(len(x), len(y))
    rms = np.array([np.sqrt(np.mean(x[i:i+W]**2)) for i in range(0, n-W, H)])
    loud = rms > 0.15*rms.max()
    errs, lvls = [], []
    for j, i in enumerate(range(0, n-W, H)):
        if not loud[j]: continue
        pin = peaks(x[i:i+W])
        if not pin: continue
        Sx, hz = spec(x[i:i+W]); Sy, _ = spec(y[i:i+W])
        fx = np.arange(len(Sx))*hz
        ex = Sx[(fx>70)&(fx<5000)].sum(); ey = Sy[(fx>70*min(ratio,1))&(fx<5000*max(ratio,1))].sum()
        ain = np.array([p[1] for p in pin]); mx = ain.max()
        for oi in np.argsort(-ain)[:6]:
            f_i = pin[oi][0]; fe = f_i*ratio
            if fe > 0.42*SR: continue
            wi = (fx > f_i*2**(-0.35/12)) & (fx < f_i*2**(0.35/12))      # +-35 cents
            wo = (fx > fe*2**(-0.6/12)) & (fx < fe*2**(0.6/12))          # +-60 cents
            if not wi.any() or not wo.any(): continue
            a = Sx[wi].sum()/ex; b = Sy[wo].sum()/ey
            errs.append(10*np.log10((b+1e-30)/(a+1e-30)))
            lvls.append(20*np.log10(ain[oi]/mx))
    e = np.array(errs); l = np.array(lvls)
    return e, l


HEAD = (f"{'target':<18}{'shift':>6}{'lat_ms':>8}{'warble_c':>10}{'rough_c':>9}{'am_db':>8}"
        f"{'lost_%':>8}{'p10_dB':>8}{'<-6dB%':>8}")


def row(tag: str, s: int, x: np.ndarray, y: np.ndarray) -> str:
    ya, lat = align(x, y)
    ratio = 2 ** (s / 12)
    r = analyse(x, ya, ratio)
    e, _ = level_err(x, ya, ratio)
    return (f"{tag:<18}{s:>+6d}{lat:>8.1f}{r['warble']:>10.2f}{r['rough']:>9.2f}{r['am']:>8.2f}"
            f"{r['lost']:>8.1f}{np.percentile(e, 10):>8.2f}{100 * np.mean(e < -6):>8.1f}")


def per_window_report(x: np.ndarray, ya: np.ndarray, ratio: float) -> None:
    """Where in the file does the shifter lose partials? (one row per 0.25 s)"""
    W, H = int(0.25 * SR), int(0.125 * SR)
    n = min(len(x), len(ya))
    rms = np.array([np.sqrt(np.mean(x[i:i + W] ** 2)) for i in range(0, n - W, H)])
    loud = rms > 0.15 * rms.max()
    print(f"{'t(s)':>7}{'worst_dB':>10}{'below -6 dB':>14}")
    for j, i in enumerate(range(0, n - W, H)):
        if not loud[j]:
            continue
        pin = peaks(x[i:i + W])
        if not pin:
            continue
        Sx, hz = spec(x[i:i + W])
        Sy, _ = spec(ya[i:i + W])
        fx = np.arange(len(Sx)) * hz
        ex = Sx[(fx > 70) & (fx < 5000)].sum()
        ey = Sy[(fx > 70 * min(ratio, 1)) & (fx < 5000 * max(ratio, 1))].sum()
        ain = np.array([p[1] for p in pin])
        errs = []
        for oi in np.argsort(-ain)[:6]:
            f_i = pin[oi][0]
            fe = f_i * ratio
            if fe > 0.42 * SR:
                continue
            wi = (fx > f_i * 2 ** (-0.35 / 12)) & (fx < f_i * 2 ** (0.35 / 12))
            wo = (fx > fe * 2 ** (-0.6 / 12)) & (fx < fe * 2 ** (0.6 / 12))
            errs.append(10 * np.log10((Sy[wo].sum() / ey + 1e-30) / (Sx[wi].sum() / ex + 1e-30)))
        e = np.array(errs)
        print(f"{i / SR:7.3f}{e.min():10.2f}{f'{int((e < -6).sum())}/{len(e)}':>14}")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("wav")
    ap.add_argument("--shifts", default="-12,-7,7,12")
    ap.add_argument("--targets", default="archetype,shift:current")
    ap.add_argument("--params-smooth", default="")
    ap.add_argument("--per-window", action="store_true", help="per-0.25 s partial level error for each render")
    a = ap.parse_args()

    src = Path(a.wav)
    stem = src.stem[:40]
    out_dir = ROOT / "build/real_audio" / stem
    out_dir.mkdir(parents=True, exist_ok=True)
    x = ra.load_48k_mono(src)
    inp = out_dir / "input_48k.wav"
    wavfile.write(str(inp), SR, x.astype(np.float32))
    shifts = [int(s) for s in a.shifts.split(",")]
    print(f"{stem}: {len(x) / SR:.2f} s")
    print(HEAD)
    for target in [t.strip() for t in a.targets.split(",") if t.strip()]:
        outs = ra.render(target, inp, out_dir, shifts, a.params_smooth)
        for s in shifts:
            y = load(outs[s])
            print(row(target, s, x, y), flush=True)
            if a.per_window:
                ya, _ = align(x, y)
                per_window_report(x, ya, 2 ** (s / 12))
    print(f"renders kept in {out_dir}")


if __name__ == "__main__":
    main()
