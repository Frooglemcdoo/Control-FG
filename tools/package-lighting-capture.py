"""Package a completed capture as independently readable ZIPs below 200 MB.

Large files use numbered chunks; each ZIP includes reconstruction metadata.
Uses only Python's standard library and never modifies the source capture.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile
import zipfile

MAX_ZIP_BYTES = 200_000_000
BLOCK_BYTES = 1024 * 1024


def file_hash(stream):
    digest = hashlib.sha256()
    while block := stream.read(BLOCK_BYTES):
        digest.update(block)
    return digest.hexdigest()


def path_hash(path):
    with path.open('rb') as stream:
        return file_hash(stream)


def package(source, limit=MAX_ZIP_BYTES):
    source = Path(source).resolve(strict=True)
    if not source.is_dir():
        raise ValueError('Select the completed capture folder, not a ZIP.')
    files = sorted(p for p in source.rglob('*') if p.is_file())
    if not files:
        raise ValueError('The selected folder contains no files.')
    if any(p.is_symlink() or source not in p.resolve().parents for p in files):
        raise ValueError('Capture files must be inside the selected folder.')
    # Reserve space for compression expansion, ZIP headers and metadata.
    payload_limit = int(limit * 0.90)
    output = Path(tempfile.mkdtemp(prefix=source.name + '-SEND-', dir=source.parent))
    archive = None
    records = []
    budget = 0
    part = 0
    archive_path = None

    def finish():
        nonlocal archive
        if archive is None:
            return
        archive.writestr('capture-index.json', json.dumps({'version': 1, 'files': records}, ensure_ascii=False))
        archive.close()
        archive = None
        if archive_path.stat().st_size >= limit:
            raise ValueError('ZIP size check failed; originals are intact.')
        with zipfile.ZipFile(archive_path) as check:
            if check.testzip() is not None:
                raise ValueError('ZIP integrity check failed.')
        archive_path.rename(archive_path.with_suffix(''))

    try:
        for file in files:
            relative = file.relative_to(source).as_posix()
            if len(relative.encode('utf-8')) > 4096:
                raise ValueError('Capture path is too long.')
            with file.open('rb') as stream:
                before = os.fstat(stream.fileno())
                total = before.st_size
                offset = 0
                while offset < total or (total == 0 and offset == 0):
                    if archive is None or budget >= payload_limit or len(records) >= 128:
                        finish()
                        part += 1
                        archive_path = output / f'capture-part-{part:03d}.zip.pending'
                        archive = zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=6)
                        records = []
                        budget = 0
                    length = min(total - offset, payload_limit - budget)
                    # Stable internal paths avoid collisions with user filenames.
                    entry = f'data/{len(records):03d}'
                    digest = hashlib.sha256()
                    with archive.open(entry, 'w', force_zip64=True) as target:
                        remaining = length
                        while remaining:
                            block = stream.read(min(remaining, BLOCK_BYTES))
                            if not block:
                                raise ValueError(f'Capture file changed while reading: {relative}')
                            target.write(block)
                            digest.update(block)
                            remaining -= len(block)
                    records.append(dict(path=relative, entry=entry, offset=offset,
                                        size=length, total=total, sha256=digest.hexdigest()))
                    offset += length
                    budget += length
                    if total == 0:
                        break
                after = os.fstat(stream.fileno())
                if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
                    raise ValueError(f'Capture file changed while reading: {relative}')
        finish()
        inventory = {p.name: path_hash(p)
                     for p in sorted(output.glob('capture-part-*.zip'))}
        with zipfile.ZipFile(output / 'capture-manifest.zip', 'w', zipfile.ZIP_DEFLATED) as index:
            index.writestr('parts.json', json.dumps(inventory))
        if (output / 'capture-manifest.zip').stat().st_size >= limit:
            raise ValueError('Package inventory exceeds the upload limit.')
        return output
    except BaseException:
        if archive is not None:
            archive.close()
        (output / 'INCOMPLETE-DO-NOT-SEND.txt').write_text('Packaging failed. Originals are intact. Run again on a completed capture.\n')
        raise


def restore(source, destination):
    """Reassemble verified chunks into a new folder; reject missing/duplicate parts."""
    source, destination = Path(source), Path(destination)
    if (source / 'INCOMPLETE-DO-NOT-SEND.txt').exists():
        raise ValueError('This capture package is incomplete.')
    with zipfile.ZipFile(source / 'capture-manifest.zip') as index:
        inventory = json.loads(index.read('parts.json'))
    actual = {p.name for p in source.glob('capture-part-*.zip')}
    if actual != set(inventory):
        raise ValueError('Missing or unexpected ZIP parts.')
    for name, expected in inventory.items():
        with (source / name).open('rb') as stream:
            if file_hash(stream) != expected:
                raise ValueError(f'ZIP integrity failure: {name}')
    groups = {}
    for part in sorted(source.glob('capture-part-*.zip')):
        with zipfile.ZipFile(part) as archive:
            index = json.loads(archive.read('capture-index.json'))
            if index['version'] != 1:
                raise ValueError('Unknown capture package version.')
            for record in index['files']:
                groups.setdefault(record['path'], []).append((part, record))
    if not groups:
        raise ValueError('No capture parts found.')
    for name, chunks in groups.items():
        path = Path(name)
        if path.is_absolute() or '..' in path.parts or '\\' in name or ':' in name:
            raise ValueError('Unsafe capture filename.')
        chunks.sort(key=lambda pair: pair[1]['offset'])
        expected = 0
        total = chunks[0][1]['total']
        for _, record in chunks:
            if record['offset'] != expected or record['total'] != total:
                raise ValueError(f'Missing or duplicate chunk: {name}')
            expected += record['size']
        if expected != total:
            raise ValueError(f'Missing final chunk: {name}')
    destination.mkdir(parents=True, exist_ok=False)
    for name, chunks in groups.items():
        target = destination / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as result:
            for part, record in chunks:
                digest = hashlib.sha256()
                size = 0
                with zipfile.ZipFile(part) as archive, archive.open(record['entry']) as chunk:
                    while block := chunk.read(BLOCK_BYTES):
                        result.write(block)
                        digest.update(block)
                        size += len(block)
                if size != record['size'] or digest.hexdigest() != record['sha256']:
                    raise ValueError(f'Chunk integrity failure: {name}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder', nargs='?')
    parser.add_argument('--restore-to', type=Path)
    args = parser.parse_args()
    folder = args.folder or input('Paste the completed capture folder path: ').strip().strip('"')
    if args.restore_to:
        restore(folder, args.restore_to)
        print(f'Restored to: {args.restore_to}')
    else:
        output = package(folder)
        print(f'Verified. Send ALL ZIP files in:\n{output}\nEvery ZIP is below 200 MB. Originals retained.')


if __name__ == '__main__':
    main()
