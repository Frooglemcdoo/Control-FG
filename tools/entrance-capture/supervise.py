"""Out-of-process progress monitor: remains responsive if the PIX native call blocks."""
import argparse
import datetime
import json
from pathlib import Path
import subprocess
import sys
import time
import zipfile
from capture import HERE, collect_logs, report, restore_proxy, write_json

def deadline(phase):
    return {'capturing': 180, 'detaching': 30}.get(phase)

def diagnostics(out):
    # Preserve complete existing diagnostics; do not include large raw captures.
    target = out.with_name(out.name + '-Diagnostics.zip')
    with zipfile.ZipFile(target, 'w', zipfile.ZIP_DEFLATED, allowZip64=True) as z:
        for p in out.iterdir():
            if p.is_file() and p.suffix.lower() in ('.json', '.txt'):
                z.write(p, p.name)
        logs = out / 'logs'
        if logs.exists():
            from capture import regular_files
            for p in regular_files(logs):
                z.write(p, str(p.relative_to(out)))
    return target

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--game-args', default='')
    args = parser.parse_args()
    started = time.time()
    out = HERE / ('Entrance-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
    child = subprocess.Popen([sys.executable, '-u', str(HERE / 'capture.py'), '--exe', str(args.exe),
                              '--game-args=' + args.game_args, '--out', str(out)])
    last_notice = 0
    timed_out = False
    state = {}
    try:
        while child.poll() is None:
            try:
                state = json.loads((out / 'progress.json').read_text(encoding='utf-8'))
            except (OSError, ValueError):
                state = {}
            limit = deadline(state.get('phase'))
            elapsed = max(0, time.time() - state.get('started', time.time()))
            if limit and time.monotonic() - last_notice >= 10:
                print(f"PIX {state['phase']}: {int(elapsed)}s elapsed / {limit}s limit", flush=True)
                last_notice = time.monotonic()
            if limit and elapsed >= limit:
                timed_out = True
                write_json(out / 'watchdog-timeout.json', {'state': state, 'elapsed_seconds': elapsed,
                    'reason': 'PIX operation did not return before the watchdog deadline.',
                    'action': 'Stop collector worker only. Game not forcibly terminated.',
                    'root_cause': 'unknown; timeout does not identify a renderer or driver fault'})
                print('PIX TIMEOUT. Stopping collector. Close Control manually if it remains frozen.', flush=True)
                child.terminate()
                try: child.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    child.kill(); child.wait(timeout=10)
                break
            time.sleep(.5)
    except KeyboardInterrupt:
        child.terminate()
        child.wait(timeout=10)
        if out.exists():
            write_json(out / 'interrupted.json', {'message': 'User interrupted collector.'})
    finally:
        try:
            state = json.loads((out / 'progress.json').read_text(encoding='utf-8'))
        except (OSError, ValueError):
            pass
        if state.get('baseline'):
            try: restore_proxy(args.exe.resolve().parent)
            except Exception as e:
                if out.exists(): write_json(out / 'restore-error.json', {'error': str(e)})
                print('Proxy restoration needs attention: ' + str(e), flush=True)
    if out.exists():
        try: collect_logs(args.exe.resolve(), out, started)
        except Exception as e: write_json(out / 'supervisor-log-error.json', {'error': str(e)})
        report(out)
        if timed_out or child.returncode != 0:
            if timed_out:
                session = out / ('baseline-session.json' if state.get('baseline') else 'session.json')
                if session.exists():
                    data = json.loads(session.read_text(encoding='utf-8'))
                    for item in data.get('captures', []):
                        if item['status'] == 'capturing':
                            item.update(status='timed_out', error='See watchdog-timeout.json')
                    write_json(session, data)
                    report(out)
            print('Send this diagnostic archive: ' + str(diagnostics(out)), flush=True)
    return 1 if timed_out or child.returncode != 0 else 0

if __name__ == '__main__':
    sys.exit(main())
