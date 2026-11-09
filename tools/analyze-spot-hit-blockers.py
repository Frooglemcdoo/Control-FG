"""Summarize a validated hit log and compare reconstructed blocker meshes.

Requires NumPy. Instance IDs are local to this capture, never asset identifiers.
Triangle reconstruction uses recorded barycentrics, not guessed surface normals.
"""
import argparse
import collections
import importlib.util
import json
from pathlib import Path

import numpy as np


def reconstruct_triangles(records):
    triangles = {}
    groups = collections.defaultdict(list)
    for record in records:
        groups[(record['geometry_index'], record['primitive_index'])].append(record)
    for key, group in groups.items():
        weights = np.array([record['barycentrics'] for record in group])
        positions = np.array([record['hit_position'] for record in group])
        vertices, _, rank, singular = np.linalg.lstsq(weights, positions, rcond=None)
        # Reject underdetermined or poorly conditioned fits; extrapolation from
        # almost identical samples cannot establish a triangle's actual shape.
        if rank != 3 or singular[0] / singular[-1] >= 100:
            continue
        residual = np.max(np.abs(weights @ vertices - positions))
        if residual < 1e-5:
            triangles[key] = vertices
    return triangles


def analyze(capture):
    valid = [record for record in capture['records'] if record['valid']]
    hits = [record for record in valid if record['hit']]
    lights = []
    meshes = {}
    for light in sorted({record['light_id'] for record in valid}):
        rays = [record for record in valid if record['light_id'] == light]
        blockers = [record for record in rays if record['hit']]
        endpoints = np.array([np.array(record['origin']) + record['t_max'] * np.array(record['direction']) for record in rays])
        summary = dict(light_id=light, valid_rays=len(rays), hits=len(blockers),
                       endpoint_mean=endpoints.mean(axis=0).tolist(),
                       endpoint_spread=np.ptp(endpoints, axis=0).tolist(), blockers=[])
        counts = collections.Counter(record['instance_index'] for record in blockers)
        for instance, count in counts.most_common(4):
            group = [record for record in blockers if record['instance_index'] == instance]
            positions = np.array([record['hit_position'] for record in group])
            summary['blockers'].append(dict(
                instance_index=instance, hits=count,
                position_min=positions.min(axis=0).tolist(), position_max=positions.max(axis=0).tolist(),
                endpoint_gap_quantiles=np.quantile([record['endpoint_gap'] for record in group], [0, .5, 1]).tolist()))
        if counts:
            instance = counts.most_common(1)[0][0]
            meshes[(light, instance)] = reconstruct_triangles([record for record in blockers if record['instance_index'] == instance])
        lights.append(summary)
    comparisons = []
    keys = list(meshes)
    for index, first in enumerate(keys):
        for second in keys[index + 1:]:
            common = meshes[first].keys() & meshes[second].keys()
            if len(common) < 3:
                continue
            differences = np.array([meshes[second][key] - meshes[first][key] for key in sorted(common)])
            translation = differences.reshape(-1, 3).mean(axis=0)
            residual = float(np.max(np.abs(differences - translation)))
            comparisons.append(dict(first_light_instance=first, second_light_instance=second,
                                    shared_triangles=len(common), translation=translation.tolist(),
                                    max_translation_residual=residual,
                                    matches_translation=residual < 1e-4))
    return dict(capture_id=capture['capture_id'], extent=capture['extent'],
                valid_rays=len(valid), hits=len(hits), lights=lights, mesh_comparisons=comparisons)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('hit_decoder', Path(__file__).with_name('decode-spot-hit-capture.py'))
    decoder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(decoder)
    capture = decoder.decode_log(args.log.read_text(encoding='utf-8-sig'))
    print(json.dumps(analyze(capture), indent=2))


if __name__ == '__main__':
    main()
