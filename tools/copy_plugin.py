"""Copy the built Shift Listen VST3 bundle into a VST3 folder so the DAW picks it up.

    python tools/copy_plugin.py                 -> C:\\Program Files\\Common Files\\VST3  (needs admin)
    python tools/copy_plugin.py --user          -> %LOCALAPPDATA%\\Programs\\Common\\VST3 (no admin)
    python tools/copy_plugin.py --dest <folder>
    python tools/copy_plugin.py --standalone    -> also copy the Standalone exe next to the bundle

The bundle carries its own shift_capi.dll, so this is a plain folder copy. Build first
(`cmake --build build/bench --target shift_capi`, then the ShiftListen_SyncDll target) and check the DSP with
`python tools/smoke_plugin.py`: this script refuses to install a bundle whose DLL is older than shift.cpp.
"""
import argparse
import os
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ARTEFACTS = ROOT / "build/host/tools/listen_plugin/ShiftListen_artefacts/Release"
BUNDLE = ARTEFACTS / "VST3/Shift Listen.vst3"
STANDALONE = ARTEFACTS / "Standalone/Shift Listen.exe"
SYSTEM_VST3 = Path(r"C:\Program Files\Common Files\VST3")
USER_VST3 = Path(os.environ.get("LOCALAPPDATA", "")) / "Programs/Common/VST3"


def check_fresh(bundle: Path) -> None:
    """The plugin doesn't relink when only shift.cpp changed, so a stale DSP is the classic failure here."""
    dll = bundle / "Contents/x86_64-win/shift_capi.dll"
    if not dll.exists():
        sys.exit(f"no DSP dll inside the bundle ({dll}) -- build the ShiftListen_SyncDll target, not the format targets")
    newest = max((ROOT / n).stat().st_mtime for n in ("shift.cpp", "shift.hpp"))
    if dll.stat().st_mtime < newest:
        sys.exit("shift.cpp/.hpp is newer than the bundle's shift_capi.dll -- rebuild first:\n"
                 "  $env:PATH = \"C:\\msys64\\ucrt64\\bin;$env:PATH\"; cmake --build build/bench --target shift_capi\n"
                 "  cmd /c '\"...\\vcvars64.bat\" && cmake --build build/host --target ShiftListen_SyncDll -j 2'")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--dest", type=Path, help="target VST3 folder")
    ap.add_argument("--user", action="store_true", help="per-user VST3 folder instead of Program Files (no admin)")
    ap.add_argument("--standalone", action="store_true", help="also copy the Standalone exe into the destination")
    ap.add_argument("--force", action="store_true", help="install even if the bundle's DSP looks stale")
    a = ap.parse_args()

    if not BUNDLE.is_dir():
        sys.exit(f"no bundle at {BUNDLE} -- build the ShiftListen_SyncDll target first")
    if not a.force:
        check_fresh(BUNDLE)

    dest = a.dest if a.dest else (USER_VST3 if a.user else SYSTEM_VST3)
    target = dest / BUNDLE.name
    try:
        dest.mkdir(parents=True, exist_ok=True)
        if target.exists():
            shutil.rmtree(target)          # a stale file left inside the bundle would otherwise survive
        shutil.copytree(BUNDLE, target)
        if a.standalone and STANDALONE.exists():
            shutil.copy2(STANDALONE, dest / STANDALONE.name)
    except PermissionError:
        sys.exit(f"no permission to write {dest}\n"
                 "  run the task/terminal as administrator, or install per-user: python tools/copy_plugin.py --user")

    print(f"installed {target}")
    print("rescan plugins in the DAW; the project must run at 48 kHz or the plugin bypasses")


if __name__ == "__main__":
    main()
