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
import re
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
VST2HOST = ROOT / "build/host/vst2host/vst2host.exe"
SHIFTBENCH = ROOT / "build/bench/shiftbench.exe"
DEFAULT_PLUGIN = "BL-PitchShift.vst3"
DEFAULT_SHIFT_PARAM = "Factor"
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


def plugin_path(plugin: Path) -> Path:
    path = plugin if plugin.is_absolute() else ROOT / plugin
    if not path.exists():
        sys.exit(f"plugin not found: {path}")
    return path


def host_exe(plugin: Path) -> Path:
    """VST3 bundles/files go through the JUCE host; VST2 DLLs through the clean-room vst2host."""
    return VST2HOST if plugin_path(plugin).suffix.lower() == ".dll" else VSTHOST


def plugin_info(plugin: Path, refresh: bool = False, probe: list[str] | None = None) -> dict:
    """`<host> info` for a plugin, cached in reference/plugins/<stem>.json. VST2 hosts only sweep the display
    texts of the parameters in `probe`; the cache is refreshed if a requested one wasn't swept yet."""
    path = plugin_path(plugin)
    cache = ROOT / "reference/plugins" / f"{path.stem}.json"
    probe = probe or []

    def swept(d: dict) -> bool:
        names = {p["name"].lower(): p for p in d["params"]}
        return all(n.lower() in names and len(names[n.lower()]["values"]) > 1 for n in probe)

    if refresh or not cache.exists() or not swept(json.loads(cache.read_text())):
        cmd = [str(host_exe(plugin)), "info", str(path)] + (["--probe", ",".join(probe)] if probe else [])
        res = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
        if res.returncode:
            sys.exit(res.stderr[-3000:])
        cache.parent.mkdir(parents=True, exist_ok=True)
        cache.write_text(res.stdout, encoding="utf-8")
    return json.loads(cache.read_text(encoding="utf-8"))


def shift_normaliser(info: dict, name: str):
    """Semitones -> normalised value of the plugin's shift parameter, interpolated from the parameter's own
    display texts, so any range or taper works as long as the text shows a number (--shift-scale for cents)."""
    prm = next((p for p in info["params"] if p["name"].lower() == name.lower()), None)
    if prm is None:
        sys.exit(f"no parameter {name!r}; plugin has: {[p['name'] for p in info['params']]}")
    by_value: dict[float, list[float]] = {}
    for norm, text in prm["values"]:
        m = re.search(r"[-+]?\d+(?:\.\d+)?", text)
        if m:
            by_value.setdefault(float(m.group()), []).append(norm)
    xs = sorted(by_value)
    ys = [float(np.mean(by_value[x])) for x in xs]
    if len(xs) < 2:
        sys.exit(f"cannot read numeric values from {name!r} display texts: {prm['values'][:3]}")

    def to_norm(value: float) -> float:
        if not xs[0] <= value <= xs[-1]:
            sys.exit(f"{name} value {value} outside its range {xs[0]}..{xs[-1]}")
        return float(np.interp(value, xs, ys))

    return to_norm


# ---------------------------------------------------------------------------- processing

def run_processor(target: str, jobs: list[tuple[str, str, dict]], run_dir: Path, block: int | None,
                  procs: int | None = None, plugin: Path | None = None) -> None:
    if target == "vst":
        exe, first = host_exe(plugin), str(plugin_path(plugin))
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
    # 2 plugin instances max by default: each loads the plugin's GL-heavy DLL, and this machine has
    # little free RAM. --procs 1 for the leanest run.
    heavy = target == "vst" and exe == VST2HOST          # 100 MB+ amp-sim DLLs: one instance unless asked
    chunks = max(1, procs or (1 if heavy else min(2, (os.cpu_count() or 2) // 2))) if target == "vst" else 1
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
                floor = metrics.FLOORS.get(name)
                below_floor = floor is not None and c <= floor and r <= floor
                ok = delta >= -tol or below_floor
                fails += not ok
                row += f"{r:10.2f}{delta:+9.2f}  {'ok' if ok else 'XX'}"
        lines.append(row)
    if ref is not None:
        lines.append(f"-- {fails} metric(s) worse than ref beyond tolerance")
    return "\n".join(lines), fails


# ---------------------------------------------------------------------------- commands

def cmd_info(plugin: Path, probe: list[str] | None = None) -> None:
    d = plugin_info(plugin, refresh=True, probe=probe)
    print(f"(cached -> reference/plugins/{plugin_path(plugin).stem}.json)")
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
    plugin = Path(a.plugin) if a.target == "vst" else None
    to_norm = shift_normaliser(plugin_info(plugin, probe=[a.shift_param]), a.shift_param) if plugin else None
    target_label = f"vst:{plugin_path(plugin).stem}" if plugin else a.target
    tdir = f"vst-{plugin_path(plugin).stem}" if plugin else a.target.replace(":", "-")
    run_id = time.strftime("%Y%m%d-%H%M%S") + f"-{suite['name']}" + (f"-{a.tag}" if a.tag else "")
    run_dir = ROOT / "results" / tdir / run_id
    (run_dir / "wav").mkdir(parents=True, exist_ok=True)

    t = time.time()
    for spec in suite["signals"]:
        signal_cache(spec)
    t_render = time.time() - t

    # One setting at a time: process, measure, delete its WAVs. This machine has little free RAM
    # (see findings.md "Machine"), so worker count stays low and nothing accumulates across settings.
    workers = a.workers or max(1, min(3, (os.cpu_count() or 2) - 1))
    records, n_jobs, t_proc, t_meas = [], 0, 0.0, 0.0
    with ProcessPoolExecutor(max_workers=workers) as ex:
        for si, (setting, name) in enumerate(zip(settings, names)):
            jobs, meas = [], []
            for spec in suite["signals"]:
                inp = signal_cache(spec)
                for s in shifts:
                    if spec["kind"] == "silence" and s != shifts[-1]:
                        continue
                    out = run_dir / "wav" / f"{si}_{spec['id']}_{s:+d}.wav"
                    params = dict(setting)
                    if plugin:
                        params[a.shift_param] = f"{to_norm(s * a.shift_scale):.6f}"
                    else:
                        params["shift"] = str(s)
                    jobs.append((str(inp), str(out), params))
                    meas.append((spec, s, str(out), name))
            n_jobs += len(jobs)

            t = time.time()
            run_processor(a.target, jobs, run_dir, a.block, a.procs, plugin)
            t_proc += time.time() - t

            t = time.time()
            records += list(ex.map(_measure_job, meas, chunksize=4))
            t_meas += time.time() - t
            # Checkpoint after every setting: a low-memory kill then loses one setting, not the sweep.
            (run_dir / "metrics.partial.json").write_text(json.dumps({
                "target": target_label, "suite": suite["name"], "settings": names[: si + 1], "shifts": shifts,
                "records": records}, indent=1))
            if len(settings) > 1:
                print(f"  setting {si + 1}/{len(settings)} [{name}] done", flush=True)
            if not a.keep_wav:
                for _, o, _ in jobs:
                    for p in (Path(o), Path(o + ".json")):
                        p.unlink(missing_ok=True)
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
            txt, fails = scorecard_text(f"{target_label} [{name}]", card, rcard, ref_label, len(recs_c))
        else:
            card = aggregate(recs)
            txt, fails = scorecard_text(f"{target_label} [{name}]", card, None, None, len(recs))
        cards[name] = card
        total_fails += fails
        blocks.append(txt)

    summary = "\n\n".join(blocks)
    summary += f"\n\nrun: {run_dir.relative_to(ROOT)}  jobs={n_jobs}  render {t_render:.0f}s  process {t_proc:.0f}s  measure {t_meas:.0f}s"
    if errors:
        summary += f"\n!! {len(errors)} metric errors, first: {errors[0]['signal']} {errors[0]['shift']}: {errors[0]['metrics']['error']}"
    (run_dir / "scorecard.txt").write_text(summary + "\n")
    (run_dir / "metrics.json").write_text(json.dumps({
        "target": target_label, "suite": suite["name"], "settings": names, "shifts": shifts,
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
    ap.add_argument("--plugin", default=DEFAULT_PLUGIN, help="VST3 path (relative to repo root or absolute)")
    ap.add_argument("--shift-param", default=DEFAULT_SHIFT_PARAM, help="plugin parameter that sets the shift")
    ap.add_argument("--shift-scale", type=float, default=1.0,
                    help="parameter units per semitone as shown in its display text (100 for cents)")
    ap.add_argument("--suite", default="quick")
    ap.add_argument("--shifts", help="override suite shifts, e.g. 7,12")
    ap.add_argument("--params", help="static params 'Name=value;...' (vst: normalised or @text)")
    ap.add_argument("--sweep", help="'Name=a,b;Other=x,y' cartesian sweep of settings")
    ap.add_argument("--block", type=int)
    ap.add_argument("--compare", help="reference/baseline.json or a results run dir")
    ap.add_argument("--compare-setting", type=int, help="setting index when --compare is a multi-setting run")
    ap.add_argument("--gate", action="store_true", help="exit 1 if any metric worse than --compare beyond tolerance")
    ap.add_argument("--tag")
    ap.add_argument("--workers", type=int, help="metric worker processes (default min(3, cpus-1))")
    ap.add_argument("--procs", type=int, help="concurrent vsthost instances (default 2)")
    ap.add_argument("--keep-wav", action="store_true")
    ap.add_argument("--no-report", action="store_true")
    ap.add_argument("--info", action="store_true")
    ap.add_argument("--promote", help="run dir -> reference/baseline.json")
    ap.add_argument("--promote-shift", help="run dir -> reference/best_shift.json")
    ap.add_argument("--setting", type=int, help="setting index for --promote")
    a = ap.parse_args()

    if a.info:
        cmd_info(Path(a.plugin), [a.shift_param])
    elif a.promote:
        cmd_promote(a.promote, a.setting, ROOT / "reference/baseline.json")
    elif a.promote_shift:
        cmd_promote(a.promote_shift, a.setting, ROOT / "reference/best_shift.json")
    else:
        sys.exit(cmd_run(a))


if __name__ == "__main__":
    main()
