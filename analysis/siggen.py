"""Deterministic synthetic test signals with analytic ideal pitch-shifted counterparts.

Every signal is additive: a set of notes, each a fundamental frequency curve f(t), a partial
table and per-partial amplitude envelopes, plus optional unpitched components (pick noise,
clicks, noise bursts). The *ideal* shift of a signal is the same model with every frequency
curve scaled by 2**(semis/12) and every envelope left untouched -- i.e. a perfect,
zero-latency, formant-naive pitch shifter. Metrics compare processor output against it.

Spec (dict, from suites/*.json):
  {"id": "...", "kind": sine|harmonic|pluck|chord|sweep|vibrato|bend|burst|click|noise|silence|staccato|decay,
   ...kind-specific keys...}
"""
from __future__ import annotations

import argparse
import json
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np

SR = 48000
LEVEL_DBFS = -12.0
MAX_PARTIAL_HZ = 8000.0   # input band limit: +12 st keeps every ideal partial < 16 kHz

NOTE_NAMES = {"C": 0, "C#": 1, "Db": 1, "D": 2, "D#": 3, "Eb": 3, "E": 4, "F": 5, "F#": 6, "Gb": 6,
              "G": 7, "G#": 8, "Ab": 8, "A": 9, "A#": 10, "Bb": 10, "B": 11}


def note_hz(n: str | float) -> float:
    """'E2' -> 82.41 Hz; numbers pass through."""
    if isinstance(n, (int, float)):
        return float(n)
    name, octave = (n[:2], n[2:]) if len(n) > 2 and n[1] in "#b" else (n[:1], n[1:])
    midi = 12 * (int(octave) + 1) + NOTE_NAMES[name]
    return 440.0 * 2.0 ** ((midi - 69) / 12.0)


@dataclass
class Note:
    f_curve: np.ndarray          # fundamental Hz per sample (full signal length)
    partials: list[tuple[int, float]]
    env: np.ndarray              # amplitude envelope (full length), shared by partials
    decay_tilt: float = 0.0      # extra per-partial exponential decay: exp(-tilt*(k-1)*t_since_onset)
    onset: float = 0.0           # seconds
    phases: np.ndarray | None = None
    ratios: list[float] | None = None   # per-partial frequency multiple of f0 (inharmonic); default k

    def ratio(self, i: int, k: int) -> float:
        return self.ratios[i] if self.ratios is not None else float(k)


@dataclass
class Signal:
    spec: dict
    n: int
    notes: list[Note] = field(default_factory=list)
    unpitched: np.ndarray | None = None       # added identically to input and ideal
    onsets: list[float] = field(default_factory=list)
    steady: tuple[float, float] | None = None  # window (s) where pitch is stationary
    pitched: bool = True
    gain: float = 1.0

    def render(self, semis: float = 0.0) -> np.ndarray:
        ratio = 2.0 ** (semis / 12.0)
        t = np.arange(self.n) / SR
        y = np.zeros(self.n)
        for note in self.notes:
            f = note.f_curve * ratio
            phase0 = 2 * np.pi * np.cumsum(f) / SR
            since = np.maximum(t - note.onset, 0.0)
            for i, (k, a) in enumerate(note.partials):
                r = note.ratio(i, k)
                if r * note.f_curve.max() > MAX_PARTIAL_HZ:
                    continue   # dropped from input *and* ideal so they stay comparable
                ph = note.phases[i] if note.phases is not None else 0.0
                tilt = np.exp(-note.decay_tilt * (k - 1) * since) if note.decay_tilt else 1.0
                y += a * tilt * note.env * np.sin(r * phase0 + ph)
        if self.unpitched is not None:
            y = y + self.unpitched
        return (y * self.gain).astype(np.float32)

    def expected_partials(self, semis: float, t0: float, t1: float) -> list[float]:
        """Shifted partial frequencies (Hz) present during [t0, t1] (steady-state notes)."""
        ratio = 2.0 ** (semis / 12.0)
        i0, i1 = int(t0 * SR), min(int(t1 * SR), self.n - 1)
        out = []
        for note in self.notes:
            if note.env[i0:i1].max(initial=0.0) <= 1e-4:
                continue
            f = float(np.median(note.f_curve[i0:i1])) * ratio
            out += [note.ratio(i, k) * f for i, (k, _) in enumerate(note.partials)
                    if note.ratio(i, k) * note.f_curve.max() <= MAX_PARTIAL_HZ]
        return sorted(out)

    def meta(self) -> dict:
        return {"id": self.spec["id"], "kind": self.spec["kind"], "n": self.n, "sr": SR,
                "onsets": self.onsets, "steady": self.steady, "pitched": self.pitched,
                "f0s": [float(np.median(nt.f_curve)) for nt in self.notes]}


# ---------------------------------------------------------------------------- helpers

def _seeded(spec: dict) -> np.random.Generator:
    # hash() is salted per process; derive a stable seed from the id instead.
    seed = spec.get("seed", int.from_bytes(spec["id"].encode()[:8].ljust(8, b"\0"), "little") % (2**32))
    return np.random.default_rng(seed)


def _ar_env(n: int, onset: float, dur: float, attack: float = 0.005, release: float = 0.02) -> np.ndarray:
    t = np.arange(n) / SR - onset
    env = np.clip(t / attack, 0, 1)
    env *= np.clip((dur - t) / release, 0, 1)
    env[t < 0] = 0.0
    return env ** 2 * (3 - 2 * env)  # smoothstep edges, no clicks


def _pluck_env(n: int, onset: float, dur: float, t60: float) -> np.ndarray:
    t = np.arange(n) / SR - onset
    env = np.exp(-6.9078 * np.maximum(t, 0) / t60) * np.clip(t / 0.002, 0, 1)
    env *= np.clip((dur - t) / 0.03, 0, 1)
    env[t < 0] = 0.0
    return env


def _saw_partials(count: int = 40, rolloff: float = 1.0) -> list[tuple[int, float]]:
    return [(k, 1.0 / k**rolloff) for k in range(1, count + 1)]


def _pluck_partials(count: int = 40, pluck_pos: float = 0.18) -> list[tuple[int, float]]:
    # ideal plucked string: a_k ~ sin(k*pi*p) / k^2, softened to 1/k^1.5 for pickup brightness
    return [(k, abs(np.sin(k * np.pi * pluck_pos)) / k**1.5 + 1e-3 / k) for k in range(1, count + 1)]


def _pick_noise(n: int, onsets: list[float], rng: np.random.Generator, level: float = 0.25) -> np.ndarray:
    y = np.zeros(n)
    burst = int(0.004 * SR)
    for o in onsets:
        i = int(o * SR)
        seg = rng.standard_normal(burst) * np.exp(-np.arange(burst) / (0.0008 * SR))
        seg = np.diff(np.concatenate([[0.0], seg]))          # high-passed click
        y[i:i + burst] += level * seg[: max(0, n - i)]
    return y


def _rand_phases(count: int, rng: np.random.Generator) -> np.ndarray:
    return rng.uniform(0, 2 * np.pi, count)


DEFAULT_FORMANTS = (700.0, 1200.0, 2600.0)


def formant_gain(f: float | np.ndarray, formants=DEFAULT_FORMANTS, width_oct: float = 0.12, floor: float = 0.15):
    """Fixed spectral envelope for the `formant` kind: Gaussian bumps in log-frequency (~+17 dB peaks)."""
    f = np.asarray(f, dtype=float)
    return floor + sum(np.exp(-0.5 * (np.log2(np.maximum(f, 1.0) / F) / width_oct) ** 2) for F in formants)


# ---------------------------------------------------------------------------- builders

def build(spec: dict) -> Signal:
    kind = spec["kind"]
    dur = float(spec.get("dur", 2.0))
    lead = float(spec.get("lead", 0.2))           # silence before first event
    n = int((lead + dur + float(spec.get("trail", 0.3))) * SR)
    rng = _seeded(spec)
    sig = Signal(spec=spec, n=n)
    const = lambda hz: np.full(n, hz)  # noqa: E731

    if kind in ("sine", "harmonic"):
        f0 = note_hz(spec["note"])
        parts = [(1, 1.0)] if kind == "sine" else _saw_partials(spec.get("partials", 40), spec.get("rolloff", 1.0))
        sig.notes.append(Note(const(f0), parts, _ar_env(n, lead, dur), onset=lead,
                              phases=_rand_phases(len(parts), rng)))
        sig.onsets, sig.steady = [lead], (lead + 0.3, lead + dur - 0.1)

    elif kind in ("pluck", "chord"):
        notes = spec["notes"] if kind == "chord" else [spec["note"]]
        strum = float(spec.get("strum_ms", 15.0)) / 1000.0
        t60 = float(spec.get("t60", 3.0))
        for j, nm in enumerate(notes):
            f0, on = note_hz(nm), lead + j * strum
            parts = _pluck_partials(spec.get("partials", 40), spec.get("pluck_pos", 0.18))
            sig.notes.append(Note(const(f0), parts, _pluck_env(n, on, dur, t60 * (82.4 / f0) ** 0.3),
                                  decay_tilt=float(spec.get("tilt", 0.6)), onset=on,
                                  phases=_rand_phases(len(parts), rng)))
            sig.onsets.append(on)
        if spec.get("pick", True):
            sig.unpitched = _pick_noise(n, sig.onsets[:1] if kind == "chord" else sig.onsets, rng)
        last = sig.onsets[-1]
        sig.steady = (last + 0.25, min(last + 1.25, lead + dur - 0.1))

    elif kind == "dyad":
        for j, nm in enumerate(spec["notes"]):
            f0 = note_hz(nm)
            parts = _saw_partials(spec.get("partials", 20), 1.3)
            sig.notes.append(Note(const(f0), parts, _ar_env(n, lead, dur), onset=lead,
                                  phases=_rand_phases(len(parts), rng)))
        sig.onsets, sig.steady = [lead], (lead + 0.3, lead + dur - 0.1)

    elif kind in ("guitar", "gchord"):
        # Closer to a real DI: stretched (inharmonic) partials, slow random pitch drift plus a sharp attack that
        # settles, pluck-position and pickup-position comb filtering, per-partial decay, pick noise. These are what
        # make a period tracker jitter on real guitar, which clean harmonic tones never exercise.
        notes = spec["notes"] if kind == "gchord" else [spec["note"]]
        strum = float(spec.get("strum_ms", 20.0)) / 1000.0
        t60 = float(spec.get("t60", 3.0))
        drift_c = float(spec.get("drift_cents", 3.0))
        drift_rate = float(spec.get("drift_rate", 1.5))
        attack_c = float(spec.get("attack_cents", 6.0))
        count = int(spec.get("partials", 40))
        pluck_pos, pickup_pos = float(spec.get("pluck_pos", 0.18)), float(spec.get("pickup_pos", 0.22))
        t = np.arange(n) / SR
        for j, nm in enumerate(notes):
            f0, on = note_hz(nm), lead + j * strum
            B = float(spec.get("inharm", 4e-5 * (f0 / 82.4) ** 0.5))   # stiffer (plain) strings higher up
            rates = rng.uniform(0.2, drift_rate, 3)
            phs = rng.uniform(0, 2 * np.pi, 3)
            wander = sum(np.sin(2 * np.pi * r * t + p) for r, p in zip(rates, phs))
            wander *= drift_c / max(np.max(np.abs(wander)), 1e-9)
            since = np.maximum(t - on, 0.0)
            cents = wander + attack_c * np.exp(-since / 0.08) * (t >= on)
            parts, ratios = [], []
            for k in range(1, count + 1):
                amp = abs(np.sin(k * np.pi * pluck_pos)) / k**1.5 * (0.35 + 0.65 * abs(np.sin(k * np.pi * pickup_pos)))
                parts.append((k, amp + 1e-3 / k))
                ratios.append(k * np.sqrt(1.0 + B * k * k))
            sig.notes.append(Note(f0 * 2 ** (cents / 1200.0), parts, _pluck_env(n, on, dur, t60 * (82.4 / f0) ** 0.3),
                                  decay_tilt=float(spec.get("tilt", 0.8)), onset=on,
                                  phases=_rand_phases(len(parts), rng), ratios=ratios))
            sig.onsets.append(on)
        sig.unpitched = _pick_noise(n, sig.onsets, rng, 0.2 if kind == "guitar" else 0.12)
        last = sig.onsets[-1]
        sig.steady = (last + 0.3, min(last + 1.3, lead + dur - 0.1))

    elif kind == "formant":
        # Harmonic source through a fixed spectral envelope. The ideal (like every ideal here) is
        # formant-naive: partial amplitudes travel with the partials. probe.py compares the output's
        # partial amplitudes with both hypotheses to tell whether a shifter preserves formants.
        f0 = note_hz(spec.get("note", "A2"))
        formants = tuple(spec.get("formants", DEFAULT_FORMANTS))
        parts = [(k, float(formant_gain(k * f0, formants)) / np.sqrt(k))
                 for k in range(1, int(MAX_PARTIAL_HZ // f0) + 1)]
        sig.notes.append(Note(const(f0), parts, _ar_env(n, lead, dur), onset=lead,
                              phases=_rand_phases(len(parts), rng)))
        sig.onsets, sig.steady = [lead], (lead + 0.3, lead + dur - 0.1)

    elif kind == "sweep":
        f_lo, f_hi = float(spec.get("f_lo", 80.0)), float(spec.get("f_hi", 2000.0))
        t = np.clip((np.arange(n) / SR - lead) / dur, 0, 1)
        sig.notes.append(Note(f_lo * (f_hi / f_lo) ** t, [(1, 1.0)], _ar_env(n, lead, dur, 0.02, 0.05), onset=lead))
        sig.onsets, sig.steady = [lead], None

    elif kind in ("vibrato", "bend"):
        f0 = note_hz(spec["note"])
        t = np.arange(n) / SR - lead
        if kind == "vibrato":
            cents = float(spec.get("depth_cents", 30.0)) * np.sin(2 * np.pi * float(spec.get("rate", 5.0)) * np.maximum(t, 0))
        else:
            b0, blen = float(spec.get("bend_at", 0.6)), float(spec.get("bend_len", 0.3))
            u = np.clip((t - b0) / blen, 0, 1)
            cents = float(spec.get("cents", 200.0)) * (u * u * (3 - 2 * u))
        parts = _saw_partials(spec.get("partials", 20), 1.3)
        sig.notes.append(Note(f0 * 2 ** (cents / 1200.0), parts, _ar_env(n, lead, dur), onset=lead,
                              phases=_rand_phases(len(parts), rng)))
        sig.onsets, sig.steady = [lead], None

    elif kind == "burst":
        f0 = note_hz(spec.get("note", "A4"))
        period, on_len, count = float(spec.get("period", 0.4)), float(spec.get("on", 0.15)), int(spec.get("count", 5))
        env = np.zeros(n)
        for i in range(count):
            env += _ar_env(n, lead + i * period, on_len, 0.001, 0.005)
            sig.onsets.append(lead + i * period)
        parts = [(1, 1.0)] if spec.get("sine", True) else _saw_partials(20, 1.3)
        sig.notes.append(Note(const(f0), parts, env, onset=lead))
        sig.steady = None

    elif kind == "staccato":
        f0 = note_hz(spec.get("note", "E3"))
        bpm, div, count = float(spec.get("bpm", 140)), int(spec.get("div", 16)), int(spec.get("count", 16))
        step = 60.0 / bpm * 4.0 / div
        for i in range(count):
            on = lead + i * step
            parts = _pluck_partials(30)
            sig.notes.append(Note(const(f0), parts, _pluck_env(n, on, step * 0.8, 1.0), decay_tilt=0.6, onset=on,
                                  phases=_rand_phases(len(parts), rng)))
            sig.onsets.append(on)
        sig.unpitched = _pick_noise(n, sig.onsets, rng, 0.15)
        sig.steady = None

    elif kind == "decay":
        f0 = note_hz(spec.get("note", "A3"))
        parts = _saw_partials(20, 1.5)
        sig.notes.append(Note(const(f0), parts, _pluck_env(n, lead, dur, dur / 1.0), onset=lead,
                              phases=_rand_phases(len(parts), rng)))   # t60 == dur: ends at -60 dB
        sig.onsets, sig.steady = [lead], (lead + 0.2, lead + dur * 0.5)

    elif kind == "click":
        y = np.zeros(n)
        for i in range(int(spec.get("count", 4))):
            on = lead + i * float(spec.get("period", 0.5))
            if int(on * SR) >= n:
                break   # spec asked for more clicks than its duration holds
            y[int(on * SR)] = 1.0
            sig.onsets.append(on)
        sig.unpitched, sig.pitched = y, False

    elif kind == "noise":
        y = rng.standard_normal(n) * _ar_env(n, lead, dur, 0.01, 0.05)
        if spec.get("pink", False):
            spec_ = np.fft.rfft(y)
            spec_[1:] /= np.sqrt(np.arange(1, len(spec_)))
            y = np.fft.irfft(spec_, n)
        sig.unpitched, sig.pitched, sig.onsets = y, False, [lead]

    elif kind == "silence":
        sig.unpitched, sig.pitched = np.zeros(n), False

    else:
        raise ValueError(f"unknown signal kind {kind!r}")

    # Normalise the *input* to LEVEL_DBFS peak; the ideal renders reuse the same gain.
    peak = float(np.max(np.abs(sig.render(0.0)))) if kind != "silence" else 0.0
    sig.gain = 10 ** (LEVEL_DBFS / 20) / peak if peak > 0 else 1.0
    return sig


def write_wav(path: Path, x: np.ndarray) -> None:
    from scipy.io import wavfile
    wavfile.write(str(path), SR, np.asarray(x, dtype=np.float32))


def main() -> None:
    ap = argparse.ArgumentParser(description="Render a suite's input signals (and optionally ideals).")
    ap.add_argument("--suite", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--ideal", action="store_true", help="also render ideal shifted signals")
    a = ap.parse_args()
    suite = json.loads(Path(a.suite).read_text())
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    for spec in suite["signals"]:
        sig = build(spec)
        write_wav(out / f"{spec['id']}.wav", sig.render(0.0))
        if a.ideal:
            for s in suite["shifts"]:
                write_wav(out / f"{spec['id']}__ideal{s:+d}.wav", sig.render(s))
    print(f"rendered {len(suite['signals'])} signals -> {out}")


if __name__ == "__main__":
    main()
