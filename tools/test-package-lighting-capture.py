"""Exercise incompressible large-file splitting, reconstruction and missing parts."""
import importlib.util
import os
from pathlib import Path
import tempfile

spec = importlib.util.spec_from_file_location('pack', Path(__file__).with_name('package-lighting-capture.py'))
pack = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pack)
with tempfile.TemporaryDirectory() as root:
    root = Path(root)
    source = root / 'Shadow test'
    (source / 'nested').mkdir(parents=True)
    (source / 'lighting.log').write_bytes(os.urandom(510_000))
    (source / 'nested' / 'image ü.png').write_bytes(os.urandom(30_000))
    (source / 'empty').touch()
    output = pack.package(source, 200_000)
    assert len(list(output.glob('capture-part-*.zip'))) >= 3
    assert all(p.stat().st_size < 200_000 for p in output.glob('*.zip'))
    restored = root / 'restored'
    pack.restore(output, restored)
    for p in source.rglob('*'):
        if p.is_file():
            assert p.read_bytes() == (restored / p.relative_to(source)).read_bytes()
    next(output.glob('capture-part-*.zip')).unlink()
    try:
        pack.restore(output, root / 'incomplete')
    except ValueError as error:
        assert 'Missing' in str(error)
    else:
        raise AssertionError('Missing part accepted')
    assert not (root / 'incomplete').exists()
print('PASS: size caps, random oversized log, Unicode paths, empty files, exact restoration, missing part rejection')
