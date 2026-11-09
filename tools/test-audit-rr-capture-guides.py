"""Exercise capture corruption and intentional ABI exceptions in the guide audit."""
import json
from pathlib import Path
import tempfile
import unittest
import numpy as np
from audit_rr_capture_guides import audit


class GuideAuditTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name)
        (self.path / 'metadata.json').write_text(json.dumps(dict(
            width=3, height=1, frame=1, rr_active=True, reset=0,
            gi_boundary={}, hit_distance_bound=False)))
        self.raw = np.array([[.25,.5,.75,-4], [.25,.5,.75,-2.5],
                             [-1,-1,-1,0]], dtype='<f2')
        self.write('raw-albedo.rgba16f', self.raw)
        self.write('diffuse.rgba16f', [[.25,.5,.75,1],[.1,.2,.3,1],[0,0,0,1]])
        self.write('specular.rgba16f', np.zeros((3,4)))
        self.write('normal-roughness.rgba16f', [[0,0,1,1]]*3)
        self.write('native-gbuffer1.rgba8', np.zeros((3,4)), 'u1')
        self.write('hair-responsivity.r16f', [0,-.375,0])
        self.write('pre-filter-gi.rgba16f', [[2,3,4,1]]*3)
        self.write('gi-at-rr-input.rgba16f', [[2,3,4,2]]*3)

    def write(self, name, data, dtype='<f2'):
        (self.path / name).write_bytes(np.asarray(data, dtype=dtype).tobytes())

    def test_partial_hair_and_alpha_are_not_false_mismatches(self):
        result = audit(self.path)
        self.assertEqual(result['exact_albedo_pixels'], 1)
        self.assertEqual(result['exact_albedo_mismatch_pixels'], 0)
        self.assertEqual(result['fallback_pixels'], 1)
        self.assertTrue(result['gi_rgb_equal'])
        self.assertTrue(result['gi_output_alpha_equals_red'])
        self.assertEqual(result['responsivity_nonzero_outside_valid_hair'], 0)

    def test_full_coverage_mismatch_is_reported(self):
        self.raw[0,0] = .5
        self.write('raw-albedo.rgba16f', self.raw)
        self.assertEqual(audit(self.path)['exact_albedo_mismatch_pixels'], 1)

    def test_responsivity_leak_is_reported(self):
        self.write('hair-responsivity.r16f', [-.5,-.375,0])
        self.assertEqual(audit(self.path)['responsivity_nonzero_outside_valid_hair'], 1)

    def test_truncated_buffer_is_rejected(self):
        self.write('normal-roughness.rgba16f', [0])
        with self.assertRaisesRegex(ValueError, 'size does not match'):
            audit(self.path)

    def test_nonfinite_guide_is_reported(self):
        self.write('specular.rgba16f', [[float('nan'),0,0,1]]*3)
        self.assertEqual(audit(self.path)['nonfinite']['specular_rgb'], 3)


if __name__ == '__main__':
    unittest.main()
