from pt_scene_correlation import analyze


def build(seq, position='2000', count=1):
    return [dict(kind=1, sequence=seq, count=count),
            dict(kind=3, sequence=seq, ordinal=0, geometry_type=0, index_format=57,
                 vertex_format=6, transform='0', index='1008', position=position,
                 stride=12, index_count=6, vertex_count=4)]


def binding(offset=8, stride=2, size=32):
    return dict(kind=6, sequence=1, ordinal=0, index='1000',
                constants=[offset, stride, 24, 0, 0, 0, 0, 0, size, 96])


end = dict(kind='end', write_ok=True, dropped=0)
table = dict(kind=5, sequence=1, count=1, vertex_count=1)
r = analyze(build(1) + [table, binding(), end])
assert r['unique_candidate'] == 1 and r['complete'] and r['incomplete_tables'] == 0
assert not r['lifetime_verified'] and not r['renderer_ready']
assert analyze(build(1) + build(2) + [binding()])['unique_candidate'] == 1
assert analyze(build(1) + build(2, '3000') + [binding()])['ambiguous_candidate'] == 1
assert analyze([binding()] + build(1))['missing_candidate'] == 1
assert analyze(build(1, count=2) + [binding()])['missing_candidate'] == 1
assert analyze(build(1) + [binding(size=16)])['missing_candidate'] == 1
assert analyze(build(1) + [binding(stride=3)])['invalid_index_binding'] == 1
assert analyze([table])['incomplete_tables'] == 1
assert not analyze([])['complete']
print('PASS M7 correlation: unique, duplicate, reused address, missing, partial, bounds, stride and truncated samples.')
