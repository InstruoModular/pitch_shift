"""Shift Listen VST3 must equal shiftbench `current` (same shift.cpp) delayed by exactly one firmware block.

    python tools/smoke_plugin.py      (after: cmake --build build/bench --target shift_capi; build ShiftListen_SyncDll)
"""
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(r"C:\Users\Kian\Desktop\vst_analysis")
sys.path.insert(0, str(ROOT / "analysis"))
import numpy as np
from scipy.io import wavfile

import run_suite

SP = ROOT / "build/smoke_plugin"   # scratch WAVs, gitignored build dir
SP.mkdir(exist_ok=True)
PLUGIN = ROOT / "build/host/tools/listen_plugin/ShiftListen_artefacts/Release/VST3/Shift Listen.vst3"
suite = {s["id"]: s for s in run_suite.load_suite("full")["signals"]}
cases = [("pluck_G3", 7), ("pluck_G3", -12), ("chord_Emaj", 7), ("chord_Emaj", -12)]

host_lines, bench_lines = [], []
for sid, s in cases:
    inp = run_suite.signal_cache(suite[sid])
    host_lines.append(f"{inp}\t{SP / f'plugin_{sid}_{s:+d}.wav'}\tTranspose={(s + 12) / 24:.6f}")
    bench_lines.append(f"{inp}\t{SP / f'bench_{sid}_{s:+d}.wav'}\tshift={s}")
(SP / "host.tsv").write_text("\n".join(host_lines) + "\n")
(SP / "bench.tsv").write_text("\n".join(bench_lines) + "\n")

r = subprocess.run([str(run_suite.VSTHOST), "run", str(PLUGIN), "--manifest", str(SP / "host.tsv"), "--block", "64"],
                   cwd=ROOT, capture_output=True, text=True)
print("vsthost:", r.stderr.strip()[-300:])
env = dict(os.environ, PATH=run_suite.MSYS_BIN + os.pathsep + os.environ["PATH"])
r = subprocess.run([str(run_suite.SHIFTBENCH), "run", "current", "--manifest", str(SP / "bench.tsv")],
                   cwd=ROOT, env=env, capture_output=True, text=True)
print("shiftbench:", r.stderr.strip()[-300:])

block = json.loads(subprocess.run([str(run_suite.SHIFTBENCH), "info"], env=env, capture_output=True, text=True).stdout)["block"]
ok = True
for sid, s in cases:
    meta = json.loads(Path(str(SP / f"plugin_{sid}_{s:+d}.wav") + ".json").read_text())
    _, yp = wavfile.read(SP / f"plugin_{sid}_{s:+d}.wav")
    _, yb = wavfile.read(SP / f"bench_{sid}_{s:+d}.wav")
    yp, yb = yp.astype(np.float64), yb.astype(np.float64)
    n = min(len(yp) - block, len(yb))
    diff = float(np.max(np.abs(yp[block:block + n] - yb[:n])))
    peak = float(np.max(np.abs(yb[:n])))
    lead = float(np.max(np.abs(yp[:block])))
    good = diff < 1e-6 and meta["reported_latency"] == block and lead == 0.0 and peak > 1e-3
    ok &= good
    print(f"{'PASS' if good else 'FAIL'}  {sid:<11}{s:+3d}  reported latency {meta['reported_latency']} (block {block}), "
          f"param {meta['params']}, max |plugin[n+{block}] - bench[n]| = {diff:.2e}, peak {peak:.3f}")
print("SMOKE TEST:", "PASS" if ok else "FAIL")
