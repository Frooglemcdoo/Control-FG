import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile
import capture

class AcquisitionTests(unittest.TestCase):
    def test_zip64_preserves_bytes_and_checksums(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / 'session'; root.mkdir()
            data = bytes(range(256)) * 10000
            (root / 'frame.wpix').write_bytes(data)
            # Force the >ZIP32 branch with a small fixture.
            with patch.object(zipfile, 'ZIP64_LIMIT', 1024):
                archive = capture.pack(root)
            with zipfile.ZipFile(archive) as z:
                self.assertEqual(z.read('frame.wpix'), data)
                self.assertIsNone(z.testzip())
                records = json.loads(z.read('checksums.json'))
                self.assertEqual(records[0]['sha256'], hashlib.sha256(data).hexdigest())

    def test_failed_pack_preserves_capture(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / 'session'; root.mkdir()
            (root / 'frame.wpix').write_bytes(b'capture')
            with patch.object(capture, 'require_space', side_effect=RuntimeError('disk full')):
                with self.assertRaises(RuntimeError): capture.pack(root)
            self.assertEqual((root / 'frame.wpix').read_bytes(), b'capture')
            self.assertFalse(root.with_suffix('.zip').exists())

    def test_baseline_restores_on_exception(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); proxy = root / 'dxgi.dll'; proxy.write_bytes(b'test proxy')
            with patch.object(capture, 'KNOWN_PROXIES', {capture.sha256(proxy)}):
                with self.assertRaisesRegex(RuntimeError, 'capture failed'):
                    with capture.disabled_proxy(root):
                        self.assertFalse(proxy.exists())
                        raise RuntimeError('capture failed')
                self.assertEqual(proxy.read_bytes(), b'test proxy')
                self.assertFalse((root / 'dxgi.dll.entrance-e1-backup').exists())

    def test_unknown_proxy_untouched(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); proxy = root / 'dxgi.dll'; proxy.write_bytes(b'other mod')
            with self.assertRaises(RuntimeError):
                with capture.disabled_proxy(root): self.fail('must not enter')
            self.assertEqual(proxy.read_bytes(), b'other mod')

    def test_restore_never_overwrites_new_proxy(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); proxy = root / 'dxgi.dll'; proxy.write_bytes(b'new')
            backup = root / 'dxgi.dll.entrance-e1-backup'; backup.write_bytes(b'old')
            with patch.object(capture, 'KNOWN_PROXIES', {capture.sha256(backup)}):
                with self.assertRaises(RuntimeError): capture.restore_proxy(root)
            self.assertEqual(proxy.read_bytes(), b'new')
            self.assertEqual(backup.read_bytes(), b'old')

    def test_missing_capture_report_is_not_success(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); capture.report(root)
            report = (root / 'REPORT.txt').read_text()
            self.assertEqual(report.count(': MISSING;'), 8)
            self.assertIn('NOT CERTIFIED', report)

    def test_reparse_symlink_exclusion(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / 'session'; root.mkdir()
            outside = Path(temp) / 'outside'; outside.write_bytes(b'unrelated')
            try: (root / 'link').symlink_to(outside)
            except OSError: self.skipTest('Symlink creation unavailable')
            self.assertEqual(list(capture.regular_files(root)), [])

if __name__ == '__main__':
    unittest.main()
