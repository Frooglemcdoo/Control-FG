"""R210: tighten native GI history rejection, preserving every other token."""
import base64,hashlib,struct
from pathlib import Path
from dxbc_transform import transform_shex
SOURCE_SHA='d54d94fdbb783796cc33bc78ab20dde687d90d9cba6f24c8092a6b177eec5d15'
def generate(data):
 if hashlib.sha256(data).hexdigest()!=SOURCE_SHA:raise ValueError('Unknown native GI shader')
 def patch(w):
  for start,end,old,new in [(311,318,.03,.01),(356,366,40.,400.)]:
   bits=struct.unpack('<I',struct.pack('<f',old))[0]
   matches=[i for i in range(start+1,end) if w[i]==bits]
   if len(matches)!=1:raise ValueError('Unexpected GI immediate layout')
   w[matches[0]]=struct.unpack('<I',struct.pack('<f',new))[0]
 return transform_shex(data,patch)
if __name__=='__main__':
 root=Path(__file__).resolve().parents[1]
 source=base64.b64decode((root/'research/gi-temporal-r208/diffuse-temporal.dxbc.b64').read_text())
 result=generate(source);out=root/'build';out.mkdir(exist_ok=True)
 (out/'gi-motion-history.dxbc').write_bytes(result)
 (out/'gi_motion_history.generated.h').write_text('#pragma once\ninline constexpr unsigned char kGIMotionHistoryShader[]={\n'+','.join(map(str,result))+'\n};\n')
 print('GI motion shader SHA256',hashlib.sha256(result).hexdigest())
