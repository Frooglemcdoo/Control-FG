"""Offline R424 proposal screening. Does not render or simulate new visibility.

For a fixed proposal q and captured cosine proposal p, the second moment under q
can be estimated from p samples as mean(C**2 * p/q), where C=f/p is the original
sample contribution. The energy is unchanged analytically by f/q weighting;
reweighting the old pixels alone would NOT produce an unbiased new image.

Train on other spatial folds only. Geometry reconstructed from endpoint minus
origin is approximate. Near/invalid directions receive conservative p/q bounds.
No temporal, GPU-cost or scene-wide quality conclusion follows from this test.
"""
from pathlib import Path
import argparse
import json
import numpy as np

LUMA = np.array([.2126, .7152, .0722])
FRACTIONS = (0., .125, .25, .5)


def producer_units(x, y, frame, batch=0):
    """Single-ray native-budget Owned hash; checked against generated IR."""
    with np.errstate(over='ignore'):
        seed = (np.asarray(x, dtype='u4')*np.uint32(73856093)) ^ (np.asarray(y, dtype='u4')*np.uint32(19349663))
        seed ^= np.asarray(frame, dtype='u4')*np.uint32(83492791)
        seed ^= np.uint32(batch)*np.uint32(2654435761)
        result = []
        for salt in [0x68bc21eb, 0x02e5be93, 0x967a889b]:
            v = seed ^ np.uint32(salt)
            v ^= v >> 16; v *= np.uint32(0x7feb352d)
            v ^= v >> 15; v *= np.uint32(0x846ca68b); v ^= v >> 16
            result.append((v >> 8).astype(np.float64)*2**-24)
    return result


def producer_direction(normal, u, v):
    n = normal/np.linalg.norm(normal, axis=1)[:, None]
    sign = np.where(n[:, 2] >= 0, 1., -1.)
    a = -1/(sign+n[:, 2]); b = n[:, 0]*n[:, 1]*a
    t = np.column_stack([1+sign*n[:, 0]**2*a, sign*b, -sign*n[:, 0]])
    bt = np.column_stack([b, sign+n[:, 1]**2*a, -n[:, 1]])
    return t*(np.cos(2*np.pi*u)*np.sqrt(1-v))[:, None] + bt*(np.sin(2*np.pi*u)*np.sqrt(1-v))[:, None] + n*np.sqrt(v)[:, None]


def cells(direction, side=8):
    u = np.mod(np.arctan2(direction[:, 1], direction[:, 0]) / (2*np.pi), 1)
    v = np.clip((direction[:, 2]+1)/2, 0, 1)
    return np.minimum((u*side).astype(int), side-1) + side*np.minimum((v*side).astype(int), side-1)


def learn(index, contribution, base_pdf, train, valid, side=8):
    selected = train & valid
    # A learned proposal need not be the ideal density to preserve energy.
    # Density correction here prevents preferentially sampled directions from
    # gaining mass solely because they were sampled more often.
    mass = np.bincount(index[selected], weights=contribution[selected]/base_pdf[selected], minlength=side*side)
    if mass.sum() <= 0:
        return np.full(side*side, 1/(side*side))
    # Uniform guide floor; the separate cosine mixture supplies primary support.
    return .99*mass/mass.sum() + .01/(side*side)


def ratios(base_pdf, guide_pdf, fraction):
    assert 0 <= fraction <= .5
    return base_pdf / ((1-fraction)*base_pdf + fraction*guide_pdf)


def load_capture(root, label):
    data = np.load(root/f'{label}-trace-arrays.npz')
    summary, ray, tags = data['summary'], data['ray'], data['tags']
    findings = json.loads((root/f'{label}-findings.json').read_text())
    x, y, w, h = findings['roi']
    yy, xx = np.mgrid[y:y+h, x:x+w]
    # These are the previously checked capture-specific masks, not runtime IDs.
    mask = ((tags >= -3) & (tags <= -2)) if label == 'hair' else (
        (xx >= 1045) & (xx < 1155) & (yy >= 700) & (yy < 790) & (tags > 0))
    frames = sorted((root/label/'full-screen-rr').iterdir())
    meta = json.loads((frames[0]/'metadata.json').read_text())
    resource = next(r for r in meta['resources'] if r['file'].startswith('raw-albedo.'))
    raw = np.fromfile(frames[0]/resource['file'], dtype='<f2').reshape(resource['height'], resource['width'], 4)[y:y+h, x:x+w, :3]
    mask &= np.isfinite(raw).all(-1) & (raw >= 0).all(-1) & (raw <= 1).all(-1)
    assert np.all(summary[..., 2].view('u4') == 1), 'Only the captured single-ray case is supported'
    r = ray[mask].astype(np.float64)
    vector = r[:, 32:35]-r[:, 48:51]
    distance = np.linalg.norm(vector, axis=1)
    direction = vector/np.maximum(distance[:, None], 1e-30)
    cosine = np.sum(direction*r[:, 52:55], axis=1)
    valid = (distance >= .05) & np.isfinite(direction).all(1) & (cosine > 0) & (cosine <= 1.00001)
    base_pdf = np.clip(cosine, 1e-12, 1)/np.pi
    return dict(label=label, frame=findings['frame'], rgb=summary[mask, 4:7].astype(np.float64),
                direction=direction, base_pdf=base_pdf, valid=valid,
                normal=r[:, 52:55], origin=r[:, 48:51],
                x=xx[mask], y=yy[mask], distance=distance,
                branch=ray[mask].view('u4')[:, 19])


def screen(data, side=8, layout='pixel', seed=0, grouping='whole', folds=2):
    index = cells(data['direction'], side)
    c = data['rgb']@LUMA
    p = data['base_pdf']
    # Random folds and contiguous block folds probe different spatial leakage.
    if layout == 'pixel':
        fold = np.random.default_rng(seed).integers(0, folds, len(c))
    else:
        fold = ((data['x']//32) + 3*(data['y']//32) + seed) % folds
    all_ratio = {a: np.ones(len(c)) for a in FRACTIONS}
    normal_group = ((data['normal'] > 0)*np.array([1, 2, 4])).sum(1)
    if grouping == 'whole':
        group = np.zeros(len(c), int)
    elif grouping == 'normal':
        group = normal_group
    elif grouping == 'world_normal':
        keys = np.column_stack([np.floor(data['origin']/.25).astype(int), normal_group])
        _, group = np.unique(keys, axis=0, return_inverse=True)
    else:
        raise ValueError(grouping)
    fallback = np.zeros(len(c), bool)
    for f in range(folds):
        train, test = fold != f, fold == f
        assert not np.any(train & test)
        for g in np.unique(group[test]):
            local_train, local_test = train & (group == g), test & (group == g)
            if np.count_nonzero(local_train & data['valid']) < 256:
                fallback[local_test] = True
                continue
            mass = learn(index, c, p, local_train, data['valid'], side)
            qg = mass[index[local_test]]*(side*side)/(4*np.pi)
            for a in FRACTIONS:
                all_ratio[a][local_test] = ratios(p[local_test], qg, a)
    results = []
    known = data['valid']
    for a, weight in all_ratio.items():
        low, high = weight.copy(), weight.copy()
        low[~known & ~fallback] = 1 if a == 0 else 0
        high[~known & ~fallback] = 1/(1-a)
        result = dict(fraction=a)
        for name, values in [('luminance', c), ('red', data['rgb'][:, 0]),
                             ('green', data['rgb'][:, 1]), ('blue', data['rgb'][:, 2])]:
            energy = values**2
            base = energy.sum()
            result[name+'_second_moment_ratio_bounds'] = [float((energy*low).sum()/base), float((energy*high).sum()/base)]
        result['miss_second_moment_ratio_bounds'] = [
            float((c[data['branch'] == 1]**2*w[data['branch'] == 1]).sum()/(c[data['branch'] == 1]**2).sum())
            for w in [low, high]]
        result['per_fold_luminance_upper_ratio'] = [float(((c**2*high)[fold == f]).sum()/((c**2)[fold == f]).sum()) for f in range(folds)]
        result['two_stage_whole_region_upper_ratios'] = [float((c**2*np.where(fold == f, high, 1)).sum()/(c**2).sum()) for f in range(folds)]
        results.append(result)
    return dict(side=side, layout=layout, seed=seed, grouping=grouping, folds=folds,
                native_fallback_samples=int(fallback.sum()), samples=len(c),
                uncertain_direction_count=int((~known).sum()), trials=results)


def self_test():
    import sys
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'tools'))
    from importlib import import_module
    emitted = import_module('test-gi-live-sampling')
    for variant in [0, 1]:
        reference = emitted.evaluate(variant, 1, frames=4096, owned=True)
        calculated = producer_units(np.full(4096, 343, 'u4'), np.full(4096, 714, 'u4'), np.arange(4096, dtype='u4'), 2)
        for a, b in zip(reference, calculated):
            assert np.array_equal(a.ravel(), b)
    # Exact discrete second moments, including missed light and a stale proposal.
    p = np.array([.1, .2, .3, .4])
    field = np.array([20., .5, 0., 2.])
    for guide in [np.array([.8, .1, .05, .05]), np.array([.01, .01, .97, .01])]:
        for a in FRACTIONS:
            q = (1-a)*p+a*guide
            c = field/p
            assert np.allclose(np.sum(p*c*c*ratios(p, guide, a)), np.sum(field*field/q))
            assert np.allclose(np.sum(q*field/q), np.sum(field))
            assert np.max(ratios(p, guide, a)) <= 1/(1-a)+1e-12
    # Held-out contributions cannot alter the learned guide.
    index = np.array([0, 1, 2, 3]); train = np.array([1, 0, 1, 0], bool)
    valid = np.ones(4, bool); c = field.copy()
    first = learn(index, c, p, train, valid, 2)
    c[~train] = 1e20
    assert np.array_equal(first, learn(index, c, p, train, valid, 2))
    # Constant cosine-domain lighting: a mismatched guide adds variance.
    q = .5*p+.5*np.array([.97, .01, .01, .01])
    assert np.sum(p*p/q) > 1
    print('Generated-IR single-ray hashes, exact moments, energy, support, stale-guide and held-out leakage checks passed.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture_root', nargs='?', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    self_test()
    if args.capture_root is None:
        return
    report = dict(scope='Conditional offline second-moment screening, not rendered variance or a runtime fix. Geometry directions reconstructed, no new rays traced.', captures={})
    for label in ['hair', 'jaw']:
        data = load_capture(args.capture_root, label)
        report['captures'][label] = [screen(data, side, layout, seed, grouping)
            for side in [4, 8, 16] for layout in ['pixel', 'block']
            for seed in (range(4) if layout == 'pixel' else [0])
            for grouping in ['whole', 'normal', 'world_normal']]
        # Explicit sensitivity hypothesis, not an exported raygen constant buffer.
        # The source token+1/batch0 candidate fits endpoint directions; report the
        # fit and rerun fixed candidates rather than silently assume exact PDFs.
        fits = []
        for offset in [-1, 0, 1, 2]:
            for batch in [0, 1, 2, 3]:
                u, v, _ = producer_units(data['x'], data['y'], data['frame']+offset, batch)
                d = producer_direction(data['normal'], u, v)
                angle = np.degrees(np.arccos(np.clip(np.sum(d*data['direction'], axis=1), -1, 1)))
                fits.append(dict(frame_offset=offset, batch=batch, median_angle_degrees=float(np.median(angle[data['valid']]))))
        best = min(fits, key=lambda fit: fit['median_angle_degrees'])
        assert best['frame_offset'] == 1 and best['batch'] == 0
        u, v, range_unit = producer_units(data['x'], data['y'], data['frame']+1)
        hypothesized = dict(data, direction=producer_direction(data['normal'], u, v), base_pdf=np.sqrt(v)/np.pi, valid=np.ones(len(v), bool))
        miss = data['branch'] == 1
        range_error = data['distance'][miss]-3*(.8+.4*range_unit[miss])
        report.setdefault('seed_hypothesis', {})[label] = dict(
            limitation='Seed/3-unit range inferred from captured geometry, not exported raygen constants; sensitivity analysis only.',
            fits=fits, miss_range_error_percentiles=np.percentile(range_error, [0, 1, 50, 99, 100]).tolist(),
            trials=[screen(hypothesized, 8, layout, 0, grouping)
                    for layout in ['pixel', 'block'] for grouping in ['whole', 'normal', 'world_normal']])
    output = json.dumps(report, indent=2)+'\n'
    if args.output:
        args.output.write_text(output)
    else:
        print(output)


if __name__ == '__main__':
    main()
