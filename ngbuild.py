#!/usr/bin/env python
import subprocess
import shutil
from pathlib import Path
import time
print("[PIO] ngbuild is being executed")
# 1. Go to the growui directory (adjust path if needed)
growui_dir = Path("growui")          # <-- relative to project root
if not growui_dir.is_dir():
    raise SystemExit(f"Could not find '{growui_dir}' – make sure it exists")

print(f"=== Running ng build in {growui_dir} ===")
result = subprocess.run(
    ["npm.cmd", "run", "build","--production"],
    cwd=str(growui_dir),
    capture_output=True,          # returns bytes for stdout/stderr
)

# Decode output ourselves – UTF‑8 with error replacement
stdout_text = result.stdout.decode("utf-8", errors="replace")
stderr_text = result.stderr.decode("utf-8", errors="replace")

if result.returncode != 0:
    print(stdout_text)
    print(stderr_text)
    raise SystemExit(f"Angular build failed (exit code {result.returncode})")


base_dir = Path(growui_dir.parent, "data", "angular-www").resolve()

license_file = base_dir / "3rdpartylicenses.txt"
routes_file  = base_dir / "prerendered-routes.json"
time.sleep(1)
for f in (license_file, routes_file):
    print(f"Checking {f}")          # debug: show absolute path
    if f.exists():
        try:
            f.unlink()
            print(f"Deleted {f.name}")
        except Exception as e:
            print(f"Could not delete {f.name}: {e}")
    else:
        print(f"{f} does not exist")
        

# ------------------------------------------------------------------
# Locate and run PlatformIO (auto‑lookup)
# ------------------------------------------------------------------
import shutil
from pathlib import Path

# 1. Try to find platformio.exe in the system PATH
pio_exe = shutil.which("platformio.exe")

# 2. If still None, look in the active user’s home directory
if pio_exe is None:
    user_home = Path.home()                      # e.g., C:\Users\<username>
    user_path = user_home / ".platformio" / "penv" / "Scripts" / "platformio.exe"
    if user_path.is_file():
        pio_exe = str(user_path)

# 3. If still None, look relative to this script
if pio_exe is None:
    potential_rel = Path(__file__).resolve().parent.parent / ".platformio" / "penv" / "Scripts" / "platformio.exe"
    if potential_rel.is_file():
        pio_exe = str(potential_rel)

# 4. If nothing found, abort
if pio_exe is None:
    raise SystemExit("Could not locate platformio.exe in PATH, user home, or relative location")

print(f"Running PlatformIO: {pio_exe} run --target buildfs --environment esp32dev")

result_pio = subprocess.run(
    [pio_exe, "run", "--target", "buildfs", "--environment", "esp32dev"],
    capture_output=True,
)

stdout_text_pio = result_pio.stdout.decode("utf-8", errors="replace")
stderr_text_pio = result_pio.stderr.decode("utf-8", errors="replace")

if result_pio.returncode != 0:
    print(stdout_text_pio)
    print(stderr_text_pio)
    raise SystemExit(f"PlatformIO build failed (exit code {result_pio.returncode})")