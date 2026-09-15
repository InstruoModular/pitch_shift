"""Clone the firmware Shift (or an existing variant) into a new, self-registering shiftbench variant.

    python tools/make_variant.py <name> [--from current|<variant>]

Creates variants/<name>.hpp and variants/<name>.cpp with class Shift_<name>, namespace shift_<name>
(so inline tables don't collide across variants) and a RegisterShifter entry named <name>.
Rebuild shiftbench afterwards (`cmake --build build/bench`; the variants/ glob re-runs automatically).
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VARIANTS = ROOT / "variants"
REG_RE = re.compile(r'^\s*#include "variant\.hpp"\s*$|^\s*static RegisterShifter .*$', re.M)


def identity(name: str) -> dict[str, str]:
    if name == "current":
        return {"hpp": ROOT / "shift.hpp", "cpp": ROOT / "shift.cpp", "cls": "Shift", "ns": "shift",
                "guard": "SHIFT_HPP", "inc": "instrument/shift.hpp"}
    return {"hpp": VARIANTS / f"{name}.hpp", "cpp": VARIANTS / f"{name}.cpp", "cls": f"Shift_{name}",
            "ns": f"shift_{name}", "guard": f"SHIFT_{name.upper()}_HPP", "inc": f"{name}.hpp"}


def rewrite(text: str, src: dict, dst: dict) -> str:
    text = re.sub(rf"\b{src['cls']}\b", dst["cls"], text)
    text = re.sub(rf"\bnamespace {src['ns']}\b", f"namespace {dst['ns']}", text)
    text = re.sub(rf"\b{src['ns']}::", f"{dst['ns']}::", text)
    text = re.sub(rf"\b{src['guard']}\b", dst["guard"], text)
    return text.replace(f'#include "{src["inc"]}"', f'#include "{dst["inc"]}"')


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("name")
    ap.add_argument("--from", dest="src", default="current")
    ap.add_argument("--force", action="store_true", help="overwrite an existing variant")
    a = ap.parse_args()

    if not re.fullmatch(r"[a-z][a-z0-9_]*", a.name) or a.name == "current":
        sys.exit("variant name must be lower_snake_case and not 'current'")
    src, dst = identity(a.src), identity(a.name)
    if not src["hpp"].exists():
        sys.exit(f"no such source variant: {a.src}")
    if dst["hpp"].exists() and not a.force:
        sys.exit(f"{dst['hpp'].relative_to(ROOT)} exists (use --force)")

    VARIANTS.mkdir(exist_ok=True)
    hpp = rewrite(src["hpp"].read_text(), src, dst)
    cpp = REG_RE.sub("", rewrite(src["cpp"].read_text(), src, dst)).rstrip() + "\n"
    cpp += (f'\n#include "variant.hpp"\n'
            f'static RegisterShifter reg_{a.name}{{"{a.name}", [] {{ return std::make_unique<ShiftAdapter<{dst["cls"]}>>(); }}}};\n')
    dst["hpp"].write_text(hpp)
    dst["cpp"].write_text(cpp)
    print(f"created variants/{a.name}.hpp, variants/{a.name}.cpp (class {dst['cls']}, from {a.src})")


if __name__ == "__main__":
    main()
