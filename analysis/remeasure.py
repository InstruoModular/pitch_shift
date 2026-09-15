"""Re-score a run whose WAVs were kept (--keep-wav) with the CURRENT metrics, without re-processing.

    python analysis/remeasure.py results/<target>/<run> [--compare reference/baseline.json] [--workers 2]

Keeps the previous metrics.json as metrics.prev.json, writes a fresh metrics.json + scorecard.txt and prints the
scorecard. Use it when metrics change mid-investigation (e.g. adding modulation metrics) so plugin/variant runs
don't have to be repeated.
"""
from __future__ import annotations

import argparse
import json
import shutil
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_suite  # noqa: E402


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("run_dir")
    ap.add_argument("--compare")
    ap.add_argument("--workers", type=int, default=2)
    a = ap.parse_args()

    run = Path(a.run_dir)
    src = run / "metrics.json"
    if not src.exists():
        src = run / "metrics.partial.json"
    data = json.loads(src.read_text())
    if not (run / "wav").is_dir():
        sys.exit(f"{run} has no wav/ folder - it was run without --keep-wav")

    specs = {s["id"]: s for s in run_suite.load_suite(data["suite"])["signals"]}
    jobs, missing = [], 0
    for si, name in enumerate(data["settings"]):
        for r in (x for x in data["records"] if x["setting"] == name):
            wav = run / "wav" / f"{si}_{r['signal']}_{r['shift']:+d}.wav"
            if not wav.exists() or r["signal"] not in specs:
                missing += 1
                continue
            jobs.append((specs[r["signal"]], r["shift"], str(wav), name))

    with ProcessPoolExecutor(max_workers=a.workers) as ex:
        records = list(ex.map(run_suite._measure_job, jobs, chunksize=4))

    if src.name == "metrics.json":
        shutil.copyfile(src, run / "metrics.prev.json")
    ref_label, ref_records = run_suite.load_reference(a.compare, None) if a.compare else (None, None)
    blocks, cards = [], {}
    for name in data["settings"]:
        recs = [r for r in records if r["setting"] == name]
        if ref_records is not None:
            common = {run_suite.key_of(r) for r in recs} & {run_suite.key_of(r) for r in ref_records}
            card = run_suite.aggregate([r for r in recs if run_suite.key_of(r) in common])
            rcard = run_suite.aggregate([r for r in ref_records if run_suite.key_of(r) in common])
            txt, _ = run_suite.scorecard_text(f"{data['target']} [{name}] (remeasured)", card, rcard, ref_label, len(common))
        else:
            card = run_suite.aggregate(recs)
            txt, _ = run_suite.scorecard_text(f"{data['target']} [{name}] (remeasured)", card, None, None, len(recs))
        cards[name] = card
        blocks.append(txt)
    data.update({"records": records, "scorecards": cards, "remeasured": True})
    (run / "metrics.json").write_text(json.dumps(data, indent=1))
    summary = "\n\n".join(blocks) + f"\n\nremeasured {len(records)} cases ({missing} skipped: missing wav/spec)"
    (run / "scorecard.txt").write_text(summary + "\n")
    print(summary)


if __name__ == "__main__":
    main()
