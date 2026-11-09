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
979: mov r[5].w, l(0)
984: mov r[0].z, l(0)
989: loop 
990: ige r[7].w, r[0].z, cb[4][4][0].z
999: breakc r[7].w
1002: ld r[7].w, r[0].xyzw, t[19][24].yzwx
1010: ld r[11].xyzw, r[0].xyzw, t[21][26].xyzw
1018: mul r[12].xyz, r[11].yyyy, cb[0][0][7].xyzx
1027: mad r[12].xyz, cb[0][0][6].xyzx, r[11].xxxx, r[12].xyzx
1038: mad r[12].xyz, cb[0][0][8].xyzx, r[11].zzzz, r[12].xyzx
1049: add r[12].xyz, r[12].xyzx, cb[0][0][9].xyzx
1058: ld r[13].xyzw, r[0].xyzw, t[20][25].xyzw
1066: add r[14].xyz, -r[1].xyzx, r[12].xyzx
1074: dp3 r[10].w, r[14].xyzx, r[14].xyzx
1081: rsq r[10].w, r[10].w
1086: mul r[14].xyz, r[10].wwww, r[14].xyzx
1093: ieq r[10].w, r[7].w, l(9.18340949e-41)
1100: mad r[15].xyz, cb[4][4][0].yyyy, r[14].xyzx, r[5].xyzx
1111: movc r[4].xyz, r[10].wwww, r[15].xyzx, r[12].xyzx
1120: movc r[15].xyz, r[10].wwww, r[7].xyzx, r[13].xyzx
1129: mov r[16].xyz, l(0,0,0,0)
1137: mov r[12].w, l(0)
1142: mov r[14].w, l(0)
1147: loop 
1148: uge r[15].w, r[14].w, cb[2][2][0].w
1157: breakc r[15].w
1160: ishl r[15].w, r[14].w, l(2)
1167: dp4 r[17].x, cb[3][3][r[15].w].xyzw, r[4].xyzw
1177: dp4 r[17].y, cb[3][3][1+r[15].w].xyzw, r[4].xyzw
1188: dp4 r[17].z, cb[3][3][2+r[15].w].xyzw, r[4].xyzw
1199: ge r[18].xyz, r[17].xyzx, l(0,0,0,0)
1209: and r[16].w, r[18].y, r[18].x
1216: and r[16].w, r[18].z, r[16].w
1223: utof r[18].xyz, cb[3][3][3+r[15].w].xyzx
1232: lt r[19].xyz, r[17].xyzx, r[18].xyzx
1239: and r[17].w, r[19].y, r[19].x
1246: and r[17].w, r[19].z, r[17].w
1253: and r[16].w, r[16].w, r[17].w
1260: if r[16].w
1263: ld_structured r[19].xyz, r[14].w, l(0), t[13][13].xyzx
1273: ld_structured r[20].xyz, r[14].w, l(12), t[13][13].xyzx
1283: ld_structured r[21].xyzw, r[14].w, l(24), t[13][13].xyzw
1293: ld_structured r[22].xyzw, r[14].w, l(40), t[13][13].xyzw
1303: ld_structured r[23].xyz, r[14].w, l(56), t[13][13].xyzx
1313: mul_sat r[16].w, r[17].x, r[21].w
1320: add r[18].xyz, -r[17].xyzx, r[18].xyzx
1328: mul_sat r[17].w, r[21].w, r[18].x
1335: mul r[16].w, r[16].w, r[17].w
1342: mul_sat r[18].xw, r[17].yyyz, r[22].xxxy
1349: mul r[16].w, r[16].w, r[18].x
1356: mul_sat r[18].xy, r[22].xyxx, r[18].yzyy
1363: mul r[16].w, r[16].w, r[18].x
1370: mul r[16].w, r[18].w, r[16].w
1377: mul r[16].w, r[18].y, r[16].w
1384: ine r[17].w, r[23].x, l(nan)
1391: if r[17].w
1394: round_ni r[18].xyz, r[17].xyzx
1399: add r[17].xyz, r[17].xyzx, -r[18].xyzx
1407: ftou r[18].xyz, r[18].xyzx
1412: imad r[17].w, r[18].z, cb[3][3][3+r[15].w].y, r[18].y
1425: imad r[15].w, r[17].w, cb[3][3][3+r[15].w].x, r[18].x
1438: iadd r[15].w, r[22].z, r[15].w
1445: ld_structured r[15].w, r[15].w, l(0), t[14][14].xxxx
1455: mov r[18].xyz, l(0,0,0,0)
1463: mov r[22].xyz, r[17].xyzx
1468: mov r[17].w, r[15].w
1473: mov r[18].w, r[23].y
1478: loop 
1479: and r[19].w, r[17].w, l(-0)
1486: ine r[19].w, r[19].w, l(0)
1493: ge r[20].w, l(9.99999975e-06), r[18].w
1500: or r[19].w, r[19].w, r[20].w
1507: if r[19].w
1510: and r[19].w, r[17].w, l(nan)
1517: iadd r[19].w, r[23].x, r[19].w
1524: and r[24].x, r[19].w, l(127)
1531: ubfe r[24].y, l(7), l(7), r[19].w
1540: ushr r[24].z, r[19].w, l(14)
1547: ishl r[24].xyz, r[24].xyzx, l(2,2,2,0)
1557: utof r[24].xyz, r[24].xyzx
1562: mad r[24].xyz, r[22].xyzx, l(3,3,3,0), r[24].xyzx
1574: add r[18].xyz, r[24].xyzx, l(0.5,0.5,0.5,0)
1584: break 
1585: endif 
1586: iadd r[19].w, r[22].w, r[17].w
1593: ld_structured r[24].xyzw, r[19].w, l(0), t[15][15].xyzw
1603: mul r[25].xyz, r[22].xyzx, l(3,3,3,0)
1613: round_ni r[25].xyz, r[25].xyzx
1618: ftou r[26].xyz, r[25].xyzx
1623: utof r[26].xz, r[26].zzxz
1628: imul null, r[19].w, r[26].y, l(3)
1636: utof r[19].w, r[19].w
1641: mad r[19].w, r[26].x, l(9), r[19].w
1650: add r[19].w, r[26].z, r[19].w
1657: ftou r[19].w, r[19].w
1662: ishl r[20].w, l(1), r[19].w
1669: and r[21].w, r[20].w, r[24].x
1676: if r[21].w
1679: iadd r[21].w, r[23].x, r[17].w
1686: and r[26].x, r[21].w, l(127)
1693: ubfe r[26].y, l(7), l(7), r[21].w
1702: ushr r[26].z, r[21].w, l(14)
1709: ishl r[26].xyz, r[26].xyzx, l(2,2,2,0)
1719: utof r[26].xyz, r[26].xyzx
1724: mad r[26].xyz, r[22].xyzx, l(3,3,3,0), r[26].xyzx
1736: add r[18].xyz, r[26].xyzx, l(0.5,0.5,0.5,0)
1746: break 
1747: endif 
1748: mad r[22].xyz, r[22].xyzx, l(3,3,3,0), -r[25].xyzx
1761: and r[20].w, r[20].w, r[24].y
1768: ubfe r[21].w, r[19].w, l(0), r[24].y
1777: countbits r[21].w, r[21].w
1782: iadd r[21].w, r[21].w, r[24].w
1789: or r[21].w, r[21].w, l(-0)
1796: not r[23].w, r[24].y
1801: and r[23].w, r[23].w, r[24].x
1808: ubfe r[19].w, r[19].w, l(0), r[23].w
1817: countbits r[19].w, r[19].w
1822: iadd r[19].w, r[19].w, r[24].z
1829: movc r[17].w, r[20].w, r[21].w, r[19].w
1838: mul r[18].w, r[18].w, l(0.333333343)
1845: mov r[18].xyz, l(0,0,0,0)
1853: endloop 
1854: else 
1855: mov r[18].xyz, l(0,0,0,0)
1863: endif 
1864: mul r[17].xyz, r[18].xyzx, cb[2][2][0].xyzx
1873: mov r[22].x, r[19].x
1878: mov r[22].y, r[20].x
1883: mov r[22].z, r[21].x
1888: dp3 r[22].x, r[22].xyzx, r[15].xyzx
1895: mov r[24].x, r[19].y
1900: mov r[24].y, r[20].y
1905: mov r[24].z, r[21].y
1910: dp3 r[22].y, r[24].xyzx, r[15].xyzx
1917: mov r[21].x, r[19].z
1922: mov r[21].y, r[20].z
1927: dp3 r[22].z, r[21].xyzx, r[15].xyzx
1934: mov_sat r[19].xyz, r[22].xyzx
1939: mul r[19].xyz, r[19].xyzx, r[19].xyzx
1946: mov_sat r[22].xyz, -r[22].xyzx
1952: mul r[20].xyz, r[22].xyzx, r[22].xyzx
1959: lt r[21].xyz, l(0,0,0,0), r[19].xyzx
1969: if r[21].x
1972: sample_l r[22].xyz, r[17].xyzx, t[16][16].xyzw, s[1][6], l(0)
1985: mul r[22].xyz, r[19].xxxx, r[22].xyzx
1992: else 
1993: mov r[22].xyz, l(0,0,0,0)
2001: endif 
2002: lt r[24].xyz, l(0,0,0,0), r[20].xyzx
2012: if r[24].x
2015: sample_l r[25].xyz, r[17].xyzx, t[16][17].xyzw, s[1][6], l(0)
2028: mad r[22].xyz, r[25].xyzx, r[20].xxxx, r[22].xyzx
2037: endif 
2038: if r[21].y
2041: sample_l r[21].xyw, r[17].xyzx, t[16][18].xywz, s[1][6], l(0)
2054: mad r[22].xyz, r[21].xywx, r[19].yyyy, r[22].xyzx
2063: endif 
2064: if r[24].y
2067: sample_l r[19].xyw, r[17].xyzx, t[16][19].xywz, s[1][6], l(0)
2080: mad r[22].xyz, r[19].xywx, r[20].yyyy, r[22].xyzx
2089: endif 
2090: if r[21].z
2093: sample_l r[19].xyw, r[17].xyzx, t[16][20].xywz, s[1][6], l(0)
2106: mad r[22].xyz, r[19].xywx, r[19].zzzz, r[22].xyzx
2115: endif 
2116: if r[24].z
2119: sample_l r[17].xyz, r[17].xyzx, t[16][21].xyzw, s[1][6], l(0)
2132: mad r[22].xyz, r[17].xyzx, r[20].zzzz, r[22].xyzx
2141: endif 
2142: ieq r[15].w, r[23].x, l(nan)
2149: movc r[17].xyz, r[15].wwww, l(1,0.349999994,0,0), r[22].xyzx
2161: mad r[15].w, r[16].w, l(-2), l(3)
2170: mul r[17].w, r[16].w, r[16].w
2177: mul r[15].w, r[15].w, r[17].w
2184: add r[17].w, r[23].z, l(-1)
2191: mad r[15].w, r[15].w, r[17].w, l(1)
2200: mul r[17].w, r[15].w, r[16].w
2207: mad r[16].xyz, r[17].xyzx, r[17].wwww, r[16].xyzx
2216: mad r[12].w, r[16].w, r[15].w, r[12].w
2225: endif 
2226: iadd r[14].w, r[14].w, l(1)
2233: endloop 
2234: lt r[4].x, l(0), r[12].w
2241: div r[15].xyz, r[16].xyzx, r[12].wwww
2248: movc r[4].xyz, r[4].xxxx, r[15].xyzx, r[16].xyzx
2257: add r[4].xyz, r[4].xyzx, cb[2][2][1].xyzx
2266: mad r[15].xyz, r[2].xyzx, r[2].wwww, -r[11].xyzx
2276: dp3 r[14].w, r[15].xyzx, r[15].xyzx
2283: sqrt r[14].w, r[14].w
2288: movc r[14].w, r[10].w, cb[4][4][0].x, r[14].w
2299: add r[5].w, r[5].w, r[14].w
2306: if r[10].w
2309: mul r[15].xyz, r[3].wwww, r[4].xyzx
2316: else 
2317: ld_structured r[17].xyz, r[7].w, l(52), t[5][5].xyzx
2327: imul null, r[10].w, r[7].w, l(35)
2335: mov r[19].x, r[13].w
2340: mov r[19].y, r[11].w
2345: sample_l r[20].xyz, r[19].xyxx, t[22][r[10].w].xyzw, s[0][5], l(0)
2360: mul r[17].xyz, r[17].xyzx, r[20].xyzx
2367: dp3 r[10].w, r[17].xyzx, r[17].xyzx
2374: ge r[10].w, r[10].w, l(9.99999975e-05)
2381: if r[10].w
2384: mul_sat r[10].w, r[14].w, l(50)
2391: mul r[4].xyz, r[4].xyzx, r[10].wwww
2398: ld_structured r[19].zw, r[7].w, l(68), t[5][5].xxxy
2408: imad r[20].xy, r[7].wwww, l(35,35,0,0), l(2,4,0,0)
2423: sample_l r[10].w, r[19].xyxx, t[22][r[20].x].yzwx, s[3][19], l(0)
2438: add r[11].w, -r[19].z, r[19].w
2446: mad r[21].y, r[10].w, r[11].w, r[19].z
2455: sample_l r[10].w, r[19].xyxx, t[22][r[20].y].yzwx, s[3][19], l(0)
2470: ld_structured r[7].w, r[7].w, l(0), t[4][4].xxxx
2480: ushr r[11].w, r[7].w, l(24)
2487: ubfe r[19].xy, l(8,8,0,0), l(16,8,0,0), r[7].wwww
2502: utof r[20].x, r[11].w
2507: utof r[20].yz, r[19].xxyx
2512: mul r[19].xyz, r[10].wwww, r[20].xyzx
2519: mul r[19].xyz, r[19].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
2529: dp3 r[7].w, r[19].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
2539: dp3 r[21].x, r[13].xyzx, -r[14].xyzx
2547: sample_l r[14].xyz, r[21].xyxx, t[12][12].xyzw, s[1][6], l(0)
2560: mad r[14].xyw, r[19].xyxz, r[14].xxxx, r[14].yyyy
2569: add r[14].xyw, -r[14].xyxw, l(1,1,0,1)
2580: mul r[14].xyw, r[14].xyxw, cb[1][1][15].wwww
2589: mul r[14].xyz, r[14].zzzz, r[14].xywx
2596: mul r[4].xyz, r[4].xyzx, r[14].xyzx
2603: mul r[14].xyz, r[12].yyyy, cb[0][0][3].xyzx
2612: mad r[14].xyz, cb[0][0][2].xyzx, r[12].xxxx, r[14].xyzx
2623: mad r[12].xyz, cb[0][0][4].xyzx, r[12].zzzz, r[14].xyzx
2634: add r[12].xyz, r[12].xyzx, cb[0][0][5].xyzx
2643: mul r[14].xyz, r[13].yyyy, cb[0][0][3].xyzx
2652: mad r[13].xyw, cb[0][0][2].xyxz, r[13].xxxx, r[14].xyxz
2663: mad r[13].xyz, cb[0][0][4].xyzx, r[13].zzzz, r[13].xywx
2674: add r[11].xyz, r[11].xyzx, l(150,75,75,0)
2684: mul r[11].xyz, r[11].xyzx, l(0.0533333346,0.0533333346,0.0533333346,0)
2694: round_ni r[11].xyz, r[11].xyzx
2699: max r[11].xyz, r[11].xyzx, l(0,0,0,0)
2709: min r[11].xyz, r[11].xyzx, l(15,7,11,0)
2719: ftoi r[11].xyz, r[11].xyzx
2724: imad r[6].x, r[11].x, l(12), r[11].z
2733: ishl r[6].y, r[11].y, l(8)
2740: ld r[10].w, r[6].xyzz, t[17][22].yzwx
2748: dp3 r[11].x, -r[12].xyzx, -r[12].xyzx
2757: mad r[9].xyz, r[13].xyzx, l(0.0700000003,0.0700000003,0.0700000003,0), r[12].xyzx
2769: rsq r[11].x, r[11].x
2774: mul r[11].xyz, r[11].xxxx, -r[12].xyzx
2782: add r[11].w, -r[7].w, l(1)
2790: mov r[14].xyz, l(0,0,0,0)
2798: mov r[13].w, l(0)
2803: mov r[14].w, r[10].w
2808: loop 
2809: ine r[15].w, r[14].w, l(9.18340949e-41)
2816: ilt r[16].w, r[13].w, l(256)
2823: and r[15].w, r[15].w, r[16].w
2830: breakc r[15].w
2833: ld_structured r[19].xyzw, r[14].w, l(0), t[7][7].xyzw
2843: ld_structured r[20].xyzw, r[14].w, l(16), t[7][7].xyzw
2853: ld_structured r[21].xyzw, r[14].w, l(36), t[7][7].xyzw
2863: ld_structured r[22].xyzw, r[14].w, l(52), t[7][7].xyzw
2873: ld_structured r[15].w, r[14].w, l(144), t[7][7].xxxx
2883: ld_structured r[16].w, r[14].w, l(156), t[7][7].xxxx
2893: add r[19].xyz, -r[12].xyzx, r[19].xyzx
2901: dp3 r[17].w, r[19].xyzx, r[19].xyzx
2908: rsq r[18].w, r[17].w
2913: mul r[23].xyz, r[18].wwww, r[19].xyzx
2920: sqrt r[23].w, r[17].w
2925: add r[24].x, r[20].w, -r[23].w
2933: mul r[20].w, r[20].w, l(0.200000003)
2940: div_sat r[20].w, r[24].x, r[20].w
2947: mad r[24].x, r[20].w, l(-2), l(3)
2956: mul r[20].w, r[20].w, r[20].w
2963: mul r[20].w, r[20].w, r[24].x
2970: mad r[15].w, r[15].w, r[15].w, r[17].w
2979: max r[15].w, r[15].w, l(9.99999975e-05)
2986: mul r[15].w, r[15].w, l(3.14159274)
2993: rcp r[15].w, r[15].w
2998: add r[15].w, -r[21].x, r[15].w
3006: max r[15].w, r[15].w, l(0)
3013: mul r[15].w, r[20].w, r[15].w
3020: mul r[20].xyz, r[15].wwww, r[20].xyzx
3027: and r[24].xy, r[16].wwww, l(4,2,0,0)
3037: mov r[25].x, r[21].w
3042: mov r[25].yz, r[22].xxyx
3047: dp3 r[15].w, -r[23].xyzx, r[25].xyzx
3055: mad_sat r[15].w, r[15].w, r[21].y, r[21].z
3064: mul r[15].w, r[15].w, r[15].w
3071: mul r[21].xyz, r[15].wwww, r[20].xyzx
3078: movc r[20].xyz, r[24].xxxx, r[21].xyzx, r[20].xyzx
3087: if r[24].y
3090: ld_structured r[21].xy, r[14].w, l(68), t[7][7].xyxx
3100: ld_structured r[24].xyz, r[14].w, l(80), t[7][7].xyzx
3110: ld_structured r[25].xyz, r[14].w, l(96), t[7][7].xyzx
3120: ld_structured r[26].xyw, r[14].w, l(112), t[7][7].xyxz
3130: ld_structured r[27].xyzw, r[14].w, l(128), t[7][7].xyzw
3140: mov r[28].x, r[22].w
3145: mov r[28].y, r[24].x
3150: mov r[28].z, r[25].x
3155: mov r[28].w, r[26].x
3160: dp4 r[28].x, r[28].xyzw, r[9].xyzw
3167: mov r[29].x, r[21].x
3172: mov r[29].y, r[24].y
3177: mov r[29].z, r[25].y
3182: mov r[29].w, r[26].y
3187: dp4 r[28].y, r[29].xyzw, r[9].xyzw
3194: mov r[26].x, r[21].y
3199: mov r[26].y, r[24].z
3204: mov r[26].z, r[25].z
3209: dp4 r[28].z, r[26].xyzw, r[9].xyzw
3216: dp3 r[15].w, r[28].xyzx, r[28].xyzx
3223: rsq r[15].w, r[15].w
3228: mul r[21].xyz, r[15].wwww, r[28].xyzx
3235: lt r[24].xyzw, abs(r[21].yzxz), abs(r[21].xxyy)
3244: lt r[22].xyw, l(0,0,0,0), r[21].xyxz
3254: and r[24].xy, r[24].ywyy, r[24].xzxx
3261: movc r[22].xyw, r[22].xyxw, l(0,2,0,4), l(1,3,0,5)
3276: movc r[15].w, r[24].y, r[22].y, r[22].w
3285: movc r[15].w, r[24].x, r[22].x, r[15].w
3294: if r[15].w
3297: ine r[16].w, r[15].w, l(1)
3304: if r[16].w
3307: div r[24].xyz, r[21].xxyx, r[21].yzzy
3314: div r[16].w, -r[21].z, r[21].y
3322: mad r[25].y, r[16].w, l(-0.5), l(0.5)
3331: div r[21].xw, r[21].xxxz, -r[21].yyyy
3339: mad r[21].xw, r[21].xxxw, l(0.5,0,0,-0.5), l(0.5,0,0,0.5)
3354: ieq r[26].xyz, r[15].wwww, l(2,3,4,0)
3364: mad r[24].xyz, r[24].xyzx, l(0.5,0.5,-0.5,0), l(0.5,0.5,0.5,0)
3379: div r[16].w, r[21].y, -r[21].z
3387: mad r[24].w, r[16].w, l(-0.5), l(0.5)
3396: movc r[21].yz, r[26].zzzz, r[24].yyzy, r[24].yywy
3405: movc r[21].xy, r[26].yyyy, r[21].xwxx, r[21].yzyy
3414: mov r[25].x, r[24].x
3419: movc r[22].xy, r[26].xxxx, r[25].xyxx, r[21].xyxx
3428: else 
3429: div r[24].xy, r[21].zyzz, -r[21].xxxx
3437: mad r[22].xy, r[24].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
3452: endif 
3453: else 
3454: div r[16].w, -r[21].z, r[21].x
3462: mad r[22].x, r[16].w, l(0.5), l(0.5)
3471: div r[16].w, r[21].y, r[21].x
3478: mad r[22].y, r[16].w, l(-0.5), l(0.5)
3487: endif 
3488: add r[21].xy, r[22].xyxx, l(-0.5,-0.5,0,0)
3498: mad r[21].yz, r[21].xxyx, r[19].wwww, l(0,0.5,0.5,0)
3510: utof r[15].w, r[15].w
3515: mul r[15].w, r[15].w, l(0.166666672)
3522: mad r[21].x, r[21].y, l(0.166666672), r[15].w
3531: mad r[21].xy, r[21].xzxx, r[27].zwzz, r[27].xyxx
3540: sample_l r[21].xy, r[21].xyxx, t[6][6].xyzw, s[1][6], l(0)
3553: add r[15].w, r[23].w, l(-0.0500000007)
3560: mul r[16].w, r[22].z, r[15].w
3567: mad r[17].w, -r[21].x, r[21].x, r[21].y
3577: max r[17].w, r[17].w, l(9.99999975e-06)
3584: mad r[15].w, -r[15].w, r[22].z, r[21].x
3594: mad r[15].w, r[15].w, r[15].w, r[17].w
3603: div r[15].w, r[17].w, r[15].w
3610: min r[15].w, r[15].w, l(1)
3617: lt r[16].w, r[16].w, r[21].x
3624: movc r[15].w, r[16].w, l(1), r[15].w
3633: mul r[20].xyz, r[15].wwww, r[20].xyzx
3640: endif 
3641: mad r[19].xyz, r[19].xyzx, r[18].wwww, r[11].xyzx
3650: dp3 r[15].w, r[19].xyzx, r[19].xyzx
3657: rsq r[15].w, r[15].w
3662: mul r[19].xyz, r[15].wwww, r[19].xyzx
3669: dp3 r[15].w, r[11].xyzx, r[19].xyzx
3676: dp3_sat r[16].w, r[13].xyzx, r[23].xyzx
3683: add r[15].w, -abs(r[15].w), l(1)
3691: max r[15].w, r[15].w, l(0)
3698: mul r[17].w, r[15].w, r[15].w
3705: mul r[17].w, r[17].w, r[17].w
3712: mul r[15].w, r[15].w, r[17].w
3719: mad r[15].w, r[11].w, r[15].w, r[7].w
3728: add_sat r[15].w, -r[15].w, l(1)
3736: mul r[15].w, r[16].w, r[15].w
3743: mul r[15].w, r[15].w, l(0.318309873)
3750: mad r[14].xyz, r[15].wwww, r[20].xyzx, r[14].xyzx
3759: iadd r[13].w, r[13].w, l(1)
3766: iadd r[6].w, r[6].y, r[13].w
3773: ld r[14].w, r[6].xwzz, t[17][22].yzwx
3781: endloop 
3782: ld r[6].w, r[6].xyzz, t[18][23].yzwx
3790: mul r[9].xyz, r[12].zzzz, r[13].xyzx
3797: mad r[8].xyz, r[9].xyzx, l(0.000300000014,0.000300000014,0.000300000014,0), r[12].xyzx
3809: mov r[19].xzw, r[6].xxzz
3814: mov r[9].xyz, r[14].xyzx
3819: mov r[6].x, l(0)
3824: mov r[10].w, r[6].w
3829: loop 
3830: ine r[13].w, r[10].w, l(9.18340949e-41)
3837: ilt r[14].w, r[6].x, l(256)
3844: and r[13].w, r[13].w, r[14].w
3851: breakc r[13].w
3854: iadd r[6].x, r[6].x, l(1)
3861: iadd r[19].y, r[6].x, r[6].y
3868: ld r[13].w, r[19].xyzw, t[18][23].yzwx
3876: ld_structured r[20].xyzw, r[10].w, l(68), t[11][11].xyzw
3886: ld_structured r[21].xyz, r[10].w, l(84), t[11][11].xyzx
3896: ld_structured r[22].xyzw, r[10].w, l(96), t[11][11].xyzw
3906: ld_structured r[23].xyzw, r[10].w, l(112), t[11][11].xyzw
3916: ld_structured r[24].xyzw, r[10].w, l(128), t[11][11].xyzw
3926: mov r[25].x, r[20].w
3931: mov r[25].y, r[22].x
3936: mov r[25].z, r[23].x
3941: mov r[25].w, r[24].x
3946: dp4 r[25].x, r[25].xyzw, r[8].xyzw
3953: mov r[26].x, r[21].x
3958: mov r[26].y, r[22].y
3963: mov r[26].z, r[23].y
3968: mov r[26].w, r[24].y
3973: dp4 r[25].y, r[26].xyzw, r[8].xyzw
3980: mov r[26].x, r[21].y
3985: mov r[26].y, r[22].z
3990: mov r[26].z, r[23].z
3995: mov r[26].w, r[24].z
4000: dp4 r[25].z, r[26].xyzw, r[8].xyzw
4007: mov r[24].x, r[21].z
4012: mov r[24].y, r[22].w
4017: mov r[24].z, r[23].w
4022: dp4 r[14].w, r[24].xyzw, r[8].xyzw
4029: rcp r[15].w, r[14].w
4034: mul r[21].xyz, r[15].wwww, r[25].xyzx
4041: mad r[22].xyz, r[21].xyzx, l(0.5,0.5,1,0), l(0.5,0.5,0,0)
4056: mov_sat r[23].xyz, r[22].xyzx
4061: eq r[22].xyz, r[22].xyzx, r[23].xyzx
4068: and r[15].w, r[22].y, r[22].x
4075: and r[15].w, r[22].z, r[15].w
4082: if r[15].w
4085: ld_structured r[22].xyzw, r[10].w, l(0), t[11][11].xyzw
4095: ld_structured r[15].w, r[10].w, l(156), t[11][11].xxxx
4105: ld_structured r[16].w, r[10].w, l(200), t[11][11].xxxx
4115: ld_structured r[23].xyzw, r[10].w, l(288), t[11][11].xyzw
4125: ld_structured r[17].w, r[10].w, l(304), t[11][11].xxxx
4135: mad r[24].xy, r[21].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
4150: and r[24].zw, r[17].wwww, l(0,0,2,768)
4160: if r[24].z
4163: ld_structured r[26].xyzw, r[10].w, l(272), t[11][11].xyzw
4173: mad r[25].xy, r[24].xyxx, r[26].zwzz, r[26].xyxx
4182: sample_l r[25].xy, r[25].xyxx, t[9][9].xyzw, s[1][6], l(0)
4195: add r[26].xyz, -r[12].xyzx, r[22].xyzx
4203: dp3 r[18].w, r[26].xyzx, r[26].xyzx
4210: sqrt r[18].w, r[18].w
4215: add r[18].w, -r[22].w, r[18].w
4223: mul r[19].y, r[15].w, r[18].w
4230: mad r[20].w, -r[25].x, r[25].x, r[25].y
4240: max r[20].w, r[20].w, l(9.99999975e-06)
4247: mad r[18].w, -r[18].w, r[15].w, r[25].x
4257: mad r[18].w, r[18].w, r[18].w, r[20].w
4266: div r[18].w, r[20].w, r[18].w
4273: min r[18].w, r[18].w, l(1)
4280: lt r[19].y, r[19].y, r[25].x
4287: movc r[18].w, r[19].y, l(1), r[18].w
4296: else 
4297: mov r[18].w, l(1)
4302: endif 
4303: ieq r[25].xy, r[24].wwww, l(256,512,0,0)
4313: if r[25].x
4316: mad r[25].xw, r[24].xxxy, r[23].zzzw, r[23].xxxy
4325: sample_l r[19].y, r[25].xwxx, t[8][8].yxzw, s[1][6], l(0)
4338: ge r[19].y, r[19].y, r[21].z
4345: and r[19].y, r[19].y, l(1)
4352: else 
4353: ieq r[20].w, r[24].w, l(512)
4360: if r[20].w
4363: mad r[21].zw, r[24].xxxy, r[23].zzzw, r[23].xxxy
4372: sample_l r[21].zw, r[21].zwzz, t[9][9].zwxy, s[1][6], l(0)
4385: add r[23].xyz, -r[12].xyzx, r[22].xyzx
4393: dp3 r[20].w, r[23].xyzx, r[23].xyzx
4400: sqrt r[20].w, r[20].w
4405: add r[20].w, -r[22].w, r[20].w
4413: mul r[22].w, r[15].w, r[20].w
4420: mad r[21].w, -r[21].z, r[21].z, r[21].w
4430: max r[21].w, r[21].w, l(9.99999975e-06)
4437: mad r[15].w, -r[20].w, r[15].w, r[21].z
4447: mad r[15].w, r[15].w, r[15].w, r[21].w
4456: div r[15].w, r[21].w, r[15].w
4463: min r[15].w, r[15].w, l(1)
4470: lt r[20].w, r[22].w, r[21].z
4477: movc r[19].y, r[20].w, l(1), r[15].w
4486: else 
4487: mov r[19].y, l(1)
4492: endif 
4493: endif 
4494: ine r[15].w, r[24].z, l(0)
4501: and r[15].w, r[15].w, r[25].y
4508: add r[20].w, -r[18].w, r[19].y
4516: mad r[20].w, r[16].w, r[20].w, r[18].w
4525: mad r[19].y, r[19].y, r[16].w, l(1)
4534: add r[16].w, -r[16].w, r[19].y
4542: min r[16].w, r[16].w, r[18].w
4549: movc r[15].w, r[15].w, r[20].w, r[16].w
4558: ne r[16].w, r[15].w, l(0)
4565: if r[16].w
4568: ld_structured r[23].xyz, r[10].w, l(16), t[11][11].xyzx
4578: ld_structured r[26].xyzw, r[10].w, l(48), t[11][11].xyzw
4588: add r[16].w, -r[25].z, r[26].w
4596: mul r[18].w, r[26].w, l(0.200000003)
4603: div_sat r[16].w, r[16].w, r[18].w
4610: mad r[18].w, r[16].w, l(-2), l(3)
4619: mul r[16].w, r[16].w, r[16].w
4626: mul r[16].w, r[16].w, r[18].w
4633: add r[23].xyz, -r[12].xyzx, r[23].xyzx
4641: dp3 r[18].w, r[23].xyzx, r[23].xyzx
4648: mul r[19].y, r[20].y, r[20].y
4655: mad r[18].w, r[20].z, r[18].w, r[19].y
4664: max r[18].w, r[18].w, l(0.25)
4671: mul r[18].w, r[18].w, l(3.14159274)
4678: rcp r[18].w, r[18].w
4683: add r[18].w, -r[20].x, r[18].w
4691: max r[18].w, r[18].w, l(0)
4698: mul r[16].w, r[16].w, r[18].w
4705: mul r[20].xyz, r[16].wwww, r[26].xyzx
4712: and r[16].w, r[17].w, l(1)
4719: if r[16].w
4722: ld_structured r[23].xyzw, r[10].w, l(256), t[11][11].xyzw
4732: and r[16].w, r[17].w, l(1.43492963e-42)
4739: add r[21].zw, r[21].xxxy, r[21].xxxy
4746: mad r[17].w, r[21].x, r[21].x, l(1)
4755: mad r[17].w, r[21].y, r[21].y, r[17].w
4764: div r[21].xy, r[21].zwzz, r[17].wwww
4771: mad r[21].xy, r[21].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
4786: movc r[21].xy, r[16].wwww, r[21].xyxx, r[24].xyxx
4795: mad r[21].xy, r[21].xyxx, r[23].zwzz, r[23].xyxx
4804: sample_l r[21].xyz, r[21].xyxx, t[10][10].xyzw, s[2][11], l(0)
4817: mul r[20].xyz, r[20].xyzx, r[21].xyzx
4824: endif 
4825: ge r[14].w, r[14].w, r[25].z
4832: and r[14].w, r[14].w, l(1)
4839: mul r[14].w, r[14].w, r[15].w
4846: mul r[20].xyz, r[14].wwww, r[20].xyzx
4853: add r[21].xyz, -r[12].xyzx, r[22].xyzx
4861: dp3 r[14].w, r[21].xyzx, r[21].xyzx
4868: rsq r[14].w, r[14].w
4873: mul r[22].xyz, r[14].wwww, r[21].xyzx
4880: mad r[21].xyz, r[21].xyzx, r[14].wwww, r[11].xyzx
4889: dp3 r[14].w, r[21].xyzx, r[21].xyzx
4896: rsq r[14].w, r[14].w
4901: mul r[21].xyz, r[14].wwww, r[21].xyzx
4908: dp3 r[14].w, r[11].xyzx, r[21].xyzx
4915: dp3_sat r[15].w, r[13].xyzx, r[22].xyzx
4922: add r[14].w, -abs(r[14].w), l(1)
4930: max r[14].w, r[14].w, l(0)
4937: mul r[16].w, r[14].w, r[14].w
4944: mul r[16].w, r[16].w, r[16].w
4951: mul r[14].w, r[14].w, r[16].w
4958: mad r[14].w, r[11].w, r[14].w, r[7].w
4967: add_sat r[14].w, -r[14].w, l(1)
4975: mul r[14].w, r[15].w, r[14].w
4982: mul r[14].w, r[14].w, l(0.318309873)
4989: mad r[9].xyz, r[14].wwww, r[20].xyzx, r[9].xyzx
4998: endif 
4999: endif 
5000: mov r[10].w, r[13].w
5005: endloop 
5006: mad r[4].xyz, r[4].xyzx, l(0.318309873,0.318309873,0.318309873,0), r[9].xyzx
5018: mul r[15].xyz, r[17].xyzx, r[4].xyzx
5025: else 
5026: mov r[15].xyz, l(0,0,0,0)
5034: endif 
5035: endif 
5036: mad r[10].xyz, r[3].xyzx, r[15].xyzx, r[10].xyzx
5045: iadd r[0].z, r[0].z, l(1)
5052: endloop 
5053: add r[0].xy, -cb[1][1][17].yzyy, cb[1][1][17].xwxx
5065: add r[0].z, cb[1][1][17].y, cb[1][1][17].x
5076: add r[0].z, -r[0].x, r[0].z
5084: add r[0].x, -r[0].x, r[1].w
5092: div r[0].z, l(1,1,1,1), r[0].z
5102: mul_sat r[0].x, r[0].z, r[0].x
5109: mad r[0].z, r[0].x, l(-2), l(3)
5118: mul r[0].x, r[0].x, r[0].x
5125: mul r[0].x, r[0].x, r[0].z
5132: mad r[0].x, r[0].x, r[0].y, cb[1][1][17].z
5143: mul r[0].xyz, r[0].xxxx, r[10].xyzx
5150: itof r[0].w, cb[4][4][0].z
5157: div r[1].xyz, r[0].xyzx, r[0].wwww
5164: div r[0].x, r[5].w, r[0].w
5171: mov r[1].w, l(1)
5176: else 
5177: mov r[1].xyzw, l(0,0,0,0)
5185: mov r[0].x, l(65504)
5190: endif 
5191: store_uav_typed type30[2][2].xyzw, type32.xyyy, r[0].xxxx
5198: store_uav_typed type30[0][0].xyzw, type32.xyyy, r[1].xyzw
5205: store_uav_typed type30[1][1].xyzw, type32.xyyy, l(1,1,1,1)
5215: ret 