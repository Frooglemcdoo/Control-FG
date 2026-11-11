"""Offline candidate only. Not imported by the production shader generator."""
from character_surface import words, dst, src, reg, transform_shex
from character_diffuse import filtered_diffuse, ins, ui


def guard_instructions(temporary):
    return ins('ieq',dst(temporary,1),src(temporary,0),ui(0)) + ins('if',src(temporary,0),flags=1<<18)


def guarded_diffuse(data, info):
    original = words(data)
    instructions = info['instructions']
    load = next(i for i in instructions if i['name'] == 'ld' and i['operands'][-1] == 't[3][3].xyzw')
    firstbit = next(i for i in reversed(instructions[:instructions.index(load)]) if i['name'] == 'firstbit_lo')
    temp_decl = next(i['dword'] for i in instructions if i['name'] == 'dcl_temps')
    temporary = original[temp_decl+1]+13
    lane = 'xyzw'.index(firstbit['operands'][0].split('.')[-1])
    snapshot = ins('mov', dst(temporary,1), src(reg(firstbit['operands'][0]), lane*85))
    insert_snapshot = firstbit['dword']+(original[firstbit['dword']]>>24)
    begin = load['dword']+(original[load['dword']]>>24)
    filtered = filtered_diffuse(data, info)
    filtered_words = words(filtered)
    length = len(filtered_words)-len(original)
    guard = guard_instructions(temporary)
    end = ins('endif')
    def patch(w):
        w[begin+length:begin+length] = end
        w[begin:begin] = guard
        w[insert_snapshot:insert_snapshot] = snapshot
        w[temp_decl+1] += 1
    result = transform_shex(filtered, patch)
    # Exact reversible proof: no native instructions, bindings or signatures edited.
    restored = words(result)
    del restored[insert_snapshot:insert_snapshot+len(snapshot)]
    del restored[begin:begin+len(guard)+length+len(end)]
    restored[temp_decl+1] -= 14
    restored[1] = len(restored)
    assert restored == original
    return result, dict(snapshot=snapshot, guard=guard, end=end, temporary=temporary)
