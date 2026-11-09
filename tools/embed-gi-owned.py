"""Embed the validated owned GI stages for the hash-matched in-game pipeline."""
import pathlib,sys
names=('Proposals','Trace','Shading','Resolve')
if len(sys.argv)!=6:raise SystemExit('output proposals trace shading resolve')
lines=['#pragma once']
for name,path in zip(names,sys.argv[2:]):
 data=pathlib.Path(path).read_bytes()
 if data[:4]!=b'DXBC' or len(data)<32:raise ValueError('Invalid shader container: '+path)
 lines.append('inline const unsigned char kGIOwned'+name+'[]={'+','.join('0x%02x'%b for b in data)+'};')
out=pathlib.Path(sys.argv[1]);out.parent.mkdir(parents=True,exist_ok=True);out.write_text('\n'.join(lines)+'\n')
