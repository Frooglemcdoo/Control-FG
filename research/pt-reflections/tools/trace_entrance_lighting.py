"""Resolve declared lighting bindings against E3 descriptor snapshots.

Input inventory comes from map_entrance_pipelines.py. This is deliberately a
static binding trace, not a GPU dependency graph or instruction-level liveness.
Only the captured common root-table layout and space-zero SRV/UAVs are supported.
"""
import argparse
import json
import re
import zipfile
from pathlib import Path


def trace(source, inventory):
    result = {'schema': 1, 'limitations': __doc__, 'views': {}}
    with zipfile.ZipFile(source) as src:
        for view, data in inventory['views'].items():
            project = next(n for n in src.namelist() if n.startswith(view + '/') and n.endswith('/project/CreatePSOs.cpp'))
            base = project.rsplit('/', 1)[0] + '/'
            descriptors = {}
            for name in src.namelist():
                if not name.startswith(base + 'Descriptors_'):
                    continue
                for line_no, line in enumerate(src.read(name).decode().splitlines(), 1):
                    m = re.search(r'Create(ShaderResource|UnorderedAccess)View_\w+\(GetResource\((\d+)\).*GetCpuDescriptor\(g_descriptorHeap_(\d+).Get\(\), (\d+)\)', line)
                    if m:
                        key = (m[3], int(m[4]))
                        value = dict(resource=m[2], kind='srv' if m[1] == 'ShaderResource' else 'uav')
                        if key in descriptors and descriptors[key] != value:
                            raise ValueError(f'{view}: mutable descriptor snapshot {key}')
                        descriptors[key] = value
            # Validate supported descriptor layout rather than assuming offsets.
            init = src.read(base + 'FrameResources_000.cpp').decode()
            roots = {}
            for m in re.finditer(r'// ApiObjectId\s*=\s*(\d+)\s*\{\s*(static D3D12_ROOT_PARAMETER.*?)CreateAndTrackRootSignature\(', init, re.S):
                body = m[2]
                tables = {}
                for t in re.finditer(r'rootParameters\[(\d+)\].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;.*?\{(.*?)rootParameters\[\1\].DescriptorTable', body, re.S):
                    ranges = re.findall(r'D3D12_DESCRIPTOR_RANGE_TYPE_(\w+), (\d+), (\d+), (\d+), (\d+)', t[2])
                    if ranges == [('CBV', '5', '4', '0', '4294967295'), ('SRV', '36', '0', '0', '4294967295'), ('UAV', '8', '0', '0', '4294967295')]:
                        tables[int(t[1])] = True
                roots[m[1]] = tables
            calls = []
            for name in sorted(n for n in src.namelist() if n.startswith(base + 'CommandLists_') and n.endswith('.cpp')):
                states = {}
                pending_rtvs = []
                for line_no, line in enumerate(src.read(name).decode().splitlines(), 1):
                    if 'D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =' in line:
                        pending_rtvs = []
                    rtv = re.search(r'CreateRenderTargetView\(GetResource\((\d+)\).*&rtvDesc, rtvHandle\)', line)
                    if rtv:
                        pending_rtvs.append(rtv[1])
                    m = re.search(r'GetCommandList\((\d+)\)->(\w+)\((.*)', line)
                    if not m:
                        continue
                    cl, op, args = m.groups()
                    if op == 'Reset':
                        states[cl] = {'tables': {}, 'roots': {}, 'rtvs': []}
                        p = re.search(r'GetPipelineState\((\d+)\)', args)
                        states[cl]['pipeline'] = p[1] if p else None
                    state = states.setdefault(cl, {'tables': {}, 'roots': {}, 'rtvs': []})
                    if op == 'SetPipelineState':
                        state['pipeline'] = re.search(r'GetPipelineState\((\d+)\)', args)[1]
                    elif op in ('SetComputeRootSignature', 'SetGraphicsRootSignature'):
                        domain = 'compute' if 'Compute' in op else 'graphics'
                        root = re.search(r'GetRootSignature\((\d+)\)', args)[1]
                        if state['roots'].get(domain) != root:
                            state['tables'][domain] = {}
                        state['roots'][domain] = root
                    elif op in ('SetComputeRootDescriptorTable', 'SetGraphicsRootDescriptorTable'):
                        domain = 'compute' if 'Compute' in op else 'graphics'
                        t = re.search(r'(\d+), GetGpuDescriptor\(g_descriptorHeap_(\d+).Get\(\), (\d+)\)', args)
                        if t:
                            state['tables'].setdefault(domain, {})[int(t[1])] = (t[2], int(t[3]))
                    elif op == 'OMSetRenderTargets':
                        n = int(args.split(',')[0])
                        state['rtvs'] = pending_rtvs[:n] if '&rtvHandle' in args else []
                    elif op in ('Dispatch', 'DrawInstanced', 'DrawIndexedInstanced'):
                        pipeline = data['pipelines'].get(state.get('pipeline'))
                        if not pipeline:
                            continue
                        resolved = []
                        unresolved = []
                        for shader in pipeline['shaders']:
                            stage = shader['stage']
                            if stage not in ('compute', 'pixel'):
                                continue
                            domain, param = ('compute', 0) if stage == 'compute' else ('graphics', 1)
                            table = state['tables'].get(domain, {}).get(param)
                            root = state['roots'].get(domain)
                            if not table or param not in roots.get(root, {}):
                                continue
                            for binding in shader['bindings'] or []:
                                if binding['space'] != 0 or binding['kind'] not in (2, 4, 5, 6, 7, 8):
                                    continue
                                kind = 'uav' if binding['kind'] in (4, 6, 8) else 'srv'
                                index = table[1] + (41 if kind == 'uav' else 5) + binding['slot']
                                desc = descriptors.get((table[0], index))
                                if not desc or desc['kind'] != kind:
                                    unresolved.append(dict(name=binding['name'], kind=kind, heap=table[0], index=index))
                                    continue
                                resolved.append(dict(name=binding['name'], stage=stage, slot=binding['slot'],
                                                     kind=kind, resource=desc['resource'], heap=table[0], index=index))
                        calls.append(dict(file=name[len(base):], line=line_no, command_list=cl,
                                          pipeline=state.get('pipeline'), operation=op,
                                          rtvs=list(state['rtvs']) if op.startswith('Draw') else [], bindings=resolved,
                                          unresolved=unresolved))
            direct = next(c for c in calls if any(b['name'] == 'g_rwtSingleSpot' for b in c['bindings']))
            outputs = [b for b in direct['bindings'] if b['kind'] == 'uav']
            targets = {}
            for b in outputs:
                targets.setdefault(b['resource'], []).append(b['name'])
            aliases = {r: names for r, names in targets.items() if len(names) > 1}
            relevant = [c for c in calls if any(b['resource'] in targets for b in c['bindings']) or any(r in targets for r in c['rtvs'])]
            result['views'][view] = dict(targets=targets, calls=relevant,
                primary_output_aliases=aliases,
                validation='candidate_only; descriptor contents are exported snapshots, not independently verified replay state',
                unresolved_calls=[dict(file=c['file'], line=c['line'], pipeline=c['pipeline'], bindings=c['unresolved']) for c in calls if c['unresolved']])
    return result


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path)
    p.add_argument('inventory', type=Path)
    p.add_argument('output', type=Path)
    args = p.parse_args()
    args.output.write_text(json.dumps(trace(args.source, json.loads(args.inventory.read_text())), indent=2) + '\n')
