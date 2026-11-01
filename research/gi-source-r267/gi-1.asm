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
1123: mov r[7].w, l(0)
1128: mov r[0].z, l(0)
1133: loop 
1134: ige r[8].w, r[0].z, cb[4][4][0].z
1143: breakc r[8].w
1146: ld r[8].w, r[0].xyzw, t[21][26].yzwx
1154: ld r[16].xyzw, r[0].xyzw, t[23][28].xyzw
1162: mul r[17].xyz, r[16].yyyy, cb[0][0][7].xyzx
1171: mad r[17].xyz, cb[0][0][6].xyzx, r[16].xxxx, r[17].xyzx
1182: mad r[17].xyz, cb[0][0][8].xyzx, r[16].zzzz, r[17].xyzx
1193: add r[17].xyz, r[17].xyzx, cb[0][0][9].xyzx
1202: ld r[18].xyzw, r[0].xyzw, t[22][27].xyzw
1210: add r[19].xyz, -r[1].xyzx, r[17].xyzx
1218: dp3 r[17].w, r[19].xyzx, r[19].xyzx
1225: rsq r[17].w, r[17].w
1230: mul r[19].xyz, r[17].wwww, r[19].xyzx
1237: ieq r[17].w, r[8].w, l(9.18340949e-41)
1244: mad r[20].xyz, cb[4][4][0].yyyy, r[19].xyzx, r[5].xyzx
1255: movc r[12].xyz, r[17].wwww, r[20].xyzx, r[17].xyzx
1264: movc r[20].xyz, r[17].wwww, r[7].xyzx, r[18].xyzx
1273: mov r[21].xyz, l(0,0,0,0)
1281: mov r[19].w, l(0)
1286: mov r[20].w, l(0)
1291: loop 
1292: uge r[21].w, r[20].w, cb[2][2][0].w
1301: breakc r[21].w
1304: ishl r[21].w, r[20].w, l(2)
1311: dp4 r[22].x, cb[3][3][r[21].w].xyzw, r[12].xyzw
1321: dp4 r[22].y, cb[3][3][1+r[21].w].xyzw, r[12].xyzw
1332: dp4 r[22].z, cb[3][3][2+r[21].w].xyzw, r[12].xyzw
1343: ge r[23].xyz, r[22].xyzx, l(0,0,0,0)
1353: and r[22].w, r[23].y, r[23].x
1360: and r[22].w, r[23].z, r[22].w
1367: utof r[23].xyz, cb[3][3][3+r[21].w].xyzx
1376: lt r[24].xyz, r[22].xyzx, r[23].xyzx
1383: and r[23].w, r[24].y, r[24].x
1390: and r[23].w, r[24].z, r[23].w
1397: and r[22].w, r[22].w, r[23].w
1404: if r[22].w
1407: ld_structured r[24].xyz, r[20].w, l(0), t[15][15].xyzx
1417: ld_structured r[25].xyz, r[20].w, l(12), t[15][15].xyzx
1427: ld_structured r[26].xyzw, r[20].w, l(24), t[15][15].xyzw
1437: ld_structured r[27].xyzw, r[20].w, l(40), t[15][15].xyzw
1447: ld_structured r[28].xyz, r[20].w, l(56), t[15][15].xyzx
1457: mul_sat r[22].w, r[22].x, r[26].w
1464: add r[23].xyz, -r[22].xyzx, r[23].xyzx
1472: mul_sat r[23].x, r[26].w, r[23].x
1479: mul r[22].w, r[22].w, r[23].x
1486: mul_sat r[23].xw, r[22].yyyz, r[27].xxxy
1493: mul r[22].w, r[22].w, r[23].x
1500: mul_sat r[23].xy, r[27].xyxx, r[23].yzyy
1507: mul r[22].w, r[22].w, r[23].x
1514: mul r[22].w, r[23].w, r[22].w
1521: mul r[22].w, r[23].y, r[22].w
1528: ine r[23].x, r[28].x, l(nan)
1535: if r[23].x
1538: round_ni r[23].xyz, r[22].xyzx
1543: add r[22].xyz, r[22].xyzx, -r[23].xyzx
1551: ftou r[23].xyz, r[23].xyzx
1556: imad r[23].y, r[23].z, cb[3][3][3+r[21].w].y, r[23].y
1569: imad r[21].w, r[23].y, cb[3][3][3+r[21].w].x, r[23].x
1582: iadd r[21].w, r[27].z, r[21].w
1589: ld_structured r[21].w, r[21].w, l(0), t[16][16].xxxx
1599: mov r[23].xyz, l(0,0,0,0)
1607: mov r[27].xyz, r[22].xyzx
1612: mov r[23].w, r[21].w
1617: mov r[24].w, r[28].y
1622: loop 
1623: and r[25].w, r[23].w, l(-0)
1630: ine r[25].w, r[25].w, l(0)
1637: ge r[26].w, l(9.99999975e-06), r[24].w
1644: or r[25].w, r[25].w, r[26].w
1651: if r[25].w
1654: and r[25].w, r[23].w, l(nan)
1661: iadd r[25].w, r[28].x, r[25].w
1668: and r[29].x, r[25].w, l(127)
1675: ubfe r[29].y, l(7), l(7), r[25].w
1684: ushr r[29].z, r[25].w, l(14)
1691: ishl r[29].xyz, r[29].xyzx, l(2,2,2,0)
1701: utof r[29].xyz, r[29].xyzx
1706: mad r[29].xyz, r[27].xyzx, l(3,3,3,0), r[29].xyzx
1718: add r[23].xyz, r[29].xyzx, l(0.5,0.5,0.5,0)
1728: break 
1729: endif 
1730: iadd r[25].w, r[27].w, r[23].w
1737: ld_structured r[29].xyzw, r[25].w, l(0), t[17][17].xyzw
1747: mul r[30].xyz, r[27].xyzx, l(3,3,3,0)
1757: round_ni r[30].xyz, r[30].xyzx
1762: ftou r[31].xyz, r[30].xyzx
1767: utof r[31].xz, r[31].zzxz
1772: imul null, r[25].w, r[31].y, l(3)
1780: utof r[25].w, r[25].w
1785: mad r[25].w, r[31].x, l(9), r[25].w
1794: add r[25].w, r[31].z, r[25].w
1801: ftou r[25].w, r[25].w
1806: ishl r[26].w, l(1), r[25].w
1813: and r[28].w, r[26].w, r[29].x
1820: if r[28].w
1823: iadd r[28].w, r[28].x, r[23].w
1830: and r[31].x, r[28].w, l(127)
1837: ubfe r[31].y, l(7), l(7), r[28].w
1846: ushr r[31].z, r[28].w, l(14)
1853: ishl r[31].xyz, r[31].xyzx, l(2,2,2,0)
1863: utof r[31].xyz, r[31].xyzx
1868: mad r[31].xyz, r[27].xyzx, l(3,3,3,0), r[31].xyzx
1880: add r[23].xyz, r[31].xyzx, l(0.5,0.5,0.5,0)
1890: break 
1891: endif 
1892: mad r[27].xyz, r[27].xyzx, l(3,3,3,0), -r[30].xyzx
1905: and r[26].w, r[26].w, r[29].y
1912: ubfe r[28].w, r[25].w, l(0), r[29].y
1921: countbits r[28].w, r[28].w
1926: iadd r[28].w, r[28].w, r[29].w
1933: or r[28].w, r[28].w, l(-0)
1940: not r[29].y, r[29].y
1945: and r[29].x, r[29].y, r[29].x
1952: ubfe r[25].w, r[25].w, l(0), r[29].x
1961: countbits r[25].w, r[25].w
1966: iadd r[25].w, r[25].w, r[29].z
1973: movc r[23].w, r[26].w, r[28].w, r[25].w
1982: mul r[24].w, r[24].w, l(0.333333343)
1989: mov r[23].xyz, l(0,0,0,0)
1997: endloop 
1998: else 
1999: mov r[23].xyz, l(0,0,0,0)
2007: endif 
2008: mul r[22].xyz, r[23].xyzx, cb[2][2][0].xyzx
2017: mov r[27].x, r[24].x
2022: mov r[27].y, r[25].x
2027: mov r[27].z, r[26].x
2032: dp3 r[27].x, r[27].xyzx, r[20].xyzx
2039: mov r[29].x, r[24].y
2044: mov r[29].y, r[25].y
2049: mov r[29].z, r[26].y
2054: dp3 r[27].y, r[29].xyzx, r[20].xyzx
2061: mov r[26].x, r[24].z
2066: mov r[26].y, r[25].z
2071: dp3 r[27].z, r[26].xyzx, r[20].xyzx
2078: mov_sat r[24].xyz, r[27].xyzx
2083: mul r[24].xyz, r[24].xyzx, r[24].xyzx
2090: mov_sat r[27].xyz, -r[27].xyzx
2096: mul r[25].xyz, r[27].xyzx, r[27].xyzx
2103: lt r[26].xyz, l(0,0,0,0), r[24].xyzx
2113: if r[26].x
2116: sample_l r[27].xyz, r[22].xyzx, t[18][18].xyzw, s[1][6], l(0)
2129: mul r[27].xyz, r[24].xxxx, r[27].xyzx
2136: else 
2137: mov r[27].xyz, l(0,0,0,0)
2145: endif 
2146: lt r[29].xyz, l(0,0,0,0), r[25].xyzx
2156: if r[29].x
2159: sample_l r[30].xyz, r[22].xyzx, t[18][19].xyzw, s[1][6], l(0)
2172: mad r[27].xyz, r[30].xyzx, r[25].xxxx, r[27].xyzx
2181: endif 
2182: if r[26].y
2185: sample_l r[26].xyw, r[22].xyzx, t[18][20].xywz, s[1][6], l(0)
2198: mad r[27].xyz, r[26].xywx, r[24].yyyy, r[27].xyzx
2207: endif 
2208: if r[29].y
2211: sample_l r[24].xyw, r[22].xyzx, t[18][21].xywz, s[1][6], l(0)
2224: mad r[27].xyz, r[24].xywx, r[25].yyyy, r[27].xyzx
2233: endif 
2234: if r[26].z
2237: sample_l r[24].xyw, r[22].xyzx, t[18][22].xywz, s[1][6], l(0)
2250: mad r[27].xyz, r[24].xywx, r[24].zzzz, r[27].xyzx
2259: endif 
2260: if r[29].z
2263: sample_l r[22].xyz, r[22].xyzx, t[18][23].xyzw, s[1][6], l(0)
2276: mad r[27].xyz, r[22].xyzx, r[25].zzzz, r[27].xyzx
2285: endif 
2286: ieq r[21].w, r[28].x, l(nan)
2293: movc r[22].xyz, r[21].wwww, l(1,0.349999994,0,0), r[27].xyzx
2305: mad r[21].w, r[22].w, l(-2), l(3)
2314: mul r[23].w, r[22].w, r[22].w
2321: mul r[21].w, r[21].w, r[23].w
2328: add r[23].w, r[28].z, l(-1)
2335: mad r[21].w, r[21].w, r[23].w, l(1)
2344: mul r[23].w, r[21].w, r[22].w
2351: mad r[21].xyz, r[22].xyzx, r[23].wwww, r[21].xyzx
2360: mad r[19].w, r[22].w, r[21].w, r[19].w
2369: endif 
2370: iadd r[20].w, r[20].w, l(1)
2377: endloop 
2378: lt r[12].x, l(0), r[19].w
2385: div r[20].xyz, r[21].xyzx, r[19].wwww
2392: movc r[12].xyz, r[12].xxxx, r[20].xyzx, r[21].xyzx
2401: add r[12].xyz, r[12].xyzx, cb[2][2][1].xyzx
2410: mad r[20].xyz, r[2].xyzx, r[2].wwww, -r[16].xyzx
2420: dp3 r[20].x, r[20].xyzx, r[20].xyzx
2427: sqrt r[20].x, r[20].x
2432: movc r[20].x, r[17].w, cb[4][4][0].x, r[20].x
2443: add r[7].w, r[7].w, r[20].x
2450: if r[17].w
2453: mul r[20].yzw, r[3].wwww, r[12].xxyz
2460: else 
2461: ld_structured r[22].xyz, r[8].w, l(52), t[5][5].xyzx
2471: imul null, r[17].w, r[8].w, l(35)
2479: mov r[24].x, r[18].w
2484: mov r[24].y, r[16].w
2489: sample_l r[25].xyz, r[24].xyxx, t[24][r[17].w].xyzw, s[0][5], l(0)
2504: mul r[22].xyz, r[22].xyzx, r[25].xyzx
2511: dp3 r[16].w, r[22].xyzx, r[22].xyzx
2518: ge r[16].w, r[16].w, l(9.99999975e-05)
2525: if r[16].w
2528: mul_sat r[16].w, r[20].x, l(50)
2535: mul r[12].xyz, r[12].xyzx, r[16].wwww
2542: ld_structured r[24].zw, r[8].w, l(68), t[5][5].xxxy
2552: imad r[25].xy, r[8].wwww, l(35,35,0,0), l(2,4,0,0)
2567: sample_l r[16].w, r[24].xyxx, t[24][r[25].x].yzwx, s[4][19], l(0)
2582: add r[17].w, -r[24].z, r[24].w
2590: mad r[26].y, r[16].w, r[17].w, r[24].z
2599: sample_l r[16].w, r[24].xyxx, t[24][r[25].y].yzwx, s[4][19], l(0)
2614: ld_structured r[8].w, r[8].w, l(0), t[4][4].xxxx
2624: ushr r[17].w, r[8].w, l(24)
2631: ubfe r[24].xy, l(8,8,0,0), l(16,8,0,0), r[8].wwww
2646: utof r[25].x, r[17].w
2651: utof r[25].yz, r[24].xxyx
2656: mul r[24].xyz, r[16].wwww, r[25].xyzx
2663: mul r[24].xyz, r[24].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
2673: dp3 r[8].w, r[24].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
2683: dp3 r[26].x, r[18].xyzx, -r[19].xyzx
2691: sample_l r[19].xyz, r[26].xyxx, t[14][14].xyzw, s[1][6], l(0)
2704: mad r[24].xyz, r[24].xyzx, r[19].xxxx, r[19].yyyy
2713: add r[24].xyz, -r[24].xyzx, l(1,1,1,0)
2724: mul r[24].xyz, r[24].xyzx, cb[1][1][15].wwww
2733: mul r[19].xyz, r[19].zzzz, r[24].xyzx
2740: mul r[12].xyz, r[12].xyzx, r[19].xyzx
2747: mul r[19].xyz, r[17].yyyy, cb[0][0][3].xyzx
2756: mad r[17].xyw, cb[0][0][2].xyxz, r[17].xxxx, r[19].xyxz
2767: mad r[17].xyz, cb[0][0][4].xyzx, r[17].zzzz, r[17].xywx
2778: add r[15].xyz, r[17].xyzx, cb[0][0][5].xyzx
2787: mul r[17].xyz, r[18].yyyy, cb[0][0][3].xyzx
2796: mad r[17].xyz, cb[0][0][2].xyzx, r[18].xxxx, r[17].xyzx
2807: mad r[17].xyz, cb[0][0][4].xyzx, r[18].zzzz, r[17].xyzx
2818: add r[16].xyz, r[16].xyzx, l(150,75,75,0)
2828: mul r[16].xyz, r[16].xyzx, l(0.0533333346,0.0533333346,0.0533333346,0)
2838: round_ni r[16].xyz, r[16].xyzx
2843: max r[16].xyz, r[16].xyzx, l(0,0,0,0)
2853: min r[16].xyz, r[16].xyzx, l(15,7,11,0)
2863: ftoi r[16].xyz, r[16].xyzx
2868: uge r[16].w, r[6].w, l(1)
2875: if r[16].w
2878: dp4 r[18].x, r[14].xyzw, r[15].xyzw
2885: dp4 r[18].y, r[11].xyzw, r[15].xyzw
2892: sample_l r[16].w, r[18].xyxx, t[12][12].yzwx, s[2][8], l(0)
2905: add r[16].w, r[16].w, l(-1)
2912: mad r[16].w, r[4].w, r[16].w, l(1)
2921: mul r[18].xyz, r[8].xyzx, r[16].wwww
2928: movc r[18].xyz, r[5].wwww, r[18].xyzx, r[8].xyzx
2937: dp3 r[16].w, -r[15].xyzx, -r[15].xyzx
2946: rsq r[16].w, r[16].w
2951: mul r[19].xyz, -r[15].xyzx, r[16].wwww
2959: mad r[24].xyz, -r[15].xyzx, r[16].wwww, r[6].xyzx
2969: dp3 r[16].w, r[24].xyzx, r[24].xyzx
2976: rsq r[16].w, r[16].w
2981: mul r[24].xyz, r[16].wwww, r[24].xyzx
2988: dp3 r[16].w, r[19].xyzx, r[24].xyzx
2995: dp3_sat r[17].w, r[17].xyzx, r[6].xyzx
3002: add r[18].w, -r[8].w, l(1)
3010: add r[16].w, -abs(r[16].w), l(1)
3018: max r[16].w, r[16].w, l(0)
3025: mul r[19].x, r[16].w, r[16].w
3032: mul r[19].x, r[19].x, r[19].x
3039: mul r[16].w, r[16].w, r[19].x
3046: mad r[16].w, r[18].w, r[16].w, r[8].w
3055: add_sat r[16].w, -r[16].w, l(1)
3063: mul r[16].w, r[17].w, r[16].w
3070: mul r[16].w, r[16].w, l(0.318309873)
3077: mul r[18].xyz, r[18].xyzx, r[16].wwww
3084: else 
3085: mov r[18].xyz, l(0,0,0,0)
3093: endif 
3094: imad r[13].x, r[16].x, l(12), r[16].z
3103: ishl r[13].y, r[16].y, l(8)
3110: ld r[16].x, r[13].xyzz, t[19][24].xyzw
3118: dp3 r[16].y, -r[15].xyzx, -r[15].xyzx
3127: mad r[10].xyz, r[17].xyzx, l(0.0700000003,0.0700000003,0.0700000003,0), r[15].xyzx
3139: rsq r[16].y, r[16].y
3144: mul r[16].yzw, -r[15].xxyz, r[16].yyyy
3152: add r[17].w, -r[8].w, l(1)
3160: mov r[19].xyz, r[18].xyzx
3165: mov r[18].w, l(0)
3170: mov r[20].x, r[16].x
3175: loop 
3176: ine r[21].w, r[20].x, l(9.18340949e-41)
3183: ilt r[22].w, r[18].w, l(256)
3190: and r[21].w, r[21].w, r[22].w
3197: breakc r[21].w
3200: ld_structured r[24].xyzw, r[20].x, l(0), t[7][7].xyzw
3210: ld_structured r[25].xyzw, r[20].x, l(16), t[7][7].xyzw
3220: ld_structured r[26].xyzw, r[20].x, l(36), t[7][7].xyzw
3230: ld_structured r[27].xyzw, r[20].x, l(52), t[7][7].xyzw
3240: ld_structured r[21].w, r[20].x, l(144), t[7][7].xxxx
3250: ld_structured r[22].w, r[20].x, l(156), t[7][7].xxxx
3260: add r[24].xyz, -r[15].xyzx, r[24].xyzx
3268: dp3 r[23].w, r[24].xyzx, r[24].xyzx
3275: rsq r[28].x, r[23].w
3280: mul r[28].yzw, r[24].xxyz, r[28].xxxx
3287: sqrt r[29].x, r[23].w
3292: add r[29].y, r[25].w, -r[29].x
3300: mul r[25].w, r[25].w, l(0.200000003)
3307: div_sat r[25].w, r[29].y, r[25].w
3314: mad r[29].y, r[25].w, l(-2), l(3)
3323: mul r[25].w, r[25].w, r[25].w
3330: mul r[25].w, r[25].w, r[29].y
3337: mad r[21].w, r[21].w, r[21].w, r[23].w
3346: max r[21].w, r[21].w, l(9.99999975e-05)
3353: mul r[21].w, r[21].w, l(3.14159274)
3360: rcp r[21].w, r[21].w
3365: add r[21].w, -r[26].x, r[21].w
3373: max r[21].w, r[21].w, l(0)
3380: mul r[21].w, r[25].w, r[21].w
3387: mul r[25].xyz, r[21].wwww, r[25].xyzx
3394: and r[29].yz, r[22].wwww, l(0,4,2,0)
3404: mov r[30].x, r[26].w
3409: mov r[30].yz, r[27].xxyx
3414: dp3 r[21].w, -r[28].yzwy, r[30].xyzx
3422: mad_sat r[21].w, r[21].w, r[26].y, r[26].z
3431: mul r[21].w, r[21].w, r[21].w
3438: mul r[26].xyz, r[21].wwww, r[25].xyzx
3445: movc r[25].xyz, r[29].yyyy, r[26].xyzx, r[25].xyzx
3454: if r[29].z
3457: ld_structured r[26].xy, r[20].x, l(68), t[7][7].xyxx
3467: ld_structured r[29].yzw, r[20].x, l(80), t[7][7].xxyz
3477: ld_structured r[30].xyz, r[20].x, l(96), t[7][7].xyzx
3487: ld_structured r[31].xyw, r[20].x, l(112), t[7][7].xyxz
3497: ld_structured r[32].xyzw, r[20].x, l(128), t[7][7].xyzw
3507: mov r[33].x, r[27].w
3512: mov r[33].y, r[29].y
3517: mov r[33].z, r[30].x
3522: mov r[33].w, r[31].x
3527: dp4 r[33].x, r[33].xyzw, r[10].xyzw
3534: mov r[34].x, r[26].x
3539: mov r[34].y, r[29].z
3544: mov r[34].z, r[30].y
3549: mov r[34].w, r[31].y
3554: dp4 r[33].y, r[34].xyzw, r[10].xyzw
3561: mov r[31].x, r[26].y
3566: mov r[31].y, r[29].w
3571: mov r[31].z, r[30].z
3576: dp4 r[33].z, r[31].xyzw, r[10].xyzw
3583: dp3 r[21].w, r[33].xyzx, r[33].xyzx
3590: rsq r[21].w, r[21].w
3595: mul r[26].xyz, r[21].wwww, r[33].xyzx
3602: lt r[30].xyzw, abs(r[26].yzxz), abs(r[26].xxyy)
3611: lt r[27].xyw, l(0,0,0,0), r[26].xyxz
3621: and r[29].yz, r[30].yywy, r[30].xxzx
3628: movc r[27].xyw, r[27].xyxw, l(0,2,0,4), l(1,3,0,5)
3643: movc r[21].w, r[29].z, r[27].y, r[27].w
3652: movc r[21].w, r[29].y, r[27].x, r[21].w
3661: if r[21].w
3664: ine r[22].w, r[21].w, l(1)
3671: if r[22].w
3674: div r[29].yzw, r[26].xxxy, r[26].yyzz
3681: div r[22].w, -r[26].z, r[26].y
3689: mad r[30].y, r[22].w, l(-0.5), l(0.5)
3698: div r[26].xw, r[26].xxxz, -r[26].yyyy
3706: mad r[26].xw, r[26].xxxw, l(0.5,0,0,-0.5), l(0.5,0,0,0.5)
3721: ieq r[31].xyz, r[21].wwww, l(2,3,4,0)
3731: mad r[33].xyz, r[29].yzwy, l(0.5,0.5,-0.5,0), l(0.5,0.5,0.5,0)
3746: div r[22].w, r[26].y, -r[26].z
3754: mad r[33].w, r[22].w, l(-0.5), l(0.5)
3763: movc r[26].yz, r[31].zzzz, r[33].yyzy, r[33].yywy
3772: movc r[26].xy, r[31].yyyy, r[26].xwxx, r[26].yzyy
3781: mov r[30].x, r[33].x
3786: movc r[27].xy, r[31].xxxx, r[30].xyxx, r[26].xyxx
3795: else 
3796: div r[29].yz, r[26].zzyz, -r[26].xxxx
3804: mad r[27].xy, r[29].yzyy, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
3819: endif 
3820: else 
3821: div r[22].w, -r[26].z, r[26].x
3829: mad r[27].x, r[22].w, l(0.5), l(0.5)
3838: div r[22].w, r[26].y, r[26].x
3845: mad r[27].y, r[22].w, l(-0.5), l(0.5)
3854: endif 
3855: add r[26].xy, r[27].xyxx, l(-0.5,-0.5,0,0)
3865: mad r[26].yz, r[26].xxyx, r[24].wwww, l(0,0.5,0.5,0)
3877: utof r[21].w, r[21].w
3882: mul r[21].w, r[21].w, l(0.166666672)
3889: mad r[26].x, r[26].y, l(0.166666672), r[21].w
3898: mad r[26].xy, r[26].xzxx, r[32].zwzz, r[32].xyxx
3907: sample_l r[26].xy, r[26].xyxx, t[6][6].xyzw, s[1][6], l(0)
3920: add r[21].w, r[29].x, l(-0.0500000007)
3927: mul r[22].w, r[27].z, r[21].w
3934: mad r[23].w, -r[26].x, r[26].x, r[26].y
3944: max r[23].w, r[23].w, l(9.99999975e-06)
3951: mad r[21].w, -r[21].w, r[27].z, r[26].x
3961: mad r[21].w, r[21].w, r[21].w, r[23].w
3970: div r[21].w, r[23].w, r[21].w
3977: min r[21].w, r[21].w, l(1)
3984: lt r[22].w, r[22].w, r[26].x
3991: movc r[21].w, r[22].w, l(1), r[21].w
4000: mul r[25].xyz, r[21].wwww, r[25].xyzx
4007: endif 
4008: mad r[24].xyz, r[24].xyzx, r[28].xxxx, r[16].yzwy
4017: dp3 r[21].w, r[24].xyzx, r[24].xyzx
4024: rsq r[21].w, r[21].w
4029: mul r[24].xyz, r[21].wwww, r[24].xyzx
4036: dp3 r[21].w, r[16].yzwy, r[24].xyzx
4043: dp3_sat r[22].w, r[17].xyzx, r[28].yzwy
4050: add r[21].w, -abs(r[21].w), l(1)
4058: max r[21].w, r[21].w, l(0)
4065: mul r[23].w, r[21].w, r[21].w
4072: mul r[23].w, r[23].w, r[23].w
4079: mul r[21].w, r[21].w, r[23].w
4086: mad r[21].w, r[17].w, r[21].w, r[8].w
4095: add_sat r[21].w, -r[21].w, l(1)
4103: mul r[21].w, r[22].w, r[21].w
4110: mul r[21].w, r[21].w, l(0.318309873)
4117: mad r[19].xyz, r[21].wwww, r[25].xyzx, r[19].xyzx
4126: iadd r[18].w, r[18].w, l(1)
4133: iadd r[13].w, r[13].y, r[18].w
4140: ld r[20].x, r[13].xwzz, t[19][24].xyzw
4148: endloop 
4149: ld r[10].x, r[13].xyzz, t[20][25].xyzw
4157: mul r[18].xyz, r[15].zzzz, r[17].xyzx
4164: mad r[9].xyz, r[18].xyzx, l(0.000300000014,0.000300000014,0.000300000014,0), r[15].xyzx
4176: mov r[18].xzw, r[13].xxzz
4181: mov r[24].xyz, r[19].xyzx
4186: mov r[10].y, l(0)
4191: mov r[10].z, r[10].x
4196: loop 
4197: ine r[13].x, r[10].z, l(9.18340949e-41)
4204: ilt r[13].w, r[10].y, l(256)
4211: and r[13].x, r[13].w, r[13].x
4218: breakc r[13].x
4221: iadd r[10].y, r[10].y, l(1)
4228: iadd r[18].y, r[10].y, r[13].y
4235: ld r[13].x, r[18].xyzw, t[20][25].xyzw
4243: ld_structured r[25].xyzw, r[10].z, l(68), t[11][11].xyzw
4253: ld_structured r[26].xyz, r[10].z, l(84), t[11][11].xyzx
4263: ld_structured r[27].xyzw, r[10].z, l(96), t[11][11].xyzw
4273: ld_structured r[28].xyzw, r[10].z, l(112), t[11][11].xyzw
4283: ld_structured r[29].xyzw, r[10].z, l(128), t[11][11].xyzw
4293: mov r[30].x, r[25].w
4298: mov r[30].y, r[27].x
4303: mov r[30].z, r[28].x
4308: mov r[30].w, r[29].x
4313: dp4 r[30].x, r[30].xyzw, r[9].xyzw
4320: mov r[31].x, r[26].x
4325: mov r[31].y, r[27].y
4330: mov r[31].z, r[28].y
4335: mov r[31].w, r[29].y
4340: dp4 r[30].y, r[31].xyzw, r[9].xyzw
4347: mov r[31].x, r[26].y
4352: mov r[31].y, r[27].z
4357: mov r[31].z, r[28].z
4362: mov r[31].w, r[29].z
4367: dp4 r[30].z, r[31].xyzw, r[9].xyzw
4374: mov r[29].x, r[26].z
4379: mov r[29].y, r[27].w
4384: mov r[29].z, r[28].w
4389: dp4 r[13].w, r[29].xyzw, r[9].xyzw
4396: rcp r[16].x, r[13].w
4401: mul r[26].xyz, r[16].xxxx, r[30].xyzx
4408: mad r[27].xyz, r[26].xyzx, l(0.5,0.5,1,0), l(0.5,0.5,0,0)
4423: mov_sat r[28].xyz, r[27].xyzx
4428: eq r[27].xyz, r[27].xyzx, r[28].xyzx
4435: and r[16].x, r[27].y, r[27].x
4442: and r[16].x, r[27].z, r[16].x
4449: if r[16].x
4452: ld_structured r[27].xyzw, r[10].z, l(0), t[11][11].xyzw
4462: ld_structured r[16].x, r[10].z, l(156), t[11][11].xxxx
4472: ld_structured r[18].y, r[10].z, l(200), t[11][11].xxxx
4482: ld_structured r[28].xyzw, r[10].z, l(288), t[11][11].xyzw
4492: ld_structured r[20].x, r[10].z, l(304), t[11][11].xxxx
4502: mad r[29].xy, r[26].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
4517: and r[29].zw, r[20].xxxx, l(0,0,2,768)
4527: if r[29].z
4530: ld_structured r[31].xyzw, r[10].z, l(272), t[11][11].xyzw
4540: mad r[30].xy, r[29].xyxx, r[31].zwzz, r[31].xyxx
4549: sample_l r[30].xy, r[30].xyxx, t[9][9].xyzw, s[1][6], l(0)
4562: add r[31].xyz, -r[15].xyzx, r[27].xyzx
4570: dp3 r[21].w, r[31].xyzx, r[31].xyzx
4577: sqrt r[21].w, r[21].w
4582: add r[21].w, -r[27].w, r[21].w
4590: mul r[22].w, r[16].x, r[21].w
4597: mad r[23].w, -r[30].x, r[30].x, r[30].y
4607: max r[23].w, r[23].w, l(9.99999975e-06)
4614: mad r[21].w, -r[21].w, r[16].x, r[30].x
4624: mad r[21].w, r[21].w, r[21].w, r[23].w
4633: div r[21].w, r[23].w, r[21].w
4640: min r[21].w, r[21].w, l(1)
4647: lt r[22].w, r[22].w, r[30].x
4654: movc r[21].w, r[22].w, l(1), r[21].w
4663: else 
4664: mov r[21].w, l(1)
4669: endif 
4670: ieq r[30].xy, r[29].wwww, l(256,512,0,0)
4680: if r[30].x
4683: mad r[30].xw, r[29].xxxy, r[28].zzzw, r[28].xxxy
4692: sample_l r[22].w, r[30].xwxx, t[8][8].yzwx, s[1][6], l(0)
4705: ge r[22].w, r[22].w, r[26].z
4712: and r[22].w, r[22].w, l(1)
4719: else 
4720: ieq r[23].w, r[29].w, l(512)
4727: if r[23].w
4730: mad r[26].zw, r[29].xxxy, r[28].zzzw, r[28].xxxy
4739: sample_l r[26].zw, r[26].zwzz, t[9][9].zwxy, s[1][6], l(0)
4752: add r[28].xyz, -r[15].xyzx, r[27].xyzx
4760: dp3 r[23].w, r[28].xyzx, r[28].xyzx
4767: sqrt r[23].w, r[23].w
4772: add r[23].w, -r[27].w, r[23].w
4780: mul r[24].w, r[16].x, r[23].w
4787: mad r[25].w, -r[26].z, r[26].z, r[26].w
4797: max r[25].w, r[25].w, l(9.99999975e-06)
4804: mad r[16].x, -r[23].w, r[16].x, r[26].z
4814: mad r[16].x, r[16].x, r[16].x, r[25].w
4823: div r[16].x, r[25].w, r[16].x
4830: min r[16].x, r[16].x, l(1)
4837: lt r[23].w, r[24].w, r[26].z
4844: movc r[22].w, r[23].w, l(1), r[16].x
4853: else 
4854: mov r[22].w, l(1)
4859: endif 
4860: endif 
4861: ine r[16].x, r[29].z, l(0)
4868: and r[16].x, r[16].x, r[30].y
4875: add r[23].w, -r[21].w, r[22].w
4883: mad r[23].w, r[18].y, r[23].w, r[21].w
4892: mad r[22].w, r[22].w, r[18].y, l(1)
4901: add r[18].y, -r[18].y, r[22].w
4909: min r[18].y, r[18].y, r[21].w
4916: movc r[16].x, r[16].x, r[23].w, r[18].y
4925: ne r[18].y, r[16].x, l(0)
4932: if r[18].y
4935: ld_structured r[28].xyz, r[10].z, l(16), t[11][11].xyzx
4945: ld_structured r[31].xyzw, r[10].z, l(48), t[11][11].xyzw
4955: add r[18].y, -r[30].z, r[31].w
4963: mul r[21].w, r[31].w, l(0.200000003)
4970: div_sat r[18].y, r[18].y, r[21].w
4977: mad r[21].w, r[18].y, l(-2), l(3)
4986: mul r[18].y, r[18].y, r[18].y
4993: mul r[18].y, r[18].y, r[21].w
5000: add r[28].xyz, -r[15].xyzx, r[28].xyzx
5008: dp3 r[21].w, r[28].xyzx, r[28].xyzx
5015: mul r[22].w, r[25].y, r[25].y
5022: mad r[21].w, r[25].z, r[21].w, r[22].w
5031: max r[21].w, r[21].w, l(0.25)
5038: mul r[21].w, r[21].w, l(3.14159274)
5045: rcp r[21].w, r[21].w
5050: add r[21].w, -r[25].x, r[21].w
5058: max r[21].w, r[21].w, l(0)
5065: mul r[18].y, r[18].y, r[21].w
5072: mul r[25].xyz, r[18].yyyy, r[31].xyzx
5079: and r[18].y, r[20].x, l(1)
5086: if r[18].y
5089: ld_structured r[28].xyzw, r[10].z, l(256), t[11][11].xyzw
5099: and r[18].y, r[20].x, l(1.43492963e-42)
5106: add r[26].zw, r[26].xxxy, r[26].xxxy
5113: mad r[20].x, r[26].x, r[26].x, l(1)
5122: mad r[20].x, r[26].y, r[26].y, r[20].x
5131: div r[26].xy, r[26].zwzz, r[20].xxxx
5138: mad r[26].xy, r[26].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
5153: movc r[26].xy, r[18].yyyy, r[26].xyxx, r[29].xyxx
5162: mad r[26].xy, r[26].xyxx, r[28].zwzz, r[28].xyxx
5171: sample_l r[26].xyz, r[26].xyxx, t[10][10].xyzw, s[3][11], l(0)
5184: mul r[25].xyz, r[25].xyzx, r[26].xyzx
5191: endif 
5192: ge r[13].w, r[13].w, r[30].z
5199: and r[13].w, r[13].w, l(1)
5206: mul r[13].w, r[13].w, r[16].x
5213: mul r[25].xyz, r[13].wwww, r[25].xyzx
5220: add r[26].xyz, -r[15].xyzx, r[27].xyzx
5228: dp3 r[13].w, r[26].xyzx, r[26].xyzx
5235: rsq r[13].w, r[13].w
5240: mul r[27].xyz, r[13].wwww, r[26].xyzx
5247: mad r[26].xyz, r[26].xyzx, r[13].wwww, r[16].yzwy
5256: dp3 r[13].w, r[26].xyzx, r[26].xyzx
5263: rsq r[13].w, r[13].w
5268: mul r[26].xyz, r[13].wwww, r[26].xyzx
5275: dp3 r[13].w, r[16].yzwy, r[26].xyzx
5282: dp3_sat r[16].x, r[17].xyzx, r[27].xyzx
5289: add r[13].w, -abs(r[13].w), l(1)
5297: max r[13].w, r[13].w, l(0)
5304: mul r[18].y, r[13].w, r[13].w
5311: mul r[18].y, r[18].y, r[18].y
5318: mul r[13].w, r[13].w, r[18].y
5325: mad r[13].w, r[17].w, r[13].w, r[8].w
5334: add_sat r[13].w, -r[13].w, l(1)
5342: mul r[13].w, r[16].x, r[13].w
5349: mul r[13].w, r[13].w, l(0.318309873)
5356: mad r[24].xyz, r[13].wwww, r[25].xyzx, r[24].xyzx
5365: endif 
5366: endif 
5367: mov r[10].z, r[13].x
5372: endloop 
5373: mad r[9].xyz, r[12].xyzx, l(0.318309873,0.318309873,0.318309873,0), r[24].xyzx
5385: mul r[20].yzw, r[22].xxyz, r[9].xxyz
5392: else 
5393: mov r[20].yzw, l(0,0,0,0)
5401: endif 
5402: endif 
5403: mad r[4].xyz, r[3].xyzx, r[20].yzwy, r[4].xyzx
5412: iadd r[0].z, r[0].z, l(1)
5419: endloop 
5420: add r[0].xy, -cb[1][1][17].yzyy, cb[1][1][17].xwxx
5432: add r[0].z, cb[1][1][17].y, cb[1][1][17].x
5443: add r[0].z, -r[0].x, r[0].z
5451: add r[0].x, -r[0].x, r[1].w
5459: div r[0].z, l(1,1,1,1), r[0].z
5469: mul_sat r[0].x, r[0].z, r[0].x
5476: mad r[0].z, r[0].x, l(-2), l(3)
5485: mul r[0].x, r[0].x, r[0].x
5492: mul r[0].x, r[0].x, r[0].z
5499: mad r[0].x, r[0].x, r[0].y, cb[1][1][17].z
5510: mul r[0].xyz, r[0].xxxx, r[4].xyzx
5517: itof r[0].w, cb[4][4][0].z
5524: div r[1].xyz, r[0].xyzx, r[0].wwww
5531: div r[0].x, r[7].w, r[0].w
5538: mov r[1].w, l(1)
5543: else 
5544: mov r[1].xyzw, l(0,0,0,0)
5552: mov r[0].x, l(65504)
5557: endif 
5558: store_uav_typed type30[2][2].xyzw, type32.xyyy, r[0].xxxx
5565: store_uav_typed type30[0][0].xyzw, type32.xyyy, r[1].xyzw
5572: store_uav_typed type30[1][1].xyzw, type32.xyyy, l(1,1,1,1)
5582: ret 