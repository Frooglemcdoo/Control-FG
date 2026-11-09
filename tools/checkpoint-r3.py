"""Compatibility entry point for the current full-source packager."""
import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).with_name("checkpoint-r6.py")),run_name="__main__")
