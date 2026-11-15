"""Audit CFG, unchanged non-CB instructions, and every consumed CB register/lane."""
import re,sys
from pathlib import Path
from gi_evaluator_constants import transform,named,cfg,LOAD

def verify(source):
    before=named(source);after,report=transform(source)
    _,_,a,sa,pa,da=cfg(before);_,_,b,sb,pb,db=cfg(after)
    assert sa==sb and pa==pb, 'Control flow changed'
    def reads(blocks):
        return {m[1]:(m[2],m[3],m[4]) for lines in blocks.values() for line in lines if (m:=LOAD.match(line))}
    old,new=reads(a),reads(b)
    # Normalize uses to symbolic immutable CB addresses, independently of report mappings.
    def canonical(blocks,loads):
        out={}
        for block,lines in blocks.items():
            out[block]=[re.sub(r'%\w+',lambda m:str(loads[m[0]]) if m[0] in loads else m[0],line) for line in lines if not LOAD.match(line)]
        return out
    assert canonical(a,old)==canonical(b,new), 'Non-CB instruction/address changed'
    locations={m[1]:(block,i) for block,lines in b.items() for i,line in enumerate(lines) if (m:=LOAD.match(line))}
    for block,lines in b.items():
        for i,line in enumerate(lines):
            if LOAD.match(line):continue
            for value in re.findall(r'%\w+',line):
                if value in locations:
                    origin,j=locations[value]
                    assert (origin==block and j<i) or (origin!=block and origin in db[block]), 'Read does not dominate use'
    for move in report['moves']:
        assert sa[move['destination']]=={move['source']}, 'Speculated conditional read'
        assert move['destination'] in da[move['source']], 'Invalid preheader'
    print('PASS CB register/lane equality, unchanged arithmetic/CFG, dominating uses:',report['hoisted'],'hoisted,',report['eliminated'],'removed')

for path in sys.argv[1:]:verify(Path(path).read_text())
