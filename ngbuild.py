import subprocess
import shutil
from pathlib import Path
import time

# 1. Go to the growui directory (adjust path if needed)
growui_dir = Path("growui")          # <-- relative to project root
if not growui_dir.is_dir():
    raise SystemExit(f"Could not find '{growui_dir}' – make sure it exists")

print(f"=== Running ng build in {growui_dir} ===")
result = subprocess.run(
    ["npm.cmd", "run", "build"],
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

