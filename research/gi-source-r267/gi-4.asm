2: dcl_global_flags raw:
3: dcl_constant_buffer raw:0x308e46,0x0,0x0,0x0,0x37,0x0
10: dcl_constant_buffer raw:0x308e46,0x1,0x1,0x1,0x12,0x0
17: dcl_constant_buffer raw:0x308e46,0x2,0x2,0x2,0x2,0x0
24: dcl_constant_buffer raw:0x308e46,0x3,0x3,0x3,0x600,0x0
31: dcl_constant_buffer raw:0x308e46,0x4,0x4,0x4,0x1,0x0
38: dcl_sampler raw:0x306e46,0x0,0x5,0x5,0x1
44: dcl_sampler raw:0x306e46,0x1,0x6,0x6,0x1
50: dcl_sampler raw:0x306e46,0x2,0xb,0xb,0x1
56: dcl_sampler raw:0x306e46,0x3,0x13,0x13,0x1
62: dcl_resource raw:0x307e46,0x0,0x0,0x0,0x5555,0x0
69: dcl_resource raw:0x307e46,0x1,0x1,0x1,0x5555,0x0
76: dcl_resource raw:0x307e46,0x2,0x2,0x2,0x5555,0x0
83: dcl_resource raw:0x307e46,0x3,0x3,0x3,0x5555,0x0
90: dcl_resource_structured raw:0x307e46,0x4,0x4,0x4,0x8,0x0
97: dcl_resource_structured raw:0x307e46,0x5,0x5,0x5,0x150,0x0
104: dcl_resource raw:0x307e46,0x6,0x6,0x6,0x5555,0x0
111: dcl_resource_structured raw:0x307e46,0x7,0x7,0x7,0xa0,0x0
118: dcl_resource raw:0x307e46,0x8,0x8,0x8,0x5555,0x0
125: dcl_resource raw:0x307e46,0x9,0x9,0x9,0x5555,0x0
132: dcl_resource raw:0x307e46,0xa,0xa,0xa,0x5555,0x0
139: dcl_resource_structured raw:0x307e46,0xb,0xb,0xb,0x140,0x0
146: dcl_resource raw:0x307e46,0xc,0xc,0xc,0x5555,0x0
153: dcl_resource_structured raw:0x307e46,0xd,0xd,0xd,0x50,0x0
160: dcl_resource_structured raw:0x307e46,0xe,0xe,0xe,0x4,0x0
167: dcl_resource_structured raw:0x307e46,0xf,0xf,0xf,0x10,0x0
174: dcl_resource raw:0x307e46,0x10,0x10,0x15,0x5555,0x0
181: dcl_resource raw:0x307e46,0x11,0x16,0x16,0x4444,0x0
188: dcl_resource raw:0x307e46,0x12,0x17,0x17,0x4444,0x0
195: dcl_resource raw:0x307e46,0x13,0x18,0x18,0x4444,0x0
202: dcl_resource raw:0x307e46,0x14,0x19,0x19,0x5555,0x0
209: dcl_resource raw:0x307e46,0x15,0x1a,0x1a,0x5555,0x0
216: dcl_resource raw:0x307e46,0x16,0x0,0xffffffff,0x5555,0x1
223: dcl_unordered_access_view_typed raw:0x31ee46,0x0,0x0,0x0,0x5555,0x0
230: dcl_unordered_access_view_typed raw:0x31ee46,0x1,0x1,0x1,0x5555,0x0
237: dcl_unordered_access_view_typed raw:0x31ee46,0x2,0x2,0x2,0x5555,0x0
244: dcl_input raw:0x20032
246: dcl_temps raw:0x1e
248: dcl_thread_group raw:0x8,0x10,0x1
252: mov r[0].xy, type32.xyxx
256: mov r[0].w, l(0)
261: ld r[1].x, r[0].xyww, t[19][24].xyzw
269: ine r[1].x, r[1].x, l(9.18326936e-41)
276: if r[1].x
279: utof r[1].xy, type32.xyxx
283: add r[1].xy, r[1].xyxx, l(0.5,0.5,0,0)
293: mul r[1].xy, r[1].xyxx, cb[0][0][1].zwzz
302: ld r[1].z, r[0].xyww, t[3][3].yzxw
310: ld r[1].w, r[0].xyww, t[2][2].yzwx
318: mad r[1].xy, r[1].xyxx, l(2,-2,0,0), l(-1,1,0,0)
333: mul r[2].xyzw, r[1].yyyy, cb[0][0][15].xyzw
342: mad r[2].xyzw, cb[0][0][14].xyzw, r[1].xxxx, r[2].xyzw
353: mad r[2].xyzw, cb[0][0][16].xyzw, r[1].zzzz, r[2].xyzw
364: add r[2].xyzw, r[2].xyzw, cb[0][0][17].xyzw
373: rcp r[2].w, r[2].w
378: mul r[3].xyz, r[2].wwww, r[2].xyzx
385: dp3 r[3].w, r[3].xyzx, r[3].xyzx
392: rsq r[3].w, r[3].w
397: mul r[3].xyz, r[3].wwww, r[3].xyzx
404: mul r[4].xyzw, r[1].yyyy, cb[0][0][23].xyzw
413: mad r[4].xyzw, cb[0][0][22].xyzw, r[1].xxxx, r[4].xyzw
424: mad r[4].xyzw, cb[0][0][24].xyzw, r[1].zzzz, r[4].xyzw
435: add r[4].xyzw, r[4].xyzw, cb[0][0][25].xyzw
444: rcp r[1].x, r[4].w
449: mul r[5].xyz, r[1].xxxx, r[4].xyzx
456: mad r[1].xyz, r[4].xyzx, r[1].xxxx, -cb[0][0][54].xyzx
468: dp3 r[3].w, r[1].xyzx, r[1].xyzx
475: rsq r[3].w, r[3].w
480: mul r[1].xyz, r[1].xyzx, r[3].wwww
487: ld r[4].xyzw, r[0].xyww, t[0][0].xzyw
495: mad r[3].w, r[4].w, l(255), l(0.5)
504: ftou r[3].w, r[3].w
509: and r[3].w, r[3].w, l(254)
516: utof r[6].z, r[3].w
521: mov r[6].xy, r[4].xzxx
526: mad r[6].xyz, r[6].xyzx, l(255,255,1,0), l(0.5,0.5,0.5,0)
541: round_ni r[6].xyz, r[6].xyzx
546: ftoi r[6].xyz, r[6].xyzx
551: ubfe r[7].xyzw, l(3,4,3,4), l(1,4,1,4), r[6].zzzz
566: bfi r[6].xyzw, l(29,28,29,28), l(3,4,3,4), r[6].xyxy, r[7].xyzw
583: itof r[6].xyzw, r[6].xyzw
588: mad r[6].xyzw, r[6].xyzw, l(0.00097703957,0.000488400517,0.00097703957,0.000488400517), l(-1,-1,-1,-1)
603: mul r[6].xyzw, r[6].xyzw, l(1.29999995,1.29999995,2.5999999,2.5999999)
613: dp2 r[3].w, r[6].xyxx, r[6].xyxx
620: add r[6].xy, r[3].wwww, l(1,-1,0,0)
630: div r[6].xyz, r[6].zwyz, r[6].xxxx
637: mul r[7].xyz, r[6].yyyy, cb[0][0][7].xyzx
646: mad r[7].xyz, cb[0][0][6].xyzx, r[6].xxxx, r[7].xyzx
657: mad r[7].xyz, cb[0][0][8].xyzx, r[6].zzzz, r[7].xyzx
668: ld r[8].xyz, r[0].xyww, t[1][1].yzwx
676: mul r[4].zw, r[8].yyyz, l(0,0,255,255)
686: ftou r[4].zw, r[4].zzzw
691: ishl r[3].w, r[4].z, l(8)
698: or r[3].w, r[4].w, r[3].w
705: dp3 r[4].x, r[6].xyzx, -r[3].xyzx
713: ld_structured r[3].x, r[3].w, l(0), t[4][4].xxxx
723: ushr r[3].y, r[3].x, l(24)
730: ubfe r[3].xz, l(8,0,8,0), l(16,0,8,0), r[3].xxxx
745: utof r[6].xyz, r[3].yxzy
750: mul r[3].xyz, r[6].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
760: ld_structured r[3].w, r[3].w, l(0), t[4][4].xxxx
770: and r[3].w, r[3].w, l(255)
777: dp3 r[4].z, r[3].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
787: add r[4].w, r[4].z, l(9.99999997e-07)
794: rcp r[4].w, r[4].w
799: mul r[4].z, r[4].z, l(10000)
806: min r[4].z, r[4].z, l(1)
813: mad r[6].xyz, r[3].xyzx, r[4].wwww, l(-1,-1,-1,0)
825: mad r[6].xyz, r[4].zzzz, r[6].xyzx, l(1,1,1,0)
837: mul r[6].xyz, r[8].xxxx, r[6].xyzx
844: movc r[3].xyz, r[3].wwww, r[3].xyzx, r[6].xyzx
853: mul r[1].xyz, r[1].wwww, r[1].xyzx
860: mad r[1].xyz, -r[1].xyzx, l(0.00100000005,0.00100000005,0.00100000005,0), r[5].xyzx
873: mul r[6].xyz, r[1].wwww, r[7].xyzx
880: mad r[1].xyz, r[6].xyzx, l(0.00100000005,0.00100000005,0.00100000005,0), r[1].xyzx
892: sample_l r[4].xyz, r[4].xyxx, t[12][12].xyzw, s[1][6], l(0)
905: mul r[3].w, cb[1][1][15].w, l(0.318309873)
914: mad r[3].xyz, r[3].xyzx, r[4].xxxx, r[4].yyyy
923: add r[3].xyz, -r[3].xyzx, l(1,1,1,0)
934: max r[3].xyz, r[3].xyzx, l(0,0,0,0)
944: mul r[3].xyz, r[4].zzzz, r[3].xyzx
951: mov r[4].w, l(1)
956: mov r[6].z, l(0)
961: mov r[8].w, l(1)
966: mov r[9].w, l(1)
971: mov r[10].xyz, l(0,0,0,0)
979: mov r[0].z, l(0)
984: loop 
985: ige r[5].w, r[0].z, cb[4][4][0].z
994: breakc r[5].w
997: ld r[5].w, r[0].xyzw, t[19][24].yzwx
1005: ld r[11].xyzw, r[0].xyzw, t[21][26].xyzw
1013: mul r[12].xyz, r[11].yyyy, cb[0][0][7].xyzx
1022: mad r[12].xyz, cb[0][0][6].xyzx, r[11].xxxx, r[12].xyzx
1033: mad r[12].xyz, cb[0][0][8].xyzx, r[11].zzzz, r[12].xyzx
1044: add r[12].xyz, r[12].xyzx, cb[0][0][9].xyzx
1053: ld r[13].xyzw, r[0].xyzw, t[20][25].xyzw
1061: add r[14].xyz, -r[1].xyzx, r[12].xyzx
1069: dp3 r[7].w, r[14].xyzx, r[14].xyzx
1076: rsq r[7].w, r[7].w
1081: mul r[14].xyz, r[7].wwww, r[14].xyzx
1088: ieq r[7].w, r[5].w, l(9.18340949e-41)
1095: mad r[15].xyz, cb[4][4][0].yyyy, r[14].xyzx, r[5].xyzx
1106: movc r[4].xyz, r[7].wwww, r[15].xyzx, r[12].xyzx
1115: movc r[15].xyz, r[7].wwww, r[7].xyzx, r[13].xyzx
1124: mov r[16].xyz, l(0,0,0,0)
1132: mov r[10].w, l(0)
1137: mov r[12].w, l(0)
1142: loop 
1143: uge r[14].w, r[12].w, cb[2][2][0].w
1152: breakc r[14].w
1155: ishl r[14].w, r[12].w, l(2)
1162: dp4 r[17].x, cb[3][3][r[14].w].xyzw, r[4].xyzw
1172: dp4 r[17].y, cb[3][3][1+r[14].w].xyzw, r[4].xyzw
1183: dp4 r[17].z, cb[3][3][2+r[14].w].xyzw, r[4].xyzw
1194: ge r[18].xyz, r[17].xyzx, l(0,0,0,0)
1204: and r[15].w, r[18].y, r[18].x
1211: and r[15].w, r[18].z, r[15].w
1218: utof r[18].xyz, cb[3][3][3+r[14].w].xyzx
1227: lt r[19].xyz, r[17].xyzx, r[18].xyzx
1234: and r[16].w, r[19].y, r[19].x
1241: and r[16].w, r[19].z, r[16].w
1248: and r[15].w, r[15].w, r[16].w
1255: if r[15].w
1258: ld_structured r[19].xyz, r[12].w, l(0), t[13][13].xyzx
1268: ld_structured r[20].xyz, r[12].w, l(12), t[13][13].xyzx
1278: ld_structured r[21].xyzw, r[12].w, l(24), t[13][13].xyzw
1288: ld_structured r[22].xyzw, r[12].w, l(40), t[13][13].xyzw
1298: ld_structured r[23].xyz, r[12].w, l(56), t[13][13].xyzx
1308: mul_sat r[15].w, r[17].x, r[21].w
1315: add r[18].xyz, -r[17].xyzx, r[18].xyzx
1323: mul_sat r[16].w, r[21].w, r[18].x
1330: mul r[15].w, r[15].w, r[16].w
1337: mul_sat r[18].xw, r[17].yyyz, r[22].xxxy
1344: mul r[15].w, r[15].w, r[18].x
1351: mul_sat r[18].xy, r[22].xyxx, r[18].yzyy
1358: mul r[15].w, r[15].w, r[18].x
1365: mul r[15].w, r[18].w, r[15].w
1372: mul r[15].w, r[18].y, r[15].w
1379: ine r[16].w, r[23].x, l(nan)
1386: if r[16].w
1389: round_ni r[18].xyz, r[17].xyzx
1394: add r[17].xyz, r[17].xyzx, -r[18].xyzx
1402: ftou r[18].xyz, r[18].xyzx
1407: imad r[16].w, r[18].z, cb[3][3][3+r[14].w].y, r[18].y
1420: imad r[14].w, r[16].w, cb[3][3][3+r[14].w].x, r[18].x
1433: iadd r[14].w, r[22].z, r[14].w
1440: ld_structured r[14].w, r[14].w, l(0), t[14][14].xxxx
1450: mov r[18].xyz, l(0,0,0,0)
1458: mov r[22].xyz, r[17].xyzx
1463: mov r[16].w, r[14].w
1468: mov r[17].w, r[23].y
1473: loop 
1474: and r[18].w, r[16].w, l(-0)
1481: ine r[18].w, r[18].w, l(0)
1488: ge r[19].w, l(9.99999975e-06), r[17].w
1495: or r[18].w, r[18].w, r[19].w
1502: if r[18].w
1505: and r[18].w, r[16].w, l(nan)
1512: iadd r[18].w, r[23].x, r[18].w
1519: and r[24].x, r[18].w, l(127)
1526: ubfe r[24].y, l(7), l(7), r[18].w
1535: ushr r[24].z, r[18].w, l(14)
1542: ishl r[24].xyz, r[24].xyzx, l(2,2,2,0)
1552: utof r[24].xyz, r[24].xyzx
1557: mad r[24].xyz, r[22].xyzx, l(3,3,3,0), r[24].xyzx
1569: add r[18].xyz, r[24].xyzx, l(0.5,0.5,0.5,0)
1579: break 
1580: endif 
1581: iadd r[18].w, r[22].w, r[16].w
1588: ld_structured r[24].xyzw, r[18].w, l(0), t[15][15].xyzw
1598: mul r[25].xyz, r[22].xyzx, l(3,3,3,0)
1608: round_ni r[25].xyz, r[25].xyzx
1613: ftou r[26].xyz, r[25].xyzx
1618: utof r[26].xz, r[26].zzxz
1623: imul null, r[18].w, r[26].y, l(3)
1631: utof r[18].w, r[18].w
1636: mad r[18].w, r[26].x, l(9), r[18].w
1645: add r[18].w, r[26].z, r[18].w
1652: ftou r[18].w, r[18].w
1657: ishl r[19].w, l(1), r[18].w
1664: and r[20].w, r[19].w, r[24].x
1671: if r[20].w
1674: iadd r[20].w, r[23].x, r[16].w
1681: and r[26].x, r[20].w, l(127)
1688: ubfe r[26].y, l(7), l(7), r[20].w
1697: ushr r[26].z, r[20].w, l(14)
1704: ishl r[26].xyz, r[26].xyzx, l(2,2,2,0)
1714: utof r[26].xyz, r[26].xyzx
1719: mad r[26].xyz, r[22].xyzx, l(3,3,3,0), r[26].xyzx
1731: add r[18].xyz, r[26].xyzx, l(0.5,0.5,0.5,0)
1741: break 
1742: endif 
1743: mad r[22].xyz, r[22].xyzx, l(3,3,3,0), -r[25].xyzx
1756: and r[19].w, r[19].w, r[24].y
1763: ubfe r[20].w, r[18].w, l(0), r[24].y
1772: countbits r[20].w, r[20].w
1777: iadd r[20].w, r[20].w, r[24].w
1784: or r[20].w, r[20].w, l(-0)
1791: not r[21].w, r[24].y
1796: and r[21].w, r[21].w, r[24].x
1803: ubfe r[18].w, r[18].w, l(0), r[21].w
1812: countbits r[18].w, r[18].w
1817: iadd r[18].w, r[18].w, r[24].z
1824: movc r[16].w, r[19].w, r[20].w, r[18].w
1833: mul r[17].w, r[17].w, l(0.333333343)
1840: mov r[18].xyz, l(0,0,0,0)
1848: endloop 
1849: else 
1850: mov r[18].xyz, l(0,0,0,0)
1858: endif 
1859: mul r[17].xyz, r[18].xyzx, cb[2][2][0].xyzx
1868: mov r[22].x, r[19].x
1873: mov r[22].y, r[20].x
1878: mov r[22].z, r[21].x
1883: dp3 r[22].x, r[22].xyzx, r[15].xyzx
1890: mov r[24].x, r[19].y
1895: mov r[24].y, r[20].y
1900: mov r[24].z, r[21].y
1905: dp3 r[22].y, r[24].xyzx, r[15].xyzx
1912: mov r[21].x, r[19].z
1917: mov r[21].y, r[20].z
1922: dp3 r[22].z, r[21].xyzx, r[15].xyzx
1929: mov_sat r[19].xyz, r[22].xyzx
1934: mul r[19].xyz, r[19].xyzx, r[19].xyzx
1941: mov_sat r[22].xyz, -r[22].xyzx
1947: mul r[20].xyz, r[22].xyzx, r[22].xyzx
1954: lt r[21].xyz, l(0,0,0,0), r[19].xyzx
1964: if r[21].x
1967: sample_l r[22].xyz, r[17].xyzx, t[16][16].xyzw, s[1][6], l(0)
1980: mul r[22].xyz, r[19].xxxx, r[22].xyzx
1987: else 
1988: mov r[22].xyz, l(0,0,0,0)
1996: endif 
1997: lt r[24].xyz, l(0,0,0,0), r[20].xyzx
2007: if r[24].x
2010: sample_l r[25].xyz, r[17].xyzx, t[16][17].xyzw, s[1][6], l(0)
2023: mad r[22].xyz, r[25].xyzx, r[20].xxxx, r[22].xyzx
2032: endif 
2033: if r[21].y
2036: sample_l r[21].xyw, r[17].xyzx, t[16][18].xywz, s[1][6], l(0)
2049: mad r[22].xyz, r[21].xywx, r[19].yyyy, r[22].xyzx
2058: endif 
2059: if r[24].y
2062: sample_l r[19].xyw, r[17].xyzx, t[16][19].xywz, s[1][6], l(0)
2075: mad r[22].xyz, r[19].xywx, r[20].yyyy, r[22].xyzx
2084: endif 
2085: if r[21].z
2088: sample_l r[19].xyw, r[17].xyzx, t[16][20].xywz, s[1][6], l(0)
2101: mad r[22].xyz, r[19].xywx, r[19].zzzz, r[22].xyzx
2110: endif 
2111: if r[24].z
2114: sample_l r[17].xyz, r[17].xyzx, t[16][21].xyzw, s[1][6], l(0)
2127: mad r[22].xyz, r[17].xyzx, r[20].zzzz, r[22].xyzx
2136: endif 
2137: ieq r[14].w, r[23].x, l(nan)
2144: movc r[17].xyz, r[14].wwww, l(1,0.349999994,0,0), r[22].xyzx
2156: mad r[14].w, r[15].w, l(-2), l(3)
2165: mul r[16].w, r[15].w, r[15].w
2172: mul r[14].w, r[14].w, r[16].w
2179: add r[16].w, r[23].z, l(-1)
2186: mad r[14].w, r[14].w, r[16].w, l(1)
2195: mul r[16].w, r[14].w, r[15].w
2202: mad r[16].xyz, r[17].xyzx, r[16].wwww, r[16].xyzx
2211: mad r[10].w, r[15].w, r[14].w, r[10].w
2220: endif 
2221: iadd r[12].w, r[12].w, l(1)
2228: endloop 
2229: lt r[4].x, l(0), r[10].w
2236: div r[15].xyz, r[16].xyzx, r[10].wwww
2243: movc r[4].xyz, r[4].xxxx, r[15].xyzx, r[16].xyzx
2252: add r[4].xyz, r[4].xyzx, cb[2][2][1].xyzx
2261: if r[7].w
2264: mul r[15].xyz, r[3].wwww, r[4].xyzx
2271: else 
2272: ld_structured r[17].xyz, r[5].w, l(52), t[5][5].xyzx
2282: imul null, r[7].w, r[5].w, l(35)
2290: mov r[19].x, r[13].w
2295: mov r[19].y, r[11].w
2300: sample_l r[20].xyz, r[19].xyxx, t[22][r[7].w].xyzw, s[0][5], l(0)
2315: mul r[17].xyz, r[17].xyzx, r[20].xyzx
2322: dp3 r[7].w, r[17].xyzx, r[17].xyzx
2329: ge r[7].w, r[7].w, l(9.99999975e-05)
2336: if r[7].w
2339: mad r[20].xyz, r[2].xyzx, r[2].wwww, -r[11].xyzx
2349: dp3 r[7].w, r[20].xyzx, r[20].xyzx
2356: sqrt r[7].w, r[7].w
2361: mul r[7].w, r[7].w, l(50)
2368: min r[7].w, r[7].w, l(1)
2375: mul r[4].xyz, r[4].xyzx, r[7].wwww
2382: ld_structured r[19].zw, r[5].w, l(68), t[5][5].xxxy
2392: imad r[20].xy, r[5].wwww, l(35,35,0,0), l(2,4,0,0)
2407: sample_l r[7].w, r[19].xyxx, t[22][r[20].x].yzwx, s[3][19], l(0)
2422: add r[11].w, -r[19].z, r[19].w
2430: mad r[21].y, r[7].w, r[11].w, r[19].z
2439: sample_l r[7].w, r[19].xyxx, t[22][r[20].y].yzwx, s[3][19], l(0)
2454: ld_structured r[5].w, r[5].w, l(0), t[4][4].xxxx
2464: ushr r[11].w, r[5].w, l(24)
2471: ubfe r[19].xy, l(8,8,0,0), l(16,8,0,0), r[5].wwww
2486: utof r[20].x, r[11].w
2491: utof r[20].yz, r[19].xxyx
2496: mul r[19].xyz, r[7].wwww, r[20].xyzx
2503: mul r[19].xyz, r[19].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
2513: dp3 r[5].w, r[19].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
2523: dp3 r[21].x, r[13].xyzx, -r[14].xyzx
2531: sample_l r[14].xyz, r[21].xyxx, t[12][12].xyzw, s[1][6], l(0)
2544: mad r[14].xyw, r[19].xyxz, r[14].xxxx, r[14].yyyy
2553: add r[14].xyw, -r[14].xyxw, l(1,1,0,1)
2564: mul r[14].xyw, r[14].xyxw, cb[1][1][15].wwww
2573: mul r[14].xyz, r[14].zzzz, r[14].xywx
2580: mul r[4].xyz, r[4].xyzx, r[14].xyzx
2587: mul r[14].xyz, r[12].yyyy, cb[0][0][3].xyzx
2596: mad r[12].xyw, cb[0][0][2].xyxz, r[12].xxxx, r[14].xyxz
2607: mad r[12].xyz, cb[0][0][4].xyzx, r[12].zzzz, r[12].xywx
2618: add r[12].xyz, r[12].xyzx, cb[0][0][5].xyzx
2627: mul r[14].xyz, r[13].yyyy, cb[0][0][3].xyzx
2636: mad r[13].xyw, cb[0][0][2].xyxz, r[13].xxxx, r[14].xyxz
2647: mad r[13].xyz, cb[0][0][4].xyzx, r[13].zzzz, r[13].xywx
2658: add r[11].xyz, r[11].xyzx, l(150,75,75,0)
2668: mul r[11].xyz, r[11].xyzx, l(0.0533333346,0.0533333346,0.0533333346,0)
2678: round_ni r[11].xyz, r[11].xyzx
2683: max r[11].xyz, r[11].xyzx, l(0,0,0,0)
2693: min r[11].xyz, r[11].xyzx, l(15,7,11,0)
2703: ftoi r[11].xyz, r[11].xyzx
2708: imad r[6].x, r[11].x, l(12), r[11].z
2717: ishl r[6].y, r[11].y, l(8)
2724: ld r[7].w, r[6].xyzz, t[17][22].yzwx
2732: dp3 r[11].x, -r[12].xyzx, -r[12].xyzx
2741: mad r[9].xyz, r[13].xyzx, l(0.0700000003,0.0700000003,0.0700000003,0), r[12].xyzx
2753: rsq r[11].x, r[11].x
2758: mul r[11].xyz, r[11].xxxx, -r[12].xyzx
2766: add r[11].w, -r[5].w, l(1)
2774: mov r[14].xyz, l(0,0,0,0)
2782: mov r[12].w, l(0)
2787: mov r[13].w, r[7].w
2792: loop 
2793: ine r[14].w, r[13].w, l(9.18340949e-41)
2800: ilt r[15].w, r[12].w, l(256)
2807: and r[14].w, r[14].w, r[15].w
2814: breakc r[14].w
2817: ld_structured r[19].xyzw, r[13].w, l(0), t[7][7].xyzw
2827: ld_structured r[20].xyzw, r[13].w, l(16), t[7][7].xyzw
2837: ld_structured r[21].xyzw, r[13].w, l(36), t[7][7].xyzw
2847: ld_structured r[22].xyzw, r[13].w, l(52), t[7][7].xyzw
2857: ld_structured r[14].w, r[13].w, l(144), t[7][7].xxxx
2867: ld_structured r[15].w, r[13].w, l(156), t[7][7].xxxx
2877: add r[19].xyz, -r[12].xyzx, r[19].xyzx
2885: dp3 r[16].w, r[19].xyzx, r[19].xyzx
2892: rsq r[17].w, r[16].w
2897: mul r[23].xyz, r[17].wwww, r[19].xyzx
2904: sqrt r[18].w, r[16].w
2909: add r[23].w, -r[18].w, r[20].w
2917: mul r[20].w, r[20].w, l(0.200000003)
2924: div_sat r[20].w, r[23].w, r[20].w
2931: mad r[23].w, r[20].w, l(-2), l(3)
2940: mul r[20].w, r[20].w, r[20].w
2947: mul r[20].w, r[20].w, r[23].w
2954: mad r[14].w, r[14].w, r[14].w, r[16].w
2963: max r[14].w, r[14].w, l(9.99999975e-05)
2970: mul r[14].w, r[14].w, l(3.14159274)
2977: rcp r[14].w, r[14].w
2982: add r[14].w, -r[21].x, r[14].w
2990: max r[14].w, r[14].w, l(0)
2997: mul r[14].w, r[20].w, r[14].w
3004: mul r[20].xyz, r[14].wwww, r[20].xyzx
3011: and r[24].xy, r[15].wwww, l(4,2,0,0)
3021: mov r[25].x, r[21].w
3026: mov r[25].yz, r[22].xxyx
3031: dp3 r[14].w, -r[23].xyzx, r[25].xyzx
3039: mad_sat r[14].w, r[14].w, r[21].y, r[21].z
3048: mul r[14].w, r[14].w, r[14].w
3055: mul r[21].xyz, r[14].wwww, r[20].xyzx
3062: movc r[20].xyz, r[24].xxxx, r[21].xyzx, r[20].xyzx
3071: if r[24].y
3074: ld_structured r[21].xy, r[13].w, l(68), t[7][7].xyxx
3084: ld_structured r[24].xyz, r[13].w, l(80), t[7][7].xyzx
3094: ld_structured r[25].xyz, r[13].w, l(96), t[7][7].xyzx
3104: ld_structured r[26].xyw, r[13].w, l(112), t[7][7].xyxz
3114: ld_structured r[27].xyzw, r[13].w, l(128), t[7][7].xyzw
3124: mov r[28].x, r[22].w
3129: mov r[28].y, r[24].x
3134: mov r[28].z, r[25].x
3139: mov r[28].w, r[26].x
3144: dp4 r[28].x, r[28].xyzw, r[9].xyzw
3151: mov r[29].x, r[21].x
3156: mov r[29].y, r[24].y
3161: mov r[29].z, r[25].y
3166: mov r[29].w, r[26].y
3171: dp4 r[28].y, r[29].xyzw, r[9].xyzw
3178: mov r[26].x, r[21].y
3183: mov r[26].y, r[24].z
3188: mov r[26].z, r[25].z
3193: dp4 r[28].z, r[26].xyzw, r[9].xyzw
3200: dp3 r[14].w, r[28].xyzx, r[28].xyzx
3207: rsq r[14].w, r[14].w
3212: mul r[21].xyz, r[14].wwww, r[28].xyzx
3219: lt r[24].xyzw, abs(r[21].yzxz), abs(r[21].xxyy)
3228: lt r[22].xyw, l(0,0,0,0), r[21].xyxz
3238: and r[24].xy, r[24].ywyy, r[24].xzxx
3245: movc r[22].xyw, r[22].xyxw, l(0,2,0,4), l(1,3,0,5)
3260: movc r[14].w, r[24].y, r[22].y, r[22].w
3269: movc r[14].w, r[24].x, r[22].x, r[14].w
3278: if r[14].w
3281: ine r[15].w, r[14].w, l(1)
3288: if r[15].w
3291: div r[24].xyz, r[21].xxyx, r[21].yzzy
3298: div r[15].w, -r[21].z, r[21].y
3306: mad r[25].y, r[15].w, l(-0.5), l(0.5)
3315: div r[21].xw, r[21].xxxz, -r[21].yyyy
3323: mad r[21].xw, r[21].xxxw, l(0.5,0,0,-0.5), l(0.5,0,0,0.5)
3338: ieq r[26].xyz, r[14].wwww, l(2,3,4,0)
3348: mad r[24].xyz, r[24].xyzx, l(0.5,0.5,-0.5,0), l(0.5,0.5,0.5,0)
3363: div r[15].w, r[21].y, -r[21].z
3371: mad r[24].w, r[15].w, l(-0.5), l(0.5)
3380: movc r[21].yz, r[26].zzzz, r[24].yyzy, r[24].yywy
3389: movc r[21].xy, r[26].yyyy, r[21].xwxx, r[21].yzyy
3398: mov r[25].x, r[24].x
3403: movc r[22].xy, r[26].xxxx, r[25].xyxx, r[21].xyxx
3412: else 
3413: div r[24].xy, r[21].zyzz, -r[21].xxxx
3421: mad r[22].xy, r[24].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
3436: endif 
3437: else 
3438: div r[15].w, -r[21].z, r[21].x
3446: mad r[22].x, r[15].w, l(0.5), l(0.5)
3455: div r[15].w, r[21].y, r[21].x
3462: mad r[22].y, r[15].w, l(-0.5), l(0.5)
3471: endif 
3472: add r[21].xy, r[22].xyxx, l(-0.5,-0.5,0,0)
3482: mad r[21].yz, r[21].xxyx, r[19].wwww, l(0,0.5,0.5,0)
3494: utof r[14].w, r[14].w
3499: mul r[14].w, r[14].w, l(0.166666672)
3506: mad r[21].x, r[21].y, l(0.166666672), r[14].w
3515: mad r[21].xy, r[21].xzxx, r[27].zwzz, r[27].xyxx
3524: sample_l r[21].xy, r[21].xyxx, t[6][6].xyzw, s[1][6], l(0)
3537: add r[14].w, r[18].w, l(-0.0500000007)
3544: mul r[15].w, r[22].z, r[14].w
3551: mad r[16].w, -r[21].x, r[21].x, r[21].y
3561: max r[16].w, r[16].w, l(9.99999975e-06)
3568: mad r[14].w, -r[14].w, r[22].z, r[21].x
3578: mad r[14].w, r[14].w, r[14].w, r[16].w
3587: div r[14].w, r[16].w, r[14].w
3594: min r[14].w, r[14].w, l(1)
3601: lt r[15].w, r[15].w, r[21].x
3608: movc r[14].w, r[15].w, l(1), r[14].w
3617: mul r[20].xyz, r[14].wwww, r[20].xyzx
3624: endif 
3625: mad r[19].xyz, r[19].xyzx, r[17].wwww, r[11].xyzx
3634: dp3 r[14].w, r[19].xyzx, r[19].xyzx
3641: rsq r[14].w, r[14].w
3646: mul r[19].xyz, r[14].wwww, r[19].xyzx
3653: dp3 r[14].w, r[11].xyzx, r[19].xyzx
3660: dp3_sat r[15].w, r[13].xyzx, r[23].xyzx
3667: add r[14].w, -abs(r[14].w), l(1)
3675: max r[14].w, r[14].w, l(0)
3682: mul r[16].w, r[14].w, r[14].w
3689: mul r[16].w, r[16].w, r[16].w
3696: mul r[14].w, r[14].w, r[16].w
3703: mad r[14].w, r[11].w, r[14].w, r[5].w
3712: add_sat r[14].w, -r[14].w, l(1)
3720: mul r[14].w, r[15].w, r[14].w
3727: mul r[14].w, r[14].w, l(0.318309873)
3734: mad r[14].xyz, r[14].wwww, r[20].xyzx, r[14].xyzx
3743: iadd r[12].w, r[12].w, l(1)
3750: iadd r[6].w, r[6].y, r[12].w
3757: ld r[13].w, r[6].xwzz, t[17][22].yzwx
3765: endloop 
3766: ld r[6].w, r[6].xyzz, t[18][23].yzwx
3774: mul r[9].xyz, r[12].zzzz, r[13].xyzx
3781: mad r[8].xyz, r[9].xyzx, l(0.000300000014,0.000300000014,0.000300000014,0), r[12].xyzx
3793: mov r[19].xzw, r[6].xxzz
3798: mov r[9].xyz, r[14].xyzx
3803: mov r[6].x, l(0)
3808: mov r[7].w, r[6].w
3813: loop 
3814: ine r[12].w, r[7].w, l(9.18340949e-41)
3821: ilt r[13].w, r[6].x, l(256)
3828: and r[12].w, r[12].w, r[13].w
3835: breakc r[12].w
3838: iadd r[6].x, r[6].x, l(1)
3845: iadd r[19].y, r[6].x, r[6].y
3852: ld r[12].w, r[19].xyzw, t[18][23].yzwx
3860: ld_structured r[20].xyzw, r[7].w, l(68), t[11][11].xyzw
3870: ld_structured r[21].xyz, r[7].w, l(84), t[11][11].xyzx
3880: ld_structured r[22].xyzw, r[7].w, l(96), t[11][11].xyzw
3890: ld_structured r[23].xyzw, r[7].w, l(112), t[11][11].xyzw
3900: ld_structured r[24].xyzw, r[7].w, l(128), t[11][11].xyzw
3910: mov r[25].x, r[20].w
3915: mov r[25].y, r[22].x
3920: mov r[25].z, r[23].x
3925: mov r[25].w, r[24].x
3930: dp4 r[25].x, r[25].xyzw, r[8].xyzw
3937: mov r[26].x, r[21].x
3942: mov r[26].y, r[22].y
3947: mov r[26].z, r[23].y
3952: mov r[26].w, r[24].y
3957: dp4 r[25].y, r[26].xyzw, r[8].xyzw
3964: mov r[26].x, r[21].y
3969: mov r[26].y, r[22].z
3974: mov r[26].z, r[23].z
3979: mov r[26].w, r[24].z
3984: dp4 r[25].z, r[26].xyzw, r[8].xyzw
3991: mov r[24].x, r[21].z
3996: mov r[24].y, r[22].w
4001: mov r[24].z, r[23].w
4006: dp4 r[13].w, r[24].xyzw, r[8].xyzw
4013: rcp r[14].w, r[13].w
4018: mul r[21].xyz, r[14].wwww, r[25].xyzx
4025: mad r[22].xyz, r[21].xyzx, l(0.5,0.5,1,0), l(0.5,0.5,0,0)
4040: mov_sat r[23].xyz, r[22].xyzx
4045: eq r[22].xyz, r[22].xyzx, r[23].xyzx
4052: and r[14].w, r[22].y, r[22].x
4059: and r[14].w, r[22].z, r[14].w
4066: if r[14].w
4069: ld_structured r[22].xyzw, r[7].w, l(0), t[11][11].xyzw
4079: ld_structured r[14].w, r[7].w, l(156), t[11][11].xxxx
4089: ld_structured r[15].w, r[7].w, l(200), t[11][11].xxxx
4099: ld_structured r[23].xyzw, r[7].w, l(288), t[11][11].xyzw
4109: ld_structured r[16].w, r[7].w, l(304), t[11][11].xxxx
4119: mad r[24].xy, r[21].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
4134: and r[24].zw, r[16].wwww, l(0,0,2,768)
4144: if r[24].z
4147: ld_structured r[26].xyzw, r[7].w, l(272), t[11][11].xyzw
4157: mad r[25].xy, r[24].xyxx, r[26].zwzz, r[26].xyxx
4166: sample_l r[25].xy, r[25].xyxx, t[9][9].xyzw, s[1][6], l(0)
4179: add r[26].xyz, -r[12].xyzx, r[22].xyzx
4187: dp3 r[17].w, r[26].xyzx, r[26].xyzx
4194: sqrt r[17].w, r[17].w
4199: add r[17].w, -r[22].w, r[17].w
4207: mul r[18].w, r[14].w, r[17].w
4214: mad r[19].y, -r[25].x, r[25].x, r[25].y
4224: max r[19].y, r[19].y, l(9.99999975e-06)
4231: mad r[17].w, -r[17].w, r[14].w, r[25].x
4241: mad r[17].w, r[17].w, r[17].w, r[19].y
4250: div r[17].w, r[19].y, r[17].w
4257: min r[17].w, r[17].w, l(1)
4264: lt r[18].w, r[18].w, r[25].x
4271: movc r[17].w, r[18].w, l(1), r[17].w
4280: else 
4281: mov r[17].w, l(1)
4286: endif 
4287: ieq r[25].xy, r[24].wwww, l(256,512,0,0)
4297: if r[25].x
4300: mad r[25].xw, r[24].xxxy, r[23].zzzw, r[23].xxxy
4309: sample_l r[18].w, r[25].xwxx, t[8][8].yzwx, s[1][6], l(0)
4322: ge r[18].w, r[18].w, r[21].z
4329: and r[18].w, r[18].w, l(1)
4336: else 
4337: ieq r[19].y, r[24].w, l(512)
4344: if r[19].y
4347: mad r[21].zw, r[24].xxxy, r[23].zzzw, r[23].xxxy
4356: sample_l r[21].zw, r[21].zwzz, t[9][9].zwxy, s[1][6], l(0)
4369: add r[23].xyz, -r[12].xyzx, r[22].xyzx
4377: dp3 r[19].y, r[23].xyzx, r[23].xyzx
4384: sqrt r[19].y, r[19].y
4389: add r[19].y, -r[22].w, r[19].y
4397: mul r[20].w, r[14].w, r[19].y
4404: mad r[21].w, -r[21].z, r[21].z, r[21].w
4414: max r[21].w, r[21].w, l(9.99999975e-06)
4421: mad r[14].w, -r[19].y, r[14].w, r[21].z
4431: mad r[14].w, r[14].w, r[14].w, r[21].w
4440: div r[14].w, r[21].w, r[14].w
4447: min r[14].w, r[14].w, l(1)
4454: lt r[19].y, r[20].w, r[21].z
4461: movc r[18].w, r[19].y, l(1), r[14].w
4470: else 
4471: mov r[18].w, l(1)
4476: endif 
4477: endif 
4478: ine r[14].w, r[24].z, l(0)
4485: and r[14].w, r[14].w, r[25].y
4492: add r[19].y, -r[17].w, r[18].w
4500: mad r[19].y, r[15].w, r[19].y, r[17].w
4509: mad r[18].w, r[18].w, r[15].w, l(1)
4518: add r[15].w, -r[15].w, r[18].w
4526: min r[15].w, r[15].w, r[17].w
4533: movc r[14].w, r[14].w, r[19].y, r[15].w
4542: ne r[15].w, r[14].w, l(0)
4549: if r[15].w
4552: ld_structured r[23].xyz, r[7].w, l(16), t[11][11].xyzx
4562: ld_structured r[26].xyzw, r[7].w, l(48), t[11][11].xyzw
4572: add r[15].w, -r[25].z, r[26].w
4580: mul r[17].w, r[26].w, l(0.200000003)
4587: div_sat r[15].w, r[15].w, r[17].w
4594: mad r[17].w, r[15].w, l(-2), l(3)
4603: mul r[15].w, r[15].w, r[15].w
4610: mul r[15].w, r[15].w, r[17].w
4617: add r[23].xyz, -r[12].xyzx, r[23].xyzx
4625: dp3 r[17].w, r[23].xyzx, r[23].xyzx
4632: mul r[18].w, r[20].y, r[20].y
4639: mad r[17].w, r[20].z, r[17].w, r[18].w
4648: max r[17].w, r[17].w, l(0.25)
4655: mul r[17].w, r[17].w, l(3.14159274)
4662: rcp r[17].w, r[17].w
4667: add r[17].w, -r[20].x, r[17].w
4675: max r[17].w, r[17].w, l(0)
4682: mul r[15].w, r[15].w, r[17].w
4689: mul r[20].xyz, r[15].wwww, r[26].xyzx
4696: and r[15].w, r[16].w, l(1)
4703: if r[15].w
4706: ld_structured r[23].xyzw, r[7].w, l(256), t[11][11].xyzw
4716: and r[15].w, r[16].w, l(1.43492963e-42)
4723: add r[21].zw, r[21].xxxy, r[21].xxxy
4730: mad r[16].w, r[21].x, r[21].x, l(1)
4739: mad r[16].w, r[21].y, r[21].y, r[16].w
4748: div r[21].xy, r[21].zwzz, r[16].wwww
4755: mad r[21].xy, r[21].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
4770: movc r[21].xy, r[15].wwww, r[21].xyxx, r[24].xyxx
4779: mad r[21].xy, r[21].xyxx, r[23].zwzz, r[23].xyxx
4788: sample_l r[21].xyz, r[21].xyxx, t[10][10].xyzw, s[2][11], l(0)
4801: mul r[20].xyz, r[20].xyzx, r[21].xyzx
4808: endif 
4809: ge r[13].w, r[13].w, r[25].z
4816: and r[13].w, r[13].w, l(1)
4823: mul r[13].w, r[13].w, r[14].w
4830: mul r[20].xyz, r[13].wwww, r[20].xyzx
4837: add r[21].xyz, -r[12].xyzx, r[22].xyzx
4845: dp3 r[13].w, r[21].xyzx, r[21].xyzx
4852: rsq r[13].w, r[13].w
4857: mul r[22].xyz, r[13].wwww, r[21].xyzx
4864: mad r[21].xyz, r[21].xyzx, r[13].wwww, r[11].xyzx
4873: dp3 r[13].w, r[21].xyzx, r[21].xyzx
4880: rsq r[13].w, r[13].w
4885: mul r[21].xyz, r[13].wwww, r[21].xyzx
4892: dp3 r[13].w, r[11].xyzx, r[21].xyzx
4899: dp3_sat r[14].w, r[13].xyzx, r[22].xyzx
4906: add r[13].w, -abs(r[13].w), l(1)
4914: max r[13].w, r[13].w, l(0)
4921: mul r[15].w, r[13].w, r[13].w
4928: mul r[15].w, r[15].w, r[15].w
4935: mul r[13].w, r[13].w, r[15].w
4942: mad r[13].w, r[11].w, r[13].w, r[5].w
4951: add_sat r[13].w, -r[13].w, l(1)
4959: mul r[13].w, r[14].w, r[13].w
4966: mul r[13].w, r[13].w, l(0.318309873)
4973: mad r[9].xyz, r[13].wwww, r[20].xyzx, r[9].xyzx
4982: endif 
4983: endif 
4984: mov r[7].w, r[12].w
4989: endloop 
4990: mad r[4].xyz, r[4].xyzx, l(0.318309873,0.318309873,0.318309873,0), r[9].xyzx
5002: mul r[15].xyz, r[17].xyzx, r[4].xyzx
5009: else 
5010: mov r[15].xyz, l(0,0,0,0)
5018: endif 
5019: endif 
5020: mad r[10].xyz, r[3].xyzx, r[15].xyzx, r[10].xyzx
5029: iadd r[0].z, r[0].z, l(1)
5036: endloop 
5037: add r[0].xy, -cb[1][1][17].yzyy, cb[1][1][17].xwxx
5049: add r[0].z, cb[1][1][17].y, cb[1][1][17].x
5060: add r[0].z, -r[0].x, r[0].z
5068: add r[0].x, -r[0].x, r[1].w
5076: div r[0].z, l(1,1,1,1), r[0].z
5086: mul_sat r[0].x, r[0].z, r[0].x
5093: mad r[0].z, r[0].x, l(-2), l(3)
5102: mul r[0].x, r[0].x, r[0].x
5109: mul r[0].x, r[0].x, r[0].z
5116: mad r[0].x, r[0].x, r[0].y, cb[1][1][17].z
5127: mul r[0].xyz, r[0].xxxx, r[10].xyzx
5134: itof r[0].w, cb[4][4][0].z
5141: div r[0].xyz, r[0].xyzx, r[0].wwww
5148: mov r[0].w, l(1)
5153: else 
5154: mov r[0].xyzw, l(0,0,0,0)
5162: endif 
5163: ld_uav_typed r[1].x, type32.xyyy, type30[1][1].xyzw
5170: add r[1].x, r[1].x, l(1)
5177: ld_uav_typed r[2].xyzw, type32.xyyy, type30[0][0].xyzw
5184: add r[0].xyzw, r[0].xyzw, r[2].xyzw
5191: div r[0].xyzw, r[0].xyzw, r[1].xxxx
5198: store_uav_typed type30[0][0].xyzw, type32.xyyy, r[0].xyzw
5205: ld_uav_typed r[0].x, type32.xyyy, type30[2][2].xyzw
5212: div r[0].x, r[0].x, r[1].x
5219: store_uav_typed type30[2][2].xyzw, type32.xyyy, r[0].xxxx
5226: ret 