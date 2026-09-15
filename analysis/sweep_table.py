"""Compact settings x metrics table for a sweep run, with the reference row, and per-shift latency.

    python analysis/sweep_table.py results/<target>/<run> [--ref reference/baseline.json] [--shifts]

Aggregates are recomputed over the cases the run and the reference share, so a partial suite compares fairly.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from run_suite import aggregate, key_of, load_reference  # noqa: E402

COLS = [("lat_ms", "lat"), ("lat_ms_max", "latmax"), ("pitch_err_cents", "pitch"),
        ("env_mod_hi_db", "buzz"), ("env_mod_note_db", "envmod"), ("fm_rough_cents", "fmrough"),
        ("am_rough_db", "amrough"), ("grain_noise_p90_db", "gnoise"),
        ("sinad_db", "sinad"), ("sinad_poly_db", "poly"), ("poly_pitch_err_cents", "ppitch"),
        ("flam_db", "flam"), ("attack_smear", "smear"), ("lsd_db", "lsd"), ("cpu_worst_block_pct", "cpu%")]


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("run_dir")
    ap.add_argument("--ref", default="reference/baseline.json")
    ap.add_argument("--shifts", action="store_true", help="also print median lat_ms per shift")
    a = ap.parse_args()
    run = Path(a.run_dir)
    p = run / "metrics.json"
    if not p.exists():
        p = run / "metrics.partial.json"
    data = json.loads(p.read_text())
    ref_label, ref_records = load_reference(a.ref, None)
    ref_keys = {key_of(r) for r in ref_records}

    width = max(12, *(len(n) for n in data["settings"]))
    print("setting".ljust(width) + "".join(h.rjust(8) for _, h in COLS))
    for name in data["settings"]:
        recs = [r for r in data["records"] if r["setting"] == name and key_of(r) in ref_keys]
        card = aggregate(recs)
        print(name.ljust(width) + "".join((f"{card[m]:8.2f}" if m in card else "       -") for m, _ in COLS))
    common = {key_of(r) for r in data["records"]} & ref_keys
    rcard = aggregate([r for r in ref_records if key_of(r) in common])
    print(("REF " + ref_label.split(":")[1][:width - 4] if ":" in ref_label else "REF").ljust(width)
          + "".join((f"{rcard[m]:8.2f}" if m in rcard else "       -") for m, _ in COLS))

    if a.shifts:
        shifts = sorted({r["shift"] for r in data["records"]})
        print("\nmedian lat_ms by shift".ljust(width) + "".join(f"{s:+8d}" for s in shifts))
        rows = [(n, [r for r in data["records"] if r["setting"] == n]) for n in data["settings"]]
        rows.append(("REF", [r for r in ref_records if key_of(r) in common]))
        for n, recs in rows:
            vals = []
            for s in shifts:
                v = [r["metrics"]["lat_ms"] for r in recs if r["shift"] == s and "lat_ms" in r["metrics"]]
                vals.append(f"{np.median(v):8.1f}" if v else "       -")
            print(n.ljust(width) + "".join(vals))


if __name__ == "__main__":
    main()
