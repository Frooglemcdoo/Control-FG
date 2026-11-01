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
257: dcl_input raw:0x20032
259: dcl_temps raw:0x23
261: dcl_thread_group raw:0x8,0x10,0x1
265: mov r[0].xy, type32.xyxx
269: mov r[0].w, l(0)
274: ld r[1].x, r[0].xyww, t[21][26].xyzw
282: ine r[1].x, r[1].x, l(9.18326936e-41)
289: if r[1].x
292: utof r[1].xy, type32.xyxx
296: add r[1].xy, r[1].xyxx, l(0.5,0.5,0,0)
306: mul r[1].xy, r[1].xyxx, cb[0][0][1].zwzz
315: ld r[1].z, r[0].xyww, t[3][3].yzxw
323: ld r[1].w, r[0].xyww, t[2][2].yzwx
331: mad r[1].xy, r[1].xyxx, l(2,-2,0,0), l(-1,1,0,0)
346: mul r[2].xyzw, r[1].yyyy, cb[0][0][15].xyzw
355: mad r[2].xyzw, cb[0][0][14].xyzw, r[1].xxxx, r[2].xyzw
366: mad r[2].xyzw, cb[0][0][16].xyzw, r[1].zzzz, r[2].xyzw
377: add r[2].xyzw, r[2].xyzw, cb[0][0][17].xyzw
386: rcp r[2].w, r[2].w
391: mul r[3].xyz, r[2].wwww, r[2].xyzx
398: dp3 r[3].w, r[3].xyzx, r[3].xyzx
405: rsq r[3].w, r[3].w
410: mul r[3].xyz, r[3].wwww, r[3].xyzx
417: mul r[4].xyzw, r[1].yyyy, cb[0][0][23].xyzw
426: mad r[4].xyzw, cb[0][0][22].xyzw, r[1].xxxx, r[4].xyzw
437: mad r[4].xyzw, cb[0][0][24].xyzw, r[1].zzzz, r[4].xyzw
448: add r[4].xyzw, r[4].xyzw, cb[0][0][25].xyzw
457: rcp r[1].x, r[4].w
462: mul r[5].xyz, r[1].xxxx, r[4].xyzx
469: mad r[1].xyz, r[4].xyzx, r[1].xxxx, -cb[0][0][54].xyzx
481: dp3 r[3].w, r[1].xyzx, r[1].xyzx
488: rsq r[3].w, r[3].w
493: mul r[1].xyz, r[1].xyzx, r[3].wwww
500: ld r[4].xyzw, r[0].xyww, t[0][0].xzyw
508: mad r[3].w, r[4].w, l(255), l(0.5)
517: ftou r[3].w, r[3].w
522: and r[3].w, r[3].w, l(254)
529: utof r[6].z, r[3].w
534: mov r[6].xy, r[4].xzxx
539: mad r[6].xyz, r[6].xyzx, l(255,255,1,0), l(0.5,0.5,0.5,0)
554: round_ni r[6].xyz, r[6].xyzx
559: ftoi r[6].xyz, r[6].xyzx
564: ubfe r[7].xyzw, l(3,4,3,4), l(1,4,1,4), r[6].zzzz
579: bfi r[6].xyzw, l(29,28,29,28), l(3,4,3,4), r[6].xyxy, r[7].xyzw
596: itof r[6].xyzw, r[6].xyzw
601: mad r[6].xyzw, r[6].xyzw, l(0.00097703957,0.000488400517,0.00097703957,0.000488400517), l(-1,-1,-1,-1)
616: mul r[6].xyzw, r[6].xyzw, l(1.29999995,1.29999995,2.5999999,2.5999999)
626: dp2 r[3].w, r[6].xyxx, r[6].xyxx
633: add r[6].xy, r[3].wwww, l(1,-1,0,0)
643: div r[6].xyz, r[6].zwyz, r[6].xxxx
650: mul r[7].xyz, r[6].yyyy, cb[0][0][7].xyzx
659: mad r[7].xyz, cb[0][0][6].xyzx, r[6].xxxx, r[7].xyzx
670: mad r[7].xyz, cb[0][0][8].xyzx, r[6].zzzz, r[7].xyzx
681: ld r[8].xyz, r[0].xyww, t[1][1].yzwx
689: mul r[4].zw, r[8].yyyz, l(0,0,255,255)
699: ftou r[4].zw, r[4].zzzw
704: ishl r[3].w, r[4].z, l(8)
711: or r[3].w, r[4].w, r[3].w
718: dp3 r[4].x, r[6].xyzx, -r[3].xyzx
726: ld_structured r[3].x, r[3].w, l(0), t[4][4].xxxx
736: ushr r[3].y, r[3].x, l(24)
743: ubfe r[3].xz, l(8,0,8,0), l(16,0,8,0), r[3].xxxx
758: utof r[6].xyz, r[3].yxzy
763: mul r[3].xyz, r[6].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
773: ld_structured r[3].w, r[3].w, l(0), t[4][4].xxxx
783: and r[3].w, r[3].w, l(255)
790: dp3 r[4].z, r[3].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
800: add r[4].w, r[4].z, l(9.99999997e-07)
807: rcp r[4].w, r[4].w
812: mul r[4].z, r[4].z, l(10000)
819: min r[4].z, r[4].z, l(1)
826: mad r[6].xyz, r[3].xyzx, r[4].wwww, l(-1,-1,-1,0)
838: mad r[6].xyz, r[4].zzzz, r[6].xyzx, l(1,1,1,0)
850: mul r[6].xyz, r[8].xxxx, r[6].xyzx
857: movc r[3].xyz, r[3].wwww, r[3].xyzx, r[6].xyzx
866: mul r[1].xyz, r[1].wwww, r[1].xyzx
873: mad r[1].xyz, -r[1].xyzx, l(0.00100000005,0.00100000005,0.00100000005,0), r[5].xyzx
886: mul r[6].xyz, r[1].wwww, r[7].xyzx
893: mad r[1].xyz, r[6].xyzx, l(0.00100000005,0.00100000005,0.00100000005,0), r[1].xyzx
905: sample_l r[4].xyz, r[4].xyxx, t[14][14].xyzw, s[1][6], l(0)
918: mul r[3].w, cb[1][1][15].w, l(0.318309873)
927: ld_structured r[6].xyz, l(0), l(0), t[13][13].xyzx
937: ld_structured r[8].xyz, l(0), l(16), t[13][13].xyzx
947: ld_structured r[9].xy, l(0), l(32), t[13][13].xyxx
957: ld_structured r[9].zw, l(0), l(48), t[13][13].xxxy
967: ld_structured r[10].xy, l(0), l(64), t[13][13].xyxx
977: ld_structured r[11].xw, l(0), l(80), t[13][13].xxxy
987: ld_structured r[4].w, l(0), l(96), t[13][13].xxxx
997: ld_structured r[5].w, l(0), l(104), t[13][13].xxxx
1007: and r[5].w, r[5].w, l(1)
1014: mad r[3].xyz, r[3].xyzx, r[4].xxxx, r[4].yyyy
1023: add r[3].xyz, -r[3].xyzx, l(1,1,1,0)
1034: max r[3].xyz, r[3].xyzx, l(0,0,0,0)
1044: mul r[3].xyz, r[4].zzzz, r[3].xyzx
1051: mov r[12].w, l(1)
1056: mov r[13].z, l(0)
1061: mov r[14].xy, r[9].xzxx
1066: mov r[14].z, r[10].x
1071: mov r[14].w, r[11].x
1076: mov r[15].w, l(1)
1081: mov r[11].xy, r[9].ywyy
1086: mov r[11].z, r[10].y
1091: mov r[9].w, l(1)
1096: mov r[10].w, l(1)
1101: mov r[4].xyz, l(0,0,0,0)
1109: mov r[6].w, cb[1][1][16].z
1116: mov r[7].w, l(0)
1121: mov r[0].z, l(0)
1126: loop 
1127: ige r[8].w, r[0].z, cb[4][4][0].z
1136: breakc r[8].w
1139: ld r[8].w, r[0].xyzw, t[21][26].yzwx
1147: ld r[16].xyzw, r[0].xyzw, t[23][28].xyzw
1155: mul r[17].xyz, r[16].yyyy, cb[0][0][7].xyzx
1164: mad r[17].xyz, cb[0][0][6].xyzx, r[16].xxxx, r[17].xyzx
1175: mad r[17].xyz, cb[0][0][8].xyzx, r[16].zzzz, r[17].xyzx
1186: add r[17].xyz, r[17].xyzx, cb[0][0][9].xyzx
1195: ld r[18].xyzw, r[0].xyzw, t[22][27].xyzw
1203: add r[19].xyz, -r[1].xyzx, r[17].xyzx
1211: dp3 r[17].w, r[19].xyzx, r[19].xyzx
1218: rsq r[17].w, r[17].w
1223: mul r[19].xyz, r[17].wwww, r[19].xyzx
1230: ieq r[17].w, r[8].w, l(9.18340949e-41)
1237: mad r[20].xyz, cb[4][4][0].yyyy, r[19].xyzx, r[5].xyzx
1248: movc r[12].xyz, r[17].wwww, r[20].xyzx, r[17].xyzx
1257: movc r[20].xyz, r[17].wwww, r[7].xyzx, r[18].xyzx
1266: mov r[21].xyz, l(0,0,0,0)
1274: mov r[19].w, l(0)
1279: mov r[20].w, l(0)
1284: loop 
1285: uge r[21].w, r[20].w, cb[2][2][0].w
1294: breakc r[21].w
1297: ishl r[21].w, r[20].w, l(2)
1304: dp4 r[22].x, cb[3][3][r[21].w].xyzw, r[12].xyzw
1314: dp4 r[22].y, cb[3][3][1+r[21].w].xyzw, r[12].xyzw
1325: dp4 r[22].z, cb[3][3][2+r[21].w].xyzw, r[12].xyzw
1336: ge r[23].xyz, r[22].xyzx, l(0,0,0,0)
1346: and r[22].w, r[23].y, r[23].x
1353: and r[22].w, r[23].z, r[22].w
1360: utof r[23].xyz, cb[3][3][3+r[21].w].xyzx
1369: lt r[24].xyz, r[22].xyzx, r[23].xyzx
1376: and r[23].w, r[24].y, r[24].x
1383: and r[23].w, r[24].z, r[23].w
1390: and r[22].w, r[22].w, r[23].w
1397: if r[22].w
1400: ld_structured r[24].xyz, r[20].w, l(0), t[15][15].xyzx
1410: ld_structured r[25].xyz, r[20].w, l(12), t[15][15].xyzx
1420: ld_structured r[26].xyzw, r[20].w, l(24), t[15][15].xyzw
1430: ld_structured r[27].xyzw, r[20].w, l(40), t[15][15].xyzw
1440: ld_structured r[28].xyz, r[20].w, l(56), t[15][15].xyzx
1450: mul_sat r[22].w, r[22].x, r[26].w
1457: add r[23].xyz, -r[22].xyzx, r[23].xyzx
1465: mul_sat r[23].x, r[26].w, r[23].x
1472: mul r[22].w, r[22].w, r[23].x
1479: mul_sat r[23].xw, r[22].yyyz, r[27].xxxy
1486: mul r[22].w, r[22].w, r[23].x
1493: mul_sat r[23].xy, r[27].xyxx, r[23].yzyy
1500: mul r[22].w, r[22].w, r[23].x
1507: mul r[22].w, r[23].w, r[22].w
1514: mul r[22].w, r[23].y, r[22].w
1521: ine r[23].x, r[28].x, l(nan)
1528: if r[23].x
1531: round_ni r[23].xyz, r[22].xyzx
1536: add r[22].xyz, r[22].xyzx, -r[23].xyzx
1544: ftou r[23].xyz, r[23].xyzx
1549: imad r[23].y, r[23].z, cb[3][3][3+r[21].w].y, r[23].y
1562: imad r[21].w, r[23].y, cb[3][3][3+r[21].w].x, r[23].x
1575: iadd r[21].w, r[27].z, r[21].w
1582: ld_structured r[21].w, r[21].w, l(0), t[16][16].xxxx
1592: mov r[23].xyz, l(0,0,0,0)
1600: mov r[27].xyz, r[22].xyzx
1605: mov r[23].w, r[21].w
1610: mov r[24].w, r[28].y
1615: loop 
1616: and r[25].w, r[23].w, l(-0)
1623: ine r[25].w, r[25].w, l(0)
1630: ge r[26].w, l(9.99999975e-06), r[24].w
1637: or r[25].w, r[25].w, r[26].w
1644: if r[25].w
1647: and r[25].w, r[23].w, l(nan)
1654: iadd r[25].w, r[28].x, r[25].w
1661: and r[29].x, r[25].w, l(127)
1668: ubfe r[29].y, l(7), l(7), r[25].w
1677: ushr r[29].z, r[25].w, l(14)
1684: ishl r[29].xyz, r[29].xyzx, l(2,2,2,0)
1694: utof r[29].xyz, r[29].xyzx
1699: mad r[29].xyz, r[27].xyzx, l(3,3,3,0), r[29].xyzx
1711: add r[23].xyz, r[29].xyzx, l(0.5,0.5,0.5,0)
1721: break 
1722: endif 
1723: iadd r[25].w, r[27].w, r[23].w
1730: ld_structured r[29].xyzw, r[25].w, l(0), t[17][17].xyzw
1740: mul r[30].xyz, r[27].xyzx, l(3,3,3,0)
1750: round_ni r[30].xyz, r[30].xyzx
1755: ftou r[31].xyz, r[30].xyzx
1760: utof r[31].xz, r[31].zzxz
1765: imul null, r[25].w, r[31].y, l(3)
1773: utof r[25].w, r[25].w
1778: mad r[25].w, r[31].x, l(9), r[25].w
1787: add r[25].w, r[31].z, r[25].w
1794: ftou r[25].w, r[25].w
1799: ishl r[26].w, l(1), r[25].w
1806: and r[28].w, r[26].w, r[29].x
1813: if r[28].w
1816: iadd r[28].w, r[28].x, r[23].w
1823: and r[31].x, r[28].w, l(127)
1830: ubfe r[31].y, l(7), l(7), r[28].w
1839: ushr r[31].z, r[28].w, l(14)
1846: ishl r[31].xyz, r[31].xyzx, l(2,2,2,0)
1856: utof r[31].xyz, r[31].xyzx
1861: mad r[31].xyz, r[27].xyzx, l(3,3,3,0), r[31].xyzx
1873: add r[23].xyz, r[31].xyzx, l(0.5,0.5,0.5,0)
1883: break 
1884: endif 
1885: mad r[27].xyz, r[27].xyzx, l(3,3,3,0), -r[30].xyzx
1898: and r[26].w, r[26].w, r[29].y
1905: ubfe r[28].w, r[25].w, l(0), r[29].y
1914: countbits r[28].w, r[28].w
1919: iadd r[28].w, r[28].w, r[29].w
1926: or r[28].w, r[28].w, l(-0)
1933: not r[29].y, r[29].y
1938: and r[29].x, r[29].y, r[29].x
1945: ubfe r[25].w, r[25].w, l(0), r[29].x
1954: countbits r[25].w, r[25].w
1959: iadd r[25].w, r[25].w, r[29].z
1966: movc r[23].w, r[26].w, r[28].w, r[25].w
1975: mul r[24].w, r[24].w, l(0.333333343)
1982: mov r[23].xyz, l(0,0,0,0)
1990: endloop 
1991: else 
1992: mov r[23].xyz, l(0,0,0,0)
2000: endif 
2001: mul r[22].xyz, r[23].xyzx, cb[2][2][0].xyzx
2010: mov r[27].x, r[24].x
2015: mov r[27].y, r[25].x
2020: mov r[27].z, r[26].x
2025: dp3 r[27].x, r[27].xyzx, r[20].xyzx
2032: mov r[29].x, r[24].y
2037: mov r[29].y, r[25].y
2042: mov r[29].z, r[26].y
2047: dp3 r[27].y, r[29].xyzx, r[20].xyzx
2054: mov r[26].x, r[24].z
2059: mov r[26].y, r[25].z
2064: dp3 r[27].z, r[26].xyzx, r[20].xyzx
2071: mov_sat r[24].xyz, r[27].xyzx
2076: mul r[24].xyz, r[24].xyzx, r[24].xyzx
2083: mov_sat r[27].xyz, -r[27].xyzx
2089: mul r[25].xyz, r[27].xyzx, r[27].xyzx
2096: lt r[26].xyz, l(0,0,0,0), r[24].xyzx
2106: if r[26].x
2109: sample_l r[27].xyz, r[22].xyzx, t[18][18].xyzw, s[1][6], l(0)
2122: mul r[27].xyz, r[24].xxxx, r[27].xyzx
2129: else 
2130: mov r[27].xyz, l(0,0,0,0)
2138: endif 
2139: lt r[29].xyz, l(0,0,0,0), r[25].xyzx
2149: if r[29].x
2152: sample_l r[30].xyz, r[22].xyzx, t[18][19].xyzw, s[1][6], l(0)
2165: mad r[27].xyz, r[30].xyzx, r[25].xxxx, r[27].xyzx
2174: endif 
2175: if r[26].y
2178: sample_l r[26].xyw, r[22].xyzx, t[18][20].xywz, s[1][6], l(0)
2191: mad r[27].xyz, r[26].xywx, r[24].yyyy, r[27].xyzx
2200: endif 
2201: if r[29].y
2204: sample_l r[24].xyw, r[22].xyzx, t[18][21].xywz, s[1][6], l(0)
2217: mad r[27].xyz, r[24].xywx, r[25].yyyy, r[27].xyzx
2226: endif 
2227: if r[26].z
2230: sample_l r[24].xyw, r[22].xyzx, t[18][22].xywz, s[1][6], l(0)
2243: mad r[27].xyz, r[24].xywx, r[24].zzzz, r[27].xyzx
2252: endif 
2253: if r[29].z
2256: sample_l r[22].xyz, r[22].xyzx, t[18][23].xyzw, s[1][6], l(0)
2269: mad r[27].xyz, r[22].xyzx, r[25].zzzz, r[27].xyzx
2278: endif 
2279: ieq r[21].w, r[28].x, l(nan)
2286: movc r[22].xyz, r[21].wwww, l(1,0.349999994,0,0), r[27].xyzx
2298: mad r[21].w, r[22].w, l(-2), l(3)
2307: mul r[23].w, r[22].w, r[22].w
2314: mul r[21].w, r[21].w, r[23].w
2321: add r[23].w, r[28].z, l(-1)
2328: mad r[21].w, r[21].w, r[23].w, l(1)
2337: mul r[23].w, r[21].w, r[22].w
2344: mad r[21].xyz, r[22].xyzx, r[23].wwww, r[21].xyzx
2353: mad r[19].w, r[22].w, r[21].w, r[19].w
2362: endif 
2363: iadd r[20].w, r[20].w, l(1)
2370: endloop 
2371: lt r[12].x, l(0), r[19].w
2378: div r[20].xyz, r[21].xyzx, r[19].wwww
2385: movc r[12].xyz, r[12].xxxx, r[20].xyzx, r[21].xyzx
2394: add r[12].xyz, r[12].xyzx, cb[2][2][1].xyzx
2403: mad r[20].xyz, r[2].xyzx, r[2].wwww, -r[16].xyzx
2413: dp3 r[20].x, r[20].xyzx, r[20].xyzx
2420: sqrt r[20].x, r[20].x
2425: movc r[20].x, r[17].w, cb[4][4][0].x, r[20].x
2436: add r[7].w, r[7].w, r[20].x
2443: if r[17].w
2446: mul r[20].yzw, r[3].wwww, r[12].xxyz
2453: else 
2454: ld_structured r[22].xyz, r[8].w, l(52), t[5][5].xyzx
2464: imul null, r[17].w, r[8].w, l(35)
2472: mov r[24].x, r[18].w
2477: mov r[24].y, r[16].w
2482: sample_l r[25].xyz, r[24].xyxx, t[24][r[17].w].xyzw, s[0][5], l(0)
2497: mul r[22].xyz, r[22].xyzx, r[25].xyzx
2504: dp3 r[16].w, r[22].xyzx, r[22].xyzx
2511: ge r[16].w, r[16].w, l(9.99999975e-05)
2518: if r[16].w
2521: mul_sat r[16].w, r[20].x, l(50)
2528: mul r[12].xyz, r[12].xyzx, r[16].wwww
2535: ld_structured r[24].zw, r[8].w, l(68), t[5][5].xxxy
2545: imad r[25].xy, r[8].wwww, l(35,35,0,0), l(2,4,0,0)
2560: sample_l r[16].w, r[24].xyxx, t[24][r[25].x].yzwx, s[4][19], l(0)
2575: add r[17].w, -r[24].z, r[24].w
2583: mad r[26].y, r[16].w, r[17].w, r[24].z
2592: sample_l r[16].w, r[24].xyxx, t[24][r[25].y].yzwx, s[4][19], l(0)
2607: ld_structured r[8].w, r[8].w, l(0), t[4][4].xxxx
2617: ushr r[17].w, r[8].w, l(24)
2624: ubfe r[24].xy, l(8,8,0,0), l(16,8,0,0), r[8].wwww
2639: utof r[25].x, r[17].w
2644: utof r[25].yz, r[24].xxyx
2649: mul r[24].xyz, r[16].wwww, r[25].xyzx
2656: mul r[24].xyz, r[24].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
2666: dp3 r[8].w, r[24].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
2676: dp3 r[26].x, r[18].xyzx, -r[19].xyzx
2684: sample_l r[19].xyz, r[26].xyxx, t[14][14].xyzw, s[1][6], l(0)
2697: mad r[24].xyz, r[24].xyzx, r[19].xxxx, r[19].yyyy
2706: add r[24].xyz, -r[24].xyzx, l(1,1,1,0)
2717: mul r[24].xyz, r[24].xyzx, cb[1][1][15].wwww
2726: mul r[19].xyz, r[19].zzzz, r[24].xyzx
2733: mul r[12].xyz, r[12].xyzx, r[19].xyzx
2740: mul r[19].xyz, r[17].yyyy, cb[0][0][3].xyzx
2749: mad r[17].xyw, cb[0][0][2].xyxz, r[17].xxxx, r[19].xyxz
2760: mad r[17].xyz, cb[0][0][4].xyzx, r[17].zzzz, r[17].xywx
2771: add r[15].xyz, r[17].xyzx, cb[0][0][5].xyzx
2780: mul r[17].xyz, r[18].yyyy, cb[0][0][3].xyzx
2789: mad r[17].xyz, cb[0][0][2].xyzx, r[18].xxxx, r[17].xyzx
2800: mad r[17].xyz, cb[0][0][4].xyzx, r[18].zzzz, r[17].xyzx
2811: add r[16].xyz, r[16].xyzx, l(150,75,75,0)
2821: mul r[16].xyz, r[16].xyzx, l(0.0533333346,0.0533333346,0.0533333346,0)
2831: round_ni r[16].xyz, r[16].xyzx
2836: max r[16].xyz, r[16].xyzx, l(0,0,0,0)
2846: min r[16].xyz, r[16].xyzx, l(15,7,11,0)
2856: ftoi r[16].xyz, r[16].xyzx
2861: uge r[16].w, r[6].w, l(1)
2868: if r[16].w
2871: dp4 r[18].x, r[14].xyzw, r[15].xyzw
2878: dp4 r[18].y, r[11].xyzw, r[15].xyzw
2885: sample_l r[16].w, r[18].xyxx, t[12][12].yzwx, s[2][8], l(0)
2898: add r[16].w, r[16].w, l(-1)
2905: mad r[16].w, r[4].w, r[16].w, l(1)
2914: mul r[18].xyz, r[8].xyzx, r[16].wwww
2921: movc r[18].xyz, r[5].wwww, r[18].xyzx, r[8].xyzx
2930: dp3 r[16].w, -r[15].xyzx, -r[15].xyzx
2939: rsq r[16].w, r[16].w
2944: mul r[19].xyz, -r[15].xyzx, r[16].wwww
2952: mad r[24].xyz, -r[15].xyzx, r[16].wwww, r[6].xyzx
2962: dp3 r[16].w, r[24].xyzx, r[24].xyzx
2969: rsq r[16].w, r[16].w
2974: mul r[24].xyz, r[16].wwww, r[24].xyzx
2981: dp3 r[16].w, r[19].xyzx, r[24].xyzx
2988: dp3_sat r[17].w, r[17].xyzx, r[6].xyzx
2995: add r[18].w, -r[8].w, l(1)
3003: add r[16].w, -abs(r[16].w), l(1)
3011: max r[16].w, r[16].w, l(0)
3018: mul r[19].x, r[16].w, r[16].w
3025: mul r[19].x, r[19].x, r[19].x
3032: mul r[16].w, r[16].w, r[19].x
3039: mad r[16].w, r[18].w, r[16].w, r[8].w
3048: add_sat r[16].w, -r[16].w, l(1)
3056: mul r[16].w, r[17].w, r[16].w
3063: mul r[16].w, r[16].w, l(0.318309873)
3070: mul r[18].xyz, r[18].xyzx, r[16].wwww
3077: else 
3078: mov r[18].xyz, l(0,0,0,0)
3086: endif 
3087: imad r[13].x, r[16].x, l(12), r[16].z
3096: ishl r[13].y, r[16].y, l(8)
3103: ld r[16].x, r[13].xyzz, t[19][24].xyzw
3111: dp3 r[16].y, -r[15].xyzx, -r[15].xyzx
3120: mad r[10].xyz, r[17].xyzx, l(0.0700000003,0.0700000003,0.0700000003,0), r[15].xyzx
3132: rsq r[16].y, r[16].y
3137: mul r[16].yzw, -r[15].xxyz, r[16].yyyy
3145: add r[17].w, -r[8].w, l(1)
3153: mov r[19].xyz, r[18].xyzx
3158: mov r[18].w, l(0)
3163: mov r[20].x, r[16].x
3168: loop 
3169: ine r[21].w, r[20].x, l(9.18340949e-41)
3176: ilt r[22].w, r[18].w, l(256)
3183: and r[21].w, r[21].w, r[22].w
3190: breakc r[21].w
3193: ld_structured r[24].xyzw, r[20].x, l(0), t[7][7].xyzw
3203: ld_structured r[25].xyzw, r[20].x, l(16), t[7][7].xyzw
3213: ld_structured r[26].xyzw, r[20].x, l(36), t[7][7].xyzw
3223: ld_structured r[27].xyzw, r[20].x, l(52), t[7][7].xyzw
3233: ld_structured r[21].w, r[20].x, l(144), t[7][7].xxxx
3243: ld_structured r[22].w, r[20].x, l(156), t[7][7].xxxx
3253: add r[24].xyz, -r[15].xyzx, r[24].xyzx
3261: dp3 r[23].w, r[24].xyzx, r[24].xyzx
3268: rsq r[28].x, r[23].w
3273: mul r[28].yzw, r[24].xxyz, r[28].xxxx
3280: sqrt r[29].x, r[23].w
3285: add r[29].y, r[25].w, -r[29].x
3293: mul r[25].w, r[25].w, l(0.200000003)
3300: div_sat r[25].w, r[29].y, r[25].w
3307: mad r[29].y, r[25].w, l(-2), l(3)
3316: mul r[25].w, r[25].w, r[25].w
3323: mul r[25].w, r[25].w, r[29].y
3330: mad r[21].w, r[21].w, r[21].w, r[23].w
3339: max r[21].w, r[21].w, l(9.99999975e-05)
3346: mul r[21].w, r[21].w, l(3.14159274)
3353: rcp r[21].w, r[21].w
3358: add r[21].w, -r[26].x, r[21].w
3366: max r[21].w, r[21].w, l(0)
3373: mul r[21].w, r[25].w, r[21].w
3380: mul r[25].xyz, r[21].wwww, r[25].xyzx
3387: and r[29].yz, r[22].wwww, l(0,4,2,0)
3397: mov r[30].x, r[26].w
3402: mov r[30].yz, r[27].xxyx
3407: dp3 r[21].w, -r[28].yzwy, r[30].xyzx
3415: mad_sat r[21].w, r[21].w, r[26].y, r[26].z
3424: mul r[21].w, r[21].w, r[21].w
3431: mul r[26].xyz, r[21].wwww, r[25].xyzx
3438: movc r[25].xyz, r[29].yyyy, r[26].xyzx, r[25].xyzx
3447: if r[29].z
3450: ld_structured r[26].xy, r[20].x, l(68), t[7][7].xyxx
3460: ld_structured r[29].yzw, r[20].x, l(80), t[7][7].xxyz
3470: ld_structured r[30].xyz, r[20].x, l(96), t[7][7].xyzx
3480: ld_structured r[31].xyw, r[20].x, l(112), t[7][7].xyxz
3490: ld_structured r[32].xyzw, r[20].x, l(128), t[7][7].xyzw
3500: mov r[33].x, r[27].w
3505: mov r[33].y, r[29].y
3510: mov r[33].z, r[30].x
3515: mov r[33].w, r[31].x
3520: dp4 r[33].x, r[33].xyzw, r[10].xyzw
3527: mov r[34].x, r[26].x
3532: mov r[34].y, r[29].z
3537: mov r[34].z, r[30].y
3542: mov r[34].w, r[31].y
3547: dp4 r[33].y, r[34].xyzw, r[10].xyzw
3554: mov r[31].x, r[26].y
3559: mov r[31].y, r[29].w
3564: mov r[31].z, r[30].z
3569: dp4 r[33].z, r[31].xyzw, r[10].xyzw
3576: dp3 r[21].w, r[33].xyzx, r[33].xyzx
3583: rsq r[21].w, r[21].w
3588: mul r[26].xyz, r[21].wwww, r[33].xyzx
3595: lt r[30].xyzw, abs(r[26].yzxz), abs(r[26].xxyy)
3604: lt r[27].xyw, l(0,0,0,0), r[26].xyxz
3614: and r[29].yz, r[30].yywy, r[30].xxzx
3621: movc r[27].xyw, r[27].xyxw, l(0,2,0,4), l(1,3,0,5)
3636: movc r[21].w, r[29].z, r[27].y, r[27].w
3645: movc r[21].w, r[29].y, r[27].x, r[21].w
3654: if r[21].w
3657: ine r[22].w, r[21].w, l(1)
3664: if r[22].w
3667: div r[29].yzw, r[26].xxxy, r[26].yyzz
3674: div r[22].w, -r[26].z, r[26].y
3682: mad r[30].y, r[22].w, l(-0.5), l(0.5)
3691: div r[26].xw, r[26].xxxz, -r[26].yyyy
3699: mad r[26].xw, r[26].xxxw, l(0.5,0,0,-0.5), l(0.5,0,0,0.5)
3714: ieq r[31].xyz, r[21].wwww, l(2,3,4,0)
3724: mad r[33].xyz, r[29].yzwy, l(0.5,0.5,-0.5,0), l(0.5,0.5,0.5,0)
3739: div r[22].w, r[26].y, -r[26].z
3747: mad r[33].w, r[22].w, l(-0.5), l(0.5)
3756: movc r[26].yz, r[31].zzzz, r[33].yyzy, r[33].yywy
3765: movc r[26].xy, r[31].yyyy, r[26].xwxx, r[26].yzyy
3774: mov r[30].x, r[33].x
3779: movc r[27].xy, r[31].xxxx, r[30].xyxx, r[26].xyxx
3788: else 
3789: div r[29].yz, r[26].zzyz, -r[26].xxxx
3797: mad r[27].xy, r[29].yzyy, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
3812: endif 
3813: else 
3814: div r[22].w, -r[26].z, r[26].x
3822: mad r[27].x, r[22].w, l(0.5), l(0.5)
3831: div r[22].w, r[26].y, r[26].x
3838: mad r[27].y, r[22].w, l(-0.5), l(0.5)
3847: endif 
3848: add r[26].xy, r[27].xyxx, l(-0.5,-0.5,0,0)
3858: mad r[26].yz, r[26].xxyx, r[24].wwww, l(0,0.5,0.5,0)
3870: utof r[21].w, r[21].w
3875: mul r[21].w, r[21].w, l(0.166666672)
3882: mad r[26].x, r[26].y, l(0.166666672), r[21].w
3891: mad r[26].xy, r[26].xzxx, r[32].zwzz, r[32].xyxx
3900: sample_l r[26].xy, r[26].xyxx, t[6][6].xyzw, s[1][6], l(0)
3913: add r[21].w, r[29].x, l(-0.0500000007)
3920: mul r[22].w, r[27].z, r[21].w
3927: mad r[23].w, -r[26].x, r[26].x, r[26].y
3937: max r[23].w, r[23].w, l(9.99999975e-06)
3944: mad r[21].w, -r[21].w, r[27].z, r[26].x
3954: mad r[21].w, r[21].w, r[21].w, r[23].w
3963: div r[21].w, r[23].w, r[21].w
3970: min r[21].w, r[21].w, l(1)
3977: lt r[22].w, r[22].w, r[26].x
3984: movc r[21].w, r[22].w, l(1), r[21].w
3993: mul r[25].xyz, r[21].wwww, r[25].xyzx
4000: endif 
4001: mad r[24].xyz, r[24].xyzx, r[28].xxxx, r[16].yzwy
4010: dp3 r[21].w, r[24].xyzx, r[24].xyzx
4017: rsq r[21].w, r[21].w
4022: mul r[24].xyz, r[21].wwww, r[24].xyzx
4029: dp3 r[21].w, r[16].yzwy, r[24].xyzx
4036: dp3_sat r[22].w, r[17].xyzx, r[28].yzwy
4043: add r[21].w, -abs(r[21].w), l(1)
4051: max r[21].w, r[21].w, l(0)
4058: mul r[23].w, r[21].w, r[21].w
4065: mul r[23].w, r[23].w, r[23].w
4072: mul r[21].w, r[21].w, r[23].w
4079: mad r[21].w, r[17].w, r[21].w, r[8].w
4088: add_sat r[21].w, -r[21].w, l(1)
4096: mul r[21].w, r[22].w, r[21].w
4103: mul r[21].w, r[21].w, l(0.318309873)
4110: mad r[19].xyz, r[21].wwww, r[25].xyzx, r[19].xyzx
4119: iadd r[18].w, r[18].w, l(1)
4126: iadd r[13].w, r[13].y, r[18].w
4133: ld r[20].x, r[13].xwzz, t[19][24].xyzw
4141: endloop 
4142: ld r[10].x, r[13].xyzz, t[20][25].xyzw
4150: mul r[18].xyz, r[15].zzzz, r[17].xyzx
4157: mad r[9].xyz, r[18].xyzx, l(0.000300000014,0.000300000014,0.000300000014,0), r[15].xyzx
4169: mov r[18].xzw, r[13].xxzz
4174: mov r[24].xyz, r[19].xyzx
4179: mov r[10].y, l(0)
4184: mov r[10].z, r[10].x
4189: loop 
4190: ine r[13].x, r[10].z, l(9.18340949e-41)
4197: ilt r[13].w, r[10].y, l(256)
4204: and r[13].x, r[13].w, r[13].x
4211: breakc r[13].x
4214: iadd r[10].y, r[10].y, l(1)
4221: iadd r[18].y, r[10].y, r[13].y
4228: ld r[13].x, r[18].xyzw, t[20][25].xyzw
4236: ld_structured r[25].xyzw, r[10].z, l(68), t[11][11].xyzw
4246: ld_structured r[26].xyz, r[10].z, l(84), t[11][11].xyzx
4256: ld_structured r[27].xyzw, r[10].z, l(96), t[11][11].xyzw
4266: ld_structured r[28].xyzw, r[10].z, l(112), t[11][11].xyzw
4276: ld_structured r[29].xyzw, r[10].z, l(128), t[11][11].xyzw
4286: mov r[30].x, r[25].w
4291: mov r[30].y, r[27].x
4296: mov r[30].z, r[28].x
4301: mov r[30].w, r[29].x
4306: dp4 r[30].x, r[30].xyzw, r[9].xyzw
4313: mov r[31].x, r[26].x
4318: mov r[31].y, r[27].y
4323: mov r[31].z, r[28].y
4328: mov r[31].w, r[29].y
4333: dp4 r[30].y, r[31].xyzw, r[9].xyzw
4340: mov r[31].x, r[26].y
4345: mov r[31].y, r[27].z
4350: mov r[31].z, r[28].z
4355: mov r[31].w, r[29].z
4360: dp4 r[30].z, r[31].xyzw, r[9].xyzw
4367: mov r[29].x, r[26].z
4372: mov r[29].y, r[27].w
4377: mov r[29].z, r[28].w
4382: dp4 r[13].w, r[29].xyzw, r[9].xyzw
4389: rcp r[16].x, r[13].w
4394: mul r[26].xyz, r[16].xxxx, r[30].xyzx
4401: mad r[27].xyz, r[26].xyzx, l(0.5,0.5,1,0), l(0.5,0.5,0,0)
4416: mov_sat r[28].xyz, r[27].xyzx
4421: eq r[27].xyz, r[27].xyzx, r[28].xyzx
4428: and r[16].x, r[27].y, r[27].x
4435: and r[16].x, r[27].z, r[16].x
4442: if r[16].x
4445: ld_structured r[27].xyzw, r[10].z, l(0), t[11][11].xyzw
4455: ld_structured r[16].x, r[10].z, l(156), t[11][11].xxxx
4465: ld_structured r[18].y, r[10].z, l(200), t[11][11].xxxx
4475: ld_structured r[28].xyzw, r[10].z, l(288), t[11][11].xyzw
4485: ld_structured r[20].x, r[10].z, l(304), t[11][11].xxxx
4495: mad r[29].xy, r[26].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
4510: and r[29].zw, r[20].xxxx, l(0,0,2,768)
4520: if r[29].z
4523: ld_structured r[31].xyzw, r[10].z, l(272), t[11][11].xyzw
4533: mad r[30].xy, r[29].xyxx, r[31].zwzz, r[31].xyxx
4542: sample_l r[30].xy, r[30].xyxx, t[9][9].xyzw, s[1][6], l(0)
4555: add r[31].xyz, -r[15].xyzx, r[27].xyzx
4563: dp3 r[21].w, r[31].xyzx, r[31].xyzx
4570: sqrt r[21].w, r[21].w
4575: add r[21].w, -r[27].w, r[21].w
4583: mul r[22].w, r[16].x, r[21].w
4590: mad r[23].w, -r[30].x, r[30].x, r[30].y
4600: max r[23].w, r[23].w, l(9.99999975e-06)
4607: mad r[21].w, -r[21].w, r[16].x, r[30].x
4617: mad r[21].w, r[21].w, r[21].w, r[23].w
4626: div r[21].w, r[23].w, r[21].w
4633: min r[21].w, r[21].w, l(1)
4640: lt r[22].w, r[22].w, r[30].x
4647: movc r[21].w, r[22].w, l(1), r[21].w
4656: else 
4657: mov r[21].w, l(1)
4662: endif 
4663: ieq r[30].xy, r[29].wwww, l(256,512,0,0)
4673: if r[30].x
4676: mad r[30].xw, r[29].xxxy, r[28].zzzw, r[28].xxxy
4685: sample_l r[22].w, r[30].xwxx, t[8][8].yzwx, s[1][6], l(0)
4698: ge r[22].w, r[22].w, r[26].z
4705: and r[22].w, r[22].w, l(1)
4712: else 
4713: ieq r[23].w, r[29].w, l(512)
4720: if r[23].w
4723: mad r[26].zw, r[29].xxxy, r[28].zzzw, r[28].xxxy
4732: sample_l r[26].zw, r[26].zwzz, t[9][9].zwxy, s[1][6], l(0)
4745: add r[28].xyz, -r[15].xyzx, r[27].xyzx
4753: dp3 r[23].w, r[28].xyzx, r[28].xyzx
4760: sqrt r[23].w, r[23].w
4765: add r[23].w, -r[27].w, r[23].w
4773: mul r[24].w, r[16].x, r[23].w
4780: mad r[25].w, -r[26].z, r[26].z, r[26].w
4790: max r[25].w, r[25].w, l(9.99999975e-06)
4797: mad r[16].x, -r[23].w, r[16].x, r[26].z
4807: mad r[16].x, r[16].x, r[16].x, r[25].w
4816: div r[16].x, r[25].w, r[16].x
4823: min r[16].x, r[16].x, l(1)
4830: lt r[23].w, r[24].w, r[26].z
4837: movc r[22].w, r[23].w, l(1), r[16].x
4846: else 
4847: mov r[22].w, l(1)
4852: endif 
4853: endif 
4854: ine r[16].x, r[29].z, l(0)
4861: and r[16].x, r[16].x, r[30].y
4868: add r[23].w, -r[21].w, r[22].w
4876: mad r[23].w, r[18].y, r[23].w, r[21].w
4885: mad r[22].w, r[22].w, r[18].y, l(1)
4894: add r[18].y, -r[18].y, r[22].w
4902: min r[18].y, r[18].y, r[21].w
4909: movc r[16].x, r[16].x, r[23].w, r[18].y
4918: ne r[18].y, r[16].x, l(0)
4925: if r[18].y
4928: ld_structured r[28].xyz, r[10].z, l(16), t[11][11].xyzx
4938: ld_structured r[31].xyzw, r[10].z, l(48), t[11][11].xyzw
4948: add r[18].y, -r[30].z, r[31].w
4956: mul r[21].w, r[31].w, l(0.200000003)
4963: div_sat r[18].y, r[18].y, r[21].w
4970: mad r[21].w, r[18].y, l(-2), l(3)
4979: mul r[18].y, r[18].y, r[18].y
4986: mul r[18].y, r[18].y, r[21].w
4993: add r[28].xyz, -r[15].xyzx, r[28].xyzx
5001: dp3 r[21].w, r[28].xyzx, r[28].xyzx
5008: mul r[22].w, r[25].y, r[25].y
5015: mad r[21].w, r[25].z, r[21].w, r[22].w
5024: max r[21].w, r[21].w, l(0.25)
5031: mul r[21].w, r[21].w, l(3.14159274)
5038: rcp r[21].w, r[21].w
5043: add r[21].w, -r[25].x, r[21].w
5051: max r[21].w, r[21].w, l(0)
5058: mul r[18].y, r[18].y, r[21].w
5065: mul r[25].xyz, r[18].yyyy, r[31].xyzx
5072: and r[18].y, r[20].x, l(1)
5079: if r[18].y
5082: ld_structured r[28].xyzw, r[10].z, l(256), t[11][11].xyzw
5092: and r[18].y, r[20].x, l(1.43492963e-42)
5099: add r[26].zw, r[26].xxxy, r[26].xxxy
5106: mad r[20].x, r[26].x, r[26].x, l(1)
5115: mad r[20].x, r[26].y, r[26].y, r[20].x
5124: div r[26].xy, r[26].zwzz, r[20].xxxx
5131: mad r[26].xy, r[26].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
5146: movc r[26].xy, r[18].yyyy, r[26].xyxx, r[29].xyxx
5155: mad r[26].xy, r[26].xyxx, r[28].zwzz, r[28].xyxx
5164: sample_l r[26].xyz, r[26].xyxx, t[10][10].xyzw, s[3][11], l(0)
5177: mul r[25].xyz, r[25].xyzx, r[26].xyzx
5184: endif 
5185: ge r[13].w, r[13].w, r[30].z
5192: and r[13].w, r[13].w, l(1)
5199: mul r[13].w, r[13].w, r[16].x
5206: mul r[25].xyz, r[13].wwww, r[25].xyzx
5213: add r[26].xyz, -r[15].xyzx, r[27].xyzx
5221: dp3 r[13].w, r[26].xyzx, r[26].xyzx
5228: rsq r[13].w, r[13].w
5233: mul r[27].xyz, r[13].wwww, r[26].xyzx
5240: mad r[26].xyz, r[26].xyzx, r[13].wwww, r[16].yzwy
5249: dp3 r[13].w, r[26].xyzx, r[26].xyzx
5256: rsq r[13].w, r[13].w
5261: mul r[26].xyz, r[13].wwww, r[26].xyzx
5268: dp3 r[13].w, r[16].yzwy, r[26].xyzx
5275: dp3_sat r[16].x, r[17].xyzx, r[27].xyzx
5282: add r[13].w, -abs(r[13].w), l(1)
5290: max r[13].w, r[13].w, l(0)
5297: mul r[18].y, r[13].w, r[13].w
5304: mul r[18].y, r[18].y, r[18].y
5311: mul r[13].w, r[13].w, r[18].y
5318: mad r[13].w, r[17].w, r[13].w, r[8].w
5327: add_sat r[13].w, -r[13].w, l(1)
5335: mul r[13].w, r[16].x, r[13].w
5342: mul r[13].w, r[13].w, l(0.318309873)
5349: mad r[24].xyz, r[13].wwww, r[25].xyzx, r[24].xyzx
5358: endif 
5359: endif 
5360: mov r[10].z, r[13].x
5365: endloop 
5366: mad r[9].xyz, r[12].xyzx, l(0.318309873,0.318309873,0.318309873,0), r[24].xyzx
5378: mul r[20].yzw, r[22].xxyz, r[9].xxyz
5385: else 
5386: mov r[20].yzw, l(0,0,0,0)
5394: endif 
5395: endif 
5396: mad r[4].xyz, r[3].xyzx, r[20].yzwy, r[4].xyzx
5405: iadd r[0].z, r[0].z, l(1)
5412: endloop 
5413: add r[0].xy, -cb[1][1][17].yzyy, cb[1][1][17].xwxx
5425: add r[0].z, cb[1][1][17].y, cb[1][1][17].x
5436: add r[0].z, -r[0].x, r[0].z
5444: add r[0].x, -r[0].x, r[1].w
5452: div r[0].z, l(1,1,1,1), r[0].z
5462: mul_sat r[0].x, r[0].z, r[0].x
5469: mad r[0].z, r[0].x, l(-2), l(3)
5478: mul r[0].x, r[0].x, r[0].x
5485: mul r[0].x, r[0].x, r[0].z
5492: mad r[0].x, r[0].x, r[0].y, cb[1][1][17].z
5503: mul r[0].xyz, r[0].xxxx, r[4].xyzx
5510: itof r[0].w, cb[4][4][0].z
5517: div r[1].xyz, r[0].xyzx, r[0].wwww
5524: div r[0].x, r[7].w, r[0].w
5531: mov r[1].w, l(1)
5536: else 
5537: mov r[1].xyzw, l(0,0,0,0)
5545: mov r[0].x, l(65504)
5550: endif 
5551: store_uav_typed type30[1][1].xyzw, type32.xyyy, r[0].xxxx
5558: store_uav_typed type30[0][0].xyzw, type32.xyyy, r[1].xyzw
5565: ret 