from pathlib import Path
root=Path(__file__).resolve().parents[1]
header='#pragma once\n'
for name,symbol in [('translate','Translate'),('clear','Clear'),('signal','Signal'),('signal_before','SignalBefore'),('raw','Raw')]:
    data=(root/f'build/rt_spot_capture_{name}.dxil').read_bytes()
    if data[:4]!=b'DXBC':raise ValueError('Invalid shader container')
    header+=f'static const unsigned char kSpotCapture{symbol}Shader[]={{\n'+','.join(f'0x{x:02x}' for x in data)+'\n};\n'
(root/'build/rt_spot_capture_table.h').write_text(header)
