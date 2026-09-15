"""Orchestrate: render suite -> process via vsthost/shiftbench -> measure -> scorecard (+report).

Examples
  python analysis/run_suite.py --target vst --info
  python analysis/run_suite.py --target vst --suite quick --sweep "Quality=0,1,2,3"
  python analysis/run_suite.py --target shift:current --suite quick --compare reference/baseline.json
  python analysis/run_suite.py --promote results/vst/<run> [--setting 2]
  python analysis/run_suite.py --promote-shift results/shift-current/<run>

Prints only the scorecard. Everything else goes to results/<target>/<run>/.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import metrics  # noqa: E402
import siggen   # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
VSTHOST = ROOT / "build/host/tools/host/vsthost_artefacts/Release/vsthost.exe"
SHIFTBENCH = ROOT / "build/bench/shiftbench.exe"
PLUGIN = ROOT / "BL-PitchShift.vst3"
MSYS_BIN = r"C:\msys64\ucrt64\bin"


# ---------------------------------------------------------------------------- helpers

def load_suite(name: str) -> dict:
    p = Path(name)
    if not p.exists():
        p = ROOT / "suites" / f"{name}.json"
    return json.loads(p.read_text())


def parse_params(s: str | None) -> dict[str, str]:
    out: dict[str, str] = {}
    for kv in filter(None, (s or "").split(";")):
        k, v = kv.split("=", 1)
        out[k.strip()] = v.strip()
    return out


def parse_sweep(s: str | None) -> list[dict[str, str]]:
    """'A=1,2;B=x,y' -> cartesian product of settings."""
    settings: list[dict[str, str]] = [{}]
    for kv in filter(None, (s or "").split(";")):
        k, vals = kv.split("=", 1)
        settings = [{**base, k.strip(): v.strip()} for base in settings for v in vals.split(",")]
    return settings


def setting_name(d: dict[str, str]) -> str:
    return ",".join(f"{k}={v}" for k, v in d.items()) or "default"


def signal_cache(spec: dict) -> Path:
    h = hashlib.sha1(json.dumps(spec, sort_keys=True).encode()).hexdigest()[:10]
    p = ROOT / "build/sigcache" / f"{spec['id']}-{h}.wav"
    if not p.exists():
        p.parent.mkdir(parents=True, exist_ok=True)
        siggen.write_wav(p, siggen.build(spec).render(0.0))
    return p


# ---------------------------------------------------------------------------- processing

def run_processor(target: str, jobs: list[tuple[str, str, dict]], run_dir: Path, block: int | None) -> None:
    if target == "vst":
        exe, first = VSTHOST, str(PLUGIN)
        block = block or 32
    else:
        exe, first = SHIFTBENCH, target.split(":", 1)[1]
    if not exe.exists():
        sys.exit(f"missing {exe} - build it first (skill: build)")

    env = dict(os.environ)
    if target != "vst":
        env["PATH"] = MSYS_BIN + os.pathsep + env["PATH"]

    # VST processing is single-threaded per process: split the manifest across processes.
    # shiftbench stays single-process so CPU timings aren't skewed by contention.
    chunks = max(1, min(8, (os.cpu_count() or 2) // 2)) if target == "vst" else 1
    procs = []
    for c in range(chunks):
        part = jobs[c::chunks]
        if not part:
            continue
        man = run_dir / f"jobs{c}.tsv"
        man.write_text("".join(f"{i}\t{o}\t{';'.join(f'{k}={v}' for k, v in p.items())}\n" for i, o, p in part))
        cmd = [str(exe), "run", first, "--manifest", str(man)]
        if block:
            cmd += ["--block", str(block)]
        procs.append(subprocess.Popen(cmd, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True))
    failed = False
    for p in procs:
        out, _ = p.communicate()
        if p.returncode:
            failed = True
            print(out.strip()[-2000:])
    if failed:
        sys.exit("processor failed")


_SIG_CACHE: dict[str, siggen.Signal] = {}


def _measure_job(args: tuple) -> dict:
    spec, semis, out_path, setting = args
    from scipy.io import wavfile
    key = json.dumps(spec, sort_keys=True)
    sig = _SIG_CACHE.get(key) or _SIG_CACHE.setdefault(key, siggen.build(spec))
    _, y = wavfile.read(out_path)
    meta_p = Path(out_path + ".json")
    meta = json.loads(meta_p.read_text()) if meta_p.exists() else {}
    extra = {k: meta[k] for k in ("cpu_ns_per_sample", "cpu_worst_block_pct") if k in meta}
    rep = meta.get("reported_latency")
    try:
        m = metrics.measure(sig, semis, y.astype(np.float64), rep, extra)
    except Exception as e:  # keep the suite running; surface the failure in the record
        m = {"error": f"{type(e).__name__}: {e}"}
    return {"setting": setting, "signal": spec["id"], "kind": spec["kind"], "shift": semis, "metrics": m}


# ---------------------------------------------------------------------------- aggregation

def aggregate(records: list[dict]) -> dict[str, float]:
    card: dict[str, float] = {}
    for name, (agg, _, _, _) in metrics.SCORECARD.items():
        vals = np.array([r["metrics"][name] for r in records if name in r["metrics"]], dtype=float)
        if len(vals) == 0:
            continue
        card[name] = float({"mean": np.mean, "median": np.median, "max": np.max, "sum": np.sum,
                            "meanabs": lambda v: np.mean(np.abs(v))}[agg](vals))
    return card


def key_of(r: dict) -> tuple:
    return (r["signal"], r["shift"])


def load_reference(path: str, setting: int | None) -> tuple[str, list[dict]]:
    p = Path(path)
    if p.is_dir():
        p = p / "metrics.json"
    data = json.loads(p.read_text())
    if "records" in data and "settings" not in data:        # promoted baseline file
        return data.get("label", p.stem), data["records"]
    names = data["settings"]
    name = names[setting or 0]
    return f"{data['target']}:{name}", [r for r in data["records"] if r["setting"] == name]


def scorecard_text(title: str, cand: dict, ref: dict | None, ref_label: str | None, n_cases: int) -> tuple[str, int]:
    lines = [f"== {title}  ({n_cases} cases)" + (f"  vs {ref_label}" if ref is not None else "")]
    hdr = f"{'metric':<22}{'cand':>10}" + (f"{'ref':>10}{'delta+':>9}  ok" if ref is not None else "")
    lines.append(hdr)
    fails = 0
    for name, (_, direction, tol, _) in metrics.SCORECARD.items():
        if name not in cand and (ref is None or name not in ref):
            continue
        c = cand.get(name)
        row = f"{name:<22}{'-' if c is None else f'{c:10.2f}':>10}"
        if ref is not None:
            r = ref.get(name)
            if c is None or r is None or direction == 0:
                row += f"{'-' if r is None else f'{r:10.2f}':>10}{'':>9}  ."
            else:
                delta = (c - r) * direction          # + means candidate better
                ok = delta >= -tol
                fails += not ok
                row += f"{r:10.2f}{delta:+9.2f}  {'ok' if ok else 'XX'}"
        lines.append(row)
    if ref is not None:
        lines.append(f"-- {fails} metric(s) worse than ref beyond tolerance")
    return "\n".join(lines), fails


# ---------------------------------------------------------------------------- commands

def cmd_info() -> None:
    out = ROOT / "reference/vst_info.json"
    res = subprocess.run([str(VSTHOST), "info", str(PLUGIN)], cwd=ROOT, capture_output=True, text=True)
    if res.returncode:
        sys.exit(res.stderr)
    out.parent.mkdir(exist_ok=True)
    out.write_text(res.stdout)
    d = json.loads(res.stdout)
    print(f"{d['name']}  in={d['inputs']} out={d['outputs']} latency={d['latency_samples']}")
    for p in d["params"]:
        v = p["values"]
        print(f"  {p['name'] or '<unnamed>':<12} steps={p['steps']:<10} {v[0][1]} .. {v[len(v) // 2][1]} .. {v[-1][1]} {p['label']}")


def cmd_promote(run: str, setting: int | None, dest: Path) -> None:
    label, records = load_reference(run, setting)
    dest.parent.mkdir(exist_ok=True)
    dest.write_text(json.dumps({"label": label, "source": str(run), "scorecard": aggregate(records),
                                "records": records}, indent=1))
    print(f"promoted {label} ({len(records)} records) -> {dest.relative_to(ROOT)}")


def cmd_run(a: argparse.Namespace) -> int:
    suite = load_suite(a.suite)
    shifts = [int(s) for s in a.shifts.split(",")] if a.shifts else suite["shifts"]
    base = parse_params(a.params)
    settings = [{**base, **s} for s in parse_sweep(a.sweep)]
    names = [setting_name(s) for s in settings]
    tdir = a.target.replace(":", "-")
    run_id = time.strftime("%Y%m%d-%H%M%S") + f"-{suite['name']}" + (f"-{a.tag}" if a.tag else "")
    run_dir = ROOT / "results" / tdir / run_id
    (run_dir / "wav").mkdir(parents=True, exist_ok=True)

    t = time.time()
    jobs, meas = [], []
    for si, (setting, name) in enumerate(zip(settings, names)):
        for spec in suite["signals"]:
            inp = signal_cache(spec)
            for s in shifts:
                if spec["kind"] == "silence" and s != shifts[-1]:
                    continue
                out = run_dir / "wav" / f"{si}_{spec['id']}_{s:+d}.wav"
                params = dict(setting)
                if a.target == "vst":
                    params["Factor"] = f"{(s + 12) / 24:.6f}"
                else:
                    params["shift"] = str(s)
                jobs.append((str(inp), str(out), params))
                meas.append((spec, s, str(out), name))
    t_render = time.time() - t

    t = time.time()
    run_processor(a.target, jobs, run_dir, a.block)
    t_proc = time.time() - t

    t = time.time()
    with ProcessPoolExecutor(max_workers=os.cpu_count()) as ex:
        records = list(ex.map(_measure_job, meas, chunksize=4))
    t_meas = time.time() - t
    errors = [r for r in records if "error" in r["metrics"]]

    ref_label, ref_records = (None, None)
    if a.compare:
        ref_label, ref_records = load_reference(a.compare, a.compare_setting)

    blocks, total_fails, cards = [], 0, {}
    for name in names:
        recs = [r for r in records if r["setting"] == name]
        if ref_records is not None:
            common = {key_of(r) for r in recs} & {key_of(r) for r in ref_records}
            recs_c = [r for r in recs if key_of(r) in common]
            ref_c = [r for r in ref_records if key_of(r) in common]
            card, rcard = aggregate(recs_c), aggregate(ref_c)
            txt, fails = scorecard_text(f"{a.target} [{name}]", card, rcard, ref_label, len(recs_c))
        else:
            card = aggregate(recs)
            txt, fails = scorecard_text(f"{a.target} [{name}]", card, None, None, len(recs))
        cards[name] = card
        total_fails += fails
        blocks.append(txt)

    summary = "\n\n".join(blocks)
    summary += f"\n\nrun: {run_dir.relative_to(ROOT)}  jobs={len(jobs)}  render {t_render:.0f}s  process {t_proc:.0f}s  measure {t_meas:.0f}s"
    if errors:
        summary += f"\n!! {len(errors)} metric errors, first: {errors[0]['signal']} {errors[0]['shift']}: {errors[0]['metrics']['error']}"
    (run_dir / "scorecard.txt").write_text(summary + "\n")
    (run_dir / "metrics.json").write_text(json.dumps({
        "target": a.target, "suite": suite["name"], "settings": names, "shifts": shifts,
        "compare": a.compare, "scorecards": cards, "records": records}, indent=1))
    print(summary)

    if not a.no_report:
        try:
            import report
            report.write(run_dir, ref_records=ref_records, ref_label=ref_label)
        except Exception as e:  # report is a convenience; never fail the run on it
            print(f"(report skipped: {type(e).__name__}: {e})")
    if not a.keep_wav:
        shutil.rmtree(run_dir / "wav", ignore_errors=True)
    return 1 if (a.gate and total_fails) else 0


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--target", default="vst", help="vst | shift:<variant>")
    ap.add_argument("--suite", default="quick")
    ap.add_argument("--shifts", help="override suite shifts, e.g. 7,12")
    ap.add_argument("--params", help="static params 'Name=value;...' (vst: normalised or @text)")
    ap.add_argument("--sweep", help="'Name=a,b;Other=x,y' cartesian sweep of settings")
    ap.add_argument("--block", type=int)
    ap.add_argument("--compare", help="reference/baseline.json or a results run dir")
    ap.add_argument("--compare-setting", type=int, help="setting index when --compare is a multi-setting run")
    ap.add_argument("--gate", action="store_true", help="exit 1 if any metric worse than --compare beyond tolerance")
    ap.add_argument("--tag")
    ap.add_argument("--keep-wav", action="store_true")
    ap.add_argument("--no-report", action="store_true")
    ap.add_argument("--info", action="store_true")
    ap.add_argument("--promote", help="run dir -> reference/baseline.json")
    ap.add_argument("--promote-shift", help="run dir -> reference/best_shift.json")
    ap.add_argument("--setting", type=int, help="setting index for --promote")
    a = ap.parse_args()

    if a.info:
        cmd_info()
    elif a.promote:
        cmd_promote(a.promote, a.setting, ROOT / "reference/baseline.json")
    elif a.promote_shift:
        cmd_promote(a.promote_shift, a.setting, ROOT / "reference/best_shift.json")
    else:
        sys.exit(cmd_run(a))


if __name__ == "__main__":
    main()
