"""Fold a tuned shiftbench variant back into the firmware Shift (shift.hpp / shift.cpp).

    python tools/fold_variant.py <variant> [--set name=value ...] [--out DIR]

- Reverses make_variant's renames (Shift_<v> -> Shift, shift_<v> -> shift, include guard, own include).
- Removes harness-only code: the ShiftAdapter set_param specialisation, the friend declaration, the
  `#include "variant.hpp"` line and the RegisterShifter entry.
- Freezes every runtime knob (`static inline <type> name = value;`) as `static constexpr`, taking the value
  from --set when given (the sweep winner), otherwise the variant's default.

--out defaults to build/fold (staging). Pass --out . to overwrite the repo-root shift.hpp/shift.cpp; the harness
registers that class as variant `current`, so `shift:current` can be checked for identity against
`shift:<variant> --params <same values>` before committing.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
KNOB_RE = re.compile(r"static inline (float|bool|size_t|uint32_t|int32_t)(\s+)(\w+)(\s*)=\s*([^;]+);")


def literal(kind: str, value: str) -> str:
    if kind == "bool":
        return "true" if value.strip().lower() in ("1", "true", "1.0") else "false"
    if kind == "float":
        v = float(value.rstrip("fF"))
        return f"{v:.1f}f" if v == int(v) else f"{v!r}f"
    return str(int(float(value)))


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("variant")
    ap.add_argument("--set", action="append", default=[], help="knob=value (repeatable, or ';'-separated)")
    ap.add_argument("--out", default=str(ROOT / "build/fold"))
    a = ap.parse_args()

    v = a.variant
    src_h, src_c = ROOT / "variants" / f"{v}.hpp", ROOT / "variants" / f"{v}.cpp"
    if not src_h.exists():
        sys.exit(f"no variant {v}")
    overrides: dict[str, str] = {}
    for item in a.set:
        for kv in filter(None, item.split(";")):
            k, val = kv.split("=", 1)
            overrides[k.strip()] = val.strip()

    def unrename(text: str) -> str:
        text = re.sub(rf"\bShift_{v}\b", "Shift", text)
        text = re.sub(rf"\bnamespace shift_{v}\b", "namespace shift", text)
        text = re.sub(rf"\bshift_{v}::", "shift::", text)
        text = re.sub(rf"\bSHIFT_{v.upper()}_HPP\b", "SHIFT_HPP", text)
        return text.replace(f'#include "{v}.hpp"', '#include "instrument/shift.hpp"')

    hpp = unrename(src_h.read_text())
    cpp = unrename(src_c.read_text())

    # harness-only code
    hpp = re.sub(r"^[ \t]*template<class> friend struct ShiftAdapter;.*\n", "", hpp, flags=re.M)
    cpp = re.sub(r"^#include \"variant\.hpp\".*\n", "", cpp, flags=re.M)
    cpp = re.sub(r"^static RegisterShifter .*\n", "", cpp, flags=re.M)
    cpp = re.sub(r"template<> bool ShiftAdapter<Shift>::set_param\(.*?\n\}\n", "", cpp, flags=re.S)

    used = set()

    def freeze(m: re.Match) -> str:
        kind, sp1, name, sp2, default = m.groups()
        used.add(name)
        value = literal(kind, overrides.get(name, default))
        return f"static constexpr {kind}{sp1}{name}{sp2}= {value};"

    hpp = KNOB_RE.sub(freeze, hpp)
    unknown = set(overrides) - used
    if unknown:
        sys.exit(f"unknown knob(s) for variant {v}: {sorted(unknown)}")
    leftovers = []
    for fname, text in (("shift.hpp", hpp), ("shift.cpp", cpp)):
        for n, line in enumerate(text.splitlines(), 1):
            code = re.sub(r"//.*$", "", line)                     # mentions inside comments are fine
            code = re.sub(r"/\*.*?\*/", "", code)
            if re.search(r"\bstatic inline\b|\bset_param\b|\bRegisterShifter\b|\bShiftAdapter\b|variant\.hpp", code):
                leftovers.append(f"  {fname}:{n}: {line.strip()[:120]}")
    if leftovers:
        sys.exit("fold left harness-only code behind; refusing to write:\n" + "\n".join(leftovers))

    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    (out / "shift.hpp").write_text(hpp)
    (out / "shift.cpp").write_text(cpp)
    frozen = ", ".join(f"{k}={overrides[k]}" for k in sorted(overrides))
    print(f"folded variant {v} -> {out / 'shift.hpp'}, {out / 'shift.cpp'} ({len(used)} knobs frozen; overrides: {frozen or 'none'})")


if __name__ == "__main__":
    main()
