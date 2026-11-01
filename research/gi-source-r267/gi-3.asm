2: dcl_global_flags raw:
3: dcl_constant_buffer raw:0x308e46,0x0,0x0,0x0,0x37,0x0
10: dcl_constant_buffer raw:0x308e46,0x1,0x1,0x1,0x12,0x0
17: dcl_constant_buffer raw:0x308e46,0x2,0x2,0x2,0x2,0x0
24: dcl_constant_buffer raw:0x308e46,0x3,0x3,0x3,0x600,0x0
31: dcl_constant_buffer raw:0x308e46,0x4,0x4,0x4,0x1,0x0
38: dcl_sampler raw:0x306e46,0x0,0x5,0x5,0x1
44: dcl_sampler raw:0x306e46,0x1,0x6,0x6,0x1
50: dcl_sampler raw:0x306e46,0x2,0x8,0x8,0x1
56: dcl_sampler raw:0x306e46,0x3,0xb,0xb,0x1
62: dcl_sampler raw:0x306e46,0x4,0x13,0x13,0x1
68: dcl_resource raw:0x307e46,0x0,0x0,0x0,0x5555,0x0
75: dcl_resource raw:0x307e46,0x1,0x1,0x1,0x5555,0x0
82: dcl_resource raw:0x307e46,0x2,0x2,0x2,0x5555,0x0
89: dcl_resource raw:0x307e46,0x3,0x3,0x3,0x5555,0x0
96: dcl_resource_structured raw:0x307e46,0x4,0x4,0x4,0x8,0x0
103: dcl_resource_structured raw:0x307e46,0x5,0x5,0x5,0x150,0x0
110: dcl_resource raw:0x307e46,0x6,0x6,0x6,0x5555,0x0
117: dcl_resource_structured raw:0x307e46,0x7,0x7,0x7,0xa0,0x0
124: dcl_resource raw:0x307e46,0x8,0x8,0x8,0x5555,0x0
131: dcl_resource raw:0x307e46,0x9,0x9,0x9,0x5555,0x0
138: dcl_resource raw:0x307e46,0xa,0xa,0xa,0x5555,0x0
145: dcl_resource_structured raw:0x307e46,0xb,0xb,0xb,0x140,0x0
152: dcl_resource raw:0x307e46,0xc,0xc,0xc,0x5555,0x0
159: dcl_resource_structured raw:0x307e46,0xd,0xd,0xd,0x240,0x0
166: dcl_resource raw:0x307e46,0xe,0xe,0xe,0x5555,0x0
173: dcl_resource_structured raw:0x307e46,0xf,0xf,0xf,0x50,0x0
180: dcl_resource_structured raw:0x307e46,0x10,0x10,0x10,0x4,0x0
187: dcl_resource_structured raw:0x307e46,0x11,0x11,0x11,0x10,0x0
194: dcl_resource raw:0x307e46,0x12,0x12,0x17,0x5555,0x0
201: dcl_resource raw:0x307e46,0x13,0x18,0x18,0x4444,0x0
208: dcl_resource raw:0x307e46,0x14,0x19,0x19,0x4444,0x0
215: dcl_resource raw:0x307e46,0x15,0x1a,0x1a,0x4444,0x0
222: dcl_resource raw:0x307e46,0x16,0x1b,0x1b,0x5555,0x0
229: dcl_resource raw:0x307e46,0x17,0x1c,0x1c,0x5555,0x0
236: dcl_resource_structured raw:0x307e46,0x18,0x1d,0x1d,0x4,0x0
243: dcl_resource_structured raw:0x307e46,0x19,0x1e,0x1e,0x4,0x0
250: dcl_resource raw:0x307e46,0x1a,0x0,0xffffffff,0x5555,0x1
257: dcl_unordered_access_view_typed raw:0x31ee46,0x0,0x0,0x0,0x5555,0x0
264: dcl_unordered_access_view_typed raw:0x31ee46,0x1,0x1,0x1,0x5555,0x0
271: dcl_unordered_access_view_typed raw:0x31ee46,0x2,0x2,0x2,0x5555,0x0
278: dcl_input raw:0x24000
280: dcl_input raw:0x21032
282: dcl_temps raw:0x23
284: dcl_thread_group raw:0x8,0x10,0x1
288: ld_structured r[0].x, l(0), l(0), t[24][29].xxxx
298: utof r[0].y, type36.x
302: utof r[0].zw, type33.xxxy
306: mul r[0].w, r[0].w, cb[0][0][0].x
315: mad r[0].z, r[0].w, l(0.125), r[0].z
324: mad r[0].y, r[0].z, l(128), r[0].y
333: ftou r[0].y, r[0].y
338: uge r[0].x, r[0].y, r[0].x
345: if r[0].x
348: ret 
349: endif 
350: ld_structured r[0].x, r[0].y, l(0), t[25][30].xxxx
360: and r[1].x, r[0].x, l(9.18340949e-41)
367: ushr r[1].y, r[0].x, l(16)
374: mov r[1].w, l(0)
379: ld r[0].x, r[1].xyww, t[21][26].xyzw
387: ine r[0].x, r[0].x, l(9.18326936e-41)
394: if r[0].x
397: utof r[0].xy, r[1].xyxx
402: add r[0].xy, r[0].xyxx, l(0.5,0.5,0,0)
412: mul r[0].xy, r[0].xyxx, cb[0][0][1].zwzz
421: ld r[0].z, r[1].xyww, t[3][3].yzxw
429: ld r[0].w, r[1].xyww, t[2][2].yzwx
437: mad r[0].xy, r[0].xyxx, l(2,-2,0,0), l(-1,1,0,0)
452: mul r[2].xyzw, r[0].yyyy, cb[0][0][15].xyzw
461: mad r[2].xyzw, cb[0][0][14].xyzw, r[0].xxxx, r[2].xyzw
472: mad r[2].xyzw, cb[0][0][16].xyzw, r[0].zzzz, r[2].xyzw
483: add r[2].xyzw, r[2].xyzw, cb[0][0][17].xyzw
492: rcp r[2].w, r[2].w
497: mul r[3].xyz, r[2].wwww, r[2].xyzx
504: dp3 r[3].w, r[3].xyzx, r[3].xyzx
511: rsq r[3].w, r[3].w
516: mul r[3].xyz, r[3].wwww, r[3].xyzx
523: mul r[4].xyzw, r[0].yyyy, cb[0][0][23].xyzw
532: mad r[4].xyzw, cb[0][0][22].xyzw, r[0].xxxx, r[4].xyzw
543: mad r[4].xyzw, cb[0][0][24].xyzw, r[0].zzzz, r[4].xyzw
554: add r[4].xyzw, r[4].xyzw, cb[0][0][25].xyzw
563: rcp r[0].x, r[4].w
568: mul r[5].xyz, r[0].xxxx, r[4].xyzx
575: mad r[0].xyz, r[4].xyzx, r[0].xxxx, -cb[0][0][54].xyzx
587: dp3 r[3].w, r[0].xyzx, r[0].xyzx
594: rsq r[3].w, r[3].w
599: mul r[0].xyz, r[0].xyzx, r[3].wwww
606: ld r[4].xyzw, r[1].xyww, t[0][0].xzyw
614: mad r[3].w, r[4].w, l(255), l(0.5)
623: ftou r[3].w, r[3].w
628: and r[3].w, r[3].w, l(254)
635: utof r[6].z, r[3].w
640: mov r[6].xy, r[4].xzxx
645: mad r[6].xyz, r[6].xyzx, l(255,255,1,0), l(0.5,0.5,0.5,0)
660: round_ni r[6].xyz, r[6].xyzx
665: ftoi r[6].xyz, r[6].xyzx
670: ubfe r[7].xyzw, l(3,4,3,4), l(1,4,1,4), r[6].zzzz
685: bfi r[6].xyzw, l(29,28,29,28), l(3,4,3,4), r[6].xyxy, r[7].xyzw
702: itof r[6].xyzw, r[6].xyzw
707: mad r[6].xyzw, r[6].xyzw, l(0.00097703957,0.000488400517,0.00097703957,0.000488400517), l(-1,-1,-1,-1)
722: mul r[6].xyzw, r[6].xyzw, l(1.29999995,1.29999995,2.5999999,2.5999999)
732: dp2 r[3].w, r[6].xyxx, r[6].xyxx
739: add r[6].xy, r[3].wwww, l(1,-1,0,0)
749: div r[6].xyz, r[6].zwyz, r[6].xxxx
756: mul r[7].xyz, r[6].yyyy, cb[0][0][7].xyzx
765: mad r[7].xyz, cb[0][0][6].xyzx, r[6].xxxx, r[7].xyzx
776: mad r[7].xyz, cb[0][0][8].xyzx, r[6].zzzz, r[7].xyzx
787: ld r[8].xyz, r[1].xyww, t[1][1].yzwx
795: mul r[4].zw, r[8].yyyz, l(0,0,255,255)
805: ftou r[4].zw, r[4].zzzw
810: ishl r[3].w, r[4].z, l(8)
817: or r[3].w, r[4].w, r[3].w
824: dp3 r[4].x, r[6].xyzx, -r[3].xyzx
832: ld_structured r[3].x, r[3].w, l(0), t[4][4].xxxx
842: ushr r[3].y, r[3].x, l(24)
849: ubfe r[3].xz, l(8,0,8,0), l(16,0,8,0), r[3].xxxx
864: utof r[6].xyz, r[3].yxzy
869: mul r[3].xyz, r[6].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
879: ld_structured r[3].w, r[3].w, l(0), t[4][4].xxxx
889: and r[3].w, r[3].w, l(255)
896: dp3 r[4].z, r[3].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
906: add r[4].w, r[4].z, l(9.99999997e-07)
913: rcp r[4].w, r[4].w
918: mul r[4].z, r[4].z, l(10000)
925: min r[4].z, r[4].z, l(1)
932: mad r[6].xyz, r[3].xyzx, r[4].wwww, l(-1,-1,-1,0)
944: mad r[6].xyz, r[4].zzzz, r[6].xyzx, l(1,1,1,0)
956: mul r[6].xyz, r[8].xxxx, r[6].xyzx
963: movc r[3].xyz, r[3].wwww, r[3].xyzx, r[6].xyzx
972: mul r[0].xyz, r[0].wwww, r[0].xyzx
979: mad r[0].xyz, -r[0].xyzx, l(0.00100000005,0.00100000005,0.00100000005,0), r[5].xyzx
992: mul r[6].xyz, r[0].wwww, r[7].xyzx
999: mad r[0].xyz, r[6].xyzx, l(0.00100000005,0.00100000005,0.00100000005,0), r[0].xyzx
1011: sample_l r[4].xyz, r[4].xyxx, t[14][14].xyzw, s[1][6], l(0)
1024: mul r[3].w, cb[1][1][15].w, l(0.318309873)
1033: ld_structured r[6].xyz, l(0), l(0), t[13][13].xyzx
1043: ld_structured r[8].xyz, l(0), l(16), t[13][13].xyzx
1053: ld_structured r[9].xy, l(0), l(32), t[13][13].xyxx
1063: ld_structured r[9].zw, l(0), l(48), t[13][13].xxxy
1073: ld_structured r[10].xy, l(0), l(64), t[13][13].xyxx
1083: ld_structured r[11].xw, l(0), l(80), t[13][13].xxxy
1093: ld_structured r[4].w, l(0), l(96), t[13][13].xxxx
1103: ld_structured r[5].w, l(0), l(104), t[13][13].xxxx
1113: and r[5].w, r[5].w, l(1)
1120: mad r[3].xyz, r[3].xyzx, r[4].xxxx, r[4].yyyy
1129: add r[3].xyz, -r[3].xyzx, l(1,1,1,0)
1140: max r[3].xyz, r[3].xyzx, l(0,0,0,0)
1150: mul r[3].xyz, r[4].zzzz, r[3].xyzx
1157: mov r[12].w, l(1)
1162: mov r[13].z, l(0)
1167: mov r[14].xy, r[9].xzxx
1172: mov r[14].z, r[10].x
1177: mov r[14].w, r[11].x
1182: mov r[15].w, l(1)
1187: mov r[11].xy, r[9].ywyy
1192: mov r[11].z, r[10].y
1197: mov r[9].w, l(1)
1202: mov r[10].w, l(1)
1207: mov r[4].xyz, l(0,0,0,0)
1215: mov r[6].w, cb[1][1][16].z
1222: mov r[7].w, l(0)
1227: mov r[1].z, l(0)
1232: loop 
1233: ige r[8].w, r[1].z, cb[4][4][0].z
1242: breakc r[8].w
1245: ld r[8].w, r[1].xyzw, t[21][26].yzwx
1253: ld r[16].xyzw, r[1].xyzw, t[23][28].xyzw
1261: mul r[17].xyz, r[16].yyyy, cb[0][0][7].xyzx
1270: mad r[17].xyz, cb[0][0][6].xyzx, r[16].xxxx, r[17].xyzx
1281: mad r[17].xyz, cb[0][0][8].xyzx, r[16].zzzz, r[17].xyzx
1292: add r[17].xyz, r[17].xyzx, cb[0][0][9].xyzx
1301: ld r[18].xyzw, r[1].xyzw, t[22][27].xyzw
1309: add r[19].xyz, -r[0].xyzx, r[17].xyzx
1317: dp3 r[17].w, r[19].xyzx, r[19].xyzx
1324: rsq r[17].w, r[17].w
1329: mul r[19].xyz, r[17].wwww, r[19].xyzx
1336: ieq r[17].w, r[8].w, l(9.18340949e-41)
1343: mad r[20].xyz, cb[4][4][0].yyyy, r[19].xyzx, r[5].xyzx
1354: movc r[12].xyz, r[17].wwww, r[20].xyzx, r[17].xyzx
1363: movc r[20].xyz, r[17].wwww, r[7].xyzx, r[18].xyzx
1372: mov r[21].xyz, l(0,0,0,0)
1380: mov r[19].w, l(0)
1385: mov r[20].w, l(0)
1390: loop 
1391: uge r[21].w, r[20].w, cb[2][2][0].w
1400: breakc r[21].w
1403: ishl r[21].w, r[20].w, l(2)
1410: dp4 r[22].x, cb[3][3][r[21].w].xyzw, r[12].xyzw
1420: dp4 r[22].y, cb[3][3][1+r[21].w].xyzw, r[12].xyzw
1431: dp4 r[22].z, cb[3][3][2+r[21].w].xyzw, r[12].xyzw
1442: ge r[23].xyz, r[22].xyzx, l(0,0,0,0)
1452: and r[22].w, r[23].y, r[23].x
1459: and r[22].w, r[23].z, r[22].w
1466: utof r[23].xyz, cb[3][3][3+r[21].w].xyzx
1475: lt r[24].xyz, r[22].xyzx, r[23].xyzx
1482: and r[23].w, r[24].y, r[24].x
1489: and r[23].w, r[24].z, r[23].w
1496: and r[22].w, r[22].w, r[23].w
1503: if r[22].w
1506: ld_structured r[24].xyz, r[20].w, l(0), t[15][15].xyzx
1516: ld_structured r[25].xyz, r[20].w, l(12), t[15][15].xyzx
1526: ld_structured r[26].xyzw, r[20].w, l(24), t[15][15].xyzw
1536: ld_structured r[27].xyzw, r[20].w, l(40), t[15][15].xyzw
1546: ld_structured r[28].xyz, r[20].w, l(56), t[15][15].xyzx
1556: mul_sat r[22].w, r[22].x, r[26].w
1563: add r[23].xyz, -r[22].xyzx, r[23].xyzx
1571: mul_sat r[23].x, r[26].w, r[23].x
1578: mul r[22].w, r[22].w, r[23].x
1585: mul_sat r[23].xw, r[22].yyyz, r[27].xxxy
1592: mul r[22].w, r[22].w, r[23].x
1599: mul_sat r[23].xy, r[27].xyxx, r[23].yzyy
1606: mul r[22].w, r[22].w, r[23].x
1613: mul r[22].w, r[23].w, r[22].w
1620: mul r[22].w, r[23].y, r[22].w
1627: ine r[23].x, r[28].x, l(nan)
1634: if r[23].x
1637: round_ni r[23].xyz, r[22].xyzx
1642: add r[22].xyz, r[22].xyzx, -r[23].xyzx
1650: ftou r[23].xyz, r[23].xyzx
1655: imad r[23].y, r[23].z, cb[3][3][3+r[21].w].y, r[23].y
1668: imad r[21].w, r[23].y, cb[3][3][3+r[21].w].x, r[23].x
1681: iadd r[21].w, r[27].z, r[21].w
1688: ld_structured r[21].w, r[21].w, l(0), t[16][16].xxxx
1698: mov r[23].xyz, l(0,0,0,0)
1706: mov r[27].xyz, r[22].xyzx
1711: mov r[23].w, r[21].w
1716: mov r[24].w, r[28].y
1721: loop 
1722: and r[25].w, r[23].w, l(-0)
1729: ine r[25].w, r[25].w, l(0)
1736: ge r[26].w, l(9.99999975e-06), r[24].w
1743: or r[25].w, r[25].w, r[26].w
1750: if r[25].w
1753: and r[25].w, r[23].w, l(nan)
1760: iadd r[25].w, r[28].x, r[25].w
1767: and r[29].x, r[25].w, l(127)
1774: ubfe r[29].y, l(7), l(7), r[25].w
1783: ushr r[29].z, r[25].w, l(14)
1790: ishl r[29].xyz, r[29].xyzx, l(2,2,2,0)
1800: utof r[29].xyz, r[29].xyzx
1805: mad r[29].xyz, r[27].xyzx, l(3,3,3,0), r[29].xyzx
1817: add r[23].xyz, r[29].xyzx, l(0.5,0.5,0.5,0)
1827: break 
1828: endif 
1829: iadd r[25].w, r[27].w, r[23].w
1836: ld_structured r[29].xyzw, r[25].w, l(0), t[17][17].xyzw
1846: mul r[30].xyz, r[27].xyzx, l(3,3,3,0)
1856: round_ni r[30].xyz, r[30].xyzx
1861: ftou r[31].xyz, r[30].xyzx
1866: utof r[31].xz, r[31].zzxz
1871: imul null, r[25].w, r[31].y, l(3)
1879: utof r[25].w, r[25].w
1884: mad r[25].w, r[31].x, l(9), r[25].w
1893: add r[25].w, r[31].z, r[25].w
1900: ftou r[25].w, r[25].w
1905: ishl r[26].w, l(1), r[25].w
1912: and r[28].w, r[26].w, r[29].x
1919: if r[28].w
1922: iadd r[28].w, r[28].x, r[23].w
1929: and r[31].x, r[28].w, l(127)
1936: ubfe r[31].y, l(7), l(7), r[28].w
1945: ushr r[31].z, r[28].w, l(14)
1952: ishl r[31].xyz, r[31].xyzx, l(2,2,2,0)
1962: utof r[31].xyz, r[31].xyzx
1967: mad r[31].xyz, r[27].xyzx, l(3,3,3,0), r[31].xyzx
1979: add r[23].xyz, r[31].xyzx, l(0.5,0.5,0.5,0)
1989: break 
1990: endif 
1991: mad r[27].xyz, r[27].xyzx, l(3,3,3,0), -r[30].xyzx
2004: and r[26].w, r[26].w, r[29].y
2011: ubfe r[28].w, r[25].w, l(0), r[29].y
2020: countbits r[28].w, r[28].w
2025: iadd r[28].w, r[28].w, r[29].w
2032: or r[28].w, r[28].w, l(-0)
2039: not r[29].y, r[29].y
2044: and r[29].x, r[29].y, r[29].x
2051: ubfe r[25].w, r[25].w, l(0), r[29].x
2060: countbits r[25].w, r[25].w
2065: iadd r[25].w, r[25].w, r[29].z
2072: movc r[23].w, r[26].w, r[28].w, r[25].w
2081: mul r[24].w, r[24].w, l(0.333333343)
2088: mov r[23].xyz, l(0,0,0,0)
2096: endloop 
2097: else 
2098: mov r[23].xyz, l(0,0,0,0)
2106: endif 
2107: mul r[22].xyz, r[23].xyzx, cb[2][2][0].xyzx
2116: mov r[27].x, r[24].x
2121: mov r[27].y, r[25].x
2126: mov r[27].z, r[26].x
2131: dp3 r[27].x, r[27].xyzx, r[20].xyzx
2138: mov r[29].x, r[24].y
2143: mov r[29].y, r[25].y
2148: mov r[29].z, r[26].y
2153: dp3 r[27].y, r[29].xyzx, r[20].xyzx
2160: mov r[26].x, r[24].z
2165: mov r[26].y, r[25].z
2170: dp3 r[27].z, r[26].xyzx, r[20].xyzx
2177: mov_sat r[24].xyz, r[27].xyzx
2182: mul r[24].xyz, r[24].xyzx, r[24].xyzx
2189: mov_sat r[27].xyz, -r[27].xyzx
2195: mul r[25].xyz, r[27].xyzx, r[27].xyzx
2202: lt r[26].xyz, l(0,0,0,0), r[24].xyzx
2212: if r[26].x
2215: sample_l r[27].xyz, r[22].xyzx, t[18][18].xyzw, s[1][6], l(0)
2228: mul r[27].xyz, r[24].xxxx, r[27].xyzx
2235: else 
2236: mov r[27].xyz, l(0,0,0,0)
2244: endif 
2245: lt r[29].xyz, l(0,0,0,0), r[25].xyzx
2255: if r[29].x
2258: sample_l r[30].xyz, r[22].xyzx, t[18][19].xyzw, s[1][6], l(0)
2271: mad r[27].xyz, r[30].xyzx, r[25].xxxx, r[27].xyzx
2280: endif 
2281: if r[26].y
2284: sample_l r[26].xyw, r[22].xyzx, t[18][20].xywz, s[1][6], l(0)
2297: mad r[27].xyz, r[26].xywx, r[24].yyyy, r[27].xyzx
2306: endif 
2307: if r[29].y
2310: sample_l r[24].xyw, r[22].xyzx, t[18][21].xywz, s[1][6], l(0)
2323: mad r[27].xyz, r[24].xywx, r[25].yyyy, r[27].xyzx
2332: endif 
2333: if r[26].z
2336: sample_l r[24].xyw, r[22].xyzx, t[18][22].xywz, s[1][6], l(0)
2349: mad r[27].xyz, r[24].xywx, r[24].zzzz, r[27].xyzx
2358: endif 
2359: if r[29].z
2362: sample_l r[22].xyz, r[22].xyzx, t[18][23].xyzw, s[1][6], l(0)
2375: mad r[27].xyz, r[22].xyzx, r[25].zzzz, r[27].xyzx
2384: endif 
2385: ieq r[21].w, r[28].x, l(nan)
2392: movc r[22].xyz, r[21].wwww, l(1,0.349999994,0,0), r[27].xyzx
2404: mad r[21].w, r[22].w, l(-2), l(3)
2413: mul r[23].w, r[22].w, r[22].w
2420: mul r[21].w, r[21].w, r[23].w
2427: add r[23].w, r[28].z, l(-1)
2434: mad r[21].w, r[21].w, r[23].w, l(1)
2443: mul r[23].w, r[21].w, r[22].w
2450: mad r[21].xyz, r[22].xyzx, r[23].wwww, r[21].xyzx
2459: mad r[19].w, r[22].w, r[21].w, r[19].w
2468: endif 
2469: iadd r[20].w, r[20].w, l(1)
2476: endloop 
2477: lt r[12].x, l(0), r[19].w
2484: div r[20].xyz, r[21].xyzx, r[19].wwww
2491: movc r[12].xyz, r[12].xxxx, r[20].xyzx, r[21].xyzx
2500: add r[12].xyz, r[12].xyzx, cb[2][2][1].xyzx
2509: mad r[20].xyz, r[2].xyzx, r[2].wwww, -r[16].xyzx
2519: dp3 r[20].x, r[20].xyzx, r[20].xyzx
2526: sqrt r[20].x, r[20].x
2531: movc r[20].x, r[17].w, cb[4][4][0].x, r[20].x
2542: add r[7].w, r[7].w, r[20].x
2549: if r[17].w
2552: mul r[20].yzw, r[3].wwww, r[12].xxyz
2559: else 
2560: ld_structured r[22].xyz, r[8].w, l(52), t[5][5].xyzx
2570: imul null, r[17].w, r[8].w, l(35)
2578: mov r[24].x, r[18].w
2583: mov r[24].y, r[16].w
2588: sample_l r[25].xyz, r[24].xyxx, t[26][r[17].w].xyzw, s[0][5], l(0)
2603: mul r[22].xyz, r[22].xyzx, r[25].xyzx
2610: dp3 r[16].w, r[22].xyzx, r[22].xyzx
2617: ge r[16].w, r[16].w, l(9.99999975e-05)
2624: if r[16].w
2627: mul_sat r[16].w, r[20].x, l(50)
2634: mul r[12].xyz, r[12].xyzx, r[16].wwww
2641: ld_structured r[24].zw, r[8].w, l(68), t[5][5].xxxy
2651: imad r[25].xy, r[8].wwww, l(35,35,0,0), l(2,4,0,0)
2666: sample_l r[16].w, r[24].xyxx, t[26][r[25].x].yzwx, s[4][19], l(0)
2681: add r[17].w, -r[24].z, r[24].w
2689: mad r[26].y, r[16].w, r[17].w, r[24].z
2698: sample_l r[16].w, r[24].xyxx, t[26][r[25].y].yzwx, s[4][19], l(0)
2713: ld_structured r[8].w, r[8].w, l(0), t[4][4].xxxx
2723: ushr r[17].w, r[8].w, l(24)
2730: ubfe r[24].xy, l(8,8,0,0), l(16,8,0,0), r[8].wwww
2745: utof r[25].x, r[17].w
2750: utof r[25].yz, r[24].xxyx
2755: mul r[24].xyz, r[16].wwww, r[25].xyzx
2762: mul r[24].xyz, r[24].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
2772: dp3 r[8].w, r[24].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
2782: dp3 r[26].x, r[18].xyzx, -r[19].xyzx
2790: sample_l r[19].xyz, r[26].xyxx, t[14][14].xyzw, s[1][6], l(0)
2803: mad r[24].xyz, r[24].xyzx, r[19].xxxx, r[19].yyyy
2812: add r[24].xyz, -r[24].xyzx, l(1,1,1,0)
2823: mul r[24].xyz, r[24].xyzx, cb[1][1][15].wwww
2832: mul r[19].xyz, r[19].zzzz, r[24].xyzx
2839: mul r[12].xyz, r[12].xyzx, r[19].xyzx
2846: mul r[19].xyz, r[17].yyyy, cb[0][0][3].xyzx
2855: mad r[17].xyw, cb[0][0][2].xyxz, r[17].xxxx, r[19].xyxz
2866: mad r[17].xyz, cb[0][0][4].xyzx, r[17].zzzz, r[17].xywx
2877: add r[15].xyz, r[17].xyzx, cb[0][0][5].xyzx
2886: mul r[17].xyz, r[18].yyyy, cb[0][0][3].xyzx
2895: mad r[17].xyz, cb[0][0][2].xyzx, r[18].xxxx, r[17].xyzx
2906: mad r[17].xyz, cb[0][0][4].xyzx, r[18].zzzz, r[17].xyzx
2917: add r[16].xyz, r[16].xyzx, l(150,75,75,0)
2927: mul r[16].xyz, r[16].xyzx, l(0.0533333346,0.0533333346,0.0533333346,0)
2937: round_ni r[16].xyz, r[16].xyzx
2942: max r[16].xyz, r[16].xyzx, l(0,0,0,0)
2952: min r[16].xyz, r[16].xyzx, l(15,7,11,0)
2962: ftoi r[16].xyz, r[16].xyzx
2967: uge r[16].w, r[6].w, l(1)
2974: if r[16].w
2977: dp4 r[18].x, r[14].xyzw, r[15].xyzw
2984: dp4 r[18].y, r[11].xyzw, r[15].xyzw
2991: sample_l r[16].w, r[18].xyxx, t[12][12].yzwx, s[2][8], l(0)
3004: add r[16].w, r[16].w, l(-1)
3011: mad r[16].w, r[4].w, r[16].w, l(1)
3020: mul r[18].xyz, r[8].xyzx, r[16].wwww
3027: movc r[18].xyz, r[5].wwww, r[18].xyzx, r[8].xyzx
3036: dp3 r[16].w, -r[15].xyzx, -r[15].xyzx
3045: rsq r[16].w, r[16].w
3050: mul r[19].xyz, -r[15].xyzx, r[16].wwww
3058: mad r[24].xyz, -r[15].xyzx, r[16].wwww, r[6].xyzx
3068: dp3 r[16].w, r[24].xyzx, r[24].xyzx
3075: rsq r[16].w, r[16].w
3080: mul r[24].xyz, r[16].wwww, r[24].xyzx
3087: dp3 r[16].w, r[19].xyzx, r[24].xyzx
3094: dp3_sat r[17].w, r[17].xyzx, r[6].xyzx
3101: add r[18].w, -r[8].w, l(1)
3109: add r[16].w, -abs(r[16].w), l(1)
3117: max r[16].w, r[16].w, l(0)
3124: mul r[19].x, r[16].w, r[16].w
3131: mul r[19].x, r[19].x, r[19].x
3138: mul r[16].w, r[16].w, r[19].x
3145: mad r[16].w, r[18].w, r[16].w, r[8].w
3154: add_sat r[16].w, -r[16].w, l(1)
3162: mul r[16].w, r[17].w, r[16].w
3169: mul r[16].w, r[16].w, l(0.318309873)
3176: mul r[18].xyz, r[18].xyzx, r[16].wwww
3183: else 
3184: mov r[18].xyz, l(0,0,0,0)
3192: endif 
3193: imad r[13].x, r[16].x, l(12), r[16].z
3202: ishl r[13].y, r[16].y, l(8)
3209: ld r[16].x, r[13].xyzz, t[19][24].xyzw
3217: dp3 r[16].y, -r[15].xyzx, -r[15].xyzx
3226: mad r[10].xyz, r[17].xyzx, l(0.0700000003,0.0700000003,0.0700000003,0), r[15].xyzx
3238: rsq r[16].y, r[16].y
3243: mul r[16].yzw, -r[15].xxyz, r[16].yyyy
3251: add r[17].w, -r[8].w, l(1)
3259: mov r[19].xyz, r[18].xyzx
3264: mov r[18].w, l(0)
3269: mov r[20].x, r[16].x
3274: loop 
3275: ine r[21].w, r[20].x, l(9.18340949e-41)
3282: ilt r[22].w, r[18].w, l(256)
3289: and r[21].w, r[21].w, r[22].w
3296: breakc r[21].w
3299: ld_structured r[24].xyzw, r[20].x, l(0), t[7][7].xyzw
3309: ld_structured r[25].xyzw, r[20].x, l(16), t[7][7].xyzw
3319: ld_structured r[26].xyzw, r[20].x, l(36), t[7][7].xyzw
3329: ld_structured r[27].xyzw, r[20].x, l(52), t[7][7].xyzw
3339: ld_structured r[21].w, r[20].x, l(144), t[7][7].xxxx
3349: ld_structured r[22].w, r[20].x, l(156), t[7][7].xxxx
3359: add r[24].xyz, -r[15].xyzx, r[24].xyzx
3367: dp3 r[23].w, r[24].xyzx, r[24].xyzx
3374: rsq r[28].x, r[23].w
3379: mul r[28].yzw, r[24].xxyz, r[28].xxxx
3386: sqrt r[29].x, r[23].w
3391: add r[29].y, r[25].w, -r[29].x
3399: mul r[25].w, r[25].w, l(0.200000003)
3406: div_sat r[25].w, r[29].y, r[25].w
3413: mad r[29].y, r[25].w, l(-2), l(3)
3422: mul r[25].w, r[25].w, r[25].w
3429: mul r[25].w, r[25].w, r[29].y
3436: mad r[21].w, r[21].w, r[21].w, r[23].w
3445: max r[21].w, r[21].w, l(9.99999975e-05)
3452: mul r[21].w, r[21].w, l(3.14159274)
3459: rcp r[21].w, r[21].w
3464: add r[21].w, -r[26].x, r[21].w
3472: max r[21].w, r[21].w, l(0)
3479: mul r[21].w, r[25].w, r[21].w
3486: mul r[25].xyz, r[21].wwww, r[25].xyzx
3493: and r[29].yz, r[22].wwww, l(0,4,2,0)
3503: mov r[30].x, r[26].w
3508: mov r[30].yz, r[27].xxyx
3513: dp3 r[21].w, -r[28].yzwy, r[30].xyzx
3521: mad_sat r[21].w, r[21].w, r[26].y, r[26].z
3530: mul r[21].w, r[21].w, r[21].w
3537: mul r[26].xyz, r[21].wwww, r[25].xyzx
3544: movc r[25].xyz, r[29].yyyy, r[26].xyzx, r[25].xyzx
3553: if r[29].z
3556: ld_structured r[26].xy, r[20].x, l(68), t[7][7].xyxx
3566: ld_structured r[29].yzw, r[20].x, l(80), t[7][7].xxyz
3576: ld_structured r[30].xyz, r[20].x, l(96), t[7][7].xyzx
3586: ld_structured r[31].xyw, r[20].x, l(112), t[7][7].xyxz
3596: ld_structured r[32].xyzw, r[20].x, l(128), t[7][7].xyzw
3606: mov r[33].x, r[27].w
3611: mov r[33].y, r[29].y
3616: mov r[33].z, r[30].x
3621: mov r[33].w, r[31].x
3626: dp4 r[33].x, r[33].xyzw, r[10].xyzw
3633: mov r[34].x, r[26].x
3638: mov r[34].y, r[29].z
3643: mov r[34].z, r[30].y
3648: mov r[34].w, r[31].y
3653: dp4 r[33].y, r[34].xyzw, r[10].xyzw
3660: mov r[31].x, r[26].y
3665: mov r[31].y, r[29].w
3670: mov r[31].z, r[30].z
3675: dp4 r[33].z, r[31].xyzw, r[10].xyzw
3682: dp3 r[21].w, r[33].xyzx, r[33].xyzx
3689: rsq r[21].w, r[21].w
3694: mul r[26].xyz, r[21].wwww, r[33].xyzx
3701: lt r[30].xyzw, abs(r[26].yzxz), abs(r[26].xxyy)
3710: lt r[27].xyw, l(0,0,0,0), r[26].xyxz
3720: and r[29].yz, r[30].yywy, r[30].xxzx
3727: movc r[27].xyw, r[27].xyxw, l(0,2,0,4), l(1,3,0,5)
3742: movc r[21].w, r[29].z, r[27].y, r[27].w
3751: movc r[21].w, r[29].y, r[27].x, r[21].w
3760: if r[21].w
3763: ine r[22].w, r[21].w, l(1)
3770: if r[22].w
3773: div r[29].yzw, r[26].xxxy, r[26].yyzz
3780: div r[22].w, -r[26].z, r[26].y
3788: mad r[30].y, r[22].w, l(-0.5), l(0.5)
3797: div r[26].xw, r[26].xxxz, -r[26].yyyy
3805: mad r[26].xw, r[26].xxxw, l(0.5,0,0,-0.5), l(0.5,0,0,0.5)
3820: ieq r[31].xyz, r[21].wwww, l(2,3,4,0)
3830: mad r[33].xyz, r[29].yzwy, l(0.5,0.5,-0.5,0), l(0.5,0.5,0.5,0)
3845: div r[22].w, r[26].y, -r[26].z
3853: mad r[33].w, r[22].w, l(-0.5), l(0.5)
3862: movc r[26].yz, r[31].zzzz, r[33].yyzy, r[33].yywy
3871: movc r[26].xy, r[31].yyyy, r[26].xwxx, r[26].yzyy
3880: mov r[30].x, r[33].x
3885: movc r[27].xy, r[31].xxxx, r[30].xyxx, r[26].xyxx
3894: else 
3895: div r[29].yz, r[26].zzyz, -r[26].xxxx
3903: mad r[27].xy, r[29].yzyy, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
3918: endif 
3919: else 
3920: div r[22].w, -r[26].z, r[26].x
3928: mad r[27].x, r[22].w, l(0.5), l(0.5)
3937: div r[22].w, r[26].y, r[26].x
3944: mad r[27].y, r[22].w, l(-0.5), l(0.5)
3953: endif 
3954: add r[26].xy, r[27].xyxx, l(-0.5,-0.5,0,0)
3964: mad r[26].yz, r[26].xxyx, r[24].wwww, l(0,0.5,0.5,0)
3976: utof r[21].w, r[21].w
3981: mul r[21].w, r[21].w, l(0.166666672)
3988: mad r[26].x, r[26].y, l(0.166666672), r[21].w
3997: mad r[26].xy, r[26].xzxx, r[32].zwzz, r[32].xyxx
4006: sample_l r[26].xy, r[26].xyxx, t[6][6].xyzw, s[1][6], l(0)
4019: add r[21].w, r[29].x, l(-0.0500000007)
4026: mul r[22].w, r[27].z, r[21].w
4033: mad r[23].w, -r[26].x, r[26].x, r[26].y
4043: max r[23].w, r[23].w, l(9.99999975e-06)
4050: mad r[21].w, -r[21].w, r[27].z, r[26].x
4060: mad r[21].w, r[21].w, r[21].w, r[23].w
4069: div r[21].w, r[23].w, r[21].w
4076: min r[21].w, r[21].w, l(1)
4083: lt r[22].w, r[22].w, r[26].x
4090: movc r[21].w, r[22].w, l(1), r[21].w
4099: mul r[25].xyz, r[21].wwww, r[25].xyzx
4106: endif 
4107: mad r[24].xyz, r[24].xyzx, r[28].xxxx, r[16].yzwy
4116: dp3 r[21].w, r[24].xyzx, r[24].xyzx
4123: rsq r[21].w, r[21].w
4128: mul r[24].xyz, r[21].wwww, r[24].xyzx
4135: dp3 r[21].w, r[16].yzwy, r[24].xyzx
4142: dp3_sat r[22].w, r[17].xyzx, r[28].yzwy
4149: add r[21].w, -abs(r[21].w), l(1)
4157: max r[21].w, r[21].w, l(0)
4164: mul r[23].w, r[21].w, r[21].w
4171: mul r[23].w, r[23].w, r[23].w
4178: mul r[21].w, r[21].w, r[23].w
4185: mad r[21].w, r[17].w, r[21].w, r[8].w
4194: add_sat r[21].w, -r[21].w, l(1)
4202: mul r[21].w, r[22].w, r[21].w
4209: mul r[21].w, r[21].w, l(0.318309873)
4216: mad r[19].xyz, r[21].wwww, r[25].xyzx, r[19].xyzx
4225: iadd r[18].w, r[18].w, l(1)
4232: iadd r[13].w, r[13].y, r[18].w
4239: ld r[20].x, r[13].xwzz, t[19][24].xyzw
4247: endloop 
4248: ld r[10].x, r[13].xyzz, t[20][25].xyzw
4256: mul r[18].xyz, r[15].zzzz, r[17].xyzx
4263: mad r[9].xyz, r[18].xyzx, l(0.000300000014,0.000300000014,0.000300000014,0), r[15].xyzx
4275: mov r[18].xzw, r[13].xxzz
4280: mov r[24].xyz, r[19].xyzx
4285: mov r[10].y, l(0)
4290: mov r[10].z, r[10].x
4295: loop 
4296: ine r[13].x, r[10].z, l(9.18340949e-41)
4303: ilt r[13].w, r[10].y, l(256)
4310: and r[13].x, r[13].w, r[13].x
4317: breakc r[13].x
4320: iadd r[10].y, r[10].y, l(1)
4327: iadd r[18].y, r[10].y, r[13].y
4334: ld r[13].x, r[18].xyzw, t[20][25].xyzw
4342: ld_structured r[25].xyzw, r[10].z, l(68), t[11][11].xyzw
4352: ld_structured r[26].xyz, r[10].z, l(84), t[11][11].xyzx
4362: ld_structured r[27].xyzw, r[10].z, l(96), t[11][11].xyzw
4372: ld_structured r[28].xyzw, r[10].z, l(112), t[11][11].xyzw
4382: ld_structured r[29].xyzw, r[10].z, l(128), t[11][11].xyzw
4392: mov r[30].x, r[25].w
4397: mov r[30].y, r[27].x
4402: mov r[30].z, r[28].x
4407: mov r[30].w, r[29].x
4412: dp4 r[30].x, r[30].xyzw, r[9].xyzw
4419: mov r[31].x, r[26].x
4424: mov r[31].y, r[27].y
4429: mov r[31].z, r[28].y
4434: mov r[31].w, r[29].y
4439: dp4 r[30].y, r[31].xyzw, r[9].xyzw
4446: mov r[31].x, r[26].y
4451: mov r[31].y, r[27].z
4456: mov r[31].z, r[28].z
4461: mov r[31].w, r[29].z
4466: dp4 r[30].z, r[31].xyzw, r[9].xyzw
4473: mov r[29].x, r[26].z
4478: mov r[29].y, r[27].w
4483: mov r[29].z, r[28].w
4488: dp4 r[13].w, r[29].xyzw, r[9].xyzw
4495: rcp r[16].x, r[13].w
4500: mul r[26].xyz, r[16].xxxx, r[30].xyzx
4507: mad r[27].xyz, r[26].xyzx, l(0.5,0.5,1,0), l(0.5,0.5,0,0)
4522: mov_sat r[28].xyz, r[27].xyzx
4527: eq r[27].xyz, r[27].xyzx, r[28].xyzx
4534: and r[16].x, r[27].y, r[27].x
4541: and r[16].x, r[27].z, r[16].x
4548: if r[16].x
4551: ld_structured r[27].xyzw, r[10].z, l(0), t[11][11].xyzw
4561: ld_structured r[16].x, r[10].z, l(156), t[11][11].xxxx
4571: ld_structured r[18].y, r[10].z, l(200), t[11][11].xxxx
4581: ld_structured r[28].xyzw, r[10].z, l(288), t[11][11].xyzw
4591: ld_structured r[20].x, r[10].z, l(304), t[11][11].xxxx
4601: mad r[29].xy, r[26].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
4616: and r[29].zw, r[20].xxxx, l(0,0,2,768)
4626: if r[29].z
4629: ld_structured r[31].xyzw, r[10].z, l(272), t[11][11].xyzw
4639: mad r[30].xy, r[29].xyxx, r[31].zwzz, r[31].xyxx
4648: sample_l r[30].xy, r[30].xyxx, t[9][9].xyzw, s[1][6], l(0)
4661: add r[31].xyz, -r[15].xyzx, r[27].xyzx
4669: dp3 r[21].w, r[31].xyzx, r[31].xyzx
4676: sqrt r[21].w, r[21].w
4681: add r[21].w, -r[27].w, r[21].w
4689: mul r[22].w, r[16].x, r[21].w
4696: mad r[23].w, -r[30].x, r[30].x, r[30].y
4706: max r[23].w, r[23].w, l(9.99999975e-06)
4713: mad r[21].w, -r[21].w, r[16].x, r[30].x
4723: mad r[21].w, r[21].w, r[21].w, r[23].w
4732: div r[21].w, r[23].w, r[21].w
4739: min r[21].w, r[21].w, l(1)
4746: lt r[22].w, r[22].w, r[30].x
4753: movc r[21].w, r[22].w, l(1), r[21].w
4762: else 
4763: mov r[21].w, l(1)
4768: endif 
4769: ieq r[30].xy, r[29].wwww, l(256,512,0,0)
4779: if r[30].x
4782: mad r[30].xw, r[29].xxxy, r[28].zzzw, r[28].xxxy
4791: sample_l r[22].w, r[30].xwxx, t[8][8].yzwx, s[1][6], l(0)
4804: ge r[22].w, r[22].w, r[26].z
4811: and r[22].w, r[22].w, l(1)
4818: else 
4819: ieq r[23].w, r[29].w, l(512)
4826: if r[23].w
4829: mad r[26].zw, r[29].xxxy, r[28].zzzw, r[28].xxxy
4838: sample_l r[26].zw, r[26].zwzz, t[9][9].zwxy, s[1][6], l(0)
4851: add r[28].xyz, -r[15].xyzx, r[27].xyzx
4859: dp3 r[23].w, r[28].xyzx, r[28].xyzx
4866: sqrt r[23].w, r[23].w
4871: add r[23].w, -r[27].w, r[23].w
4879: mul r[24].w, r[16].x, r[23].w
4886: mad r[25].w, -r[26].z, r[26].z, r[26].w
4896: max r[25].w, r[25].w, l(9.99999975e-06)
4903: mad r[16].x, -r[23].w, r[16].x, r[26].z
4913: mad r[16].x, r[16].x, r[16].x, r[25].w
4922: div r[16].x, r[25].w, r[16].x
4929: min r[16].x, r[16].x, l(1)
4936: lt r[23].w, r[24].w, r[26].z
4943: movc r[22].w, r[23].w, l(1), r[16].x
4952: else 
4953: mov r[22].w, l(1)
4958: endif 
4959: endif 
4960: ine r[16].x, r[29].z, l(0)
4967: and r[16].x, r[16].x, r[30].y
4974: add r[23].w, -r[21].w, r[22].w
4982: mad r[23].w, r[18].y, r[23].w, r[21].w
4991: mad r[22].w, r[22].w, r[18].y, l(1)
5000: add r[18].y, -r[18].y, r[22].w
5008: min r[18].y, r[18].y, r[21].w
5015: movc r[16].x, r[16].x, r[23].w, r[18].y
5024: ne r[18].y, r[16].x, l(0)
5031: if r[18].y
5034: ld_structured r[28].xyz, r[10].z, l(16), t[11][11].xyzx
5044: ld_structured r[31].xyzw, r[10].z, l(48), t[11][11].xyzw
5054: add r[18].y, -r[30].z, r[31].w
5062: mul r[21].w, r[31].w, l(0.200000003)
5069: div_sat r[18].y, r[18].y, r[21].w
5076: mad r[21].w, r[18].y, l(-2), l(3)
5085: mul r[18].y, r[18].y, r[18].y
5092: mul r[18].y, r[18].y, r[21].w
5099: add r[28].xyz, -r[15].xyzx, r[28].xyzx
5107: dp3 r[21].w, r[28].xyzx, r[28].xyzx
5114: mul r[22].w, r[25].y, r[25].y
5121: mad r[21].w, r[25].z, r[21].w, r[22].w
5130: max r[21].w, r[21].w, l(0.25)
5137: mul r[21].w, r[21].w, l(3.14159274)
5144: rcp r[21].w, r[21].w
5149: add r[21].w, -r[25].x, r[21].w
5157: max r[21].w, r[21].w, l(0)
5164: mul r[18].y, r[18].y, r[21].w
5171: mul r[25].xyz, r[18].yyyy, r[31].xyzx
5178: and r[18].y, r[20].x, l(1)
5185: if r[18].y
5188: ld_structured r[28].xyzw, r[10].z, l(256), t[11][11].xyzw
5198: and r[18].y, r[20].x, l(1.43492963e-42)
5205: add r[26].zw, r[26].xxxy, r[26].xxxy
5212: mad r[20].x, r[26].x, r[26].x, l(1)
5221: mad r[20].x, r[26].y, r[26].y, r[20].x
5230: div r[26].xy, r[26].zwzz, r[20].xxxx
5237: mad r[26].xy, r[26].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
5252: movc r[26].xy, r[18].yyyy, r[26].xyxx, r[29].xyxx
5261: mad r[26].xy, r[26].xyxx, r[28].zwzz, r[28].xyxx
5270: sample_l r[26].xyz, r[26].xyxx, t[10][10].xyzw, s[3][11], l(0)
5283: mul r[25].xyz, r[25].xyzx, r[26].xyzx
5290: endif 
5291: ge r[13].w, r[13].w, r[30].z
5298: and r[13].w, r[13].w, l(1)
5305: mul r[13].w, r[13].w, r[16].x
5312: mul r[25].xyz, r[13].wwww, r[25].xyzx
5319: add r[26].xyz, -r[15].xyzx, r[27].xyzx
5327: dp3 r[13].w, r[26].xyzx, r[26].xyzx
5334: rsq r[13].w, r[13].w
5339: mul r[27].xyz, r[13].wwww, r[26].xyzx
5346: mad r[26].xyz, r[26].xyzx, r[13].wwww, r[16].yzwy
5355: dp3 r[13].w, r[26].xyzx, r[26].xyzx
5362: rsq r[13].w, r[13].w
5367: mul r[26].xyz, r[13].wwww, r[26].xyzx
5374: dp3 r[13].w, r[16].yzwy, r[26].xyzx
5381: dp3_sat r[16].x, r[17].xyzx, r[27].xyzx
5388: add r[13].w, -abs(r[13].w), l(1)
5396: max r[13].w, r[13].w, l(0)
5403: mul r[18].y, r[13].w, r[13].w
5410: mul r[18].y, r[18].y, r[18].y
5417: mul r[13].w, r[13].w, r[18].y
5424: mad r[13].w, r[17].w, r[13].w, r[8].w
5433: add_sat r[13].w, -r[13].w, l(1)
5441: mul r[13].w, r[16].x, r[13].w
5448: mul r[13].w, r[13].w, l(0.318309873)
5455: mad r[24].xyz, r[13].wwww, r[25].xyzx, r[24].xyzx
5464: endif 
5465: endif 
5466: mov r[10].z, r[13].x
5471: endloop 
5472: mad r[9].xyz, r[12].xyzx, l(0.318309873,0.318309873,0.318309873,0), r[24].xyzx
5484: mul r[20].yzw, r[22].xxyz, r[9].xxyz
5491: else 
5492: mov r[20].yzw, l(0,0,0,0)
5500: endif 
5501: endif 
5502: mad r[4].xyz, r[3].xyzx, r[20].yzwy, r[4].xyzx
5511: iadd r[1].z, r[1].z, l(1)
5518: endloop 
5519: add r[0].xy, -cb[1][1][17].yzyy, cb[1][1][17].xwxx
5531: add r[0].z, cb[1][1][17].y, cb[1][1][17].x
5542: add r[0].z, -r[0].x, r[0].z
5550: add r[0].x, -r[0].x, r[0].w
5558: div r[0].z, l(1,1,1,1), r[0].z
5568: mul_sat r[0].x, r[0].z, r[0].x
5575: mad r[0].z, r[0].x, l(-2), l(3)
5584: mul r[0].x, r[0].x, r[0].x
5591: mul r[0].x, r[0].x, r[0].z
5598: mad r[0].x, r[0].x, r[0].y, cb[1][1][17].z
5609: mul r[0].xyz, r[0].xxxx, r[4].xyzx
5616: itof r[0].w, cb[4][4][0].z
5623: div r[2].xyz, r[0].xyzx, r[0].wwww
5630: div r[0].x, r[7].w, r[0].w
5637: ld_uav_typed r[0].y, r[1].xyyy, type30[2][2].yxzw
5645: add r[0].x, r[0].x, r[0].y
5652: store_uav_typed type30[2][2].xyzw, r[1].xyyy, r[0].xxxx
5660: ld_uav_typed r[0].xyzw, r[1].xyyy, type30[0][0].xyzw
5668: mov r[2].w, l(1)
5673: add r[0].xyzw, r[0].xyzw, r[2].xyzw
5680: store_uav_typed type30[0][0].xyzw, r[1].xyyy, r[0].xyzw
5688: ld_uav_typed r[0].x, r[1].xyyy, type30[1][1].xyzw
5696: add r[0].x, r[0].x, l(1)
5703: store_uav_typed type30[1][1].xyzw, r[1].xyyy, r[0].xxxx
5711: ret 
5712: else 
5713: ld_uav_typed r[0].x, r[1].xyyy, type30[2][2].xyzw
5721: add r[0].x, r[0].x, l(65504)
5728: store_uav_typed type30[2][2].xyzw, r[1].xyyy, r[0].xxxx
5736: endif 
5737: ret 