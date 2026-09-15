"""Noise-resistant CPU re-timing of a shiftbench variant.

    python tools/retime.py <variant> [--params "k=v;..."] [--from-run results/<target>/<run>] [--top 12]
                           [--signals sine_E2,chord_Emaj --shifts=-12,12] [--repeats 3]

shiftbench already takes the per-block minimum of 2 passes, but on this desktop a worst block can still be hit by
OS preemption in both. This repeats each job `--repeats` times in separate processes and keeps, per job, the
MINIMUM of the reported worst-block percentages (real algorithmic spikes survive every repeat; scheduler noise
doesn't). Jobs come from the slowest cases of --from-run (by cpu_worst_block_pct) or from --signals x --shifts.
Prints per-job minima plus p50 / p99 / max across jobs.
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "analysis"))
import run_suite  # noqa: E402


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("variant")
    ap.add_argument("--params", default="")
    ap.add_argument("--from-run", help="pick the slowest jobs of this run")
    ap.add_argument("--top", type=int, default=12)
    ap.add_argument("--signals", help="comma-separated signal ids from suites/full.json")
    ap.add_argument("--shifts", default="-12,-7,7,12")
    ap.add_argument("--repeats", type=int, default=3)
    a = ap.parse_args()

    suite = {s["id"]: s for s in run_suite.load_suite("full")["signals"]}
    cases: list[tuple[str, int]] = []
    if a.from_run:
        p = Path(a.from_run)
        data = json.loads(((p / "metrics.json") if (p / "metrics.json").exists() else (p / "metrics.partial.json")).read_text())
        recs = sorted(data["records"], key=lambda r: -r["metrics"].get("cpu_worst_block_pct", 0.0))
        seen = set()
        for r in recs:
            k = (r["signal"], r["shift"])
            if k not in seen and r["signal"] in suite:
                seen.add(k)
                cases.append(k)
            if len(cases) >= a.top:
                break
    if a.signals:
        shifts = [int(s) for s in a.shifts.split(",")]
        cases += [(sid, s) for sid in a.signals.split(",") for s in shifts]
    if not cases:
        sys.exit("no jobs: pass --from-run and/or --signals")

    params = run_suite.parse_params(a.params)
    env = dict(os.environ, PATH=run_suite.MSYS_BIN + os.pathsep + os.environ["PATH"])
    best: dict[tuple[str, int], float] = {}
    mean_ns: dict[tuple[str, int], float] = {}
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        lines = []
        for sid, s in cases:
            inp = run_suite.signal_cache(suite[sid])
            kv = ";".join([f"shift={s}"] + [f"{k}={v}" for k, v in params.items()])
            lines.append(f"{inp}\t{tmp / f'{sid}_{s:+d}.wav'}\t{kv}")
        (tmp / "jobs.tsv").write_text("\n".join(lines) + "\n")
        for rep in range(a.repeats):
            res = subprocess.run([str(run_suite.SHIFTBENCH), "run", a.variant, "--manifest", str(tmp / "jobs.tsv")],
                                 cwd=ROOT, env=env, capture_output=True, text=True)
            if res.returncode:
                sys.exit(res.stdout + res.stderr)
            for sid, s in cases:
                meta = json.loads((tmp / f"{sid}_{s:+d}.wav.json").read_text())
                k = (sid, s)
                best[k] = min(best.get(k, 1e9), meta["cpu_worst_block_pct"])
                mean_ns[k] = min(mean_ns.get(k, 1e9), meta["cpu_ns_per_sample"])

    for k in cases:
        print(f"  {k[0]:<18}{k[1]:+4d}  worst block {best[k]:6.1f} %   {mean_ns[k]:6.0f} ns/sample")
    v = np.array([best[k] for k in cases])
    print(f"{a.variant}: min-of-{a.repeats} worst block  p50 {np.percentile(v, 50):.1f} %  p99 {np.percentile(v, 99):.1f} %  "
          f"max {v.max():.1f} %  (block {json.loads(subprocess.run([str(run_suite.SHIFTBENCH), 'info'], env=env, capture_output=True, text=True).stdout)['block']})")


if __name__ == "__main__":
    main()
