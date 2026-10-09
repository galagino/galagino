import subprocess
import sys
from pathlib import Path

# romconv/ dir
ROOT = Path.cwd()

BOLD_CYAN = "\033[1;36m"
WHITE_ON_RED = "\033[1;37;41m"
RESET = "\033[0m"

def info(text):
    print(f"{BOLD_CYAN}{text}{RESET}")

def error(text):
    print(f"-- Error: {WHITE_ON_RED}{text}{RESET}")

def fatal(text):
    error(text)
    sys.exit(1)

def run(*args, cwd=None):
    workdir = ROOT / cwd if cwd else ROOT
    result = subprocess.run([sys.executable, *args], cwd=workdir, capture_output=True, text=True)
    print(result.stdout, end="")
    print(result.stderr, end="", file=sys.stderr)

    FAILURE_MARKERS = ("ERROR", "NO MAIN ROM FILES FOUND")
    combined = result.stdout.upper() + result.stderr.upper()
    reported_error = any(marker in combined for marker in FAILURE_MARKERS)
    if result.returncode != 0:
        error(f"{args[0]} exited with code {result.returncode}")
        sys.exit(result.returncode)
    if reported_error:
        error(f"{args[0]} printed an error but exited with code 0")
        sys.exit(1)
