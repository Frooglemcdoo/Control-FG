import math
import struct
import unittest
from analyze_gi_spot_trace import analyze, parse

def fixture(mask=1):
    b = bytearray(272)
    struct.pack_into('<I', b, 0, 1)
    struct.pack_into('<4I', b, 16, 16, 16, 0, 6)
    struct.pack_into('<3f', b, 32, 0, .0001, 1)
    struct.pack_into('<I', b, 208, mask)
    return b

class TraceTests(unittest.TestCase):
    def test_invalid_stage_is_absent(self):
        b = fixture()
        struct.pack_into('<f', b, 80, math.nan)  # Unexecuted visibility storage.
        _, rows = parse(b)
        self.assertNotIn('visibility', rows[0])
        self.assertEqual(analyze(b)['full_stage_records'], 0)

    def test_nonfinite_valid_stage_rejected(self):
        b = fixture(3)
        struct.pack_into('<f', b, 80, math.nan)
        with self.assertRaisesRegex(ValueError, 'Nonfinite'):
            parse(b)

    def test_truncation_and_bad_sampling(self):
        with self.assertRaisesRegex(ValueError, 'Truncated'):
            parse(fixture()[:-1])
        b = fixture()
        struct.pack_into('<I', b, 16, 17)
        with self.assertRaisesRegex(ValueError, 'sampling'):
            parse(b)

    def test_shadow_early_exit_is_not_full_contribution(self):
        b = fixture(3)
        report = analyze(b)
        self.assertEqual(report['validity_counts'], {3: 1})
        self.assertIsNone(report['partial_term_concentration']['top10_share'])

    def test_known_attenuation_and_partial_term(self):
        b = fixture(16383)
        struct.pack_into('<4f', b, 64, 1, 1, 1 / math.pi, 1 / math.pi)
        struct.pack_into('<f', b, 112, .25)
        for off in (128, 144, 160):
            struct.pack_into('<3f', b, off, 1, 1, 1)
        report = analyze(b)
        self.assertLess(report['arithmetic']['attenuation_max_abs_error'], 1e-7)
        self.assertEqual(report['brightest_partial_terms'][0]['rgb'], [.25] * 3)

if __name__ == '__main__':
    unittest.main()
