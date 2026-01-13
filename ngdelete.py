import subprocess
import shutil
from pathlib import Path
import time
# --- delete generated license and prerendered routes files ---
growui_dir = Path("growui")
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