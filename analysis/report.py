"""Write report.md + plots for a run directory (called by run_suite; also usable standalone).

    python analysis/report.py results/<target>/<run> [--compare reference/baseline.json]
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np

import metrics

# Reference palette (dataviz skill): fixed categorical order, text in ink tokens, recessive grid.
SERIES = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#4a3aa7"]
SURFACE, INK, INK2, GRID = "#fcfcfb", "#0b0b0b", "#52514e", "#e4e3df"

KEY_METRICS = ["lat_ms", "lat_ms_max", "pitch_err_cents", "if_dev_cents", "track_err_cents", "sinad_db",
               "sinad_poly_db", "am_pp_db", "lsd_db", "flam_db", "attack_smear", "pre_echo_db", "level_db"]
PLOT_METRICS = ["lat_ms", "if_dev_cents", "sinad_db", "sinad_poly_db", "am_pp_db", "lsd_db", "flam_db", "level_db"]
WORST = {"sinad_db": False, "sinad_poly_db": False, "if_dev_cents": True, "am_pp_db": True,
         "flam_db": True, "lat_ms_max": True, "lsd_db": True, "track_err_cents": True}


def _fmt(v) -> str:
    return "-" if v is None or (isinstance(v, float) and np.isnan(v)) else f"{v:.2f}"


def _table(header: list[str], rows: list[list]) -> str:
    out = ["| " + " | ".join(header) + " |", "|" + "---|" * len(header)]
    out += ["| " + " | ".join(str(c) for c in r) + " |" for r in rows]
    return "\n".join(out)


def _group(records: list[dict], field: str) -> dict:
    groups: dict = {}
    for r in records:
        groups.setdefault(r[field], []).append(r)
    return groups


def _plots(run_dir: Path, data: dict, ref_records: list[dict] | None, ref_label: str | None) -> list[str]:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    plt.rcParams.update({"font.size": 9, "axes.edgecolor": INK2, "axes.labelcolor": INK2, "xtick.color": INK2,
                         "ytick.color": INK2, "text.color": INK, "axes.facecolor": SURFACE, "figure.facecolor": SURFACE})
    pdir = run_dir / "plots"
    pdir.mkdir(exist_ok=True)
    files = []

    series = [(name, [r for r in data["records"] if r["setting"] == name]) for name in data["settings"]]
    if ref_records:
        series.append((f"ref: {ref_label}", ref_records))
    series = series[: len(SERIES)]                         # never generate extra hues
    shifts = sorted({r["shift"] for r in data["records"]})
    present = [m for m in PLOT_METRICS if any(m in r["metrics"] for _, rs in series for r in rs)]

    cols = 4
    rows = int(np.ceil(len(present) / cols))
    fig, axes = plt.subplots(rows, cols, figsize=(3.2 * cols, 2.5 * rows), squeeze=False)
    for ax, m in zip(axes.flat, present):
        agg = metrics.SCORECARD[m][0]
        agg = "mean" if agg == "meanabs" else agg          # plots keep the sign (e.g. level loss)
        for (label, rs), color in zip(series, SERIES):
            by = _group(rs, "shift")
            ys = []
            for s in shifts:
                recs = by.get(s, [])
                ys.append(metrics_agg(recs, m, agg))
            ax.plot(shifts, ys, color=color, lw=2, marker="o", ms=5, label=label,
                    markeredgecolor=SURFACE, markeredgewidth=1.5)
        ax.set_title(m, fontsize=9, color=INK, loc="left")
        ax.grid(True, color=GRID, lw=0.8)
        ax.set_axisbelow(True)
        for sp in ("top", "right"):
            ax.spines[sp].set_visible(False)
        ax.set_xticks(shifts)
    for ax in list(axes.flat)[len(present):]:
        ax.axis("off")
    handles, labels = axes.flat[0].get_legend_handles_labels()
    if len(series) > 1:
        fig.legend(handles, labels, loc="upper center", ncol=min(len(series), 4), frameon=False)
    fig.supxlabel("shift (semitones)", color=INK2)
    fig.tight_layout(rect=(0, 0, 1, 0.93 if len(series) > 1 else 1))
    f = pdir / "by_shift.png"
    fig.savefig(f, dpi=110)
    plt.close(fig)
    files.append(f.name)

    # latency by signal (first setting vs ref), horizontal bars
    first = series[0][1]
    sigs = sorted({r["signal"] for r in first})
    fig, ax = plt.subplots(figsize=(7, 0.28 * len(sigs) + 1))
    y = np.arange(len(sigs))
    width = 0.8 / (2 if ref_records else 1)
    groups = [(series[0][0], first, SERIES[0])] + ([(series[-1][0], ref_records, SERIES[len(series) - 1])] if ref_records else [])
    for gi, (label, rs, color) in enumerate(groups):
        by = _group(rs, "signal")
        vals = [metrics_agg(by.get(s, []), "lat_ms", "median") for s in sigs]
        ax.barh(y + gi * width, [0 if v is None else v for v in vals], height=width - 0.04, color=color, label=label)
    ax.set_yticks(y + width * (len(groups) - 1) / 2, sigs)
    ax.invert_yaxis()
    ax.set_xlabel("measured latency, median over shifts (ms)")
    ax.grid(True, axis="x", color=GRID, lw=0.8)
    ax.set_axisbelow(True)
    for sp in ("top", "right"):
        ax.spines[sp].set_visible(False)
    if len(groups) > 1:
        ax.legend(frameon=False)
    fig.tight_layout()
    f = pdir / "latency_by_signal.png"
    fig.savefig(f, dpi=110)
    plt.close(fig)
    files.append(f.name)
    return files


def metrics_agg(recs: list[dict], m: str, agg: str):
    vals = [r["metrics"][m] for r in recs if m in r["metrics"]]
    if not vals:
        return None
    v = np.array(vals, dtype=float)
    return float({"mean": np.mean, "median": np.median, "max": np.max, "sum": np.sum,
                  "meanabs": lambda a: np.mean(np.abs(a))}[agg](v))


def write(run_dir: Path, ref_records: list[dict] | None = None, ref_label: str | None = None) -> Path:
    run_dir = Path(run_dir)
    data = json.loads((run_dir / "metrics.json").read_text())
    md = [f"# Report: {data['target']} / {data['suite']}", "",
          f"Settings: {', '.join(data['settings'])}  ", f"Shifts: {data['shifts']}", ""]
    if (run_dir / "scorecard.txt").exists():
        md += ["## Scorecard", "```", (run_dir / "scorecard.txt").read_text().rstrip(), "```", ""]

    if len(data["settings"]) > 1:
        md += ["## Settings comparison", ""]
        rows = []
        for name in data["settings"]:
            card = data["scorecards"][name]
            rows.append([name] + [_fmt(card.get(m)) for m in KEY_METRICS])
        md += [_table(["setting"] + KEY_METRICS, rows), ""]

    for name in data["settings"]:
        recs = [r for r in data["records"] if r["setting"] == name]
        md += [f"## [{name}] by shift", ""]
        md += [_table(["shift"] + KEY_METRICS,
                      [[s] + [_fmt(metrics_agg(rs, m, metrics.SCORECARD[m][0])) for m in KEY_METRICS]
                       for s, rs in sorted(_group(recs, "shift").items())]), ""]
        md += [f"## [{name}] by signal kind", ""]
        md += [_table(["kind"] + KEY_METRICS,
                      [[k] + [_fmt(metrics_agg(rs, m, metrics.SCORECARD[m][0])) for m in KEY_METRICS]
                       for k, rs in sorted(_group(recs, "kind").items())]), ""]
        md += [f"## [{name}] worst cases", ""]
        for m, high_is_bad in WORST.items():
            have = [r for r in recs if m in r["metrics"]]
            if not have:
                continue
            have.sort(key=lambda r: r["metrics"][m], reverse=high_is_bad)
            md.append(f"- **{m}**: " + ", ".join(f"{r['signal']} {r['shift']:+d} ({r['metrics'][m]:.1f})" for r in have[:5]))
        md.append("")

    try:
        files = _plots(run_dir, data, ref_records, ref_label)
        md += ["## Plots", ""] + [f"![{f}](plots/{f})" for f in files]
    except Exception as e:
        md += [f"(plots failed: {type(e).__name__}: {e})"]

    out = run_dir / "report.md"
    out.write_text("\n".join(md) + "\n")
    return out


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("run_dir")
    ap.add_argument("--compare")
    a = ap.parse_args()
    ref_label = ref_records = None
    if a.compare:
        from run_suite import load_reference
        ref_label, ref_records = load_reference(a.compare, None)
    print(write(Path(a.run_dir), ref_records, ref_label))
