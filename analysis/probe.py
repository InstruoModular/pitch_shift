"""Technique probes for any pitch-shifter run made with --keep-wav on suites/probe.json.

    python analysis/run_suite.py --target <...> --suite probe --keep-wav --no-report --tag probe
    python analysis/probe.py results/<target>/<run> [--setting 0]

Prints compact tables (and writes probe.md/probe.json into the run dir):
  impulse    click response per shift: first arrival, energy spread, number of copies, copy spacing
             (repeated copies spaced by a constant = grain/splice length of a time-domain shifter; one smeared
             blob spanning tens of ms with energy BEFORE the peak = frame-based/vocoder)
  attack     tone-burst latency to 10 % / 50 % of steady level, minus the input's own rise, per note x shift
  sidebands  steady sine: strongest sideband offset (Hz) and level (dBc), envelope AM rate; inferred grain
             length = |1-ratio| / offset. Constant grain across notes = fixed grain; grain tracking the period
             = pitch-synchronous (PSOLA-like)
  formant    per-partial output amplitudes vs two hypotheses: amplitudes move with partials (naive) or the
             spectral envelope stays put (formant-preserving)
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from scipy import signal as sps
from scipy.io import wavfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import metrics  # noqa: E402
import siggen   # noqa: E402
from siggen import SR  # noqa: E402


def load(run: Path, si: int, sid: str, shift: int) -> np.ndarray | None:
    p = run / "wav" / f"{si}_{sid}_{shift:+d}.wav"
    if not p.exists():
        return None
    _, y = wavfile.read(p)
    return np.nan_to_num(y.astype(np.float64))


def env_ms(x: np.ndarray, ms: float) -> np.ndarray:
    k = max(1, int(ms * SR / 1000))
    return np.sqrt(np.convolve(x**2, np.ones(k) / k, mode="same"))


def _f(v, fmt="%.1f"):
    return "-" if v is None or (isinstance(v, float) and not np.isfinite(v)) else fmt % v


# ---------------------------------------------------------------------------- probes

def impulse(run: Path, si: int, spec: dict, shifts: list[int]) -> list[dict]:
    sig = siggen.build(spec)
    rows = []
    for s in shifts:
        y = load(run, si, spec["id"], s)
        if y is None:
            continue
        first, peak, spread, copies, spacing, pre = [], [], [], [], [], []
        for o in sig.onsets[1:]:                      # skip the first click: let the shifter settle
            i0 = int(o * SR)
            seg = y[i0:i0 + int(0.45 * SR)]
            e = env_ms(seg, 0.5)
            pk = e.max(initial=0.0)
            if pk <= 1e-9:
                continue
            cum = np.cumsum(seg**2) / max(np.sum(seg**2), 1e-30)
            first.append(np.argmax(e >= 0.1 * pk) / SR * 1000)
            peak.append(np.argmax(e) / SR * 1000)
            spread.append((np.argmax(cum >= 0.95) - np.argmax(cum >= 0.05)) / SR * 1000)
            pre.append(float(metrics.db(cum[max(np.argmax(e) - int(0.002 * SR), 0)] + 1e-12)))
            p, _ = sps.find_peaks(e, height=0.1 * pk, distance=int(0.004 * SR), prominence=0.05 * pk)
            copies.append(len(p))
            if len(p) > 1:
                spacing.append(float(np.median(np.diff(p))) / SR * 1000)
        med = lambda v: float(np.median(v)) if v else None  # noqa: E731
        rows.append({"shift": s, "first_ms": float(np.min(first)) if first else None, "peak_ms": med(peak),
                     "spread90_ms": med(spread), "energy_before_peak_db": med(pre), "copies": med(copies),
                     "copy_spacing_ms": med(spacing)})
    return rows


def attack(run: Path, si: int, spec: dict, shifts: list[int]) -> list[dict]:
    sig = siggen.build(spec)
    on_len = float(spec.get("on", 0.15))

    def crossings(x: np.ndarray) -> tuple[list[float], list[float]]:
        e = env_ms(x, 2.0)
        t10, t50 = [], []
        for o in sig.onsets:
            i0 = int(o * SR)
            st = e[i0 + int(0.12 * SR): i0 + int((on_len - 0.02) * SR)]
            w = e[i0: i0 + int(0.15 * SR)]
            if len(st) == 0 or np.median(st) <= 1e-7:
                continue
            lvl = float(np.median(st))
            for frac, out in ((0.1, t10), (0.5, t50)):
                hit = np.nonzero(w >= frac * lvl)[0]
                out.append(hit[0] / SR * 1000 if len(hit) else np.nan)
        return t10, t50

    i10, i50 = crossings(sig.render(0.0).astype(np.float64))
    base10, base50 = float(np.nanmedian(i10)), float(np.nanmedian(i50))
    rows = []
    for s in shifts:
        y = load(run, si, spec["id"], s)
        if y is None:
            continue
        t10, t50 = crossings(y)
        rows.append({"shift": s, "lat10_ms": float(np.nanmedian(t10)) - base10 if t10 else None,
                     "lat50_ms": float(np.nanmedian(t50)) - base50 if t50 else None,
                     "lat50_max_ms": float(np.nanmax(t50)) - base50 if t50 else None})
    return rows


def sidebands(run: Path, si: int, spec: dict, shifts: list[int]) -> list[dict]:
    sig = siggen.build(spec)
    f0 = float(np.median(sig.notes[0].f_curve))
    t0, t1 = sig.steady
    rows = []
    for s in shifts:
        y = load(run, si, spec["id"], s)
        if y is None:
            continue
        ratio = 2 ** (s / 12)
        fc = f0 * ratio
        seg = y[int((t0 + 0.05) * SR): int(t1 * SR)]
        mag, hpb = metrics.spectrum(seg, 1 << 18, bh=True)
        f_pk, _ = metrics.find_peak_hz(mag, hpb, fc, 50)
        f = np.arange(len(mag)) * hpb
        span = min(0.45 * fc, 300.0)
        near = np.abs(f - fc) <= span
        top = float(mag[near].max(initial=0.0))
        # Sidebands relative to the band maximum (a crude splicer can cancel its own carrier), outside a
        # 12-bin guard so Blackman-Harris leakage (~-92 dB) never reads as a sideband.
        guard = 12.0 * SR / len(seg) + 1.0
        band = near & (np.abs(f - f_pk) > guard)
        sb_off = sb_db = None
        if band.any() and top > 0:
            i = np.nonzero(band)[0][np.argmax(mag[band])]
            level = float(metrics.db(mag[i] ** 2) - metrics.db(top**2))
            if level > -80.0:
                sb_off, sb_db = float(abs(f[i] - f_pk)), level
        frame = int(round(max(2, np.ceil(0.005 * fc)) * SR / fc))
        r = metrics.db(metrics.rms_env(seg, frame, frame // 4, hann=True) ** 2)
        k = len(r)
        am_rate = am_pp = None
        if k > 32:
            r = r - np.polyval(np.polyfit(np.arange(k), r, 1), np.arange(k))
            am_pp = float(np.percentile(r, 98) - np.percentile(r, 2))
            if am_pp >= 0.02:                      # below this the "rate" is just noise in the envelope
                sp = np.abs(np.fft.rfft(r * np.hanning(k), 16384))
                fr = np.fft.rfftfreq(16384, (frame // 4) / SR)
                b = (fr >= 1.0) & (fr <= 200.0)
                am_rate = float(fr[b][np.argmax(sp[b])])
        # Splice interval from the envelope rate. A one-splice-per-grain scheduler (like shift.hpp) splices at
        # |1-r|/grain; a two-head 50 %-overlap crossfader at 2|1-r|/grain. Report both grain readings.
        g1 = abs(1 - ratio) * 1000.0 / am_rate if am_rate else None
        rows.append({"shift": s, "f0": f0, "carrier_err_c": float(metrics.cents(f_pk, fc)),
                     "sideband_hz": sb_off, "sideband_db": sb_db, "am_rate_hz": am_rate, "am_pp_db": am_pp,
                     "splice_interval_ms": (1000.0 / am_rate) if am_rate else None,
                     "grain_1splice_ms": g1, "grain_2head_ms": (2 * g1) if g1 else None,
                     "grain1_in_periods": (g1 / 1000.0 * fc) if g1 else None})
    return rows


def formant(run: Path, si: int, spec: dict, shifts: list[int]) -> list[dict]:
    sig = siggen.build(spec)
    f0 = float(np.median(sig.notes[0].f_curve))
    formants = tuple(spec.get("formants", siggen.DEFAULT_FORMANTS))
    t0, t1 = sig.steady
    rows = []
    for s in shifts:
        y = load(run, si, spec["id"], s)
        if y is None:
            continue
        ratio = 2 ** (s / 12)
        seg = y[int((t0 + 0.05) * SR): int(t1 * SR)]
        mag, hpb = metrics.spectrum(seg, 1 << 18, bh=True)
        ks, out, naive, keep = [], [], [], []
        for k, _ in sig.notes[0].partials:
            fk = k * f0 * ratio
            if fk > 12000 or k * f0 > siggen.MAX_PARTIAL_HZ:
                continue
            _, a = metrics.find_peak_hz(mag, hpb, fk, 25)
            if a <= 0:
                continue
            ks.append(k)
            out.append(np.log10(a))
            naive.append(np.log10(formant_amp := siggen.formant_gain(k * f0, formants) / np.sqrt(k)))
            keep.append(np.log10(siggen.formant_gain(fk, formants) / np.sqrt(k)))
        if len(ks) < 8:
            continue
        out_a, nai_a, kep_a = (np.array(v) - np.mean(v) for v in (out, naive, keep))
        c_naive = float(np.corrcoef(out_a, nai_a)[0, 1])
        c_keep = float(np.corrcoef(out_a, kep_a)[0, 1])
        rows.append({"shift": s, "partials": len(ks), "corr_naive": c_naive, "corr_preserving": c_keep,
                     "verdict": ("preserving" if c_keep > c_naive + 0.05 else "naive" if c_naive > c_keep + 0.05
                                 else "unclear")})
    return rows


# ---------------------------------------------------------------------------- report

def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("run_dir")
    ap.add_argument("--setting", type=int, default=0)
    a = ap.parse_args()
    run = Path(a.run_dir)
    data = json.loads((run / "metrics.json").read_text())
    suite = json.loads((Path(__file__).resolve().parent.parent / "suites/probe.json").read_text())
    shifts = data["shifts"]
    specs = {sp["id"]: sp for sp in suite["signals"]}
    out: dict = {"target": data["target"], "setting": data["settings"][a.setting]}
    lines = [f"# Probe: {data['target']} [{data['settings'][a.setting]}]", ""]

    def table(title: str, rows: list[dict], cols: list[tuple[str, str]], lead: str | None = None):
        lines.append(f"## {title}")
        hdr = ([lead] if lead else []) + [c for c, _ in cols]
        lines.append(" | ".join(h.rjust(10) for h in hdr))
        for r in rows:
            vals = ([str(r.get(lead, ""))] if lead else []) + [_f(r.get(c), fm) for c, fm in cols]
            lines.append(" | ".join(v.rjust(10) for v in vals))
        lines.append("")

    imp = impulse(run, a.setting, specs["probe_click"], shifts)
    out["impulse"] = imp
    table("impulse (click response)", imp, [("shift", "%d"), ("first_ms", "%.1f"), ("peak_ms", "%.1f"),
          ("spread90_ms", "%.1f"), ("energy_before_peak_db", "%.1f"), ("copies", "%.0f"), ("copy_spacing_ms", "%.1f")])

    att = []
    for sid in ("probe_burst_E2", "probe_burst_A3", "probe_burst_E5"):
        for r in attack(run, a.setting, specs[sid], shifts):
            att.append({"note": sid.split("_")[-1], **r})
    out["attack"] = att
    table("attack latency (tone bursts, input rise removed)", att,
          [("shift", "%d"), ("lat10_ms", "%.1f"), ("lat50_ms", "%.1f"), ("lat50_max_ms", "%.1f")], lead="note")

    sb = []
    for sid in ("probe_sine_E2", "probe_sine_A3", "probe_sine_E4", "probe_sine_E5"):
        for r in sidebands(run, a.setting, specs[sid], shifts):
            sb.append({"note": sid.split("_")[-1], **r})
    out["sidebands"] = sb
    table("steady-sine modulation (splice/grain structure)", sb,
          [("shift", "%d"), ("carrier_err_c", "%.2f"), ("sideband_hz", "%.1f"), ("sideband_db", "%.1f"),
           ("am_rate_hz", "%.1f"), ("am_pp_db", "%.2f"), ("splice_interval_ms", "%.1f"), ("grain_1splice_ms", "%.1f"),
           ("grain_2head_ms", "%.1f"), ("grain1_in_periods", "%.1f")],
          lead="note")

    fm = []
    for sid in ("probe_formant_A2", "probe_formant_E3"):
        for r in formant(run, a.setting, specs[sid], shifts):
            fm.append({"note": sid.split("_")[-1], **r})
    out["formant"] = fm
    lines.append("## formant behaviour (correlation of output partial amplitudes with each hypothesis)")
    lines.append(" | ".join(h.rjust(12) for h in ("note", "shift", "partials", "corr_naive", "corr_preserv", "verdict")))
    for r in fm:
        lines.append(" | ".join(v.rjust(12) for v in (r["note"], str(r["shift"]), str(r["partials"]),
                                                       "%.3f" % r["corr_naive"], "%.3f" % r["corr_preserving"], r["verdict"])))
    lines.append("")

    text = "\n".join(lines)
    (run / "probe.md").write_text(text + "\n")
    (run / "probe.json").write_text(json.dumps(out, indent=1))
    print(text)


if __name__ == "__main__":
    main()
