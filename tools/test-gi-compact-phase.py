"""Receiver coverage, payload coexistence and pinned shader transformation checks."""
import argparse
import unittest
from pathlib import Path
from gi_compact_phase import compact_shared_hits, compact_raygen
from gi_single_producer import edit_export


class Contract(unittest.TestCase):
    def test_disjoint_coverage(self):
        for width in (2, 16, 1920, 2560, 3840):
            columns = [[2*x+p for x in range(width//2)] for p in (0, 1)]
            self.assertFalse(set(columns[0]) & set(columns[1]))
            self.assertEqual(sorted(columns[0]+columns[1]), list(range(width)))

    def test_payload_namespaces(self):
        for ray in range(64):
            for tag in (0, 0x80000000, 0xa0000000, 0xb0000000):
                value = tag | ray
                compact = value & 0xe0000000 == 0xa0000000
                self.assertEqual(compact, tag in (0xa0000000, 0xb0000000))
                decoded = value & (63 if compact else 0x7fffffff)
                self.assertEqual(decoded, ray)
            for mat in range(256):
                value = 0xc0000000 | (mat << 22) | (65535 << 6) | ray
                self.assertNotEqual(value & 0xe0000000, 0xa0000000)
                self.assertEqual(value & 63, ray)

    def test_shader_contract(self):
        source = Path(SOURCE).read_text(encoding='utf-8-sig')
        shared = compact_shared_hits(source)
        self.assertEqual(shared.count('%GIIsCompact ='), 2)
        for phase in (0, 1):
            result = compact_raygen(source, 0, phase)
            self.assertEqual(result.count('%GICompactIndex ='), 1)
            # Hit/miss exports in this library are excluded at pipeline linking;
            # only the separate shared library is allowed to replace them.
            def erase(body):
                return '<raygen>'
            self.assertEqual(edit_export(result, 'rayGenMain', erase),
                             edit_export(source, 'rayGenMain', erase))
            self.assertIn(f'or i32 %GICompactTwice, {phase}', result)
        with self.assertRaises(ValueError):
            compact_raygen(source, 1, 0)
        with self.assertRaises(ValueError):
            compact_raygen(source, 0, 2)
        # A mismatched source must fail closed rather than partially patching.
        with self.assertRaises(ValueError):
            compact_shared_hits(source.replace('i32 145, i8 0', 'i32 145, i8 2'))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', required=True)
    args, remaining = parser.parse_known_args()
    SOURCE = args.source
    unittest.main(argv=['test-gi-compact-phase'] + remaining)
