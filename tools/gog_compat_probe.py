from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import struct
import sys
import time
import zipfile
from pathlib import Path
from typing import Optional

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from pe_tools import PE  # noqa: E402

FILES = ("Control_DX12.exe", "d3d_rmdwin10_f.dll", "renderer_rmdwin10_f.dll")
OPTIONAL_FILES = ("Control.exe",)

KNOWN_TARGETS = {
    "steam_21225456": {
        "Control_DX12.exe": "9441DB3AE75B267ABD989846AD0895E3FE24ABCC0F06E58F93C34FC8D4736506",
        "d3d_rmdwin10_f.dll": "CCEB99CBD9C019AF907C24C44701A53C8D230965C1FB31C325233E4C80214AA5",
        "renderer_rmdwin10_f.dll": "EBFB5B47CEBC2D4482E912B8090BD343F717BAB8A178B5A6385DEC9105E7C433",
    },
    "epic_0.0.518.2177": {
        "Control_DX12.exe": "E1D11616941FAD767B20CD0FB1AB442771A912FF34EF404B5FB95136F53B6175",
        "d3d_rmdwin10_f.dll": "EB598AAD837AF7B42305EB24D5F026AF416D46F055BDC5A2DA063BB99E6ABFC1",
        "renderer_rmdwin10_f.dll": "85BAAF7026AFF1403ABAD6F497441963CE2F6012E9B84D462FED972E42723C6E",
    },
}

RVA_PROBES = {
    "d3d_rmdwin10_f.dll": {
        "DlssContextPointer": 0x111BE0,
        "NativeDevicePointer": 0x136D28,
        "EngineFrameCounter": 0x136C68,
        "D3dTlsIndex": 0x1115FC,
        "D3dFallbackContextPointer": 0x111C18,
        "D3D12CreateDeviceImport": 0x5D510,
        "D3D12DeviceIid": 0x662A0,
        "CommandQueueCreateCall": 0x4332D,
        "ControlCommandQueueCtorCall": 0x2B47E,
        "ControlCommandQueueCtor": 0x43280,
        "ControlGlobalD3D12DevicePointer": 0x111C08,
        "ControlNgxInitCall": 0x1E88A,
        "ControlNgxInitWrapper": 0x52CC0,
        "NativeNgxSetupHelper": 0x1CDF0,
        "NativeNgxSetupCall": 0x1FDBB,
    },
    "renderer_rmdwin10_f.dll": {
        "RenderToTextureCtorImport": 0x5DFD80,
        "HudPrimaryRttCall": 0x134702,
        "HudSecondaryRttCall": 0x134870,
        "RRShaderReflectionTarget": 0x1296788,
        "RRShaderDiffuseGIColor": 0x12973E8,
        "RRShaderDiffuseGIWeightUav": 0x1297478,
        "RRShaderDiffuseGIWeightSrv": 0x12974A0,
        "RRShaderGBufferCandidate0": 0x1296440,
        "RRShaderGBufferCandidate1": 0x1296468,
        "RRShaderGBufferCandidate2": 0x1296490,
        "RRShaderGBufferCandidate3": 0x12964B8,
        "RRShaderGBufferCandidate4": 0x12964E0,
        "RRShaderLightBufferDiffuse": 0x1297EF8,
        "RRShaderLightBufferSpecular": 0x1297F20,
        "RRReflectionDiffuseFunctionBegin": 0x128D20,
        "RRGBufferNormalsFunctionBegin": 0x1268A0,
        "RRLightInputsFunctionBegin": 0xC86B0,
        "RRLightBuffersFunctionBegin": 0x1371A0,
        "RRRadianceFunctionBegin": 0xA4AD0,
        "RRShadowFunctionBegin": 0x23ED10,
        "RRAlbedoPrepare": 0x13E4D0,
        "RRAlbedoJoin": 0x13EBB0,
        "RRNoisyReset": 0x13E20,
        "RRNativeFrameByte": 0x914620,
        "RRWireframeOption0": 0x802A33,
        "RRWireframeOption1": 0x802A6A,
        "RRDrawAbiVtable": 0x631C08,
    },
}

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest().upper()

def validate_game_path(path: Optional[str]) -> Optional[Path]:
    if not path:
        return None
    try:
        p = Path(path).expanduser().resolve()
    except Exception:
        return None
    return p if all((p / name).is_file() for name in FILES) else None

def _registry_paths() -> list[Path]:
    out: list[Path] = []
    try:
        import winreg
    except ImportError:
        return out
    roots = [
        (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\GOG.com\Games"),
        (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\GOG.com\Games"),
        (winreg.HKEY_CURRENT_USER, r"SOFTWARE\GOG.com\Games"),
    ]
    for hive, base_name in roots:
        try:
            with winreg.OpenKey(hive, base_name) as base:
                i = 0
                while True:
                    try:
                        sub_name = winreg.EnumKey(base, i)
                    except OSError:
                        break
                    i += 1
                    try:
                        with winreg.OpenKey(base, sub_name) as sub:
                            values = {}
                            j = 0
                            while True:
                                try:
                                    name, value, _kind = winreg.EnumValue(sub, j)
                                except OSError:
                                    break
                                j += 1
                                values[name.lower()] = value
                            game_name = str(values.get("gamename", values.get("name", "")))
                            candidate_text = values.get("path", values.get("installpath", values.get("install_path")))
                            candidate = validate_game_path(str(candidate_text)) if candidate_text else None
                            if candidate and (not game_name or "control" in game_name.lower()):
                                out.append(candidate)
                    except OSError:
                        pass
        except OSError:
            pass
    return out

def find_gog() -> list[Path]:
    roots: list[Path] = list(_registry_paths())
    candidates = [
        Path(r"C:\GOG Games\Control"),
        Path(r"C:\GOG Games\Control Ultimate Edition"),
    ]
    for env_name in ("ProgramFiles", "ProgramFiles(x86)"):
        value = os.environ.get(env_name)
        if value:
            candidates += [
                Path(value) / "GOG Galaxy" / "Games" / "Control",
                Path(value) / "GOG Galaxy" / "Games" / "Control Ultimate Edition",
            ]
    for candidate in candidates:
        good = validate_game_path(str(candidate))
        if good:
            roots.append(good)
    return list(dict.fromkeys(roots))

def steam_roots() -> list[Path]:
    out: list[Path] = []
    try:
        import winreg
        keys = [
            (winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam"),
            (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\Valve\Steam"),
            (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Valve\Steam"),
        ]
        for hive, key_name in keys:
            try:
                with winreg.OpenKey(hive, key_name) as key:
                    for value_name in ("SteamPath", "InstallPath"):
                        try:
                            out.append(Path(winreg.QueryValueEx(key, value_name)[0]))
                        except OSError:
                            pass
            except OSError:
                pass
    except ImportError:
        pass
    return list(dict.fromkeys(out))

def find_steam() -> Optional[Path]:
    for root in steam_roots():
        libraries = [root]
        vdf = root / "steamapps" / "libraryfolders.vdf"
        if vdf.is_file():
            text = vdf.read_text(encoding="utf-8", errors="ignore")
            libraries += [Path(x.replace("\\\\", "\\")) for x in re.findall(r'"path"\s+"([^"]+)"', text)]
        for library in dict.fromkeys(libraries):
            acf = library / "steamapps" / "appmanifest_870780.acf"
            if not acf.is_file():
                continue
            text = acf.read_text(encoding="utf-8", errors="ignore")
            m = re.search(r'"installdir"\s+"([^"]+)"', text)
            if m:
                candidate = validate_game_path(str(library / "steamapps" / "common" / m.group(1)))
                if candidate:
                    return candidate
    return None

def find_epic() -> Optional[Path]:
    program_data = os.environ.get("ProgramData")
    if not program_data:
        return None
    manifest_dir = Path(program_data) / "Epic" / "EpicGamesLauncher" / "Data" / "Manifests"
    if not manifest_dir.is_dir():
        return None
    candidates = []
    for item in manifest_dir.glob("*.item"):
        try:
            data = json.loads(item.read_text(encoding="utf-8-sig"))
            display = str(data.get("DisplayName", ""))
            install = validate_game_path(data.get("InstallLocation"))
            if install and re.search(r"(?i)^control(?:\s|$)", display):
                candidates.append(install)
        except Exception:
            pass
    return candidates[0] if len(candidates) == 1 else None

def pe_summary(path: Path) -> dict:
    data = path.read_bytes()
    h = struct.unpack_from("<I", data, 0x3C)[0]
    coff = h + 4
    machine, section_count, timestamp = struct.unpack_from("<HHI", data, coff)
    optional_size = struct.unpack_from("<H", data, coff + 16)[0]
    optional = coff + 20
    image_base = struct.unpack_from("<Q", data, optional + 24)[0]
    size_image = struct.unpack_from("<I", data, optional + 56)[0]
    section_table = optional + optional_size
    sections = []
    for i in range(section_count):
        p = section_table + 40 * i
        name = data[p:p+8].rstrip(b"\0").decode("ascii", errors="replace")
        virtual_size, va, raw_size, raw = struct.unpack_from("<IIII", data, p + 8)
        raw_bytes = data[raw:raw+raw_size] if raw and raw_size else b""
        sections.append({
            "name": name,
            "virtual_address": hex(va),
            "virtual_size": virtual_size,
            "raw_size": raw_size,
            "sha256": hashlib.sha256(raw_bytes).hexdigest().upper() if raw_bytes else None,
        })
    pe = PE(path)
    return {
        "machine": hex(machine),
        "timestamp": timestamp,
        "image_base": hex(image_base),
        "size_of_image": size_image,
        "sections": sections,
        "export_count": len(pe.exports),
    }

def required_symbols() -> list[str]:
    target = ROOT / "src" / "target.h"
    if not target.is_file():
        return []
    text = target.read_text(encoding="utf-8", errors="ignore")
    return sorted(set(re.findall(r'"([?][^"\r\n]{8,})"', text)))

def file_record(path: Path) -> dict:
    rec = {"path": str(path), "size": path.stat().st_size, "sha256": sha256(path)}
    try:
        rec["pe"] = pe_summary(path)
    except Exception as exc:
        rec["pe_error"] = f"{type(exc).__name__}: {exc}"
    return rec

def export_compare(reference_file: Path, candidate_file: Path, symbols: list[str]) -> dict:
    try:
        a = PE(reference_file)
        b = PE(candidate_file)
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}
    rows = []
    for sym in symbols:
        if sym in a.exports or sym in b.exports:
            rows.append({
                "symbol": sym,
                "reference_rva": hex(a.exports[sym]) if sym in a.exports else None,
                "gog_rva": hex(b.exports[sym]) if sym in b.exports else None,
                "same_rva": sym in a.exports and sym in b.exports and a.exports[sym] == b.exports[sym],
            })
    return {
        "reference_export_count": len(a.exports),
        "gog_export_count": len(b.exports),
        "required_symbols": rows,
        "missing_from_gog": sorted(name for name in a.exports if name not in b.exports)[:200],
        "new_in_gog": sorted(name for name in b.exports if name not in a.exports)[:200],
    }

def raw_to_rva(pe: PE, raw_offset: int) -> Optional[int]:
    for _name, va, raw_size, raw_ptr in pe.sections:
        if raw_ptr <= raw_offset < raw_ptr + raw_size:
            return va + (raw_offset - raw_ptr)
    return None

def relocation_probe(reference_file: Path, candidate_file: Path, probes: dict[str, int]) -> list[dict]:
    try:
        a = PE(reference_file)
        b = PE(candidate_file)
    except Exception as exc:
        return [{"error": f"{type(exc).__name__}: {exc}"}]
    rows = []
    for label, rva in probes.items():
        row = {"label": label, "reference_rva": hex(rva)}
        try:
            center = a.off(rva)
        except Exception:
            row["status"] = "reference_rva_unmapped"
            rows.append(row)
            continue
        found = False
        for length in (64, 48, 32, 24, 16):
            half = length // 2
            start = max(0, center - half)
            pattern = a.data[start:start + length]
            if len(pattern) != length:
                continue
            matches = []
            pos = b.data.find(pattern)
            while pos != -1 and len(matches) < 4:
                matches.append(pos)
                pos = b.data.find(pattern, pos + 1)
            if len(matches) == 1:
                gog_rva = raw_to_rva(b, matches[0] + half)
                row.update({
                    "status": "unique_exact_window",
                    "window_bytes": length,
                    "gog_rva": hex(gog_rva) if gog_rva is not None else None,
                    "delta": (gog_rva - rva) if gog_rva is not None else None,
                })
                found = True
                break
            if len(matches) > 1:
                row.update({"status": "ambiguous_exact_window", "window_bytes": length, "match_count_at_least": len(matches)})
                found = True
                break
        if not found:
            row["status"] = "no_exact_window_match"
        rows.append(row)
    return rows

def source_literal_inventory() -> list[dict]:
    rows = []
    seen = set()
    for path in sorted((ROOT / "src").rglob("*")):
        if path.suffix.lower() not in {".h", ".hpp", ".cpp", ".inc"} or not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        for line_no, line in enumerate(text.splitlines(), 1):
            for m in re.finditer(r"0x[0-9A-Fa-f]{5,}", line):
                value = int(m.group(0), 16)
                if value >= 0x100000000:
                    continue
                key = (str(path.relative_to(ROOT)), line_no, value)
                if key in seen:
                    continue
                seen.add(key)
                rows.append({
                    "file": str(path.relative_to(ROOT)).replace("\\", "/"),
                    "line": line_no,
                    "literal": hex(value),
                    "context": line.strip()[:240],
                })
    return rows

def choose_reference(steam: Optional[Path], epic: Optional[Path]) -> tuple[Optional[str], Optional[Path]]:
    if steam:
        return "steam_21225456", steam
    if epic:
        return "epic_0.0.518.2177", epic
    return None, None

def classify(hashes: dict[str, str]) -> str:
    for label, expected in KNOWN_TARGETS.items():
        if all(hashes[name] == expected[name] for name in FILES):
            return f"IDENTICAL_TO_{label.upper()}"
    for label, expected in KNOWN_TARGETS.items():
        if (hashes["d3d_rmdwin10_f.dll"] == expected["d3d_rmdwin10_f.dll"] and
                hashes["renderer_rmdwin10_f.dll"] == expected["renderer_rmdwin10_f.dll"]):
            return f"SAME_ENGINE_DLLS_AS_{label.upper()}_DIFFERENT_EXE"
    return "THIRD_PROFILE_REQUIRED"

def main() -> int:
    ap = argparse.ArgumentParser(description="Read-only Control FG GOG compatibility collector.")
    ap.add_argument("--gog", help="GOG Control install folder (auto-detected if omitted)")
    ap.add_argument("--steam", help="Steam Control install folder for byte-window comparison")
    ap.add_argument("--epic", help="Epic Control install folder for byte-window comparison")
    ap.add_argument("--output", help="Output directory")
    args = ap.parse_args()

    gog = validate_game_path(args.gog)
    if not gog:
        candidates = find_gog()
        gog = candidates[0] if len(candidates) == 1 else None
        if len(candidates) > 1:
            print("Multiple possible GOG Control installs detected; pass --gog PATH.")
            for c in candidates:
                print("  ", c)
            return 2
    if not gog:
        print("GOG Control install was not auto-detected.")
        print('Run again with: py -3 tools\\gog_compat_probe.py --gog "X:\\Path\\To\\Control"')
        return 2

    steam = validate_game_path(args.steam) or find_steam()
    epic = validate_game_path(args.epic) or find_epic()
    reference_label, reference = choose_reference(steam, epic)

    stamp = time.strftime("%Y%m%d-%H%M%S")
    out = Path(args.output).resolve() if args.output else (ROOT / "compatibility-output" / f"GOG-{stamp}")
    out.mkdir(parents=True, exist_ok=True)

    report = {
        "collector": "Control-FG GOG compatibility probe v1",
        "generated_local": time.strftime("%Y-%m-%d %H:%M:%S"),
        "gog_path": str(gog),
        "steam_path": str(steam) if steam else None,
        "epic_path": str(epic) if epic else None,
        "reference_profile": reference_label,
        "known_targets": KNOWN_TARGETS,
        "stores": {"gog": {}, "steam": {}, "epic": {}},
        "comparison": {},
        "source_large_hex_literals": source_literal_inventory(),
    }

    for name in FILES:
        report["stores"]["gog"][name] = file_record(gog / name)
        if steam:
            report["stores"]["steam"][name] = file_record(steam / name)
        if epic:
            report["stores"]["epic"][name] = file_record(epic / name)
    for name in OPTIONAL_FILES:
        p = gog / name
        if p.is_file():
            report["stores"]["gog"][name] = file_record(p)

    gog_hashes = {name: report["stores"]["gog"][name]["sha256"] for name in FILES}
    report["comparison"]["classification"] = classify(gog_hashes)
    report["comparison"]["known_target_matches"] = {
        label: {name: gog_hashes[name] == expected[name] for name in FILES}
        for label, expected in KNOWN_TARGETS.items()
    }

    if reference:
        symbols = required_symbols()
        report["comparison"]["reference"] = reference_label
        report["comparison"]["exports"] = {}
        report["comparison"]["relocations"] = {}
        for name in ("d3d_rmdwin10_f.dll", "renderer_rmdwin10_f.dll"):
            report["comparison"]["exports"][name] = export_compare(reference / name, gog / name, symbols)
            report["comparison"]["relocations"][name] = relocation_probe(
                reference / name, gog / name, RVA_PROBES.get(name, {})
            )

    json_path = out / "gog-compatibility-report.json"
    json_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    summary = [
        "Control FG GOG compatibility probe",
        f"GOG:       {gog}",
        f"Reference: {reference_label or 'not available'}",
        "",
        f"Classification: {report['comparison']['classification']}",
        "",
    ]
    for name in FILES:
        summary += [name, f"  GOG SHA256: {gog_hashes[name]}"]
        for label, expected in KNOWN_TARGETS.items():
            summary.append(f"  Matches {label}: {gog_hashes[name] == expected[name]}")
    if report["comparison"]["classification"] == "THIRD_PROFILE_REQUIRED":
        summary += [
            "",
            "Interpretation:",
            "  GOG has a third binary identity. Do not bypass the build lock.",
            "  Use the export and exact byte-window comparison in the JSON report to determine",
            "  whether GOG can share the existing RVA/layout profile with a third hash triple.",
        ]
    else:
        summary += [
            "",
            "Interpretation:",
            "  GOG reuses an already validated binary identity/layout profile.",
            "  No new RVA map is required for the matching target.",
        ]

    summary_path = out / "SUMMARY.txt"
    summary_path.write_text("\n".join(summary) + "\n", encoding="utf-8")

    zip_path = out.with_suffix(".zip")
    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as z:
        z.write(json_path, json_path.name)
        z.write(summary_path, summary_path.name)

    print("\n".join(summary))
    print()
    print(f"Report ZIP: {zip_path}")
    print("No game binaries were copied into the report.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
