import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

spec = importlib.util.spec_from_file_location('analysis', Path(__file__).with_name('analyze-storage-latency.py'))
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


def row(qpc, n, mean, over=0):
    return f'qpc={qpc} DIRECTSTORAGE_R247_LATENCY stage=queue_to_submit_begin level=-1 samples={n} mean_us={mean} max_us=999999 ge16ms={over} ge100ms={over}\n'


class AnalysisTests(unittest.TestCase):
    def test_baseline_and_delta(self):
        text = row(1, 100, 100000, 90) + 'qpc=2 DIRECTSTORAGE_R247_STATUS present=600\n' + row(3, 110, 91000, 91)
        result = a.analyze(text)
        self.assertEqual(len(result['intervals']), 1)
        interval = result['intervals'][0]
        self.assertEqual((interval['samples'], interval['ge100ms'], interval['present']), (10, 1, 600))
        self.assertAlmostEqual(interval['mean_ms'], 1.0)
        self.assertAlmostEqual(interval['rounding_bound_ms'], .00105)

    def test_reset_and_unchanged(self):
        result = a.analyze(row(1, 100, 100000, 90) + row(2, 100, 100000, 90) + row(3, 2, 1000) + row(4, 3, 1000))
        self.assertEqual(len(result['intervals']), 1)
        self.assertEqual(result['intervals'][0]['samples'], 1)

    def test_latest_session_without_extracting(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'logs.zip'
            with zipfile.ZipFile(path, 'w') as z:
                z.writestr('probe\\probe-20261031-120000-000-1.log', 'older')
                z.writestr('probe/probe-20261031-130000-000-2.log', 'newer')
            name, text = a.read_session(path)
            self.assertEqual(text, 'newer')


if __name__ == '__main__':
    unittest.main()
