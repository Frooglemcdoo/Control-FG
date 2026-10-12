"""Shared PIX bootstrap helpers for the Python API samples.

Each sample under api/{preview,retail}/<sample>/python/main.py adds the
parent of this module to sys.path and imports find_pix_bin_directory and
initialize_pythonnet from here, rather than carrying its own copy. The
two functions parameterize on a single "marker" DLL name so retail samples
can probe for PixApiCsExt.dll and preview samples for the experimental
variant without duplicating the discovery logic.

The discovery cascade (find_pix_bin_directory) and CLR bootstrap
(initialize_pythonnet) are otherwise identical across samples; keep them
identical here -- a sample that diverges from this module's behavior is
almost certainly a bug in that sample, not a desired override.
"""

import os
import struct


def _parse_install_version(install_dir):
    """Parse the YYMM.DD[.NNN] version from an install dir's leaf folder name
    into a tuple of ints for chronological comparison.

    PIX names version dirs YYMM.DD[.NNN][-flavor] (e.g. "2606.17-preview",
    "2602.24.004-main"). Comparing parsed integer tuples orders versions numerically, 
    so a single-digit field  like "2606.9" sorts before "2606.17" instead of after it.
    Returns an empty tuple for an unparseable name so it sorts below every real version and is
    never chosen as the newest.
    """
    leaf = os.path.basename(install_dir)
    version = leaf.split("-", 1)[0]  # drop the flavor suffix
    parts = []
    for component in version.split("."):
        if not component.isdigit():
            return ()
        parts.append(int(component))
    return tuple(parts)


def find_pix_bin_directory(marker):
    """Locate the PIX installation directory containing `marker`.

    Discovery order:
      1. PIX_DIR environment variable
      2. Convention path scan: %ProgramFiles%/Microsoft PIX Preview/<version>/ 
      (we ignore Microsoft PIX because PIX API is not shipped in a retail build yet)

    `marker` is the filename whose presence we use to validate that a
    candidate directory is in fact a PIX install hosting the surface the
    caller needs:
        - PixApiCsExt.dll                for retail samples
        - PixApiCsExt.experimental.dll   for preview samples

    A Preview install is a superset that ships both DLLs, so it can host
    whichever marker the caller asks for.

    Skips registry discovery; the shipping PIX installer does not currently
    write a well-known InstallPath value, so the samples rely on the
    standard Program Files install path that the WiX installer hard-codes.
    """
    pix_directory = os.environ.get("PIX_DIR")
    if pix_directory and os.path.isfile(os.path.join(pix_directory, marker)):
        return os.path.abspath(pix_directory)

    program_files = (os.environ.get("ProgramW6432")
                     or os.environ.get("ProgramFiles")
                     or r"C:\Program Files")
    if not os.path.isdir(program_files):
        return None

    preview_root = os.path.join(program_files, "Microsoft PIX Preview")
    if not os.path.isdir(preview_root):
        return None

    candidates = []
    try:
        for version in os.listdir(preview_root):
            install_dir = os.path.join(preview_root, version)
            if not os.path.isfile(os.path.join(install_dir, marker)):
                continue
            parsed_version = _parse_install_version(install_dir)
            if not parsed_version:
                # Skip directories whose name isn't a parseable PIX version, so
                # a malformed dir can never be chosen as the newest install.
                continue
            candidates.append((parsed_version, install_dir))
    except OSError:
        return None

    if not candidates:
        return None

    # Sort by the parsed YYMM.DD[.NNN] version (newest last). Comparing the
    # parsed integer tuple keeps the ordering correct regardless of zero-padding
    # or build-number suffixes, so the fallback to the next-latest Preview build
    # picks the right one.
    candidates.sort()
    return os.path.abspath(candidates[-1][1])


def initialize_pythonnet(bin_directory, csext_dll_name):
    """Bootstrap CoreCLR and load the PIX C# extension assembly.

    `csext_dll_name` is "PixApiCsExt.dll" for retail samples or
    "PixApiCsExt.experimental.dll" for preview samples. The DLL is loaded
    from `bin_directory`, which is the value returned by
    find_pix_bin_directory above.
    """
    python_bitness = struct.calcsize("P") * 8
    if python_bitness != 64:
        raise RuntimeError(
            f"PIX requires 64-bit Python; this interpreter is {python_bitness}-bit.")

    os.add_dll_directory(bin_directory)

    from pythonnet import load
    load("coreclr")

    import clr
    clr.AddReference(os.path.join(bin_directory, csext_dll_name))

