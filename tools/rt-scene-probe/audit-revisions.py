"""Offline R401 patch/notification evidence; does not prove all mutation writers."""
import argparse, hashlib, json, struct
import pefile
parser=argparse.ArgumentParser();parser.add_argument('renderer');args=parser.parse_args()
raw=open(args.renderer,'rb').read()
expected='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
assert hashlib.sha256(raw).hexdigest()==expected,'unsupported renderer'
p=pefile.PE(data=raw);b=p.get_memory_mapped_image()
entries={0x1504c0:'44894c2420',0x150aa0:'40574883ec40',0x1b6910:'405556574883ec30'}
for rva,hexbytes in entries.items():assert b[rva:rva+len(bytes.fromhex(hexbytes))].hex()==hexbytes
edges={0x9de96:'setProperties',0x9deb6:'setCullMode',0x1b66e3:'material constructor',0x1c9757:'material clone',0x1b70ba:'material load',0x1c9cf6:'instance properties enable',0x1c9d04:'instance properties disable'}
for rva,label in edges.items():
 assert b[rva] in [0xe8,0xe9],label
 assert rva+5+struct.unpack_from('<i',b,rva+1)[0]==0x1b6910,label
print(json.dumps(dict(renderer_sha256=expected,entry_bytes={hex(k):v for k,v in entries.items()},notification_edges={hex(k):v for k,v in edges.items()},complete_writer_coverage=False,cache_reuse=False),indent=2))
