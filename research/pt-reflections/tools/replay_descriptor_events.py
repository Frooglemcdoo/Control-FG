"""Interpret PIX C++ export descriptor writes at replay submissions.

This follows exported replay order, including ModifyDescriptors calls made after
command-list population. It does not prove the original game's asynchronous GPU
schedule: PIX inserts FlushCommandQueue and comments out native Wait calls.
"""
import re


def functions(files):
    result = {}
    for name, text in files.items():
        matches = list(re.finditer(r'^void (\w+)\(\)\s*\{\n', text, re.M))
        for i, m in enumerate(matches):
            first = text.count('\n', 0, m.end()) + 1
            if m[1] in result:
                raise ValueError(f'duplicate exported function {m[1]}')
            depth, body = 1, []
            end = matches[i+1].start() if i+1 < len(matches) else len(text)
            for n, line in enumerate(text[m.end():end].splitlines()):
                code = re.sub(r'"(?:\\.|[^"\\])*"|//.*', '', line)
                depth += code.count('{') - code.count('}')
                if depth == 0:
                    break
                body.append((name, first + n, line))
            if depth:
                raise ValueError(f'unterminated exported function {m[1]}')
            result[m[1]] = body
    return result


def descriptor(line):
    """Return every supported write, including null and non-SRV overwrites."""
    h = re.search(r'GetCpuDescriptor\(g_descriptorHeap_(\d+).Get\(\), (\d+)\)', line)
    if not h:
        return None
    create = re.search(r'\bCreate(\w+)\(', line)
    if not create:
        raise ValueError(f'unsupported descriptor operation: {line.strip()}')
    name = create[1]
    kind = ('srv' if name.startswith('ShaderResourceView') else
            'uav' if name.startswith('UnorderedAccessView') else
            'cbv' if name == 'ConstantBufferView' else
            'sampler' if name == 'Sampler' else None)
    if kind is None:
        raise ValueError(f'unsupported descriptor creator: {name}')
    resource = re.search(r'GetResource\((\d+)\)', line)
    gpuva = re.search(r'GetGpuva\((\d+), (\d+)\)', line)
    dimension = re.search(r'D3D12_(?:SRV|UAV)_DIMENSION_(\w+)', line)
    value = dict(resource=resource[1] if resource else gpuva[1] if gpuva else None,
                 kind=kind, dimension=dimension[1] if dimension else None)
    if gpuva:
        value['offset'] = int(gpuva[2])
    return (h[1], int(h[2])), value


def submitted_lines(src, base):
    """Yield command source with descriptors as seen at that submission.

    Consumers must resolve lookups before advancing the generator; the descriptor
    map is intentionally shared to avoid copying hundreds of thousands of rows.
    """
    prefixes = ('Descriptors_', 'ModifyDescriptors_', 'CommandLists_', 'RenderFrameWorker_')
    files = {n[len(base):]: src.read(n).decode().replace('\r\n', '\n')
             for n in src.namelist() if n.startswith(base) and n.endswith('.cpp') and
             n[len(base):].startswith(prefixes)}
    funcs = functions(files)
    descriptors = {}
    # Initial writes are unique in PIX's initial heap snapshot.
    for name, text in files.items():
        if not name.startswith('Descriptors_'):
            continue
        for line in text.splitlines():
            write = descriptor(line)
            if write:
                key, value = write
                if key in descriptors and descriptors[key] != value:
                    raise ValueError(f'conflicting initial descriptor {key}')
                descriptors[key] = value
    root = src.read(base + 'RenderFrame.cpp').decode()
    roots = re.findall(r'^    (RenderFrame_\d+)\(\);', root, re.M)
    if not roots:
        raise ValueError('no exported replay entry points')
    recorded, arrays, array_count = {}, {}, 0
    submission = 0
    for entry in roots:
        if entry not in funcs:
            raise ValueError(f'missing replay function {entry}')
        for file, line_no, raw in funcs[entry]:
            line = raw.strip()
            if line.startswith('//'):
                continue
            modify = re.fullmatch(r'(ModifyDescriptors_\w+)\(\);', line)
            populate = re.fullmatch(r'(PopulateCommandList_(\d+)_\d+_\d+)\(\);', line)
            if modify:
                if modify[1] not in funcs:
                    raise ValueError(f'missing descriptor update {modify[1]}')
                writes = [descriptor(row[2]) for row in funcs[modify[1]] if row[2].strip()]
                if not writes or any(w is None for w in writes):
                    raise ValueError(f'unsupported descriptor update {modify[1]}')
                for key, value in writes:
                    descriptors[key] = value
            elif populate:
                body = funcs.get(populate[1])
                if body is None:
                    raise ValueError(f'missing command population {populate[1]}')
                cl = populate[2]
                if any(re.search(r'GetCommandList\('+cl+r'\)->Reset\(', row[2]) for row in body):
                    recorded[cl] = []
                if cl not in recorded:
                    raise ValueError(f'command list {cl} has no reset')
                recorded[cl].extend(body)
            elif m := re.fullmatch(r'ID3D12CommandList\* commandLists\[(\d+)\];', line):
                arrays = {}; array_count = int(m[1])
            elif m := re.fullmatch(r'commandLists\[(\d+)\] = GetCommandList\((\d+)\).Get\(\);', line):
                arrays[int(m[1])] = m[2]
            elif m := re.fullmatch(r'commandLists\[(\d+)\] = g_utilityCommandList.Get\(\);', line):
                arrays[int(m[1])] = None  # Export-only final backbuffer copy.
            elif m := re.fullmatch(r'GetCommandQueue\((\d+)\)->ExecuteCommandLists\(_countof\(commandLists\), commandLists\);', line):
                if not arrays or sorted(arrays) != list(range(array_count)):
                    raise ValueError('unsupported submission array')
                submission += 1
                for cl in arrays.values():
                    if cl is None:
                        continue
                    if cl not in recorded:
                        raise ValueError(f'unrecorded submitted list {cl}')
                    for source_file, source_line, command in recorded[cl]:
                        yield source_file, source_line, command, descriptors, dict(
                            submission=submission, queue=m[1], submit_file=file, submit_line=line_no)
            elif 'ExecuteCommandLists' in line or 'ModifyDescriptors_' in line or 'PopulateCommandList_' in line:
                raise ValueError(f'unsupported replay statement {line}')
    if not submission:
        raise ValueError('no replay submissions')
