import unittest
import numpy as np
from gi_excess_redistribution import redistribute, Settings


class RedistributionTests(unittest.TestCase):
    def scene(self, h=9, w=9):
        c = np.full((h, w, 3), .01)
        z = np.ones((h, w))
        n = np.zeros_like(c); n[..., 2] = 1
        m = np.ones((h, w), dtype=np.uint32)
        coverage = np.ones_like(m)
        return c, z, n, m, coverage

    def test_peak_and_rgb_conservation_at_image_corner(self):
        a = self.scene(); a[0][0, 0] = [2, 1, .5]
        out, count = redistribute(*a)
        self.assertEqual(count, 1)
        np.testing.assert_allclose(out.sum((0, 1)), a[0].sum((0, 1)), atol=1e-13)
        self.assertLess(out.max(), .3)
        np.testing.assert_array_equal(out[3:], a[0][3:])

    def test_each_boundary_blocks_transport(self):
        for guide in ('depth', 'normal', 'material', 'coverage'):
            a = self.scene(); a[0][4, 3] = [2, 1, .5]
            if guide == 'depth': a[1][:, 4:] = 2
            if guide == 'normal': a[2][:, 4:] = [1, 0, 0]
            if guide == 'material': a[3][:, 4:] = 2
            if guide == 'coverage': a[4][:, 4:] = 2
            out, _ = redistribute(*a)
            np.testing.assert_array_equal(out[:, 4:], a[0][:, 4:])
            np.testing.assert_allclose(out.sum((0, 1)), a[0].sum((0, 1)), atol=1e-13)

    def test_unknown_coverage_retains_sample(self):
        a = self.scene(); a[0][4, 4] = 2; a[4][4, 4] = 0
        out, count = redistribute(*a)
        self.assertEqual(count, 0)
        np.testing.assert_array_equal(out, a[0])

    def test_isolated_surface_keeps_its_light(self):
        a = self.scene(); a[0][4, 4] = 2; a[3][4, 4] = 2
        np.testing.assert_array_equal(redistribute(*a)[0], a[0])

    def test_flat_bright_plateau_unchanged(self):
        a = self.scene(); a[0][:] = [2, 1, .5]
        np.testing.assert_array_equal(redistribute(*a)[0], a[0])

    def test_invalid_sample_does_not_poison_neighbors(self):
        a = self.scene(); a[0][4, 4] = np.nan; a[0][4, 5] = 2
        out, _ = redistribute(*a)
        self.assertTrue(np.isnan(out[4, 4]).all())
        self.assertTrue(np.isfinite(out[np.isfinite(a[0]).all(-1)]).all())

    def test_asymmetric_donor_neighborhoods_conserve_rgb(self):
        a = self.scene(); a[1][:, 5:] = 1.009
        a[0][4, 4] = [2, 1, .5]; a[0][4, 5] = [.5, 2, 1]
        out, _ = redistribute(*a)
        np.testing.assert_allclose(out.sum((0, 1)), a[0].sum((0, 1)), atol=1e-13)
        self.assertTrue((out >= 0).all())

    def test_no_history_after_light_removal(self):
        a = self.scene(); a[0][4, 4] = 2
        redistribute(*a)
        a[0][:] = 0
        np.testing.assert_array_equal(redistribute(*a)[0], a[0])

    def test_grid_mismatch_rejected(self):
        a = list(self.scene()); a[1] = a[1][:-1]
        with self.assertRaises(ValueError): redistribute(*a)


if __name__ == '__main__': unittest.main()
