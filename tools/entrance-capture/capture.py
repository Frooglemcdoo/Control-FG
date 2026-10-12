"""Control entrance acquisition. PIX API calls adapted from Microsoft's MIT samples."""
import argparse
from contextlib import contextmanager
import ctypes
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile
import time
import traceback
import zipfile

HERE = Path(__file__).resolve().parent
POSES = [
    ('01-overview', 'Entrance overview: PT ON, glossy glass, RR ON; stand still.'),
    ('02-floor', 'Look across the reflective floor toward the entrance lighting.'),
    ('03-glass', 'Frame the glass and the wall reflected in it.'),
    ('04-reverse', 'Turn around to include the surfaces/lights behind the first view.'),
    ('05-rr-off', 'Return to overview; disable RR using your normal setting. Keep RT on.'),
    ('06-composite-off', 'Same overview; RR ON, PT OFF. This is composite bypass, NOT native RT.'),
]
BASELINE_POSES = [
    ('07-baseline-overview', 'Proxy-disabled baseline: entrance overview; native RT ON, stand still.'),
    ('08-baseline-floor', 'Proxy-disabled baseline: match the earlier reflective floor view.'),
]
KNOWN_PROXIES = {
    '194abc42050b14041df4fc4fc68b5c6f26e52fd582285080aff860e6d9d5a4a3',
    'e677ae2ddf0933bbe1aa292f52f3a917b853d904ccc34a24d82dc8cf4811c48c',
}

def restore_proxy(game_dir):
    proxy = game_dir / 'dxgi.dll'
    backup = game_dir / 'dxgi.dll.entrance-e1-backup'
    if not backup.exists():
        return
    if sha256(backup) not in KNOWN_PROXIES:
        raise RuntimeError('Backup hash is not a known R1/R2 proxy. Refusing to move it.')
    if proxy.exists():
        raise RuntimeError('Both dxgi.dll and its capture backup exist. Refusing to overwrite either.')
    backup.rename(proxy)

@contextmanager
def disabled_proxy(game_dir):
    proxy = game_dir / 'dxgi.dll'
    backup = game_dir / 'dxgi.dll.entrance-e1-backup'
    if backup.exists():
        raise RuntimeError('A previous capture backup exists. Run --restore first; see README.')
    if not proxy.exists() or sha256(proxy) not in KNOWN_PROXIES:
        raise RuntimeError('Baseline requires the known R1/R2 Control-FG dxgi.dll. Unknown proxy left untouched.')
    proxy.rename(backup)
    try:
        yield
    finally:
        restore_proxy(game_dir)

def ensure_game_closed():
    r = subprocess.run(['tasklist.exe', '/FI', 'IMAGENAME eq Control_DX12.exe', '/FO', 'CSV', '/NH'],
                       capture_output=True, text=True, timeout=20)
    if r.returncode or 'control_dx12.exe' in r.stdout.lower():
        raise RuntimeError('Control is still running (or its exit could not be verified). Exit it first.')

def write_json(path, value):
    path = Path(path)
    tmp = path.with_suffix(path.suffix + '.tmp')
    tmp.write_text(json.dumps(value, indent=2, ensure_ascii=False), encoding='utf-8')
    tmp.replace(path)

def sha256(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda: f.read(8 * 1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()

def require_space(path, required):
    if shutil.disk_usage(path).free < required:
        raise RuntimeError(f'Insufficient free space at {path}: need {required / 2**30:.1f} GiB free.')

def regular_files(root):
    # Never follow junctions/symlinks into unrelated directories.
    for parent, dirs, files in os.walk(root, followlinks=False):
        def safe(p):
            return not (p.is_symlink() or getattr(p.lstat(), 'st_file_attributes', 0) & 0x400)
        dirs[:] = [d for d in dirs if safe(Path(parent) / d)]
        for name in files:
            p = Path(parent) / name
            if safe(p) and p.is_file():
                yield p

def pack(folder):
    folder = Path(folder).resolve()
    files = list(regular_files(folder))
    inventory = [{'path': str(p.relative_to(folder)), 'bytes': p.stat().st_size,
                  'sha256': sha256(p)} for p in files if p.name != 'checksums.json']
    write_json(folder / 'checksums.json', inventory)
    required = sum(p.stat().st_size for p in regular_files(folder)) + 64 * 1024**2
    require_space(folder.parent, required)
    target = folder.with_name(folder.name + '.zip')
    partial = target.with_suffix('.zip.partial')
    # ZIP64 streaming: no 2/4 GiB PowerShell Compress-Archive limit, no truncation.
    with zipfile.ZipFile(partial, 'w', zipfile.ZIP_STORED, allowZip64=True) as z:
        for p in regular_files(folder):
            z.write(p, str(p.relative_to(folder)))
    partial.replace(target)
    return target

def initialize():
    from _pix_bootstrap import find_pix_bin_directory, initialize_pythonnet
    pix_dir = find_pix_bin_directory('PixApiCsExt.dll')
    if not pix_dir:
        raise RuntimeError('Install Microsoft PIX Preview with PixApiCsExt.dll; see README. Set PIX_DIR for a custom install.')
    # Retain the DLL-directory handle for the entire process lifetime.
    global DLL_HANDLE
    DLL_HANDLE = os.add_dll_directory(pix_dir)
    initialize_pythonnet(pix_dir, 'PixApiCsExt.dll')
    return pix_dir

def wait_key(pid):
    import winsound
    u = ctypes.windll.user32
    u.GetForegroundWindow.restype = ctypes.c_void_p
    u.GetWindowThreadProcessId.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ulong)]
    # Capture only while the actual launched game is foreground, never a menu in this console.
    while u.GetAsyncKeyState(0x77) & 0x8000:
        time.sleep(.05)
    print('Return to the game. F8 = capture this view; F9 = skip (reported missing).', flush=True)
    kernel = ctypes.windll.kernel32
    kernel.OpenProcess.restype = ctypes.c_void_p
    kernel.OpenProcess.argtypes = [ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong]
    kernel.WaitForSingleObject.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
    kernel.CloseHandle.argtypes = [ctypes.c_void_p]
    handle = kernel.OpenProcess(0x00100000, False, pid)
    if not handle:
        raise RuntimeError('Cannot watch the launched game process.')
    try:
        while True:
            if kernel.WaitForSingleObject(handle, 0) != 258:
                raise RuntimeError('The launched game exited before the capture hotkey.')
            foreground_pid = ctypes.c_ulong()
            u.GetWindowThreadProcessId(u.GetForegroundWindow(), ctypes.byref(foreground_pid))
            if foreground_pid.value == pid:
                if u.GetAsyncKeyState(0x78) & 0x8000:
                    return False
                if u.GetAsyncKeyState(0x77) & 0x8000:
                    winsound.Beep(900, 100)
                    return True
            time.sleep(.05)
    finally:
        kernel.CloseHandle(handle)

def environment_snapshot(exe, out):
    result = {'os': platform.platform(), 'python': sys.version, 'exe': str(exe),
              'files': [], 'errors': [], 'env': {k: v for k, v in os.environ.items()
              if k.startswith('CONTROLFG_')}}
    for p in regular_files(exe.parent):
        if (p.parent == exe.parent and p.suffix.lower() in ('.dll', '.exe', '.dxil', '.dxbc')) or (
            'ControlFGStreamline' in p.parts and p.suffix.lower() in ('.dll', '.dxil', '.dxbc')):
            try:
                result['files'].append({'file': str(p.relative_to(exe.parent)),
                                        'bytes': p.stat().st_size, 'sha256': sha256(p)})
            except OSError as e:
                result['errors'].append(str(e))
    try:
        r = subprocess.run(['nvidia-smi', '--query-gpu=name,driver_version,memory.total',
                            '--format=csv'], capture_output=True, text=True, timeout=20)
        result['gpu'] = r.stdout
        result['gpu_query_error'] = r.stderr if r.returncode else None
    except Exception as e:
        result['gpu_query_error'] = str(e)
    write_json(out / 'environment.json', result)

def collect_logs(exe, out, started):
    errors = []
    roots = [Path(os.environ['LOCALAPPDATA']) / 'ControlFGProbe',
             Path(os.environ['LOCALAPPDATA']) / 'ControlFG', exe.parent / 'ControlFGStreamline']
    for i, root in enumerate(roots):
        if not root.exists():
            continue
        for p in regular_files(root):
            if p.suffix.lower() not in ('.log', '.ini', '.json'):
                continue
            if p.suffix.lower() == '.log' and p.stat().st_mtime < started:
                continue
            target = out / 'logs' / str(i) / p.relative_to(root)
            try:
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(p, target)
            except Exception as e:
                errors.append({'file': str(p), 'error': str(e)})
    write_json(out / 'log-copy-errors.json', errors)

def progress(out, phase, **details):
    write_json(out / 'progress.json', dict(phase=phase, started=time.time(), **details))

def capture_filename(value):
    if value is None or not str(value).strip():
        raise RuntimeError('PIX returned an empty capture filename. No capture was saved; session stopped.')
    source = Path(str(value))
    if not source.is_file() or source.stat().st_size == 0:
        raise RuntimeError('PIX returned a missing or empty capture file: ' + str(source))
    return source

def result_diagnostics(result, interface):
    # Enumerate the installed interface rather than guessing version-specific API names.
    import clr
    details = {'interface': str(interface), 'methods': [], 'values': {}}
    for method in clr.GetClrType(interface).GetMethods():
        details['methods'].append(str(method))
        name = str(method.Name)
        if name.startswith('Get') and len(method.GetParameters()) == 0 and not method.IsGenericMethod:
            try:
                value = getattr(result, name)()
                details['values'][name] = None if value is None else str(value)
            except Exception as error:
                details['values'][name] = {'error': str(error), 'hresult': getattr(error, 'HResult', None)}
    return details

def launch_capture(exe, out, game_args, baseline=False):
    import Microsoft.PIX as pix
    import Microsoft.PIX.Extension as ext
    import Microsoft.PIX.Extension.DeviceConnection as dc
    import Windows.Win32.Foundation as wf
    factory = ext.PixApiExtensions.PixCreateFactory[pix.IPixFactory]()
    notifications = dc.PixConnectionNotifications()
    conn = ext.PixApiExtensions.OpenConnectionDocument[pix.IPixConnectionDocument](
        factory, dc.PIX_CONNECTION_DESC.CreateLocal(), notifications)
    session_file = out / ('baseline-session.json' if baseline else 'session.json')
    poses = BASELINE_POSES if baseline else POSES
    manifest = {'schema': 1, 'scene': 'main entrance', 'captures': [],
                'native_baseline': 'NOT_CAPTURED: current PT OFF is only composite bypass',
                'settings_verification': 'User-directed poses/settings; inspect captures to verify.',
                'semantic_requirements': 'NOT_YET_VERIFIED; see requirements.json'}
    if baseline:
        manifest['native_baseline'] = 'Known Control-FG proxy disabled for this launch; other installed game modifications are not removed.'
    try:
        desc = dc.PIX_LAUNCH_PROCESS_DESC()
        desc.launchFlags = pix.PIX_APPLICATION_LAUNCH_FLAGS.PIX_APPLICATION_LAUNCH_FLAG_UNDER_GPU_CAPTURE
        desc.launchMode = pix.PIX_APPLICATION_LAUNCH_MODE.PIX_APPLICATION_LAUNCH_MODE_WIN32_EXECUTABLE
        info = dc.PIX_LAUNCH_PROCESS_DESC._launchInfo_e__UnionExt._win32_e__StructExt()
        info.exePath, info.commandLineArgs, info.initialWorkingDirectory = str(exe), game_args, str(exe.parent)
        union = dc.PIX_LAUNCH_PROCESS_DESC._launchInfo_e__UnionExt()
        union.win32 = info
        desc.launchInfo = union
        launched, _ = dc.PixApiExtensionsDeviceConnection.LaunchProcess[pix.IPixLaunchProcessResults](conn, desc)
        reason = launched.GetUnsupportedReason()
        if reason not in (pix.PIX_PROCESS_UNSUPPORTED_REASON.PIX_PROCESS_UNSUPPORTED_REASON_NONE,
                          pix.PIX_PROCESS_UNSUPPORTED_REASON.PIX_PROCESS_UNSUPPORTED_REASON_NOT_USING_D3D12):
            raise RuntimeError(f'PIX cannot capture the launched process: {reason}')
        pid = int(launched.GetProcessId())
        manifest['pid'] = pid
        print(('BASELINE: Control-FG proxy is disabled for this launch. Native RT ON.\n' if baseline else
               'Load the main entrance. Disable frame generation. Native RT ON; PT and RR ON.\n') +
              'Use borderless mode. Between captures Alt-Tab here for the next pose.', flush=True)
        for name, instruction in poses:
            item = {'name': name, 'requested_pose': instruction, 'status': 'pending'}
            manifest['captures'].append(item)
            write_json(session_file, manifest)
            print('\n' + instruction, flush=True)
            progress(out, 'waiting', capture=name, baseline=baseline)
            input('Press Enter to arm, then return to the game. ')
            if not wait_key(pid):
                item['status'] = 'skipped'
                write_json(session_file, manifest)
                continue
            try:
                require_space(out, 8 * 1024**3)
                require_space(tempfile.gettempdir(), 8 * 1024**3)
                item.update(status='capturing', requested_utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
                write_json(session_file, manifest)
                progress(out, 'capturing', capture=name, pid=pid, baseline=baseline)
                print(f'CAPTURE STARTED: {name}. Waiting for PIX; watchdog limit 180 seconds.', flush=True)
                result = dc.PixApiExtensionsDeviceConnection.TakeGpuCaptureResult(conn, pid)
                print('PIX request returned. Reading capture result...', flush=True)
                raw_filename = result.GetFilename()
                item['pix_original'] = None if raw_filename is None else str(raw_filename)
                write_json(session_file, manifest)
                try:
                    write_json(out / (name + '-pix-result.json'), result_diagnostics(result, pix.IPixGpuCaptureResult))
                except Exception as diagnostic_error:
                    write_json(out / (name + '-pix-result.json'), {'diagnostic_error': str(diagnostic_error)})
                source = capture_filename(raw_filename)
                progress(out, 'saving', capture=name, baseline=baseline)
                require_space(out, source.stat().st_size + 1024**3)
                target = out / (name + '.wpix')
                shutil.copy2(source, target)  # Original kept even if copy/export fails.
                item.update(status='captured', file=target.name, bytes=target.stat().st_size,
                            sha256=sha256(target), utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
                print(f'Saved {target.name}. Alt-Tab to the console.', flush=True)
                import winsound
                winsound.Beep(1200, 200)
            except Exception as e:
                item.update(status='failed', error=str(e), hresult=getattr(e, 'HResult', None),
                            traceback=traceback.format_exc())
                write_json(session_file, manifest)
                progress(out, 'failed', capture=name, baseline=baseline)
                print('CAPTURE FAILED; stopping this session: ' + str(e), flush=True)
                raise
            write_json(session_file, manifest)
    finally:
        write_json(session_file, manifest)
        progress(out, 'detaching', baseline=baseline)
        try:
            conn.DetachFromAllProcesses(wf.BOOL(True))
        except Exception:
            (out / 'detach-error.txt').write_text(traceback.format_exc(), encoding='utf-8')
        finally:
            progress(out, 'idle', baseline=baseline)

def analyze_all(out):
    results = []
    for capture in sorted(out.glob('*.wpix')):
        export = out / (capture.stem + '-analysis')
        export.mkdir(exist_ok=True)
        with (export / 'worker.log').open('w', encoding='utf-8') as log:
            try:
                r = subprocess.run([sys.executable, str(HERE / 'export.py'), str(capture), str(export)],
                                   stdout=log, stderr=subprocess.STDOUT, timeout=1800)
                results.append({'file': capture.name, 'returncode': r.returncode})
            except subprocess.TimeoutExpired:
                results.append({'file': capture.name, 'error': 'Export timeout; raw capture retained.'})
    write_json(out / 'analysis-results.json', results)
    report(out)

def report(out):
    session = out / 'session.json'
    data = json.loads(session.read_text(encoding='utf-8')) if session.exists() else {'captures': []}
    baseline_file = out / 'baseline-session.json'
    if baseline_file.exists():
        data['captures'] += json.loads(baseline_file.read_text(encoding='utf-8'))['captures']
    lines = ['CONTROL MAIN ENTRANCE E2 - ACQUISITION REPORT', '',
             'Raw .wpix files contain the GPU capture. JSON files index a subset of its contents.',
             'This report checks acquisition/export; it does not certify decoded PT requirements.', '']
    for name, _ in POSES + BASELINE_POSES:
        item = next((c for c in data['captures'] if c['name'] == name), {})
        path = out / (name + '.wpix')
        valid = path.is_file() and path.stat().st_size > 0
        lines.append(name + ': ' + ('CAPTURE PRESENT' if valid else 'MISSING') +
                     '; recorded status=' + item.get('status', 'not_attempted'))
        summary_path = out / (name + '-analysis') / 'summary.json'
        if summary_path.exists():
            s = json.loads(summary_path.read_text(encoding='utf-8'))
            lines.append('  replay=' + s['replay'] + '; indexed events=' + str(s['events']) +
                         '; complete view index=' + str(s['index_complete']))
        else:
            lines.append('  replay/export not completed')
    lines += ['', 'Baseline means the known Control-FG proxy was disabled, not a pristine game installation.',
              'NOT CERTIFIED: decoded light/material layouts,',
              'DXR shader-table interpretation, RR semantic inputs, continuous temporal behavior.',
              'Inspect requirements.json. Retain the raw captures for additional offline analysis.',
              'Do not delete them after sending the smaller JSON indexes.']
    (out / 'REPORT.txt').write_text('\n'.join(lines) + '\n', encoding='utf-8')

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', type=Path)
    parser.add_argument('--game-args', default='')
    parser.add_argument('--analyze', type=Path)
    parser.add_argument('--pack', type=Path)
    parser.add_argument('--out', type=Path)
    parser.add_argument('--restore', type=Path, help='Game directory; restore the known proxy after an interrupted baseline.')
    args = parser.parse_args()
    if args.restore:
        ensure_game_closed(); restore_proxy(args.restore.resolve()); print('Proxy restoration checked.'); return
    if args.pack:
        print(pack(args.pack)); return
    if args.analyze:
        ensure_game_closed(); initialize(); analyze_all(args.analyze.resolve()); return
    if not args.exe or not args.exe.is_file() or args.exe.name.lower() != 'control_dx12.exe':
        parser.error('--exe must point to Control_DX12.exe, not a store launcher.')
    if os.name != 'nt':
        raise RuntimeError('Acquisition requires Windows x64 and the game GPU.')
    pix_dir = initialize()
    import winreg
    try:
        ensure_game_closed()
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,
                r'SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock') as key:
            developer_mode = winreg.QueryValueEx(key, 'AllowDevelopmentWithoutDevLicense')[0] == 1
    except OSError:
        developer_mode = False
    if not developer_mode:
        raise RuntimeError('Enable Windows Developer Mode for PIX replay, then rerun. No game files have been changed.')
    out = (args.out or HERE / ('Entrance-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))).resolve()
    out.mkdir(parents=True, exist_ok=False)
    started = time.time()
    try:
        require_space(out, 40 * 1024**3)
        require_space(tempfile.gettempdir(), 8 * 1024**3)
        write_json(out / 'tool.json', {'version': 'E2', 'pix_dir': pix_dir})
        shutil.copy2(HERE / 'requirements.json', out / 'requirements.json')
        environment_snapshot(args.exe.resolve(), out)
        if (args.exe.parent / 'dxgi.dll.entrance-e1-backup').exists():
            raise RuntimeError('Previous baseline backup found. Run the README restoration command before capture.')
        launch_capture(args.exe.resolve(), out, args.game_args)
        input('EXIT the game, then press Enter for the two proxy-disabled baseline views. ')
        ensure_game_closed()
        try:
            with disabled_proxy(args.exe.resolve().parent):
                progress(out, 'starting_baseline', baseline=True)
                launch_capture(args.exe.resolve(), out, args.game_args, baseline=True)
                input('EXIT the baseline game, then press Enter to restore the mod and export. ')
                ensure_game_closed()
        except Exception:
            (out / 'baseline-error.txt').write_text(traceback.format_exc(), encoding='utf-8')
            print('Baseline failed; session stopped. See baseline-error.txt.', flush=True)
            raise
        ensure_game_closed()
        analyze_all(out)
    except BaseException:
        (out / 'failure.txt').write_text(traceback.format_exc(), encoding='utf-8')
        raise
    finally:
        try:
            collect_logs(args.exe.resolve(), out, started)
        except Exception as e:
            write_json(out / 'log-copy-errors.json', [{'error': str(e)}])
        report(out)
        print(f'All acquired data retained at: {out}', flush=True)
    print('Packaging full captures (can be many GB)...', flush=True)
    print('Send the resulting ZIP; if too large, send session.json and analysis folders first.\n' + str(pack(out)))

if __name__ == '__main__':
    main()
