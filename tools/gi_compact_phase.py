"""Build compact column phases for audited GI variant zero, offline only.

Runtime must use even full width, half-width dispatches and both phases exactly
once. Variant one has a separate compaction ABI and is deliberately rejected.
No sampling density, ray budget or shading change is introduced here.
"""
import argparse
import re
import subprocess
from pathlib import Path
from gi_single_producer import edit_export, replace_once, shared_hits, owned_raygen
from gi_live_sampling import transform


def map_x(body, parity=None):
    pattern = r'^  %DispatchRaysIndex = call i32 @dx.op.dispatchRaysIndex.i32\(i32 145, i8 0\)[^\n]*'
    matches = re.findall(pattern, body, re.M)
    if len(matches) != 1:
        raise ValueError('Expected one audited dispatch X coordinate')
    old = matches[0]
    lines = [old.replace('%DispatchRaysIndex =', '%GICompactIndex ='),
             '  %GICompactTwice = shl i32 %GICompactIndex, 1']
    if parity is None:
        lines += ['  %GICompactParityShift = lshr i32 %GITaggedSlice, 28',
                  '  %GICompactParity = and i32 %GICompactParityShift, 1',
                  '  %GICompactX = or i32 %GICompactTwice, %GICompactParity',
                  '  %DispatchRaysIndex = select i1 %GIIsCompact, i32 %GICompactX, i32 %GICompactIndex']
    else:
        lines += [f'  %DispatchRaysIndex = or i32 %GICompactTwice, {parity}']
    return replace_once(body, old, '\n'.join(lines))


def compact_shared_hits(source):
    def patch(body):
        # Top nibble A/B identifies compact owned rays. Visibility uses C..F:
        # testing bit 29 alone would incorrectly remap visibility ray writes.
        old = '  %GIRegularSlice = and i32 %GITaggedSlice, 2147483647'
        new = '''  %GICompactFlags = and i32 %GITaggedSlice, -536870912
  %GIIsCompact = icmp eq i32 %GICompactFlags, -1610612736
  %GINonCompactSlice = and i32 %GITaggedSlice, 2147483647
  %GICompactSlice = and i32 %GITaggedSlice, 63
  %GIRegularSlice = select i1 %GIIsCompact, i32 %GICompactSlice, i32 %GINonCompactSlice'''
        return map_x(replace_once(body, old, new))
    result = shared_hits(source)
    for export in ('closestHitMain', 'missMain'):
        result = edit_export(result, export, patch)
    return result


def compact_raygen(source, variant, parity):
    if variant != 0 or parity not in (0, 1):
        raise ValueError('Compact phases support only variant zero, parity 0/1')
    def patch(body):
        body = map_x(body, parity)
        return replace_once(body,
                            'or i32 %rayIndex.03, -2147483648',
                            f'or i32 %rayIndex.03, {-1610612736 + (parity << 28)}')
    return edit_export(owned_raygen(transform(source, variant), variant), 'rayGenMain', patch)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True)
    parser.add_argument('--assembler', required=True)
    parser.add_argument('--compiler-dll', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    source = Path(args.source).read_text(encoding='utf-8-sig')
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    programs = {'shared': compact_shared_hits(source)}
    programs.update({f'phase-{p}': compact_raygen(source, 0, p) for p in (0, 1)})
    for name, program in programs.items():
        ir = output / f'compact-{name}.ll'
        ir.write_text(program)
        subprocess.run([args.assembler, str(ir), str(ir.with_suffix('.dxil')),
                        args.compiler_dll], check=True)
    print('Validated compact shared hit/miss and both receiver phases; runtime not enabled')


if __name__ == '__main__':
    main()
