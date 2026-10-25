"""Collect two complete native lighting input/output pairs from one running game session."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shutil
import time


def read_pair(root, pid, ordinal, started, require_lighting=False, require_native=False):
    frames = []
    for folder in root.glob(f'rr-input-c1-{pid}-*'):
        metadata = folder / 'metadata.json'
        if not metadata.is_file() or metadata.stat().st_mtime < started:
            continue
        data = json.loads(metadata.read_text(encoding='utf-8-sig'))
        if data.get('pid') != pid or data.get('capture_ordinal') != ordinal:
            continue
        if data.get('schema') != 'control-rr-gi-r28':
            raise ValueError('Unsupported RR capture metadata schema')
        if not data.get('output_captured') or data.get('rr_result', 0) != 1:
            raise ValueError('Native output capture/evaluation did not complete')
        resources = data.get('resources', [])
        if not resources or len(resources) != data.get('buffers'):
            raise ValueError('Missing resource metadata')
        names = set()
        for resource in resources:
            name = resource['file']
            if not name or Path(name).name != name or '/' in name or '\\' in name or ':' in name or name in names:
                raise ValueError('Unsafe or duplicate capture filename')
            names.add(name)
            path = folder / name
            expected = resource['width'] * resource['height'] * resource['pixel_bytes']
            if not resource.get('copied') or expected <= 0 or not path.is_file() or path.is_symlink() or path.stat().st_size != expected:
                raise ValueError(f'Incomplete capture resource: {path.name}')
        native = data.get('evaluation_path') == 'native_sr'
        if require_native and (not native or data.get('rr_active') is not False):
            raise ValueError('Expected native lighting capture with RR inactive')
        output_role = 'native-sr-output.' if native else 'rr-output.'
        if not any(n.startswith('input-color.') for n in names) or not any(n.startswith(output_role) for n in names):
            raise ValueError('Scene input/evaluation output images missing')
        if require_lighting:
            if not data.get('lighting_buffers_captured') or any(
                not any(n.startswith(role + '.') for n in names)
                for role in ('lighting-diffuse', 'lighting-specular')):
                raise ValueError('R157 lighting accumulation buffers missing')
        frames.append((folder, data))
    frames.sort(key=lambda item: item[1]['pair_index'])
    if len(frames) < 2:
        return None
    if len(frames) != 2 or [f[1]['pair_index'] for f in frames] != [0, 1]:
        raise ValueError('Ambiguous RR pair; restart the game for a fresh session')
    first, second = [f[1] for f in frames]
    if second['frame'] != first['frame'] + 1 or second['previous_frame'] != first['frame'] or not second['consecutive']:
        raise ValueError('Captured frames were not consecutive; send the collected diagnostics')
    return frames


def collect_logs(root, local, pid, destination):
    for directory, pattern in [(root, f'probe-*-{pid}.log'), (local / 'Control-FG' / 'Diagnostics', f'Lighting-*-{pid}-*.log'), (Path(os.environ.get('TEMP', str(local))) / 'Control-FG' / 'Diagnostics', f'Lighting-*-{pid}-*.log')]:
        if not directory.exists():
            continue
        for source in directory.rglob(pattern):
            target = destination / 'logs' / source.name
            target.parent.mkdir(exist_ok=True)
            shutil.copy2(source, target)


def capture_status(root, pid):
    """Read only a bounded tail; never load a growing probe log in full."""
    logs = list(root.glob(f'probe-*-{pid}.log'))
    if not logs:
        return 'No probe log found for this game process.'
    latest = max(logs, key=lambda path: path.stat().st_mtime)
    with latest.open('rb') as stream:
        stream.seek(max(0, latest.stat().st_size - 256 * 1024))
        lines = stream.read().decode('utf-8', errors='replace').splitlines()
    events = [line for line in lines if 'RR_INPUT_CAPTURE_' in line]
    return events[-1] if events else 'No RR capture event found in the log tail; capture request detection is unconfirmed.'


def retain_metadata(root, pid, destination):
    """Keep incomplete attempts useful without claiming their raw data is valid."""
    for folder in root.glob(f'rr-input-c1-{pid}-*'):
        for name in ('metadata.json', 'metadata.pending'):
            source = folder / name
            if source.is_file():
                target = destination / 'attempt-metadata' / folder.name / name
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, target)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--diagnostics-only', action='store_true')
    parser.add_argument('--collect-existing', action='store_true')
    args = parser.parse_args()
    local = Path(os.environ['LOCALAPPDATA'])
    root = local / 'ControlFGProbe'
    started = 0 if args.collect_existing or args.diagnostics_only else time.time()
    destination = args.output / ('Native-Lighting-R157-' + time.strftime('%Y%m%d-%H%M%S') + '-' + str(args.pid) + '-' + str(time.time_ns() % 1000000000))
    destination.mkdir(parents=True, exist_ok=False)
    report = {'schema': 'control-rr-boundary-r157', 'pid': args.pid, 'started_unix': started, 'pairs': [], 'complete': False, 'diagnostics_only': args.diagnostics_only}
    print('Collecting the overlay native-lighting captures for this game session.', flush=True)
    print('Use CAPTURE LIGHT in the overlay at each position, then PACKAGE LIGHT.', flush=True)
    print('Progress and failures are recorded below; no sound or hotkey is required.', flush=True)
    try:
        if args.diagnostics_only:
            report['status'] = capture_status(root, args.pid)
            print(report['status'], flush=True)
        for ordinal, label in [(1, 'shadow-visible'), (2, 'shadow-faded')]:
            if args.diagnostics_only:
                break
            deadline = time.monotonic() + 180
            next_status = 0
            pair = None
            while time.monotonic() < deadline:
                pair = read_pair(root, args.pid, ordinal, started, require_lighting=True, require_native=True)
                if pair:
                    break
                if time.monotonic() >= next_status:
                    print(f'Pair {ordinal}: ' + capture_status(root, args.pid), flush=True)
                    next_status = time.monotonic() + 5
                if args.collect_existing:
                    break
                time.sleep(.5)
            if pair is None:
                raise TimeoutError(f'Pair {ordinal} is not complete. Read the overlay status; diagnostics are being packaged.')
            print(f'Pair {ordinal} validated. Copying images; this can take a moment...', flush=True)
            for folder, data in pair:
                target = destination / label / folder.name
                target.mkdir(parents=True)
                for name in ['metadata.json'] + [r['file'] for r in data['resources']]:
                    shutil.copy2(folder / name, target / name)
            report['pairs'].append({'label': label, 'frames': [data for _, data in pair]})
            print(f'Pair {ordinal} saved: {label}.', flush=True)
            # Overlay status is the completion notification.
        report['complete'] = not args.diagnostics_only
    except Exception as error:
        report['error'] = str(error)
        print('Capture incomplete: ' + str(error), flush=True)
    finally:
        report['last_capture_status'] = capture_status(root, args.pid)
        retain_metadata(root, args.pid, destination)
        collect_logs(root, local, args.pid, destination)
        (destination / 'boundary-report.json').write_text(json.dumps(report, indent=2))
    packager = Path(__file__).with_name('package-lighting-capture.py')
    spec = importlib.util.spec_from_file_location('packager', packager)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    output = module.package(destination)
    print(f'Send every ZIP in: {output}', flush=True)
    print('Original captures were retained. Include the video.', flush=True)
    if os.name == 'nt':
        os.startfile(output)
    return 0 if report['complete'] or args.diagnostics_only else 1


if __name__ == '__main__':
    raise SystemExit(main())
