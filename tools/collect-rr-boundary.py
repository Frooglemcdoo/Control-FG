"""Collect two complete native lighting input/output pairs from one running game session."""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shutil
import time


def validate_gi_hits(folder, frame):
    metadata = folder / 'gi-hit-slices.json'
    binary = folder / 'gi-hit-slices.bin'
    if not metadata.is_file() or metadata.is_symlink() or not binary.is_file() or binary.is_symlink():
        raise ValueError('Missing GI hit-slice capture')
    hit = json.loads(metadata.read_text())
    if (hit.get('schema') != 'gi-hit-slices-r271' or hit.get('frame') != frame
            or hit.get('gpu_fence_completed') is not True or hit.get('all_batches_recorded') is not True):
        raise ValueError('Uncompleted or mismatched GI hit capture')
    count = hit.get('expected_batches', 0)
    if not 1 <= count <= 8 or hit.get('recorded_batches') != count or len(hit.get('batches', [])) != count:
        raise ValueError('Incomplete GI batch sequence')
    if not 1 <= hit.get('rays_per_batch', 0) <= 16:
        raise ValueError('Invalid GI ray count')
    cursor = 0
    for index, batch in enumerate(hit['batches']):
        if batch.get('batch') != index or len(batch.get('resources', [])) != 3:
            raise ValueError('Missing GI batch or hit resource')
        for kind, resource in zip(('material', 'normal_uvx', 'position_uvy'), batch['resources']):
            slices = resource.get('slices', 0)
            pitch = resource.get('row_pitch', 0)
            pixel = resource.get('pixel_bytes', 0)
            if (resource.get('kind') != kind or not hit['rays_per_batch'] <= slices <= 16
                    or pixel not in (2, 4, 8, 16) or pitch != 128 * pixel
                    or resource.get('slice_region_bytes') != pitch * 128 or resource.get('offset') != cursor):
                raise ValueError('Invalid GI physical-slice layout')
            cursor += slices * 2 * pitch * 128
    if not 0 < cursor <= 128 * 1024 * 1024 or hit.get('bytes') != cursor or binary.stat().st_size != cursor:
        raise ValueError('Incomplete GI physical-slice bytes')


def copy_capture_frame(folder, data, target):
    names = ['metadata.json'] + [r['file'] for r in data['resources']]
    if data.get('gi_hit_capture_requested'):
        validate_gi_hits(folder, data['frame'])
        names += ['gi-hit-slices.json', 'gi-hit-slices.bin']
    target.mkdir(parents=True)
    for name in names:
        shutil.copy2(folder / name, target / name)


def read_pair(root, pid, ordinal, started, require_lighting=False, require_native=False, require_rr=False):
    if require_native and require_rr:
        raise ValueError('Choose one evaluation path')
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
        if data.get('gi_hit_capture_requested'):
            validate_gi_hits(folder, data['frame'])
        native = data.get('evaluation_path') == 'native_sr'
        if require_native and (not native or data.get('rr_active') is not False):
            raise ValueError('Expected native lighting capture with RR inactive')
        if require_rr:
            if data.get('evaluation_path') != 'rr' or data.get('rr_active') is not True:
                raise ValueError('Expected an active RR material capture')
            if data.get('reset') != 0:
                raise ValueError('RR history was reset on the captured frame')
            guides = {'normal-roughness.rgba16f', 'diffuse.rgba16f', 'specular.rgba16f'}
            if not guides.issubset(names):
                raise ValueError('Actual RR material guides are missing')
            for resource in resources:
                if resource['file'] in guides and (
                    resource.get('dxgi_format') != 10 or resource['pixel_bytes'] != 8 or
                    resource['width'] != data.get('width') or resource['height'] != data.get('height')):
                    raise ValueError('RR guide format or extent mismatch')
            if not {'native-gbuffer1.rgba8', 'native-gbuffer2.rgba8', 'input-depth.r32f'}.issubset(names):
                raise ValueError('Native material/depth inputs are missing')
            if not any(n.startswith('input-motion.') for n in names):
                raise ValueError('Native motion input is missing')
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
    if first.get('lighting_mode') != second.get('lighting_mode'):
        raise ValueError('Lighting mode changed within a capture pair')
    if second['frame'] != first['frame'] + 1 or second['previous_frame'] != first['frame'] or not second['consecutive']:
        raise ValueError('Captured frames were not consecutive; send the collected diagnostics')
    return frames


def validate_gi_pair(pair, ordinal):
    expected = 'clamp_only' if ordinal == 1 else 'both'
    for _, data in pair:
        b = data.get('gi_boundary', {})
        if (b.get('valid') is not True or b.get('frame') != data['frame'] or
                b.get('applied') is not True or b.get('stage') != expected or
                b.get('view') != 'normal' or b.get('cleanup') is not (ordinal == 1) or b.get('filter_build') != 'R217' or
                not isinstance(b.get('filter_color_identity'), int) or b['filter_color_identity'] <= 0):
            raise ValueError('Missing, stale or wrong GI filter observation; send diagnostics')
        if (expected == 'clamp_only' and b.get('spatial_passes') != 0) or (expected == 'both' and b.get('spatial_passes', 0) <= 0):
            raise ValueError('GI spatial pass count does not match the requested comparison')
        resources = data['resources']
        if not any(r['file'] == 'raw-gi.rgba16f' for r in resources):
            raise ValueError('GI boundary texture missing')
        if any(not isinstance(r.get('resource_identity'), int) or r['resource_identity'] <= 0 for r in resources):
            raise ValueError('Captured resource identities missing')
        # No match is useful evidence too; do not assert that a label proves provenance.
        data['gi_filter_color_matches'] = [r['file'] for r in resources if r['resource_identity'] == b['filter_color_identity']]


def validate_gi_sequence_pair(pair, ordinal):
    if ordinal not in range(1, 5):
        raise ValueError('Invalid stationary sequence ordinal')
    for _, data in pair:
        b = data.get('gi_boundary', {})
        if (data.get('gi_sequence_build') != 'R263' or data.get('sequence_pairs') != 4 or
                data.get('capture_ordinal') != ordinal or
                data.get('pre_gi_captured') is not True or data.get('pre_gi_frame') != data['frame'] or
                not 0 < data.get('pre_gi_tick_ms', 0) <= data.get('evaluation_tick_ms', 0)):
            raise ValueError('Missing or stale early GI copy')
        if (b.get('valid') is not True or b.get('frame') != data['frame'] or
                b.get('applied') is not True or b.get('stage') != 'raw_bypass' or
                b.get('view') != 'normal' or b.get('cleanup') is not False or
                b.get('filter_build') != 'R263' or b.get('spatial_passes') != 0 or
                b.get('spatial_observed') != 0):
            raise ValueError('Raw GI bypass was not verified for this frame')
        if not data.get('projection_valid') or not data.get('motion_scale_valid'):
            raise ValueError('Projection or motion scale metadata missing')
        resources = {r['file']: r for r in data['resources']}
        for name in ('pre-filter-gi.rgba16f', 'gi-at-rr-input.rgba16f'):
            r = resources.get(name, {})
            if (r.get('dxgi_format') != 10 or r.get('pixel_bytes') != 8 or
                    r.get('width') != data.get('width') or r.get('height') != data.get('height') or
                    not isinstance(r.get('resource_identity'), int) or r['resource_identity'] <= 0):
                raise ValueError('Missing or invalid GI sequence texture')
        if resources['gi-at-rr-input.rgba16f']['resource_identity'] != b.get('filter_color_identity'):
            raise ValueError('GI at RR boundary does not match the filtered color resource')
    first, second = [data for _, data in pair]
    for key in ('preset', 'width', 'height', 'lighting_mode'):
        if first.get(key) != second.get(key):
            raise ValueError('Sequence configuration changed within a pair')


def collect_logs(root, local, pid, destination):
    for directory, pattern in [(root, f'probe-*-{pid}.log'), (local / 'Control-FG' / 'Diagnostics', f'GI-Joint-R28[123]-{pid}.bin'), (local / 'Control-FG' / 'Diagnostics', f'GI-Joint-R284-{pid}-frame[0-9][0-9].bin'), (local / 'Control-FG' / 'Diagnostics', f'GI-Joint-R284-{pid}-reference.bmp'), (local / 'Control-FG' / 'Diagnostics', f'GI-Joint-R285-{pid}-frame[0-9][0-9].bin'), (local / 'Control-FG' / 'Diagnostics', f'GI-Joint-R285-{pid}-frame[0-9][0-9].json'), (local / 'Control-FG' / 'Diagnostics', f'GI-Joint-R285-{pid}-reference.bmp'), (local / 'Control-FG' / 'Diagnostics', f'GI-Spot-R274-{pid}.bin'), (local / 'Control-FG' / 'Diagnostics', f'GI-Ray-R27[78]-{pid}.bin'), (local / 'Control-FG' / 'Diagnostics', f'Lighting-*-{pid}-*.log'), (Path(os.environ.get('TEMP', str(local))) / 'Control-FG' / 'Diagnostics', f'GI-Joint-R28[123]-{pid}.bin'), (Path(os.environ.get('TEMP', str(local))) / 'Control-FG' / 'Diagnostics', f'GI-Joint-R284-{pid}-frame[0-9][0-9].bin'), (Path(os.environ.get('TEMP', str(local))) / 'Control-FG' / 'Diagnostics', f'GI-Joint-R284-{pid}-reference.bmp'), (Path(os.environ.get('TEMP', str(local))) / 'Control-FG' / 'Diagnostics', f'GI-Joint-R285-{pid}-frame[0-9][0-9].bin'), (Path(os.environ.get('TEMP', str(local))) / 'Control-FG' / 'Diagnostics', f'GI-Joint-R285-{pid}-frame[0-9][0-9].json'), (Path(os.environ.get('TEMP', str(local))) / 'Control-FG' / 'Diagnostics', f'GI-Joint-R285-{pid}-reference.bmp'), (Path(os.environ.get('TEMP', str(local))) / 'Control-FG' / 'Diagnostics', f'GI-Spot-R274-{pid}.bin'), (Path(os.environ.get('TEMP', str(local))) / 'Control-FG' / 'Diagnostics', f'GI-Ray-R27[78]-{pid}.bin'), (Path(os.environ.get('TEMP', str(local))) / 'Control-FG' / 'Diagnostics', f'Lighting-*-{pid}-*.log')]:
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


def validate_stack_sources(local,pid,rr_frame):
    import struct
    roots={local/'Control-FG'/'Diagnostics',Path(os.environ.get('TEMP',str(local)))/'Control-FG'/'Diagnostics'}
    paths={p.name:p for root in roots for p in root.glob(f'GI-Joint-R285-{pid}-frame[0-9][0-9].json')}
    if len(paths)!=8:raise ValueError('Expected all eight GI source frames')
    records=[]
    for index,p in enumerate(sorted(paths.values()),1):
        m=json.loads(p.read_text());binary=p.with_suffix('.bin')
        if m.get('schema')!='gi-stack-r285' or m.get('pid')!=pid or m.get('frame_index')!=index or m.get('overflow') is not False:raise ValueError('Incomplete GI source metadata')
        with binary.open('rb') as f:count,magic,step,phase=struct.unpack('<4I',f.read(16))
        if not 0<count<=1048576 or count!=m['records'] or binary.stat().st_size!=16+256*count or magic!=0x285 or step!=m['step'] or phase!=m['phase_x']+(m['phase_y']<<16):raise ValueError('Incomplete or inconsistent GI source bytes')
        if index==1 and m['engine_frame']!=rr_frame:raise ValueError('GI source and RR buffers are not the same engine frame')
        if records and (m['present']<=records[-1]['present'] or (m['width'],m['height'])!=(records[0]['width'],records[0]['height'])):raise ValueError('GI source ordering or resolution mismatch')
        records.append(m)
    return dict(same_frame_verified=True,engine_frame=rr_frame,source_frames=records)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--diagnostics-only', action='store_true')
    parser.add_argument('--collect-existing', action='store_true')
    parser.add_argument('--require-rr', action='store_true', help='Require settled RR output and actual material guides')
    parser.add_argument('--gi-boundary', action='store_true')
    parser.add_argument('--gi-sequence', action='store_true')
    parser.add_argument('--gi-stack', action='store_true')
    args = parser.parse_args()
    if args.gi_boundary or args.gi_sequence or args.gi_stack:
        args.require_rr = True
    if args.gi_boundary and args.gi_sequence:
        parser.error("Choose one GI capture protocol")
    local = Path(os.environ['LOCALAPPDATA'])
    root = local / 'ControlFGProbe'
    started = 0 if args.collect_existing or args.diagnostics_only else time.time()
    prefix = 'RR-Materials-R180-' if args.require_rr else 'Native-Lighting-R157-'
    if args.gi_boundary:
        prefix = 'GI-Boundary-R217-'
    if args.gi_sequence:
        prefix = 'GI-Sequence-R263-'
    if args.gi_stack:prefix='GI-Stack-R285-'
    destination = args.output / (prefix + time.strftime('%Y%m%d-%H%M%S') + '-' + str(args.pid) + '-' + str(time.time_ns() % 1000000000))
    destination.mkdir(parents=True, exist_ok=False)
    report = {'schema': 'control-rr-boundary-r157', 'pid': args.pid, 'started_unix': started, 'pairs': [], 'complete': False, 'diagnostics_only': args.diagnostics_only}
    report['expected_path'] = 'rr' if args.require_rr else 'native_sr'
    print('Collecting the overlay input/output captures for this game session.', flush=True)
    print('Recovery collection only. Do not start another capture; existing files will be retained.' if args.diagnostics_only else 'Select GI RAW BYPASS, click CAPTURE RR once, hold still for all four pairs, then PACKAGE RR.' if args.gi_sequence else 'Use CAPTURE RR in CLAMP ONLY, then BOTH at the same camera position.' if args.gi_boundary else 'Use CAPTURE RR at the painting and elevator, then PACKAGE RR.' if args.require_rr else
          'Use CAPTURE LIGHT in the overlay at each position, then PACKAGE LIGHT.', flush=True)
    print('Progress and failures are recorded below; no sound or hotkey is required.', flush=True)
    try:
        if args.diagnostics_only:
            report['status'] = capture_status(root, args.pid)
            print(report['status'], flush=True)
        positions = [(1, 'painting'), (2, 'elevator')] if args.require_rr else [(1, 'before-room'), (2, 'inside-room')]
        if args.gi_boundary:
            positions = [(1, 'clamp-only'), (2, 'both')]
        if args.gi_sequence:
            positions = [(i, f'stationary-pair-{i}') for i in range(1, 5)]
            report['sequence_build'] = 'R263'
            report['inter_pair_gaps_expected'] = True
        if args.gi_stack:positions=[(1,'full-screen-rr')]
        for ordinal, label in positions:
            if args.diagnostics_only:
                break
            deadline = time.monotonic() + 180
            next_status = 0
            pair = None
            while time.monotonic() < deadline:
                pair = read_pair(root, args.pid, ordinal, started, require_lighting=True,
                                 require_native=not args.require_rr, require_rr=args.require_rr)
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
                copy_capture_frame(folder, data, target)
            if args.gi_boundary:
                validate_gi_pair(pair, ordinal)
            if args.gi_sequence or args.gi_stack:
                validate_gi_sequence_pair(pair, ordinal)
                if report['pairs']:
                    previous = report['pairs'][-1]['frames'][-1]
                    current = pair[0][1]
                    if current['frame'] <= previous['frame'] or any(current.get(k) != previous.get(k) for k in ('preset', 'width', 'height', 'lighting_mode')):
                        raise ValueError('Sequence configuration changed or frames are out of order')
            report['pairs'].append({'label': label, 'frames': [data for _, data in pair]})
            print(f'Pair {ordinal} saved: {label}.', flush=True)
            # Overlay status is the completion notification.
        if args.gi_stack:
            report['source_join']=validate_stack_sources(local,args.pid,report['pairs'][0]['frames'][0]['frame'])
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
