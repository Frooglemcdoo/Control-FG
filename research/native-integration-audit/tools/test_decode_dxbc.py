"""Regression cases for nonstandard-length CUSTOMDATA instruction blocks."""
import pathlib
import struct
import tempfile
import unittest
import decode_dxbc


def container(words):
    rdef = struct.pack('<6I', 0, 0, 0, 0, 0x501, 0)
    shex = struct.pack('<' + 'I' * (len(words) + 2), 0x50, len(words) + 2, *words)
    chunks = [b'RDEF' + struct.pack('<I', len(rdef)) + rdef,
              b'SHEX' + struct.pack('<I', len(shex)) + shex]
    first = 40
    return b'DXBC' + bytes(16) + struct.pack('<5I', 1, first + sum(map(len, chunks)), 2,
                                            first, first + len(chunks[0])) + b''.join(chunks)


class CustomDataTests(unittest.TestCase):
    def decode(self, words):
        with tempfile.TemporaryDirectory() as root:
            path = pathlib.Path(root) / 'fixture.dxbc'
            path.write_bytes(container(words))
            return decode_dxbc.decode(path)['instructions']

    def test_immediate_constants_preserve_raw_words_and_next_instruction(self):
        op = decode_dxbc.opnames.index('customdata') | (3 << 11)
        ret = decode_dxbc.opnames.index('ret') | (1 << 24)
        result = self.decode([op, 6, 0, 0xffffffff, 0x3f800000, 0x80000000, ret])
        self.assertEqual(result[0]['data_class'], 3)
        self.assertEqual(result[0]['data_words'], [0, 0xffffffff, 0x3f800000, 0x80000000])
        self.assertEqual((result[1]['dword'], result[1]['name']), (8, 'ret'))

    def test_rejects_missing_custom_header(self):
        with self.assertRaisesRegex(ValueError, 'Truncated custom-data header'):
            self.decode([decode_dxbc.opnames.index('customdata')])

    def test_rejects_out_of_bounds_custom_payload(self):
        with self.assertRaisesRegex(ValueError, 'Invalid custom-data length'):
            self.decode([decode_dxbc.opnames.index('customdata'), 40])

    def test_rejects_zero_length_standard_instruction(self):
        with self.assertRaisesRegex(ValueError, 'Invalid instruction length'):
            self.decode([decode_dxbc.opnames.index('ret')])


if __name__ == '__main__':
    unittest.main()
