"""Two rays per receiver: one native pilot, then one fresh guided ray.
Discrete toy only; no captured directional radiance, geometry or GPU timings.
Condition on ALL pilots before constructing q. Pilot retains weight 1. Only
the second ray receives 1/(64*q). Applying q to the pilot would be biased.
"""
import argparse
import json
import numpy as np


def trial(receivers, kinds, fraction, seed=426, repetitions=8192):
    rng = np.random.default_rng(seed)
    cells = 64
    hot = (7 + (np.arange(receivers) % kinds) * 29) % cells
    signal = np.full((receivers, cells), .05)
    signal[np.arange(receivers), hot] = 100
    pilots = rng.integers(cells, size=(repetitions, receivers))
    first = np.take_along_axis(signal[None, :, :], pilots[:, :, None], axis=2)[:, :, 0]
    energy = np.zeros((repetitions, cells))
    np.add.at(energy, (np.repeat(np.arange(repetitions), receivers), pilots.ravel()),
              first.ravel() * cells / receivers)
    q = (1-fraction)/cells + fraction * energy / energy.sum(axis=1, keepdims=True)
    mean = signal.mean(axis=1)
    native_variance = signal.var(axis=1).mean()
    # Given pilots, the fresh second estimator has mean I. Its covariance with
    # the first estimator is therefore zero, even when own pilot informs q.
    second_variance = np.mean(np.sum(signal[None, :, :]**2 /
                                    (cells*cells*q[:, None, :]), axis=2) - mean**2)
    pair_variance = (native_variance + second_variance)/4
    expected_second = np.sum(q[:, None, :]*(signal[None, :, :]/(cells*q[:, None, :])), axis=2)
    error = float(np.max(abs(expected_second - mean)))
    assert error < 1e-10
    # Negative control: retroactively applying the learned q to its own pilot.
    wrong_first = first/(cells*np.take_along_axis(q, pilots, axis=1))
    wrong_mean = float(np.mean((wrong_first+mean)/2))
    return dict(receivers_per_group=receivers, distinct_lighting_fields=kinds,
                guide_fraction=fraction, rays_per_receiver=2,
                native_independent_pair_variance=float(native_variance/2),
                split_pair_variance=float(pair_variance),
                variance_ratio=float(pair_variance/(native_variance/2)),
                max_conditional_mean_error=error, expected_mean=float(mean.mean()),
                wrong_pilot_reweighting_mean=wrong_mean)


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output');a=p.parse_args()
    result={'scope':'synthetic discrete fields; independently uniform native comparator, not Control diagonal strata; group coherence assumed; no temporal history, GPU cost or image claim',
            'seed':426,'repetitions':8192,
            'trials':[trial(n,k,f) for n in (16,64,128) for k in (1,2,8,16) for f in (.125,.5)]}
    text=json.dumps(result,indent=2)+'\n'
    if a.output:
        from pathlib import Path
        Path(a.output).write_text(text)
    else: print(text,end='')
