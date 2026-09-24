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
KNOWN_STEAM = {
    "Control_DX12.exe": "9441DB3AE75B267ABD989846AD0895E3FE24ABCC0F06E58F93C34FC8D4736506",
    "d3d_rmdwin10_f.dll": "CCEB99CBD9C019AF907C24C44701A53C8D230965C1FB31C325233E4C80214AA5",
    "renderer_rmdwin10_f.dll": "EBFB5B47CEBC2D4482E912B8090BD343F717BAB8A178B5A6385DEC9105E7C433",
}

# High-value build-locked RVAs that directly affect live hooks/state.
# This is intentionally diagnostic only: it never patches or launches the game.
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
    p = Path(path).expanduser().resolve()
    return p if all((p / name).is_file() for name in FILES) else None

def find_epic() -> list[Path]:
    roots: list[Path] = []
    program_data = os.environ.get("ProgramData")
    if not program_data:
        return roots
    manifest_dir = Path(program_data) / "Epic" / "EpicGamesLauncher" / "Data" / "Manifests"
    if not manifest_dir.is_dir():
        return roots
    for item in manifest_dir.glob("*.item"):
        try:
            data = json.loads(item.read_text(encoding="utf-8-sig"))
            display = str(data.get("DisplayName", ""))
            install = validate_game_path(data.get("InstallLocation"))
            if install and re.search(r"(?i)^control(?:\s|$)", display):
                roots.append(install)
        except Exception:
            pass
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
            if not m:
                continue
            candidate = validate_game_path(str(library / "steamapps" / "common" / m.group(1)))
            if candidate:
                return candidate
    return None

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
    # Both one-line and split string constants are covered by harvesting MSVC-decorated strings.
    return sorted(set(re.findall(r'"([?][^"\r\n]{8,})"', text)))

def file_record(path: Path) -> dict:
    rec = {
        "path": str(path),
        "size": path.stat().st_size,
        "sha256": sha256(path),
    }
    try:
        rec["pe"] = pe_summary(path)
    except Exception as exc:
        rec["pe_error"] = f"{type(exc).__name__}: {exc}"
    return rec

def export_compare(steam_file: Path, epic_file: Path, symbols: list[str]) -> dict:
    try:
        s = PE(steam_file)
        e = PE(epic_file)
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}
    rows = []
    for sym in symbols:
        if sym in s.exports or sym in e.exports:
            rows.append({
                "symbol": sym,
                "steam_rva": hex(s.exports[sym]) if sym in s.exports else None,
                "epic_rva": hex(e.exports[sym]) if sym in e.exports else None,
                "same_rva": sym in s.exports and sym in e.exports and s.exports[sym] == e.exports[sym],
            })
    return {
        "steam_export_count": len(s.exports),
        "epic_export_count": len(e.exports),
        "required_symbols": rows,
        "missing_from_epic": sorted(name for name in s.exports if name not in e.exports)[:200],
        "new_in_epic": sorted(name for name in e.exports if name not in s.exports)[:200],
    }

def raw_to_rva(pe: PE, raw_offset: int) -> Optional[int]:
    for _name, va, raw_size, raw_ptr in pe.sections:
        if raw_ptr <= raw_offset < raw_ptr + raw_size:
            return va + (raw_offset - raw_ptr)
    return None

def relocation_probe(steam_file: Path, epic_file: Path, probes: dict[str, int]) -> list[dict]:
    try:
        s = PE(steam_file)
        e = PE(epic_file)
    except Exception as exc:
        return [{"error": f"{type(exc).__name__}: {exc}"}]
    rows = []
    for label, rva in probes.items():
        row = {"label": label, "steam_rva": hex(rva)}
        try:
            center = s.off(rva)
        except Exception:
            row["status"] = "steam_rva_unmapped"
            rows.append(row)
            continue

        found = False
        for length in (48, 32, 24, 16):
            half = length // 2
            start = max(0, center - half)
            pattern = s.data[start:start + length]
            if len(pattern) != length:
                continue
            matches = []
            pos = e.data.find(pattern)
            while pos != -1 and len(matches) < 4:
                matches.append(pos)
                pos = e.data.find(pattern, pos + 1)
            if len(matches) == 1:
                epic_rva = raw_to_rva(e, matches[0] + half)
                row.update({
                    "status": "unique_exact_window",
                    "window_bytes": length,
                    "epic_rva": hex(epic_rva) if epic_rva is not None else None,
                    "delta": (epic_rva - rva) if epic_rva is not None else None,
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
                # Skip common error/status/mask constants; retain address-sized candidates.
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

def main() -> int:
    ap = argparse.ArgumentParser(description="Read-only Control FG Steam/Epic compatibility collector.")
    ap.add_argument("--epic", help="Epic Control install folder (auto-detected if omitted)")
    ap.add_argument("--steam", help="Steam Control install folder (auto-detected if omitted)")
    ap.add_argument("--output", help="Output directory")
    args = ap.parse_args()

    epic = validate_game_path(args.epic)
    if not epic:
        candidates = find_epic()
        epic = candidates[0] if len(candidates) == 1 else None
        if len(candidates) > 1:
            print("Multiple Epic Control installs detected; pass --epic PATH.")
            for c in candidates:
                print("  ", c)
            return 2
    steam = validate_game_path(args.steam) or find_steam()

    if not epic:
        print("Epic Control install was not auto-detected.")
        print('Run again with: py -3 tools\\epic_compat_probe.py --epic "X:\\Path\\To\\Control"')
        return 2

    stamp = time.strftime("%Y%m%d-%H%M%S")
    out = Path(args.output).resolve() if args.output else (ROOT / "compatibility-output" / f"Epic-{stamp}")
    out.mkdir(parents=True, exist_ok=True)

    report = {
        "collector": "Control-FG Epic compatibility probe v1",
        "generated_local": time.strftime("%Y-%m-%d %H:%M:%S"),
        "epic_path": str(epic),
        "steam_path": str(steam) if steam else None,
        "known_steam_hashes": KNOWN_STEAM,
        "stores": {"epic": {}, "steam": {}},
        "comparison": {},
        "source_large_hex_literals": source_literal_inventory(),
    }

    for name in FILES:
        report["stores"]["epic"][name] = file_record(epic / name)
        if steam:
            report["stores"]["steam"][name] = file_record(steam / name)

    epic_hashes = {n: report["stores"]["epic"][n]["sha256"] for n in FILES}
    report["comparison"]["epic_matches_known_steam"] = {n: epic_hashes[n] == KNOWN_STEAM[n] for n in FILES}

    d3d_same = epic_hashes["d3d_rmdwin10_f.dll"] == KNOWN_STEAM["d3d_rmdwin10_f.dll"]
    renderer_same = epic_hashes["renderer_rmdwin10_f.dll"] == KNOWN_STEAM["renderer_rmdwin10_f.dll"]
    exe_same = epic_hashes["Control_DX12.exe"] == KNOWN_STEAM["Control_DX12.exe"]
    if d3d_same and renderer_same and exe_same:
        classification = "IDENTICAL_TO_VERIFIED_STEAM_TARGET"
    elif d3d_same and renderer_same:
        classification = "SAME_RENDERER_AND_D3D_DIFFERENT_EXE"
    else:
        classification = "STORE_BINARY_PROFILE_DIFFERS"
    report["comparison"]["classification"] = classification

    if steam:
        symbols = required_symbols()
        report["comparison"]["exports"] = {}
        report["comparison"]["relocations"] = {}
        for name in ("d3d_rmdwin10_f.dll", "renderer_rmdwin10_f.dll"):
            report["comparison"]["exports"][name] = export_compare(steam / name, epic / name, symbols)
            report["comparison"]["relocations"][name] = relocation_probe(
                steam / name, epic / name, RVA_PROBES.get(name, {})
            )

    json_path = out / "epic-compatibility-report.json"
    json_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    summary = [
        "Control FG Epic compatibility probe",
        f"Epic:  {epic}",
        f"Steam: {steam if steam else 'not detected'}",
        "",
        f"Classification: {classification}",
        "",
    ]
    for name in FILES:
        actual = epic_hashes[name]
        summary += [
            name,
            f"  Epic SHA256:   {actual}",
            f"  Steam target:  {KNOWN_STEAM[name]}",
            f"  Exact target:  {actual == KNOWN_STEAM[name]}",
        ]
    if classification == "SAME_RENDERER_AND_D3D_DIFFERENT_EXE":
        summary += [
            "",
            "Interpretation:",
            "  The engine DLLs exactly match the verified Steam target.",
            "  The likely runtime blocker is the executable hash gate, not renderer RVAs.",
            "  Add an Epic executable identity to the supported target profiles; keep DLL/RVA safety checks intact.",
        ]
    elif classification == "STORE_BINARY_PROFILE_DIFFERS":
        summary += [
            "",
            "Interpretation:",
            "  At least one build-locked engine DLL differs from the verified Steam target.",
            "  Do NOT bypass the hash gate. Use the export/relocation evidence in the JSON report",
            "  to build a separate Epic target profile and revalidate every patched/read RVA.",
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
