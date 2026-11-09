"""Reproduce sample routing evidence from the supported renderer (not redistributed)."""
from pathlib import Path
import sys, hashlib, json
root=Path(__file__).resolve().parent
sys.path.insert(0,str(root.parent/'provider-auth/tests'))
from pe_tools import PE
p=PE(sys.argv[1])
assert hashlib.sha256(p.data).hexdigest()=='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
checks={
 'specular_registration':(0x60f0,0x613c,['0x18062ee90','0x180913730','0x1809137e0']),
 'menu_sample_routing':(0x137b95,0x137c9d,['[rdi+0x6f4]','ecx,0x1','eax,0x10','0x1809137e0','0x180913bd0','0x180913c90','ecx,0x6','eax,0x4','cmovge eax,r12d','0x180913630']),
 'filter_high_count_value':(0x137608,0x13760e,['r12d,0x2']),
 'specular_allocation':(0x128ff4,0x129034,['0x1809137e0','[rbp+0x100],r9d','0x1800a2cd0']),
 'specular_provider':(0x1292cb,0x12931f,['0x18062fed8','[rbp+0x100]','0x1805dff90']),
 'gi_allocation':(0x12ba10,0x12ba75,['0x180913b20','0x18002dac0','[rbp+0x408],eax','0x1800a2cd0']),
 'specular_filter_option':(0x6050,0x609c,['0x180913580','0x180913630']),
}
for name,(a,b,needles) in checks.items():
 text=p.disasm(a,b)
 for needle in needles:assert needle in text,(name,needle)
 (root/'evidence'/f'{name}.asm').write_text(text)
assert p.string(0x62fed8)=='g_uRTReflectionRayCount'
assert p.string(0x62ee90)=='rt:Sample Count Specular'
assert p.string(0x62ef68)=='rt:Sample Count Diffuse'
assert p.string(0x62ef50)=='rt:Sample Count Shadow'
(root/'evidence/audit.json').write_text(json.dumps({'renderer_sha256':hashlib.sha256(p.data).hexdigest(),'checks':list(checks),'scope':'Static routing only; option snapshots and GI producer logs are runtime evidence.'},indent=2)+'\n')
print('PASS: native menu integer routes to specular/diffuse/shadow; separate filter-count threshold; producer allocation inputs')
