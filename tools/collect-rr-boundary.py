"""Collect two complete RR input/output pairs from one running game session."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shutil
import time


def read_pair(root, pid, ordinal, started):
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
            raise ValueError('RR output capture/evaluation did not complete')
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
        if not any(n.startswith('input-color.') for n in names) or not any(n.startswith('rr-output.') for n in names):
            raise ValueError('RR input/output images missing')
        frames.append((folder, data))
    frames.sort(key=lambda item: item[1]['pair_index'])
    if len(frames) < 2:
        return None
    if len(frames) != 2 or [f[1]['pair_index'] for f in frames] != [0, 1]:
        raise ValueError('Ambiguous RR pair; restart the game for a fresh session')
    first, second = [f[1] for f in frames]
    if second['frame'] != first['frame'] + 1 or second['previous_frame'] != first['frame'] or not second['consecutive']:
        raise ValueError('RR frames were not consecutive; send the collected diagnostics')
    return frames


def collect_logs(root, local, pid, destination):
    for directory, pattern in [(root, f'probe-*-{pid}.log'), (local / 'Control-FG' / 'Diagnostics', f'Lighting-*-{pid}-*.log'), (Path(os.environ.get('TEMP', str(local))) / 'Control-FG' / 'Diagnostics', f'Lighting-*-{pid}-*.log')]:
        if not directory.exists():
            continue
        for source in directory.rglob(pattern):
            target = destination / 'logs' / source.name
            target.parent.mkdir(exist_ok=True)
            shutil.copy2(source, target)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    local = Path(os.environ['LOCALAPPDATA'])
    root = local / 'ControlFGProbe'
    started = time.time()
    destination = args.output / ('RR-Boundary-R151-' + time.strftime('%Y%m%d-%H%M%S') + '-' + str(args.pid))
    destination.mkdir(parents=True, exist_ok=False)
    report = {'schema': 'control-rr-boundary-r151', 'pid': args.pid, 'started_unix': started, 'pairs': [], 'complete': False}
    print('Keep RR ON, DEFAULT selected and the graphics settings unchanged.', flush=True)
    print('1. Return to gameplay where the shadow is visible. Close F10.', flush=True)
    print('   Press Ctrl+Alt+F9 once and hold the camera still until the beep.', flush=True)
    print('2. Move to where the shadow fades, then press Ctrl+Alt+F9 again.', flush=True)
    print('   Hold still until the second beep. Record video of both positions.', flush=True)
    print('This captures RR input/output, not final tone-mapped display color.', flush=True)
    try:
        for ordinal, label in [(1, 'shadow-visible'), (2, 'shadow-faded')]:
            deadline = time.monotonic() + 180
            pair = None
            while time.monotonic() < deadline:
                pair = read_pair(root, args.pid, ordinal, started)
                if pair:
                    break
                time.sleep(.5)
            if pair is None:
                raise TimeoutError(f'No complete pair {ordinal}. Keep the diagnostics; do not repeatedly press the shortcut.')
            for folder, data in pair:
                target = destination / label / folder.name
                target.mkdir(parents=True)
                for name in ['metadata.json'] + [r['file'] for r in data['resources']]:
                    shutil.copy2(folder / name, target / name)
            report['pairs'].append({'label': label, 'frames': [data for _, data in pair]})
            print(f'Pair {ordinal} saved: {label}.', flush=True)
            if os.name == 'nt':
                import winsound
                winsound.MessageBeep()
        report['complete'] = True
    except Exception as error:
        report['error'] = str(error)
        print('Capture incomplete: ' + str(error), flush=True)
    finally:
        collect_logs(root, local, args.pid, destination)
        (destination / 'boundary-report.json').write_text(json.dumps(report, indent=2))
    packager = Path(__file__).with_name('package-lighting-capture.py')
    spec = importlib.util.spec_from_file_location('packager', packager)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    output = module.package(destination)
    print(f'Send every ZIP in: {output}', flush=True)
    print('Original captures were retained. Include the video.', flush=True)
    return 0 if report['complete'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
