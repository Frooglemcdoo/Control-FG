"""E3: offline PIX C++ export. Never launches Control or changes a capture.

CLI contract verified in PIX 2606.18-preview's pixtool help strings.
Generated C++ is data for inspection; this tool never compiles or runs it.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import zipfile

VERSION = 'E3'
GIB = 1024 ** 3
CODE = {'.cpp', '.h', '.hpp', '.c', '.inl', '.cmake'}
REVIEW = CODE | {'.txt', '.json', '.log', '.csv', '.cso', '.dxil', '.dxbc', '.png', '.sln', '.vcxproj', '.filters'}
SIGNALS = {
    'dispatch_rays': r'\bDispatchRays\s*\(',
    'state_object': r'\bCreateStateObject\s*\(',
    'dxil_library': r'\bD3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY\b',
    'acceleration_structure_build': r'\bBuildRaytracingAccelerationStructure\s*\(',
}

def write_json(path, data):
    tmp = path.with_suffix(path.suffix + '.partial')
    tmp.write_text(json.dumps(data, indent=2), encoding='utf-8')
    tmp.replace(path)

def files_under(root):
    # Do not follow links/junctions outside the export.
    for parent, dirs, files in os.walk(root, followlinks=False):
        def safe(p):
            return not p.is_symlink() and not (getattr(p.lstat(), 'st_file_attributes', 0) & 0x400)
        dirs[:] = [d for d in dirs if safe(Path(parent) / d)]
        for name in sorted(files):
            p = Path(parent) / name
            if safe(p) and p.is_file():
                yield p

def fingerprint(p):
    s = p.stat()
    return {'name': p.name, 'bytes': s.st_size, 'mtime_ns': s.st_mtime_ns}

def find_pix():
    override = os.environ.get('PIX_DIR')
    if override:
        p = Path(override) / 'pixtool.exe'
        if not p.is_file():
            raise RuntimeError('PIX_DIR does not contain pixtool.exe: ' + str(p))
        return p
    base = Path(os.environ.get('ProgramFiles', 'C:/Program Files')) / 'Microsoft PIX Preview'
    choices = sorted(base.glob('*/pixtool.exe'), reverse=True)
    if not choices:
        raise RuntimeError('Set PIX_DIR to the installed Microsoft PIX Preview directory.')
    return choices[0]

def game_closed():
    if os.name != 'nt':
        raise RuntimeError('Run this exporter on the Windows PC with PIX installed.')
    result = subprocess.run(['tasklist', '/FO', 'CSV', '/NH'], capture_output=True, text=True, check=True)
    if re.search(r'"Control(?:_DX12|_DX11)?\.exe"', result.stdout, re.I):
        raise RuntimeError('Close Control before offline export. No game process will be stopped by this tool.')

def run_logged(command, log, directory, timeout, interval=10):
    start = time.monotonic()
    next_notice = start
    with log.open('wb') as out:
        child = subprocess.Popen(command, stdout=out, stderr=subprocess.STDOUT)
        try:
            while child.poll() is None:
                now = time.monotonic()
                if now - start >= timeout:
                    child.terminate()
                    try:
                        child.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        child.kill()
                        child.wait()
                    return {'returncode': child.returncode, 'error': 'Offline exporter timed out; partial output retained.'}
                if now >= next_notice:
                    sizes = [p.stat().st_size for p in files_under(directory)] if directory.exists() else []
                    print(f'  {int(now-start)}s elapsed | {len(sizes)} files | {sum(sizes)/1024**2:.1f} MiB written', flush=True)
                    next_notice = now + interval
                time.sleep(0.25)
            return {'returncode': child.returncode}
        finally:
            if child.poll() is None:
                child.terminate()
                try:
                    child.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    child.kill()
                    child.wait()

def inspect_export(directory):
    inventory = []
    counts = {k: 0 for k in SIGNALS}
    locations = []
    for p in files_under(directory):
        item = {'path': p.relative_to(directory).as_posix(), 'bytes': p.stat().st_size}
        if p.suffix.lower() in CODE:
            # Generated source is streamed so a large frame does not exhaust RAM.
            digest = hashlib.sha256()
            with p.open('rb') as f:
                for line_no, line in enumerate(f, 1):
                    digest.update(line)
                    text = line.decode('utf-8', errors='replace')
                    for key, pattern in SIGNALS.items():
                        found = len(re.findall(pattern, text))
                        counts[key] += found
                        if found:
                            locations.append({'path': item['path'], 'line': line_no, 'kind': key, 'count': found})
            item['sha256'] = digest.hexdigest()
        inventory.append(item)
    resources = [i for i in inventory if Path(i['path']).name.lower() == 'resources.bin' and i['bytes'] > 0]
    # Structural evidence only: counts can include declarations/helper definitions.
    checks = {'cpp_present': any(Path(i['path']).suffix.lower() == '.cpp' and i['bytes'] for i in inventory),
              'resources_bin_present': bool(resources),
              **{k: v > 0 for k, v in counts.items()}}
    return {'files': inventory, 'code_signal_counts': counts, 'locations': locations,
            'structural_checks': checks, 'structural_checks_passed': all(checks.values()),
            'semantic_validation': 'NOT VERIFIED: source hits do not prove correct bindings, material/light layouts, or replay equivalence.'}

def reusable(record, capture, output):
    if record.get('capture') != fingerprint(capture) or record.get('status') != 'exported_unvalidated':
        return False
    for item in record.get('inventory', {}).get('files', []):
        p = output / item['path']
        if not p.is_file() or p.stat().st_size != item['bytes']:
            return False
    return bool(record.get('inventory', {}).get('files'))

def review_zip(destination, output_root, session, limit=256*1024**2):
    used = 0
    excluded = []
    partial = destination.with_suffix('.zip.partial')
    with zipfile.ZipFile(partial, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as z:
        for p in sorted(files_under(output_root)):
            if p.name == '.running':
                continue
            size = p.stat().st_size
            relative = p.relative_to(output_root).as_posix()
            if p.suffix.lower() not in REVIEW or used + size > limit:
                excluded.append({'path': relative, 'bytes': size, 'reason': 'kept locally; bulk payload or review size limit'})
                continue
            z.write(p, relative)
            used += size
        for name in ('session.json', 'analysis-results.json', 'tool.json'):
            p = session / name
            if p.is_file() and p.stat().st_size < 1024**2:
                z.write(p, 'original-metadata/' + name)
        z.writestr('review-contents.json', json.dumps({'excluded': excluded,
            'note': 'This is a review bundle, not a complete replay project. Full outputs and original captures remain on the PC.'}, indent=2))
    partial.replace(destination)
    return excluded

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('session', type=Path)
    parser.add_argument('--timeout', type=int, default=1800, help='Seconds per offline PIX export (default 1800)')
    parser.add_argument('--native-baseline', action='store_true', help='Record user-confirmed proxy-disabled provenance')
    args = parser.parse_args()
    if args.timeout < 1:
        parser.error('--timeout must be positive')
    session = args.session.resolve()
    captures = sorted(session.glob('*.wpix'))
    if not captures:
        raise RuntimeError('No .wpix captures found in ' + str(session))
    game_closed()
    pix = find_pix()
    output_root = session / 'offline-E3'
    output_root.mkdir(exist_ok=True)
    lock = output_root / '.running'
    try:
        fd = os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
    except FileExistsError:
        raise RuntimeError('Another run may be active. If it exited, delete only ' + str(lock))
    os.close(fd)
    results = []
    overall = {'version': VERSION, 'pix': str(pix), 'session': str(session),
               'provenance': 'USER_CONFIRMED_PROXY_DISABLED_NATIVE; PT/RR filenames are not comparisons' if args.native_baseline else 'UNVERIFIED',
               'captures': results, 'semantic_requirements': 'NOT_YET_VERIFIED',
               'motion_limit': 'Independent captured frames; not continuous temporal or normal frame pacing evidence.'}
    manifest = output_root / 'results.json'
    try:
        print('Offline export only. Original .wpix files are read-only inputs.', flush=True)
        print('PIX: ' + str(pix), flush=True)
        write_json(manifest, overall)
        for number, capture in enumerate(captures, 1):
            print(f'[{number}/{len(captures)}] {capture.name}', flush=True)
            per_capture = output_root / capture.stem
            per_capture.mkdir(exist_ok=True)
            state_path = per_capture / 'result.json'
            previous = json.loads(state_path.read_text()) if state_path.exists() else {}
            if previous.get('output') and reusable(previous, capture, per_capture / previous['output']):
                print('  Existing export retained; skipping.', flush=True)
                results.append(previous)
                write_json(manifest, overall)
                continue
            # Each failed attempt gets a new output directory; never --force.
            stamp = datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S-%f')
            attempt = per_capture / ('attempt-' + stamp)
            attempt.mkdir()
            project = attempt / 'project'
            record = {'capture': fingerprint(capture), 'status': 'running', 'output': project.relative_to(per_capture).as_posix()}
            results.append(record)
            write_json(state_path, record)
            write_json(manifest, overall)
            try:
                required = max(8 * GIB, capture.stat().st_size * 4)
                if shutil.disk_usage(output_root).free < required:
                    raise RuntimeError(f'Need at least {required/GIB:.1f} GiB free before this export. Existing outputs retained.')
                command = [str(pix), 'open-capture', str(capture), 'export-to-cpp', str(project)]
                record['command'] = command
                record.update(run_logged(command, attempt / 'pix-export.log', project, args.timeout))
                record['inventory'] = inspect_export(project)
                record['status'] = ('exported_unvalidated' if record.get('returncode') == 0 and
                                    record['inventory']['structural_checks_passed'] else 'incomplete')
            except Exception as error:
                record.update(status='failed', error=str(error))
            write_json(state_path, record)
            write_json(manifest, overall)
            print('  ' + record['status'], flush=True)
            if record['status'] != 'exported_unvalidated':
                print('  Continuing with other captures; see retained log and result.json.', flush=True)
        overall['status'] = 'exports_finished_unvalidated' if all(r['status'] == 'exported_unvalidated' for r in results) else 'partial_or_failed'
        write_json(manifest, overall)
    finally:
        lock.unlink(missing_ok=True)
    archive = session.with_name(session.name + '-Offline-E3-Review.zip')
    print('Packing review bundle (bulk resources.bin stays local)...', flush=True)
    excluded = review_zip(archive, output_root, session)
    print('Upload: ' + str(archive), flush=True)
    print(f'{len(excluded)} bulk/oversize files retained locally. Keep offline-E3 and the .wpix files.', flush=True)
    print('This is extraction evidence, not a verified full-PT implementation.', flush=True)
    return 0 if overall['status'] == 'exports_finished_unvalidated' else 2

if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print('Stopped. Existing captures and partial exports retained.', flush=True)
        sys.exit(130)
    except Exception as error:
        print('Export failed: ' + str(error), file=sys.stderr, flush=True)
        sys.exit(1)
