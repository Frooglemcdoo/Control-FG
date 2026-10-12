import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).parent))
import export_saved as e

class OfflineTests(unittest.TestCase):
    def test_zero_exit_does_not_establish_complete_export(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)
            (p/'empty.cpp').write_text('// no DXR data')
            self.assertFalse(e.inspect_export(p)['structural_checks_passed'])

    def test_expected_dxr_structure(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)
            (p/'frame.cpp').write_text('obj->DispatchRays(&desc);\nobj->CreateStateObject(&s);\nD3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;\nobj->BuildRaytracingAccelerationStructure(&b);')
            (p/'resources.bin').write_bytes(b'opaque-payload')
            result = e.inspect_export(p)
            self.assertTrue(result['structural_checks_passed'])
            self.assertEqual(result['code_signal_counts']['dispatch_rays'], 1)
            self.assertIn('NOT VERIFIED', result['semantic_validation'])

    def test_timeout_preserves_partial_and_original(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)
            capture = p/'original.wpix'
            capture.write_bytes(b'original capture')
            result = e.run_logged([sys.executable, '-c', 'import time; print("started", flush=True); time.sleep(20)'], p/'worker.log', p, .4)
            self.assertIn('timed out', result['error'])
            self.assertEqual(capture.read_bytes(), b'original capture')
            self.assertIn('started', (p/'worker.log').read_text())

    def test_resume_rejects_missing_or_changed_files(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)
            capture = p/'capture.wpix'
            capture.write_bytes(b'original')
            output = p/'project';output.mkdir()
            target = output/'frame.cpp';target.write_bytes(b'abc')
            record = {'capture': e.fingerprint(capture), 'status': 'exported_unvalidated',
                      'inventory': {'files': [{'path': 'frame.cpp', 'bytes': 3}]}}
            self.assertTrue(e.reusable(record, capture, output))
            target.unlink()
            self.assertFalse(e.reusable(record, capture, output))
            target.write_bytes(b'abcd')
            self.assertFalse(e.reusable(record, capture, output))

    def test_bundle_excludes_bulk_and_declares_omissions(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d); output = p/'output';output.mkdir()
            (output/'frame.cpp').write_bytes(b'code')
            (output/'resources.bin').write_bytes(b'bulk')
            (output/'big.cpp').write_bytes(b'x'*100)
            (p/'capture.wpix').write_bytes(b'capture')
            archive = p/'review.zip'
            e.review_zip(archive, output, p, limit=20)
            with zipfile.ZipFile(archive) as z:
                self.assertIn('frame.cpp', z.namelist())
                self.assertNotIn('resources.bin', z.namelist())
                self.assertNotIn('capture.wpix', z.namelist())
                exclusions = json.loads(z.read('review-contents.json'))['excluded']
                self.assertEqual(len(exclusions), 2)
            self.assertEqual((output/'resources.bin').read_bytes(), b'bulk')

    def test_nonzero_worker_reported(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)
            result=e.run_logged([sys.executable, '-c', 'raise SystemExit(7)'],p/'worker.log',p,10)
            self.assertEqual(result['returncode'],7)

if __name__ == '__main__':
    unittest.main()
