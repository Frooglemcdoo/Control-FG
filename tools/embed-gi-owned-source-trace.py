import hashlib,pathlib,sys
source=pathlib.Path(sys.argv[1]).read_bytes()
if source[:4]!=b'DXBC':raise ValueError('Expected compiled DXIL')
pathlib.Path(sys.argv[2]).write_text('#pragma once\nnamespace gi_owned_source_trace_shader {\ninline constexpr char sha[]="'+hashlib.sha256(source).hexdigest()+'";\ninline const unsigned char bytes[]={'+','.join('0x%02x'%x for x in source)+'};\n}\n')
