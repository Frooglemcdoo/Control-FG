"""Embed the reference evaluator; keep hash-matched native byte length explicit."""
import base64, pathlib, sys
root=pathlib.Path(__file__).resolve().parents[1]
native=base64.b64decode((root/'research/gi-source-r267/gi-6.dxbc.b64').read_bytes())
shader=pathlib.Path(sys.argv[1]).read_bytes()
if shader[:4]!=b'DXBC':raise ValueError('Expected DXIL container')
pathlib.Path(sys.argv[2]).parent.mkdir(parents=True,exist_ok=True)
pathlib.Path(sys.argv[2]).write_text('#pragma once\ninline constexpr size_t kNativeEvaluatorSize='+str(len(native))+';\ninline const unsigned char kEvaluatorShader[]={\n'+','.join('0x%02x'%x for x in shader)+'\n};\n')
