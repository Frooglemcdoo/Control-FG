"""Checks conservation separately from reflection-detail preservation.

This intentionally demonstrates that passing energy/edge tests is insufficient
to admit the offline spatial filter as a runtime quality fix.
"""
import importlib.util
from pathlib import Path
import numpy as np

spec = importlib.util.spec_from_file_location('probe', Path(__file__).with_name('analyze-rr-material-reuse.py'))
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)
n = np.zeros((17, 17, 3)); n[..., 2] = 1
r = np.full((17, 17), .3)
a = np.full((17, 17, 3), .2)
d = np.ones((17, 17))
for radius in (1, 2):
    guides = n, r, a, d
    x = np.ones_like(n)
    np.testing.assert_allclose(probe.reuse(x, guides, radius), x)
    x[:] = 0; x[8, 8] = [3, 2, 1]
    y = probe.reuse(x, guides, radius)
    np.testing.assert_allclose(y.sum((0, 1)), [3, 2, 1])
    assert y.min() >= 0
    np.testing.assert_allclose(y[8, 8], x[8, 8]/(2*radius+1)**2)
    # Geometry edge is protected, but a reflection edge on that same geometry
    # is not. Both are real image structure, not Monte Carlo outliers.
    x[:] = 0; x[:, :8] = 1
    edge_depth = d.copy(); edge_depth[:, 8:] = 2
    np.testing.assert_allclose(probe.reuse(x, (n, r, a, edge_depth), radius), x)
    reflected_edge = probe.reuse(x, guides, radius)
    assert reflected_edge[8, 7, 0] < 1 and reflected_edge[8, 8, 0] > 0
    # A narrow real reflected light is indistinguishable from an impulse
    # outlier to this guide-only filter, despite exact total-energy retention.
    x[:] = 0; x[:, 8] = 1
    y = probe.reuse(x, guides, radius)
    np.testing.assert_allclose(y[8, 8, 0], 1/(2*radius+1))
    print(f'Radius {radius}: PASS conservation, positivity, center retention, geometry edge; '
          f'real one-pixel reflection line retains {100*y[8,8,0]:.1f}% peak (quality limitation)')
