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
236: dcl_resource raw:0x307e46,0x18,0x0,0xffffffff,0x5555,0x1
243: dcl_unordered_access_view_typed raw:0x31ee46,0x0,0x0,0x0,0x5555,0x0
250: dcl_unordered_access_view_typed raw:0x31ee46,0x1,0x1,0x1,0x5555,0x0
257: dcl_unordered_access_view_typed raw:0x31ee46,0x2,0x2,0x2,0x5555,0x0
264: dcl_input raw:0x20032
266: dcl_temps raw:0x23
268: dcl_thread_group raw:0x8,0x10,0x1
272: mov r[0].xy, type32.xyxx
276: mov r[0].w, l(0)
281: ld r[1].x, r[0].xyww, t[21][26].xyzw
289: ine r[1].x, r[1].x, l(9.18326936e-41)
296: if r[1].x
299: utof r[1].xy, type32.xyxx
303: add r[1].xy, r[1].xyxx, l(0.5,0.5,0,0)
313: mul r[1].xy, r[1].xyxx, cb[0][0][1].zwzz
322: ld r[1].z, r[0].xyww, t[3][3].yzxw
330: ld r[1].w, r[0].xyww, t[2][2].yzwx
338: mad r[1].xy, r[1].xyxx, l(2,-2,0,0), l(-1,1,0,0)
353: mul r[2].xyzw, r[1].yyyy, cb[0][0][15].xyzw
362: mad r[2].xyzw, cb[0][0][14].xyzw, r[1].xxxx, r[2].xyzw
373: mad r[2].xyzw, cb[0][0][16].xyzw, r[1].zzzz, r[2].xyzw
384: add r[2].xyzw, r[2].xyzw, cb[0][0][17].xyzw
393: rcp r[2].w, r[2].w
398: mul r[3].xyz, r[2].wwww, r[2].xyzx
405: dp3 r[3].w, r[3].xyzx, r[3].xyzx
412: rsq r[3].w, r[3].w
417: mul r[3].xyz, r[3].wwww, r[3].xyzx
424: mul r[4].xyzw, r[1].yyyy, cb[0][0][23].xyzw
433: mad r[4].xyzw, cb[0][0][22].xyzw, r[1].xxxx, r[4].xyzw
444: mad r[4].xyzw, cb[0][0][24].xyzw, r[1].zzzz, r[4].xyzw
455: add r[4].xyzw, r[4].xyzw, cb[0][0][25].xyzw
464: rcp r[1].x, r[4].w
469: mul r[5].xyz, r[1].xxxx, r[4].xyzx
476: mad r[1].xyz, r[4].xyzx, r[1].xxxx, -cb[0][0][54].xyzx
488: dp3 r[3].w, r[1].xyzx, r[1].xyzx
495: rsq r[3].w, r[3].w
500: mul r[1].xyz, r[1].xyzx, r[3].wwww
507: ld r[4].xyzw, r[0].xyww, t[0][0].xzyw
515: mad r[3].w, r[4].w, l(255), l(0.5)
524: ftou r[3].w, r[3].w
529: and r[3].w, r[3].w, l(254)
536: utof r[6].z, r[3].w
541: mov r[6].xy, r[4].xzxx
546: mad r[6].xyz, r[6].xyzx, l(255,255,1,0), l(0.5,0.5,0.5,0)
561: round_ni r[6].xyz, r[6].xyzx
566: ftoi r[6].xyz, r[6].xyzx
571: ubfe r[7].xyzw, l(3,4,3,4), l(1,4,1,4), r[6].zzzz
586: bfi r[6].xyzw, l(29,28,29,28), l(3,4,3,4), r[6].xyxy, r[7].xyzw
603: itof r[6].xyzw, r[6].xyzw
608: mad r[6].xyzw, r[6].xyzw, l(0.00097703957,0.000488400517,0.00097703957,0.000488400517), l(-1,-1,-1,-1)
623: mul r[6].xyzw, r[6].xyzw, l(1.29999995,1.29999995,2.5999999,2.5999999)
633: dp2 r[3].w, r[6].xyxx, r[6].xyxx
640: add r[6].xy, r[3].wwww, l(1,-1,0,0)
650: div r[6].xyz, r[6].zwyz, r[6].xxxx
657: mul r[7].xyz, r[6].yyyy, cb[0][0][7].xyzx
666: mad r[7].xyz, cb[0][0][6].xyzx, r[6].xxxx, r[7].xyzx
677: mad r[7].xyz, cb[0][0][8].xyzx, r[6].zzzz, r[7].xyzx
688: ld r[8].xyz, r[0].xyww, t[1][1].yzwx
696: mul r[4].zw, r[8].yyyz, l(0,0,255,255)
706: ftou r[4].zw, r[4].zzzw
711: ishl r[3].w, r[4].z, l(8)
718: or r[3].w, r[4].w, r[3].w
725: dp3 r[4].x, r[6].xyzx, -r[3].xyzx
733: ld_structured r[3].x, r[3].w, l(0), t[4][4].xxxx
743: ushr r[3].y, r[3].x, l(24)
750: ubfe r[3].xz, l(8,0,8,0), l(16,0,8,0), r[3].xxxx
765: utof r[6].xyz, r[3].yxzy
770: mul r[3].xyz, r[6].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
780: ld_structured r[3].w, r[3].w, l(0), t[4][4].xxxx
790: and r[3].w, r[3].w, l(255)
797: dp3 r[4].z, r[3].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
807: add r[4].w, r[4].z, l(9.99999997e-07)
814: rcp r[4].w, r[4].w
819: mul r[4].z, r[4].z, l(10000)
826: min r[4].z, r[4].z, l(1)
833: mad r[6].xyz, r[3].xyzx, r[4].wwww, l(-1,-1,-1,0)
845: mad r[6].xyz, r[4].zzzz, r[6].xyzx, l(1,1,1,0)
857: mul r[6].xyz, r[8].xxxx, r[6].xyzx
864: movc r[3].xyz, r[3].wwww, r[3].xyzx, r[6].xyzx
873: mul r[1].xyz, r[1].wwww, r[1].xyzx
880: mad r[1].xyz, -r[1].xyzx, l(0.00100000005,0.00100000005,0.00100000005,0), r[5].xyzx
893: mul r[6].xyz, r[1].wwww, r[7].xyzx
900: mad r[1].xyz, r[6].xyzx, l(0.00100000005,0.00100000005,0.00100000005,0), r[1].xyzx
912: sample_l r[4].xyz, r[4].xyxx, t[14][14].xyzw, s[1][6], l(0)
925: mul r[3].w, cb[1][1][15].w, l(0.318309873)
934: ld_structured r[6].xyz, l(0), l(0), t[13][13].xyzx
944: ld_structured r[8].xyz, l(0), l(16), t[13][13].xyzx
954: ld_structured r[9].xy, l(0), l(32), t[13][13].xyxx
964: ld_structured r[9].zw, l(0), l(48), t[13][13].xxxy
974: ld_structured r[10].xy, l(0), l(64), t[13][13].xyxx
984: ld_structured r[11].xw, l(0), l(80), t[13][13].xxxy
994: ld_structured r[4].w, l(0), l(96), t[13][13].xxxx
1004: ld_structured r[5].w, l(0), l(104), t[13][13].xxxx
1014: and r[5].w, r[5].w, l(1)
1021: mad r[3].xyz, r[3].xyzx, r[4].xxxx, r[4].yyyy
1030: add r[3].xyz, -r[3].xyzx, l(1,1,1,0)
1041: max r[3].xyz, r[3].xyzx, l(0,0,0,0)
1051: mul r[3].xyz, r[4].zzzz, r[3].xyzx
1058: mov r[12].w, l(1)
1063: mov r[13].z, l(0)
1068: mov r[14].xy, r[9].xzxx
1073: mov r[14].z, r[10].x
1078: mov r[14].w, r[11].x
1083: mov r[15].w, l(1)
1088: mov r[11].xy, r[9].ywyy
1093: mov r[11].z, r[10].y
1098: mov r[9].w, l(1)
1103: mov r[10].w, l(1)
1108: mov r[4].xyz, l(0,0,0,0)
1116: mov r[6].w, cb[1][1][16].z
1123: mov r[0].z, l(0)
1128: loop 
1129: ige r[7].w, r[0].z, cb[4][4][0].z
1138: breakc r[7].w
1141: ld r[7].w, r[0].xyzw, t[21][26].yzwx
1149: ld r[16].xyzw, r[0].xyzw, t[23][28].xyzw
1157: mul r[17].xyz, r[16].yyyy, cb[0][0][7].xyzx
1166: mad r[17].xyz, cb[0][0][6].xyzx, r[16].xxxx, r[17].xyzx
1177: mad r[17].xyz, cb[0][0][8].xyzx, r[16].zzzz, r[17].xyzx
1188: add r[17].xyz, r[17].xyzx, cb[0][0][9].xyzx
1197: ld r[18].xyzw, r[0].xyzw, t[22][27].xyzw
1205: add r[19].xyz, -r[1].xyzx, r[17].xyzx
1213: dp3 r[8].w, r[19].xyzx, r[19].xyzx
1220: rsq r[8].w, r[8].w
1225: mul r[19].xyz, r[8].wwww, r[19].xyzx
1232: ieq r[8].w, r[7].w, l(9.18340949e-41)
1239: mad r[20].xyz, cb[4][4][0].yyyy, r[19].xyzx, r[5].xyzx
1250: movc r[12].xyz, r[8].wwww, r[20].xyzx, r[17].xyzx
1259: movc r[20].xyz, r[8].wwww, r[7].xyzx, r[18].xyzx
1268: mov r[21].xyz, l(0,0,0,0)
1276: mov r[17].w, l(0)
1281: mov r[19].w, l(0)
1286: loop 
1287: uge r[20].w, r[19].w, cb[2][2][0].w
1296: breakc r[20].w
1299: ishl r[20].w, r[19].w, l(2)
1306: dp4 r[22].x, cb[3][3][r[20].w].xyzw, r[12].xyzw
1316: dp4 r[22].y, cb[3][3][1+r[20].w].xyzw, r[12].xyzw
1327: dp4 r[22].z, cb[3][3][2+r[20].w].xyzw, r[12].xyzw
1338: ge r[23].xyz, r[22].xyzx, l(0,0,0,0)
1348: and r[21].w, r[23].y, r[23].x
1355: and r[21].w, r[23].z, r[21].w
1362: utof r[23].xyz, cb[3][3][3+r[20].w].xyzx
1371: lt r[24].xyz, r[22].xyzx, r[23].xyzx
1378: and r[22].w, r[24].y, r[24].x
1385: and r[22].w, r[24].z, r[22].w
1392: and r[21].w, r[21].w, r[22].w
1399: if r[21].w
1402: ld_structured r[24].xyz, r[19].w, l(0), t[15][15].xyzx
1412: ld_structured r[25].xyz, r[19].w, l(12), t[15][15].xyzx
1422: ld_structured r[26].xyzw, r[19].w, l(24), t[15][15].xyzw
1432: ld_structured r[27].xyzw, r[19].w, l(40), t[15][15].xyzw
1442: ld_structured r[28].xyz, r[19].w, l(56), t[15][15].xyzx
1452: mul_sat r[21].w, r[22].x, r[26].w
1459: add r[23].xyz, -r[22].xyzx, r[23].xyzx
1467: mul_sat r[22].w, r[26].w, r[23].x
1474: mul r[21].w, r[21].w, r[22].w
1481: mul_sat r[23].xw, r[22].yyyz, r[27].xxxy
1488: mul r[21].w, r[21].w, r[23].x
1495: mul_sat r[23].xy, r[27].xyxx, r[23].yzyy
1502: mul r[21].w, r[21].w, r[23].x
1509: mul r[21].w, r[23].w, r[21].w
1516: mul r[21].w, r[23].y, r[21].w
1523: ine r[22].w, r[28].x, l(nan)
1530: if r[22].w
1533: round_ni r[23].xyz, r[22].xyzx
1538: add r[22].xyz, r[22].xyzx, -r[23].xyzx
1546: ftou r[23].xyz, r[23].xyzx
1551: imad r[22].w, r[23].z, cb[3][3][3+r[20].w].y, r[23].y
1564: imad r[20].w, r[22].w, cb[3][3][3+r[20].w].x, r[23].x
1577: iadd r[20].w, r[27].z, r[20].w
1584: ld_structured r[20].w, r[20].w, l(0), t[16][16].xxxx
1594: mov r[23].xyz, l(0,0,0,0)
1602: mov r[27].xyz, r[22].xyzx
1607: mov r[22].w, r[20].w
1612: mov r[23].w, r[28].y
1617: loop 
1618: and r[24].w, r[22].w, l(-0)
1625: ine r[24].w, r[24].w, l(0)
1632: ge r[25].w, l(9.99999975e-06), r[23].w
1639: or r[24].w, r[24].w, r[25].w
1646: if r[24].w
1649: and r[24].w, r[22].w, l(nan)
1656: iadd r[24].w, r[28].x, r[24].w
1663: and r[29].x, r[24].w, l(127)
1670: ubfe r[29].y, l(7), l(7), r[24].w
1679: ushr r[29].z, r[24].w, l(14)
1686: ishl r[29].xyz, r[29].xyzx, l(2,2,2,0)
1696: utof r[29].xyz, r[29].xyzx
1701: mad r[29].xyz, r[27].xyzx, l(3,3,3,0), r[29].xyzx
1713: add r[23].xyz, r[29].xyzx, l(0.5,0.5,0.5,0)
1723: break 
1724: endif 
1725: iadd r[24].w, r[27].w, r[22].w
1732: ld_structured r[29].xyzw, r[24].w, l(0), t[17][17].xyzw
1742: mul r[30].xyz, r[27].xyzx, l(3,3,3,0)
1752: round_ni r[30].xyz, r[30].xyzx
1757: ftou r[31].xyz, r[30].xyzx
1762: utof r[31].xz, r[31].zzxz
1767: imul null, r[24].w, r[31].y, l(3)
1775: utof r[24].w, r[24].w
1780: mad r[24].w, r[31].x, l(9), r[24].w
1789: add r[24].w, r[31].z, r[24].w
1796: ftou r[24].w, r[24].w
1801: ishl r[25].w, l(1), r[24].w
1808: and r[26].w, r[25].w, r[29].x
1815: if r[26].w
1818: iadd r[26].w, r[28].x, r[22].w
1825: and r[31].x, r[26].w, l(127)
1832: ubfe r[31].y, l(7), l(7), r[26].w
1841: ushr r[31].z, r[26].w, l(14)
1848: ishl r[31].xyz, r[31].xyzx, l(2,2,2,0)
1858: utof r[31].xyz, r[31].xyzx
1863: mad r[31].xyz, r[27].xyzx, l(3,3,3,0), r[31].xyzx
1875: add r[23].xyz, r[31].xyzx, l(0.5,0.5,0.5,0)
1885: break 
1886: endif 
1887: mad r[27].xyz, r[27].xyzx, l(3,3,3,0), -r[30].xyzx
1900: and r[25].w, r[25].w, r[29].y
1907: ubfe r[26].w, r[24].w, l(0), r[29].y
1916: countbits r[26].w, r[26].w
1921: iadd r[26].w, r[26].w, r[29].w
1928: or r[26].w, r[26].w, l(-0)
1935: not r[28].w, r[29].y
1940: and r[28].w, r[28].w, r[29].x
1947: ubfe r[24].w, r[24].w, l(0), r[28].w
1956: countbits r[24].w, r[24].w
1961: iadd r[24].w, r[24].w, r[29].z
1968: movc r[22].w, r[25].w, r[26].w, r[24].w
1977: mul r[23].w, r[23].w, l(0.333333343)
1984: mov r[23].xyz, l(0,0,0,0)
1992: endloop 
1993: else 
1994: mov r[23].xyz, l(0,0,0,0)
2002: endif 
2003: mul r[22].xyz, r[23].xyzx, cb[2][2][0].xyzx
2012: mov r[27].x, r[24].x
2017: mov r[27].y, r[25].x
2022: mov r[27].z, r[26].x
2027: dp3 r[27].x, r[27].xyzx, r[20].xyzx
2034: mov r[29].x, r[24].y
2039: mov r[29].y, r[25].y
2044: mov r[29].z, r[26].y
2049: dp3 r[27].y, r[29].xyzx, r[20].xyzx
2056: mov r[26].x, r[24].z
2061: mov r[26].y, r[25].z
2066: dp3 r[27].z, r[26].xyzx, r[20].xyzx
2073: mov_sat r[24].xyz, r[27].xyzx
2078: mul r[24].xyz, r[24].xyzx, r[24].xyzx
2085: mov_sat r[27].xyz, -r[27].xyzx
2091: mul r[25].xyz, r[27].xyzx, r[27].xyzx
2098: lt r[26].xyz, l(0,0,0,0), r[24].xyzx
2108: if r[26].x
2111: sample_l r[27].xyz, r[22].xyzx, t[18][18].xyzw, s[1][6], l(0)
2124: mul r[27].xyz, r[24].xxxx, r[27].xyzx
2131: else 
2132: mov r[27].xyz, l(0,0,0,0)
2140: endif 
2141: lt r[29].xyz, l(0,0,0,0), r[25].xyzx
2151: if r[29].x
2154: sample_l r[30].xyz, r[22].xyzx, t[18][19].xyzw, s[1][6], l(0)
2167: mad r[27].xyz, r[30].xyzx, r[25].xxxx, r[27].xyzx
2176: endif 
2177: if r[26].y
2180: sample_l r[26].xyw, r[22].xyzx, t[18][20].xywz, s[1][6], l(0)
2193: mad r[27].xyz, r[26].xywx, r[24].yyyy, r[27].xyzx
2202: endif 
2203: if r[29].y
2206: sample_l r[24].xyw, r[22].xyzx, t[18][21].xywz, s[1][6], l(0)
2219: mad r[27].xyz, r[24].xywx, r[25].yyyy, r[27].xyzx
2228: endif 
2229: if r[26].z
2232: sample_l r[24].xyw, r[22].xyzx, t[18][22].xywz, s[1][6], l(0)
2245: mad r[27].xyz, r[24].xywx, r[24].zzzz, r[27].xyzx
2254: endif 
2255: if r[29].z
2258: sample_l r[22].xyz, r[22].xyzx, t[18][23].xyzw, s[1][6], l(0)
2271: mad r[27].xyz, r[22].xyzx, r[25].zzzz, r[27].xyzx
2280: endif 
2281: ieq r[20].w, r[28].x, l(nan)
2288: movc r[22].xyz, r[20].wwww, l(1,0.349999994,0,0), r[27].xyzx
2300: mad r[20].w, r[21].w, l(-2), l(3)
2309: mul r[22].w, r[21].w, r[21].w
2316: mul r[20].w, r[20].w, r[22].w
2323: add r[22].w, r[28].z, l(-1)
2330: mad r[20].w, r[20].w, r[22].w, l(1)
2339: mul r[22].w, r[20].w, r[21].w
2346: mad r[21].xyz, r[22].xyzx, r[22].wwww, r[21].xyzx
2355: mad r[17].w, r[21].w, r[20].w, r[17].w
2364: endif 
2365: iadd r[19].w, r[19].w, l(1)
2372: endloop 
2373: lt r[12].x, l(0), r[17].w
2380: div r[20].xyz, r[21].xyzx, r[17].wwww
2387: movc r[12].xyz, r[12].xxxx, r[20].xyzx, r[21].xyzx
2396: add r[12].xyz, r[12].xyzx, cb[2][2][1].xyzx
2405: if r[8].w
2408: mul r[20].xyz, r[3].wwww, r[12].xyzx
2415: else 
2416: ld_structured r[22].xyz, r[7].w, l(52), t[5][5].xyzx
2426: imul null, r[8].w, r[7].w, l(35)
2434: mov r[24].x, r[18].w
2439: mov r[24].y, r[16].w
2444: sample_l r[25].xyz, r[24].xyxx, t[24][r[8].w].xyzw, s[0][5], l(0)
2459: mul r[22].xyz, r[22].xyzx, r[25].xyzx
2466: dp3 r[8].w, r[22].xyzx, r[22].xyzx
2473: ge r[8].w, r[8].w, l(9.99999975e-05)
2480: if r[8].w
2483: mad r[25].xyz, r[2].xyzx, r[2].wwww, -r[16].xyzx
2493: dp3 r[8].w, r[25].xyzx, r[25].xyzx
2500: sqrt r[8].w, r[8].w
2505: mul r[8].w, r[8].w, l(50)
2512: min r[8].w, r[8].w, l(1)
2519: mul r[12].xyz, r[8].wwww, r[12].xyzx
2526: ld_structured r[24].zw, r[7].w, l(68), t[5][5].xxxy
2536: imad r[25].xy, r[7].wwww, l(35,35,0,0), l(2,4,0,0)
2551: sample_l r[8].w, r[24].xyxx, t[24][r[25].x].yzwx, s[4][19], l(0)
2566: add r[16].w, -r[24].z, r[24].w
2574: mad r[26].y, r[8].w, r[16].w, r[24].z
2583: sample_l r[8].w, r[24].xyxx, t[24][r[25].y].yzwx, s[4][19], l(0)
2598: ld_structured r[7].w, r[7].w, l(0), t[4][4].xxxx
2608: ushr r[16].w, r[7].w, l(24)
2615: ubfe r[24].xy, l(8,8,0,0), l(16,8,0,0), r[7].wwww
2630: utof r[25].x, r[16].w
2635: utof r[25].yz, r[24].xxyx
2640: mul r[24].xyz, r[8].wwww, r[25].xyzx
2647: mul r[24].xyz, r[24].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
2657: dp3 r[7].w, r[24].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
2667: dp3 r[26].x, r[18].xyzx, -r[19].xyzx
2675: sample_l r[19].xyz, r[26].xyxx, t[14][14].xyzw, s[1][6], l(0)
2688: mad r[19].xyw, r[24].xyxz, r[19].xxxx, r[19].yyyy
2697: add r[19].xyw, -r[19].xyxw, l(1,1,0,1)
2708: mul r[19].xyw, r[19].xyxw, cb[1][1][15].wwww
2717: mul r[19].xyz, r[19].zzzz, r[19].xywx
2724: mul r[12].xyz, r[12].xyzx, r[19].xyzx
2731: mul r[19].xyz, r[17].yyyy, cb[0][0][3].xyzx
2740: mad r[19].xyz, cb[0][0][2].xyzx, r[17].xxxx, r[19].xyzx
2751: mad r[17].xyz, cb[0][0][4].xyzx, r[17].zzzz, r[19].xyzx
2762: add r[15].xyz, r[17].xyzx, cb[0][0][5].xyzx
2771: mul r[17].xyz, r[18].yyyy, cb[0][0][3].xyzx
2780: mad r[17].xyz, cb[0][0][2].xyzx, r[18].xxxx, r[17].xyzx
2791: mad r[17].xyz, cb[0][0][4].xyzx, r[18].zzzz, r[17].xyzx
2802: add r[16].xyz, r[16].xyzx, l(150,75,75,0)
2812: mul r[16].xyz, r[16].xyzx, l(0.0533333346,0.0533333346,0.0533333346,0)
2822: round_ni r[16].xyz, r[16].xyzx
2827: max r[16].xyz, r[16].xyzx, l(0,0,0,0)
2837: min r[16].xyz, r[16].xyzx, l(15,7,11,0)
2847: ftoi r[16].xyz, r[16].xyzx
2852: uge r[8].w, r[6].w, l(1)
2859: if r[8].w
2862: dp4 r[18].x, r[14].xyzw, r[15].xyzw
2869: dp4 r[18].y, r[11].xyzw, r[15].xyzw
2876: sample_l r[8].w, r[18].xyxx, t[12][12].yzwx, s[2][8], l(0)
2889: add r[8].w, r[8].w, l(-1)
2896: mad r[8].w, r[4].w, r[8].w, l(1)
2905: mul r[18].xyz, r[8].wwww, r[8].xyzx
2912: movc r[18].xyz, r[5].wwww, r[18].xyzx, r[8].xyzx
2921: dp3 r[8].w, -r[15].xyzx, -r[15].xyzx
2930: rsq r[8].w, r[8].w
2935: mul r[19].xyz, r[8].wwww, -r[15].xyzx
2943: mad r[24].xyz, -r[15].xyzx, r[8].wwww, r[6].xyzx
2953: dp3 r[8].w, r[24].xyzx, r[24].xyzx
2960: rsq r[8].w, r[8].w
2965: mul r[24].xyz, r[8].wwww, r[24].xyzx
2972: dp3 r[8].w, r[19].xyzx, r[24].xyzx
2979: dp3_sat r[16].w, r[17].xyzx, r[6].xyzx
2986: add r[18].w, -r[7].w, l(1)
2994: add r[8].w, -abs(r[8].w), l(1)
3002: max r[8].w, r[8].w, l(0)
3009: mul r[19].x, r[8].w, r[8].w
3016: mul r[19].x, r[19].x, r[19].x
3023: mul r[8].w, r[8].w, r[19].x
3030: mad r[8].w, r[18].w, r[8].w, r[7].w
3039: add_sat r[8].w, -r[8].w, l(1)
3047: mul r[8].w, r[16].w, r[8].w
3054: mul r[8].w, r[8].w, l(0.318309873)
3061: mul r[18].xyz, r[18].xyzx, r[8].wwww
3068: else 
3069: mov r[18].xyz, l(0,0,0,0)
3077: endif 
3078: imad r[13].x, r[16].x, l(12), r[16].z
3087: ishl r[13].y, r[16].y, l(8)
3094: ld r[8].w, r[13].xyzz, t[19][24].yzwx
3102: dp3 r[16].x, -r[15].xyzx, -r[15].xyzx
3111: mad r[10].xyz, r[17].xyzx, l(0.0700000003,0.0700000003,0.0700000003,0), r[15].xyzx
3123: rsq r[16].x, r[16].x
3128: mul r[16].xyz, -r[15].xyzx, r[16].xxxx
3136: add r[16].w, -r[7].w, l(1)
3144: mov r[19].xyz, r[18].xyzx
3149: mov r[18].w, l(0)
3154: mov r[19].w, r[8].w
3159: loop 
3160: ine r[20].w, r[19].w, l(9.18340949e-41)
3167: ilt r[21].w, r[18].w, l(256)
3174: and r[20].w, r[20].w, r[21].w
3181: breakc r[20].w
3184: ld_structured r[24].xyzw, r[19].w, l(0), t[7][7].xyzw
3194: ld_structured r[25].xyzw, r[19].w, l(16), t[7][7].xyzw
3204: ld_structured r[26].xyzw, r[19].w, l(36), t[7][7].xyzw
3214: ld_structured r[27].xyzw, r[19].w, l(52), t[7][7].xyzw
3224: ld_structured r[20].w, r[19].w, l(144), t[7][7].xxxx
3234: ld_structured r[21].w, r[19].w, l(156), t[7][7].xxxx
3244: add r[24].xyz, -r[15].xyzx, r[24].xyzx
3252: dp3 r[22].w, r[24].xyzx, r[24].xyzx
3259: rsq r[23].w, r[22].w
3264: mul r[28].xyz, r[23].wwww, r[24].xyzx
3271: sqrt r[28].w, r[22].w
3276: add r[29].x, r[25].w, -r[28].w
3284: mul r[25].w, r[25].w, l(0.200000003)
3291: div_sat r[25].w, r[29].x, r[25].w
3298: mad r[29].x, r[25].w, l(-2), l(3)
3307: mul r[25].w, r[25].w, r[25].w
3314: mul r[25].w, r[25].w, r[29].x
3321: mad r[20].w, r[20].w, r[20].w, r[22].w
3330: max r[20].w, r[20].w, l(9.99999975e-05)
3337: mul r[20].w, r[20].w, l(3.14159274)
3344: rcp r[20].w, r[20].w
3349: add r[20].w, -r[26].x, r[20].w
3357: max r[20].w, r[20].w, l(0)
3364: mul r[20].w, r[25].w, r[20].w
3371: mul r[25].xyz, r[20].wwww, r[25].xyzx
3378: and r[29].xy, r[21].wwww, l(4,2,0,0)
3388: mov r[30].x, r[26].w
3393: mov r[30].yz, r[27].xxyx
3398: dp3 r[20].w, -r[28].xyzx, r[30].xyzx
3406: mad_sat r[20].w, r[20].w, r[26].y, r[26].z
3415: mul r[20].w, r[20].w, r[20].w
3422: mul r[26].xyz, r[20].wwww, r[25].xyzx
3429: movc r[25].xyz, r[29].xxxx, r[26].xyzx, r[25].xyzx
3438: if r[29].y
3441: ld_structured r[26].xy, r[19].w, l(68), t[7][7].xyxx
3451: ld_structured r[29].xyz, r[19].w, l(80), t[7][7].xyzx
3461: ld_structured r[30].xyz, r[19].w, l(96), t[7][7].xyzx
3471: ld_structured r[31].xyw, r[19].w, l(112), t[7][7].xyxz
3481: ld_structured r[32].xyzw, r[19].w, l(128), t[7][7].xyzw
3491: mov r[33].x, r[27].w
3496: mov r[33].y, r[29].x
3501: mov r[33].z, r[30].x
3506: mov r[33].w, r[31].x
3511: dp4 r[33].x, r[33].xyzw, r[10].xyzw
3518: mov r[34].x, r[26].x
3523: mov r[34].y, r[29].y
3528: mov r[34].z, r[30].y
3533: mov r[34].w, r[31].y
3538: dp4 r[33].y, r[34].xyzw, r[10].xyzw
3545: mov r[31].x, r[26].y
3550: mov r[31].y, r[29].z
3555: mov r[31].z, r[30].z
3560: dp4 r[33].z, r[31].xyzw, r[10].xyzw
3567: dp3 r[20].w, r[33].xyzx, r[33].xyzx
3574: rsq r[20].w, r[20].w
3579: mul r[26].xyz, r[20].wwww, r[33].xyzx
3586: lt r[29].xyzw, abs(r[26].yzxz), abs(r[26].xxyy)
3595: lt r[27].xyw, l(0,0,0,0), r[26].xyxz
3605: and r[29].xy, r[29].ywyy, r[29].xzxx
3612: movc r[27].xyw, r[27].xyxw, l(0,2,0,4), l(1,3,0,5)
3627: movc r[20].w, r[29].y, r[27].y, r[27].w
3636: movc r[20].w, r[29].x, r[27].x, r[20].w
3645: if r[20].w
3648: ine r[21].w, r[20].w, l(1)
3655: if r[21].w
3658: div r[29].xyz, r[26].xxyx, r[26].yzzy
3665: div r[21].w, -r[26].z, r[26].y
3673: mad r[30].y, r[21].w, l(-0.5), l(0.5)
3682: div r[26].xw, r[26].xxxz, -r[26].yyyy
3690: mad r[26].xw, r[26].xxxw, l(0.5,0,0,-0.5), l(0.5,0,0,0.5)
3705: ieq r[31].xyz, r[20].wwww, l(2,3,4,0)
3715: mad r[29].xyz, r[29].xyzx, l(0.5,0.5,-0.5,0), l(0.5,0.5,0.5,0)
3730: div r[21].w, r[26].y, -r[26].z
3738: mad r[29].w, r[21].w, l(-0.5), l(0.5)
3747: movc r[26].yz, r[31].zzzz, r[29].yyzy, r[29].yywy
3756: movc r[26].xy, r[31].yyyy, r[26].xwxx, r[26].yzyy
3765: mov r[30].x, r[29].x
3770: movc r[27].xy, r[31].xxxx, r[30].xyxx, r[26].xyxx
3779: else 
3780: div r[29].xy, r[26].zyzz, -r[26].xxxx
3788: mad r[27].xy, r[29].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
3803: endif 
3804: else 
3805: div r[21].w, -r[26].z, r[26].x
3813: mad r[27].x, r[21].w, l(0.5), l(0.5)
3822: div r[21].w, r[26].y, r[26].x
3829: mad r[27].y, r[21].w, l(-0.5), l(0.5)
3838: endif 
3839: add r[26].xy, r[27].xyxx, l(-0.5,-0.5,0,0)
3849: mad r[26].yz, r[26].xxyx, r[24].wwww, l(0,0.5,0.5,0)
3861: utof r[20].w, r[20].w
3866: mul r[20].w, r[20].w, l(0.166666672)
3873: mad r[26].x, r[26].y, l(0.166666672), r[20].w
3882: mad r[26].xy, r[26].xzxx, r[32].zwzz, r[32].xyxx
3891: sample_l r[26].xy, r[26].xyxx, t[6][6].xyzw, s[1][6], l(0)
3904: add r[20].w, r[28].w, l(-0.0500000007)
3911: mul r[21].w, r[27].z, r[20].w
3918: mad r[22].w, -r[26].x, r[26].x, r[26].y
3928: max r[22].w, r[22].w, l(9.99999975e-06)
3935: mad r[20].w, -r[20].w, r[27].z, r[26].x
3945: mad r[20].w, r[20].w, r[20].w, r[22].w
3954: div r[20].w, r[22].w, r[20].w
3961: min r[20].w, r[20].w, l(1)
3968: lt r[21].w, r[21].w, r[26].x
3975: movc r[20].w, r[21].w, l(1), r[20].w
3984: mul r[25].xyz, r[20].wwww, r[25].xyzx
3991: endif 
3992: mad r[24].xyz, r[24].xyzx, r[23].wwww, r[16].xyzx
4001: dp3 r[20].w, r[24].xyzx, r[24].xyzx
4008: rsq r[20].w, r[20].w
4013: mul r[24].xyz, r[20].wwww, r[24].xyzx
4020: dp3 r[20].w, r[16].xyzx, r[24].xyzx
4027: dp3_sat r[21].w, r[17].xyzx, r[28].xyzx
4034: add r[20].w, -abs(r[20].w), l(1)
4042: max r[20].w, r[20].w, l(0)
4049: mul r[22].w, r[20].w, r[20].w
4056: mul r[22].w, r[22].w, r[22].w
4063: mul r[20].w, r[20].w, r[22].w
4070: mad r[20].w, r[16].w, r[20].w, r[7].w
4079: add_sat r[20].w, -r[20].w, l(1)
4087: mul r[20].w, r[21].w, r[20].w
4094: mul r[20].w, r[20].w, l(0.318309873)
4101: mad r[19].xyz, r[20].wwww, r[25].xyzx, r[19].xyzx
4110: iadd r[18].w, r[18].w, l(1)
4117: iadd r[13].w, r[13].y, r[18].w
4124: ld r[19].w, r[13].xwzz, t[19][24].yzwx
4132: endloop 
4133: ld r[8].w, r[13].xyzz, t[20][25].yzwx
4141: mul r[10].xyz, r[15].zzzz, r[17].xyzx
4148: mad r[9].xyz, r[10].xyzx, l(0.000300000014,0.000300000014,0.000300000014,0), r[15].xyzx
4160: mov r[18].xzw, r[13].xxzz
4165: mov r[10].xyz, r[19].xyzx
4170: mov r[13].x, l(0)
4175: mov r[13].w, r[8].w
4180: loop 
4181: ine r[19].w, r[13].w, l(9.18340949e-41)
4188: ilt r[20].w, r[13].x, l(256)
4195: and r[19].w, r[19].w, r[20].w
4202: breakc r[19].w
4205: iadd r[13].x, r[13].x, l(1)
4212: iadd r[18].y, r[13].x, r[13].y
4219: ld r[18].y, r[18].xyzw, t[20][25].yxzw
4227: ld_structured r[24].xyzw, r[13].w, l(68), t[11][11].xyzw
4237: ld_structured r[25].xyz, r[13].w, l(84), t[11][11].xyzx
4247: ld_structured r[26].xyzw, r[13].w, l(96), t[11][11].xyzw
4257: ld_structured r[27].xyzw, r[13].w, l(112), t[11][11].xyzw
4267: ld_structured r[28].xyzw, r[13].w, l(128), t[11][11].xyzw
4277: mov r[29].x, r[24].w
4282: mov r[29].y, r[26].x
4287: mov r[29].z, r[27].x
4292: mov r[29].w, r[28].x
4297: dp4 r[29].x, r[29].xyzw, r[9].xyzw
4304: mov r[30].x, r[25].x
4309: mov r[30].y, r[26].y
4314: mov r[30].z, r[27].y
4319: mov r[30].w, r[28].y
4324: dp4 r[29].y, r[30].xyzw, r[9].xyzw
4331: mov r[30].x, r[25].y
4336: mov r[30].y, r[26].z
4341: mov r[30].z, r[27].z
4346: mov r[30].w, r[28].z
4351: dp4 r[29].z, r[30].xyzw, r[9].xyzw
4358: mov r[28].x, r[25].z
4363: mov r[28].y, r[26].w
4368: mov r[28].z, r[27].w
4373: dp4 r[19].w, r[28].xyzw, r[9].xyzw
4380: rcp r[20].w, r[19].w
4385: mul r[25].xyz, r[20].wwww, r[29].xyzx
4392: mad r[26].xyz, r[25].xyzx, l(0.5,0.5,1,0), l(0.5,0.5,0,0)
4407: mov_sat r[27].xyz, r[26].xyzx
4412: eq r[26].xyz, r[26].xyzx, r[27].xyzx
4419: and r[20].w, r[26].y, r[26].x
4426: and r[20].w, r[26].z, r[20].w
4433: if r[20].w
4436: ld_structured r[26].xyzw, r[13].w, l(0), t[11][11].xyzw
4446: ld_structured r[20].w, r[13].w, l(156), t[11][11].xxxx
4456: ld_structured r[21].w, r[13].w, l(200), t[11][11].xxxx
4466: ld_structured r[27].xyzw, r[13].w, l(288), t[11][11].xyzw
4476: ld_structured r[22].w, r[13].w, l(304), t[11][11].xxxx
4486: mad r[28].xy, r[25].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
4501: and r[28].zw, r[22].wwww, l(0,0,2,768)
4511: if r[28].z
4514: ld_structured r[30].xyzw, r[13].w, l(272), t[11][11].xyzw
4524: mad r[29].xy, r[28].xyxx, r[30].zwzz, r[30].xyxx
4533: sample_l r[29].xy, r[29].xyxx, t[9][9].xyzw, s[1][6], l(0)
4546: add r[30].xyz, -r[15].xyzx, r[26].xyzx
4554: dp3 r[23].w, r[30].xyzx, r[30].xyzx
4561: sqrt r[23].w, r[23].w
4566: add r[23].w, -r[26].w, r[23].w
4574: mul r[24].w, r[20].w, r[23].w
4581: mad r[25].w, -r[29].x, r[29].x, r[29].y
4591: max r[25].w, r[25].w, l(9.99999975e-06)
4598: mad r[23].w, -r[23].w, r[20].w, r[29].x
4608: mad r[23].w, r[23].w, r[23].w, r[25].w
4617: div r[23].w, r[25].w, r[23].w
4624: min r[23].w, r[23].w, l(1)
4631: lt r[24].w, r[24].w, r[29].x
4638: movc r[23].w, r[24].w, l(1), r[23].w
4647: else 
4648: mov r[23].w, l(1)
4653: endif 
4654: ieq r[29].xy, r[28].wwww, l(256,512,0,0)
4664: if r[29].x
4667: mad r[29].xw, r[28].xxxy, r[27].zzzw, r[27].xxxy
4676: sample_l r[24].w, r[29].xwxx, t[8][8].yzwx, s[1][6], l(0)
4689: ge r[24].w, r[24].w, r[25].z
4696: and r[24].w, r[24].w, l(1)
4703: else 
4704: ieq r[25].z, r[28].w, l(512)
4711: if r[25].z
4714: mad r[25].zw, r[28].xxxy, r[27].zzzw, r[27].xxxy
4723: sample_l r[25].zw, r[25].zwzz, t[9][9].zwxy, s[1][6], l(0)
4736: add r[27].xyz, -r[15].xyzx, r[26].xyzx
4744: dp3 r[27].x, r[27].xyzx, r[27].xyzx
4751: sqrt r[27].x, r[27].x
4756: add r[26].w, -r[26].w, r[27].x
4764: mul r[27].x, r[20].w, r[26].w
4771: mad r[25].w, -r[25].z, r[25].z, r[25].w
4781: max r[25].w, r[25].w, l(9.99999975e-06)
4788: mad r[20].w, -r[26].w, r[20].w, r[25].z
4798: mad r[20].w, r[20].w, r[20].w, r[25].w
4807: div r[20].w, r[25].w, r[20].w
4814: min r[20].w, r[20].w, l(1)
4821: lt r[25].z, r[27].x, r[25].z
4828: movc r[24].w, r[25].z, l(1), r[20].w
4837: else 
4838: mov r[24].w, l(1)
4843: endif 
4844: endif 
4845: ine r[20].w, r[28].z, l(0)
4852: and r[20].w, r[20].w, r[29].y
4859: add r[25].z, -r[23].w, r[24].w
4867: mad r[25].z, r[21].w, r[25].z, r[23].w
4876: mad r[24].w, r[24].w, r[21].w, l(1)
4885: add r[21].w, -r[21].w, r[24].w
4893: min r[21].w, r[21].w, r[23].w
4900: movc r[20].w, r[20].w, r[25].z, r[21].w
4909: ne r[21].w, r[20].w, l(0)
4916: if r[21].w
4919: ld_structured r[27].xyz, r[13].w, l(16), t[11][11].xyzx
4929: ld_structured r[30].xyzw, r[13].w, l(48), t[11][11].xyzw
4939: add r[21].w, -r[29].z, r[30].w
4947: mul r[23].w, r[30].w, l(0.200000003)
4954: div_sat r[21].w, r[21].w, r[23].w
4961: mad r[23].w, r[21].w, l(-2), l(3)
4970: mul r[21].w, r[21].w, r[21].w
4977: mul r[21].w, r[21].w, r[23].w
4984: add r[27].xyz, -r[15].xyzx, r[27].xyzx
4992: dp3 r[23].w, r[27].xyzx, r[27].xyzx
4999: mul r[24].y, r[24].y, r[24].y
5006: mad r[23].w, r[24].z, r[23].w, r[24].y
5015: max r[23].w, r[23].w, l(0.25)
5022: mul r[23].w, r[23].w, l(3.14159274)
5029: rcp r[23].w, r[23].w
5034: add r[23].w, -r[24].x, r[23].w
5042: max r[23].w, r[23].w, l(0)
5049: mul r[21].w, r[21].w, r[23].w
5056: mul r[24].xyz, r[21].wwww, r[30].xyzx
5063: and r[21].w, r[22].w, l(1)
5070: if r[21].w
5073: ld_structured r[27].xyzw, r[13].w, l(256), t[11][11].xyzw
5083: and r[21].w, r[22].w, l(1.43492963e-42)
5090: add r[25].zw, r[25].xxxy, r[25].xxxy
5097: mad r[22].w, r[25].x, r[25].x, l(1)
5106: mad r[22].w, r[25].y, r[25].y, r[22].w
5115: div r[25].xy, r[25].zwzz, r[22].wwww
5122: mad r[25].xy, r[25].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
5137: movc r[25].xy, r[21].wwww, r[25].xyxx, r[28].xyxx
5146: mad r[25].xy, r[25].xyxx, r[27].zwzz, r[27].xyxx
5155: sample_l r[25].xyz, r[25].xyxx, t[10][10].xyzw, s[3][11], l(0)
5168: mul r[24].xyz, r[24].xyzx, r[25].xyzx
5175: endif 
5176: ge r[19].w, r[19].w, r[29].z
5183: and r[19].w, r[19].w, l(1)
5190: mul r[19].w, r[19].w, r[20].w
5197: mul r[24].xyz, r[19].wwww, r[24].xyzx
5204: add r[25].xyz, -r[15].xyzx, r[26].xyzx
5212: dp3 r[19].w, r[25].xyzx, r[25].xyzx
5219: rsq r[19].w, r[19].w
5224: mul r[26].xyz, r[19].wwww, r[25].xyzx
5231: mad r[25].xyz, r[25].xyzx, r[19].wwww, r[16].xyzx
5240: dp3 r[19].w, r[25].xyzx, r[25].xyzx
5247: rsq r[19].w, r[19].w
5252: mul r[25].xyz, r[19].wwww, r[25].xyzx
5259: dp3 r[19].w, r[16].xyzx, r[25].xyzx
5266: dp3_sat r[20].w, r[17].xyzx, r[26].xyzx
5273: add r[19].w, -abs(r[19].w), l(1)
5281: max r[19].w, r[19].w, l(0)
5288: mul r[21].w, r[19].w, r[19].w
5295: mul r[21].w, r[21].w, r[21].w
5302: mul r[19].w, r[19].w, r[21].w
5309: mad r[19].w, r[16].w, r[19].w, r[7].w
5318: add_sat r[19].w, -r[19].w, l(1)
5326: mul r[19].w, r[20].w, r[19].w
5333: mul r[19].w, r[19].w, l(0.318309873)
5340: mad r[10].xyz, r[19].wwww, r[24].xyzx, r[10].xyzx
5349: endif 
5350: endif 
5351: mov r[13].w, r[18].y
5356: endloop 
5357: mad r[9].xyz, r[12].xyzx, l(0.318309873,0.318309873,0.318309873,0), r[10].xyzx
5369: mul r[20].xyz, r[22].xyzx, r[9].xyzx
5376: else 
5377: mov r[20].xyz, l(0,0,0,0)
5385: endif 
5386: endif 
5387: mad r[4].xyz, r[3].xyzx, r[20].xyzx, r[4].xyzx
5396: iadd r[0].z, r[0].z, l(1)
5403: endloop 
5404: add r[0].xy, -cb[1][1][17].yzyy, cb[1][1][17].xwxx
5416: add r[0].z, cb[1][1][17].y, cb[1][1][17].x
5427: add r[0].z, -r[0].x, r[0].z
5435: add r[0].x, -r[0].x, r[1].w
5443: div r[0].z, l(1,1,1,1), r[0].z
5453: mul_sat r[0].x, r[0].z, r[0].x
5460: mad r[0].z, r[0].x, l(-2), l(3)
5469: mul r[0].x, r[0].x, r[0].x
5476: mul r[0].x, r[0].x, r[0].z
5483: mad r[0].x, r[0].x, r[0].y, cb[1][1][17].z
5494: mul r[0].xyz, r[0].xxxx, r[4].xyzx
5501: itof r[0].w, cb[4][4][0].z
5508: div r[0].xyz, r[0].xyzx, r[0].wwww
5515: mov r[0].w, l(1)
5520: else 
5521: mov r[0].xyzw, l(0,0,0,0)
5529: endif 
5530: ld_uav_typed r[1].x, type32.xyyy, type30[1][1].xyzw
5537: add r[1].x, r[1].x, l(1)
5544: ld_uav_typed r[2].xyzw, type32.xyyy, type30[0][0].xyzw
5551: add r[0].xyzw, r[0].xyzw, r[2].xyzw
5558: div r[0].xyzw, r[0].xyzw, r[1].xxxx
5565: store_uav_typed type30[0][0].xyzw, type32.xyyy, r[0].xyzw
5572: ld_uav_typed r[0].x, type32.xyyy, type30[2][2].xyzw
5579: div r[0].x, r[0].x, r[1].x
5586: store_uav_typed type30[2][2].xyzw, type32.xyyy, r[0].xxxx
5593: ret 