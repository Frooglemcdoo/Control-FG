"""Portable evidence/hash portion of Verify-Source.ps1; no PowerShell execution."""
from pathlib import Path
import hashlib
import json
import re


def validate_current_checkpoint(report):
    tests = report.get('tests')
    gate = tests.get('portable_gate') if isinstance(tests, dict) else None
    if report.get('status') != 'LOCAL_PASS' or not isinstance(gate, str) or not gate.startswith('PASS'):
        raise ValueError('validation/current-release/results.json: portable_gate missing or incomplete')


def verify_source_evidence(root):
    root = Path(root).resolve()
    support_collectors = {'Collect-ControlFG-Compact-Logs.cmd', 'Collect-ControlFG-Compact-Logs.ps1'}
    if {p.name for p in root.glob('Collect-*') if p.is_file()} != support_collectors:
        raise ValueError('Only the compact support collector pair belongs in the source root')
    script = (root / 'Verify-Source.ps1').read_text(encoding='utf-8-sig')
    all_references = list(dict.fromkeys(re.findall(
        r"Join-Path\s+\$PSScriptRoot\s+'([^']+\.json)'", script)))
    # Verify-Source.ps1 also opens descriptive/reverse-engineering JSON (for
    # example shader reflection dumps). Those are source evidence inputs, not
    # portable test reports. This helper validates only package manifests,
    # frozen hashes and validation/* evidence reports.
    references = [ref for ref in all_references
                  if ref in {'SOURCE-SHA256.json', 'g3-frozen-files.json'}
                  or ref.startswith('validation/')]
    required = {'validation/part1/validation.json',
                'validation/g12-current/reflectance-validation.json',
                'validation/current-release/results.json',
                'g3-frozen-files.json', 'SOURCE-SHA256.json'}
    if not required.issubset(references):
        raise ValueError('Verify-Source evidence discovery is incomplete')
    checked = 0

    def check_file(report, name, expected):
        nonlocal checked
        path = (root / name).resolve()
        if not path.is_relative_to(root) or not path.is_file():
            raise ValueError(f'{report}: missing or invalid source path: {name}')
        if not isinstance(expected, str) or not re.fullmatch('[0-9a-fA-F]{64}', expected):
            raise ValueError(f'{report}: invalid SHA256: {name}')
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected.lower():
            raise ValueError(f'{report}: test evidence does not match source: {name}')
        checked += 1

    # One authoritative current-release checkpoint owns hashes for every file
    # intentionally advanced beyond historical milestone evidence. Both this
    # portable gate and Verify-Source.ps1 use the same checkpoint, preventing
    # hard-coded inheritance exception lists from drifting apart.
    current_ref = 'validation/current-release/results.json'
    current_report = json.loads((root / current_ref).read_text(encoding='utf-8-sig'))
    validate_current_checkpoint(current_report)
    current_entries = current_report.get('tested_sha256')
    if not isinstance(current_entries, dict) or not current_entries:
        raise ValueError(f'{current_ref}: missing source evidence')
    current_paths = set(current_entries)

    # PT reflection R1 is an experimental branch layered on the frozen v2.1.2
    # checkpoint. Validate its narrow source contract directly and exempt only
    # those intentionally changed files from the release hash checkpoint.
    pt_header = root / 'src/pt_reflections.h'
    pt_experiment = pt_header.is_file()
    pt_owned = {
        'src/probe.cpp','src/fg_overlay.h','src/streamline_bridge.h',
        'src/pt_reflections.h','Verify-Source.ps1','Verify-Build.ps1',
        'tools/verify_source_evidence.py','.github/workflows/prepare-v2.1.2.yml'
    }
    if pt_experiment:
        pt = pt_header.read_text(encoding='utf-8-sig')
        overlay = (root/'src/fg_overlay.h').read_text(encoding='utf-8-sig')
        bridge = (root/'src/streamline_bridge.h').read_text(encoding='utf-8-sig')
        probe = (root/'src/probe.cpp').read_text(encoding='utf-8-sig')
        for marker in (
            'ReflectionRayCountCallRva = 0x12930D',
            'SetProviderIatRva = 0x5DFF90',
            'CreateStateObjectVtableSlot = 62',
            'reflectionRayGeneration',
            'PT_REFLECTION_PROVIDER_HOOK',
            'PT_REFLECTION_STATE_OBJECT',
        ):
            if marker not in pt:
                raise ValueError('PT reflection R1 source contract missing: '+marker)
        for marker in ('L"PT REFLECTIONS (PROBE)"','L"NATIVE"','L"1 ray"','L"2 rays"','L"4 rays"','PTReflectionRays'):
            if marker not in overlay:
                raise ValueError('PT reflection R1 overlay contract missing: '+marker)
        if 'control_pt_reflection::InstallDevice' not in bridge:
            raise ValueError('PT reflection R1 Device5 hook is not wired')
        for marker in ('#include "pt_reflections.h"','InstallProviderHook(renderer,d3d)','PT_REFLECTION_PROBE_INSTALL'):
            if marker not in probe:
                raise ValueError('PT reflection R1 integration missing: '+marker)

    for ref in references:
        report = json.loads((root / ref).read_text(encoding='utf-8-sig'))
        if ref == 'SOURCE-SHA256.json':
            entries = report
        elif ref == 'g3-frozen-files.json':
            entries = {item['Path']: item['SHA256'] for item in report['Files']}
        else:
            if report.get('status') not in {'PASS', 'LOCAL_PASS'}:
                raise ValueError(f'{ref}: portable tests incomplete')
            entries = report.get('tested_sha256')
            if ref.endswith('/reflectance-validation.json'):
                if (report.get('normal') != 'PASS' or report.get('asan_ubsan') != 'PASS'
                        or len(report.get('assertions', [])) != 8
                        or report.get('matrix_reference_cases') != 101505
                        or report.get('rr_evaluation_enabled') is not False):
                    raise ValueError(f'{ref}: reflectance validation is incomplete')
        if not isinstance(entries, dict) or not entries:
            raise ValueError(f'{ref}: missing source evidence')
        for name, expected in entries.items():
            if ref != current_ref and name in current_paths:
                continue
            if ref == current_ref and pt_experiment and name in pt_owned:
                continue
            check_file(ref, name, expected)
    return {'references': references, 'hashes_checked': checked,
            'powershell_execution': 'NOT_RUN'}


if __name__ == '__main__':
    import sys
    try:
        print(json.dumps(verify_source_evidence(sys.argv[1] if len(sys.argv) > 1 else '.'), indent=2))
    except (ValueError, OSError, KeyError) as error:
        raise SystemExit(str(error))
