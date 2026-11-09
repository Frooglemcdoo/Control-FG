import sys
if sys.version_info < (3, 8):
    raise SystemExit("Python 3.8 or newer is required")
print("CONTROL_GI_PYTHON_OK " + sys.version.split()[0])
