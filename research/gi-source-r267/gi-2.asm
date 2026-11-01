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
216: dcl_resource_structured raw:0x307e46,0x16,0x1b,0x1b,0x4,0x0
223: dcl_resource_structured raw:0x307e46,0x17,0x1c,0x1c,0x4,0x0
230: dcl_resource raw:0x307e46,0x18,0x0,0xffffffff,0x5555,0x1
237: dcl_unordered_access_view_typed raw:0x31ee46,0x0,0x0,0x0,0x5555,0x0
244: dcl_unordered_access_view_typed raw:0x31ee46,0x1,0x1,0x1,0x5555,0x0
251: dcl_unordered_access_view_typed raw:0x31ee46,0x2,0x2,0x2,0x5555,0x0
258: dcl_input raw:0x24000
260: dcl_input raw:0x21032
262: dcl_temps raw:0x1e
264: dcl_thread_group raw:0x8,0x10,0x1
268: ld_structured r[0].x, l(0), l(0), t[22][27].xxxx
278: utof r[0].y, type36.x
282: utof r[0].zw, type33.xxxy
286: mul r[0].w, r[0].w, cb[0][0][0].x
295: mad r[0].z, r[0].w, l(0.125), r[0].z
304: mad r[0].y, r[0].z, l(128), r[0].y
313: ftou r[0].y, r[0].y
318: uge r[0].x, r[0].y, r[0].x
325: if r[0].x
328: ret 
329: endif 
330: ld_structured r[0].x, r[0].y, l(0), t[23][28].xxxx
340: and r[1].x, r[0].x, l(9.18340949e-41)
347: ushr r[1].y, r[0].x, l(16)
354: mov r[1].w, l(0)
359: ld r[0].x, r[1].xyww, t[19][24].xyzw
367: ine r[0].x, r[0].x, l(9.18326936e-41)
374: if r[0].x
377: utof r[0].xy, r[1].xyxx
382: add r[0].xy, r[0].xyxx, l(0.5,0.5,0,0)
392: mul r[0].xy, r[0].xyxx, cb[0][0][1].zwzz
401: ld r[0].z, r[1].xyww, t[3][3].yzxw
409: ld r[0].w, r[1].xyww, t[2][2].yzwx
417: mad r[0].xy, r[0].xyxx, l(2,-2,0,0), l(-1,1,0,0)
432: mul r[2].xyzw, r[0].yyyy, cb[0][0][15].xyzw
441: mad r[2].xyzw, cb[0][0][14].xyzw, r[0].xxxx, r[2].xyzw
452: mad r[2].xyzw, cb[0][0][16].xyzw, r[0].zzzz, r[2].xyzw
463: add r[2].xyzw, r[2].xyzw, cb[0][0][17].xyzw
472: rcp r[2].w, r[2].w
477: mul r[3].xyz, r[2].wwww, r[2].xyzx
484: dp3 r[3].w, r[3].xyzx, r[3].xyzx
491: rsq r[3].w, r[3].w
496: mul r[3].xyz, r[3].wwww, r[3].xyzx
503: mul r[4].xyzw, r[0].yyyy, cb[0][0][23].xyzw
512: mad r[4].xyzw, cb[0][0][22].xyzw, r[0].xxxx, r[4].xyzw
523: mad r[4].xyzw, cb[0][0][24].xyzw, r[0].zzzz, r[4].xyzw
534: add r[4].xyzw, r[4].xyzw, cb[0][0][25].xyzw
543: rcp r[0].x, r[4].w
548: mul r[5].xyz, r[0].xxxx, r[4].xyzx
555: mad r[0].xyz, r[4].xyzx, r[0].xxxx, -cb[0][0][54].xyzx
567: dp3 r[3].w, r[0].xyzx, r[0].xyzx
574: rsq r[3].w, r[3].w
579: mul r[0].xyz, r[0].xyzx, r[3].wwww
586: ld r[4].xyzw, r[1].xyww, t[0][0].xzyw
594: mad r[3].w, r[4].w, l(255), l(0.5)
603: ftou r[3].w, r[3].w
608: and r[3].w, r[3].w, l(254)
615: utof r[6].z, r[3].w
620: mov r[6].xy, r[4].xzxx
625: mad r[6].xyz, r[6].xyzx, l(255,255,1,0), l(0.5,0.5,0.5,0)
640: round_ni r[6].xyz, r[6].xyzx
645: ftoi r[6].xyz, r[6].xyzx
650: ubfe r[7].xyzw, l(3,4,3,4), l(1,4,1,4), r[6].zzzz
665: bfi r[6].xyzw, l(29,28,29,28), l(3,4,3,4), r[6].xyxy, r[7].xyzw
682: itof r[6].xyzw, r[6].xyzw
687: mad r[6].xyzw, r[6].xyzw, l(0.00097703957,0.000488400517,0.00097703957,0.000488400517), l(-1,-1,-1,-1)
702: mul r[6].xyzw, r[6].xyzw, l(1.29999995,1.29999995,2.5999999,2.5999999)
712: dp2 r[3].w, r[6].xyxx, r[6].xyxx
719: add r[6].xy, r[3].wwww, l(1,-1,0,0)
729: div r[6].xyz, r[6].zwyz, r[6].xxxx
736: mul r[7].xyz, r[6].yyyy, cb[0][0][7].xyzx
745: mad r[7].xyz, cb[0][0][6].xyzx, r[6].xxxx, r[7].xyzx
756: mad r[7].xyz, cb[0][0][8].xyzx, r[6].zzzz, r[7].xyzx
767: ld r[8].xyz, r[1].xyww, t[1][1].yzwx
775: mul r[4].zw, r[8].yyyz, l(0,0,255,255)
785: ftou r[4].zw, r[4].zzzw
790: ishl r[3].w, r[4].z, l(8)
797: or r[3].w, r[4].w, r[3].w
804: dp3 r[4].x, r[6].xyzx, -r[3].xyzx
812: ld_structured r[3].x, r[3].w, l(0), t[4][4].xxxx
822: ushr r[3].y, r[3].x, l(24)
829: ubfe r[3].xz, l(8,0,8,0), l(16,0,8,0), r[3].xxxx
844: utof r[6].xyz, r[3].yxzy
849: mul r[3].xyz, r[6].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
859: ld_structured r[3].w, r[3].w, l(0), t[4][4].xxxx
869: and r[3].w, r[3].w, l(255)
876: dp3 r[4].z, r[3].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
886: add r[4].w, r[4].z, l(9.99999997e-07)
893: rcp r[4].w, r[4].w
898: mul r[4].z, r[4].z, l(10000)
905: min r[4].z, r[4].z, l(1)
912: mad r[6].xyz, r[3].xyzx, r[4].wwww, l(-1,-1,-1,0)
924: mad r[6].xyz, r[4].zzzz, r[6].xyzx, l(1,1,1,0)
936: mul r[6].xyz, r[8].xxxx, r[6].xyzx
943: movc r[3].xyz, r[3].wwww, r[3].xyzx, r[6].xyzx
952: mul r[0].xyz, r[0].wwww, r[0].xyzx
959: mad r[0].xyz, -r[0].xyzx, l(0.00100000005,0.00100000005,0.00100000005,0), r[5].xyzx
972: mul r[6].xyz, r[0].wwww, r[7].xyzx
979: mad r[0].xyz, r[6].xyzx, l(0.00100000005,0.00100000005,0.00100000005,0), r[0].xyzx
991: sample_l r[4].xyz, r[4].xyxx, t[12][12].xyzw, s[1][6], l(0)
1004: mul r[3].w, cb[1][1][15].w, l(0.318309873)
1013: mad r[3].xyz, r[3].xyzx, r[4].xxxx, r[4].yyyy
1022: add r[3].xyz, -r[3].xyzx, l(1,1,1,0)
1033: max r[3].xyz, r[3].xyzx, l(0,0,0,0)
1043: mul r[3].xyz, r[4].zzzz, r[3].xyzx
1050: mov r[4].w, l(1)
1055: mov r[6].z, l(0)
1060: mov r[8].w, l(1)
1065: mov r[9].w, l(1)
1070: mov r[10].xyz, l(0,0,0,0)
1078: mov r[5].w, l(0)
1083: mov r[1].z, l(0)
1088: loop 
1089: ige r[7].w, r[1].z, cb[4][4][0].z
1098: breakc r[7].w
1101: ld r[7].w, r[1].xyzw, t[19][24].yzwx
1109: ld r[11].xyzw, r[1].xyzw, t[21][26].xyzw
1117: mul r[12].xyz, r[11].yyyy, cb[0][0][7].xyzx
1126: mad r[12].xyz, cb[0][0][6].xyzx, r[11].xxxx, r[12].xyzx
1137: mad r[12].xyz, cb[0][0][8].xyzx, r[11].zzzz, r[12].xyzx
1148: add r[12].xyz, r[12].xyzx, cb[0][0][9].xyzx
1157: ld r[13].xyzw, r[1].xyzw, t[20][25].xyzw
1165: add r[14].xyz, -r[0].xyzx, r[12].xyzx
1173: dp3 r[10].w, r[14].xyzx, r[14].xyzx
1180: rsq r[10].w, r[10].w
1185: mul r[14].xyz, r[10].wwww, r[14].xyzx
1192: ieq r[10].w, r[7].w, l(9.18340949e-41)
1199: mad r[15].xyz, cb[4][4][0].yyyy, r[14].xyzx, r[5].xyzx
1210: movc r[4].xyz, r[10].wwww, r[15].xyzx, r[12].xyzx
1219: movc r[15].xyz, r[10].wwww, r[7].xyzx, r[13].xyzx
1228: mov r[16].xyz, l(0,0,0,0)
1236: mov r[12].w, l(0)
1241: mov r[14].w, l(0)
1246: loop 
1247: uge r[15].w, r[14].w, cb[2][2][0].w
1256: breakc r[15].w
1259: ishl r[15].w, r[14].w, l(2)
1266: dp4 r[17].x, cb[3][3][r[15].w].xyzw, r[4].xyzw
1276: dp4 r[17].y, cb[3][3][1+r[15].w].xyzw, r[4].xyzw
1287: dp4 r[17].z, cb[3][3][2+r[15].w].xyzw, r[4].xyzw
1298: ge r[18].xyz, r[17].xyzx, l(0,0,0,0)
1308: and r[16].w, r[18].y, r[18].x
1315: and r[16].w, r[18].z, r[16].w
1322: utof r[18].xyz, cb[3][3][3+r[15].w].xyzx
1331: lt r[19].xyz, r[17].xyzx, r[18].xyzx
1338: and r[17].w, r[19].y, r[19].x
1345: and r[17].w, r[19].z, r[17].w
1352: and r[16].w, r[16].w, r[17].w
1359: if r[16].w
1362: ld_structured r[19].xyz, r[14].w, l(0), t[13][13].xyzx
1372: ld_structured r[20].xyz, r[14].w, l(12), t[13][13].xyzx
1382: ld_structured r[21].xyzw, r[14].w, l(24), t[13][13].xyzw
1392: ld_structured r[22].xyzw, r[14].w, l(40), t[13][13].xyzw
1402: ld_structured r[23].xyz, r[14].w, l(56), t[13][13].xyzx
1412: mul_sat r[16].w, r[17].x, r[21].w
1419: add r[18].xyz, -r[17].xyzx, r[18].xyzx
1427: mul_sat r[17].w, r[21].w, r[18].x
1434: mul r[16].w, r[16].w, r[17].w
1441: mul_sat r[18].xw, r[17].yyyz, r[22].xxxy
1448: mul r[16].w, r[16].w, r[18].x
1455: mul_sat r[18].xy, r[22].xyxx, r[18].yzyy
1462: mul r[16].w, r[16].w, r[18].x
1469: mul r[16].w, r[18].w, r[16].w
1476: mul r[16].w, r[18].y, r[16].w
1483: ine r[17].w, r[23].x, l(nan)
1490: if r[17].w
1493: round_ni r[18].xyz, r[17].xyzx
1498: add r[17].xyz, r[17].xyzx, -r[18].xyzx
1506: ftou r[18].xyz, r[18].xyzx
1511: imad r[17].w, r[18].z, cb[3][3][3+r[15].w].y, r[18].y
1524: imad r[15].w, r[17].w, cb[3][3][3+r[15].w].x, r[18].x
1537: iadd r[15].w, r[22].z, r[15].w
1544: ld_structured r[15].w, r[15].w, l(0), t[14][14].xxxx
1554: mov r[18].xyz, l(0,0,0,0)
1562: mov r[22].xyz, r[17].xyzx
1567: mov r[17].w, r[15].w
1572: mov r[18].w, r[23].y
1577: loop 
1578: and r[19].w, r[17].w, l(-0)
1585: ine r[19].w, r[19].w, l(0)
1592: ge r[20].w, l(9.99999975e-06), r[18].w
1599: or r[19].w, r[19].w, r[20].w
1606: if r[19].w
1609: and r[19].w, r[17].w, l(nan)
1616: iadd r[19].w, r[23].x, r[19].w
1623: and r[24].x, r[19].w, l(127)
1630: ubfe r[24].y, l(7), l(7), r[19].w
1639: ushr r[24].z, r[19].w, l(14)
1646: ishl r[24].xyz, r[24].xyzx, l(2,2,2,0)
1656: utof r[24].xyz, r[24].xyzx
1661: mad r[24].xyz, r[22].xyzx, l(3,3,3,0), r[24].xyzx
1673: add r[18].xyz, r[24].xyzx, l(0.5,0.5,0.5,0)
1683: break 
1684: endif 
1685: iadd r[19].w, r[22].w, r[17].w
1692: ld_structured r[24].xyzw, r[19].w, l(0), t[15][15].xyzw
1702: mul r[25].xyz, r[22].xyzx, l(3,3,3,0)
1712: round_ni r[25].xyz, r[25].xyzx
1717: ftou r[26].xyz, r[25].xyzx
1722: utof r[26].xz, r[26].zzxz
1727: imul null, r[19].w, r[26].y, l(3)
1735: utof r[19].w, r[19].w
1740: mad r[19].w, r[26].x, l(9), r[19].w
1749: add r[19].w, r[26].z, r[19].w
1756: ftou r[19].w, r[19].w
1761: ishl r[20].w, l(1), r[19].w
1768: and r[21].w, r[20].w, r[24].x
1775: if r[21].w
1778: iadd r[21].w, r[23].x, r[17].w
1785: and r[26].x, r[21].w, l(127)
1792: ubfe r[26].y, l(7), l(7), r[21].w
1801: ushr r[26].z, r[21].w, l(14)
1808: ishl r[26].xyz, r[26].xyzx, l(2,2,2,0)
1818: utof r[26].xyz, r[26].xyzx
1823: mad r[26].xyz, r[22].xyzx, l(3,3,3,0), r[26].xyzx
1835: add r[18].xyz, r[26].xyzx, l(0.5,0.5,0.5,0)
1845: break 
1846: endif 
1847: mad r[22].xyz, r[22].xyzx, l(3,3,3,0), -r[25].xyzx
1860: and r[20].w, r[20].w, r[24].y
1867: ubfe r[21].w, r[19].w, l(0), r[24].y
1876: countbits r[21].w, r[21].w
1881: iadd r[21].w, r[21].w, r[24].w
1888: or r[21].w, r[21].w, l(-0)
1895: not r[23].w, r[24].y
1900: and r[23].w, r[23].w, r[24].x
1907: ubfe r[19].w, r[19].w, l(0), r[23].w
1916: countbits r[19].w, r[19].w
1921: iadd r[19].w, r[19].w, r[24].z
1928: movc r[17].w, r[20].w, r[21].w, r[19].w
1937: mul r[18].w, r[18].w, l(0.333333343)
1944: mov r[18].xyz, l(0,0,0,0)
1952: endloop 
1953: else 
1954: mov r[18].xyz, l(0,0,0,0)
1962: endif 
1963: mul r[17].xyz, r[18].xyzx, cb[2][2][0].xyzx
1972: mov r[22].x, r[19].x
1977: mov r[22].y, r[20].x
1982: mov r[22].z, r[21].x
1987: dp3 r[22].x, r[22].xyzx, r[15].xyzx
1994: mov r[24].x, r[19].y
1999: mov r[24].y, r[20].y
2004: mov r[24].z, r[21].y
2009: dp3 r[22].y, r[24].xyzx, r[15].xyzx
2016: mov r[21].x, r[19].z
2021: mov r[21].y, r[20].z
2026: dp3 r[22].z, r[21].xyzx, r[15].xyzx
2033: mov_sat r[19].xyz, r[22].xyzx
2038: mul r[19].xyz, r[19].xyzx, r[19].xyzx
2045: mov_sat r[22].xyz, -r[22].xyzx
2051: mul r[20].xyz, r[22].xyzx, r[22].xyzx
2058: lt r[21].xyz, l(0,0,0,0), r[19].xyzx
2068: if r[21].x
2071: sample_l r[22].xyz, r[17].xyzx, t[16][16].xyzw, s[1][6], l(0)
2084: mul r[22].xyz, r[19].xxxx, r[22].xyzx
2091: else 
2092: mov r[22].xyz, l(0,0,0,0)
2100: endif 
2101: lt r[24].xyz, l(0,0,0,0), r[20].xyzx
2111: if r[24].x
2114: sample_l r[25].xyz, r[17].xyzx, t[16][17].xyzw, s[1][6], l(0)
2127: mad r[22].xyz, r[25].xyzx, r[20].xxxx, r[22].xyzx
2136: endif 
2137: if r[21].y
2140: sample_l r[21].xyw, r[17].xyzx, t[16][18].xywz, s[1][6], l(0)
2153: mad r[22].xyz, r[21].xywx, r[19].yyyy, r[22].xyzx
2162: endif 
2163: if r[24].y
2166: sample_l r[19].xyw, r[17].xyzx, t[16][19].xywz, s[1][6], l(0)
2179: mad r[22].xyz, r[19].xywx, r[20].yyyy, r[22].xyzx
2188: endif 
2189: if r[21].z
2192: sample_l r[19].xyw, r[17].xyzx, t[16][20].xywz, s[1][6], l(0)
2205: mad r[22].xyz, r[19].xywx, r[19].zzzz, r[22].xyzx
2214: endif 
2215: if r[24].z
2218: sample_l r[17].xyz, r[17].xyzx, t[16][21].xyzw, s[1][6], l(0)
2231: mad r[22].xyz, r[17].xyzx, r[20].zzzz, r[22].xyzx
2240: endif 
2241: ieq r[15].w, r[23].x, l(nan)
2248: movc r[17].xyz, r[15].wwww, l(1,0.349999994,0,0), r[22].xyzx
2260: mad r[15].w, r[16].w, l(-2), l(3)
2269: mul r[17].w, r[16].w, r[16].w
2276: mul r[15].w, r[15].w, r[17].w
2283: add r[17].w, r[23].z, l(-1)
2290: mad r[15].w, r[15].w, r[17].w, l(1)
2299: mul r[17].w, r[15].w, r[16].w
2306: mad r[16].xyz, r[17].xyzx, r[17].wwww, r[16].xyzx
2315: mad r[12].w, r[16].w, r[15].w, r[12].w
2324: endif 
2325: iadd r[14].w, r[14].w, l(1)
2332: endloop 
2333: lt r[4].x, l(0), r[12].w
2340: div r[15].xyz, r[16].xyzx, r[12].wwww
2347: movc r[4].xyz, r[4].xxxx, r[15].xyzx, r[16].xyzx
2356: add r[4].xyz, r[4].xyzx, cb[2][2][1].xyzx
2365: mad r[15].xyz, r[2].xyzx, r[2].wwww, -r[11].xyzx
2375: dp3 r[14].w, r[15].xyzx, r[15].xyzx
2382: sqrt r[14].w, r[14].w
2387: movc r[14].w, r[10].w, cb[4][4][0].x, r[14].w
2398: add r[5].w, r[5].w, r[14].w
2405: if r[10].w
2408: mul r[15].xyz, r[3].wwww, r[4].xyzx
2415: else 
2416: ld_structured r[17].xyz, r[7].w, l(52), t[5][5].xyzx
2426: imul null, r[10].w, r[7].w, l(35)
2434: mov r[19].x, r[13].w
2439: mov r[19].y, r[11].w
2444: sample_l r[20].xyz, r[19].xyxx, t[24][r[10].w].xyzw, s[0][5], l(0)
2459: mul r[17].xyz, r[17].xyzx, r[20].xyzx
2466: dp3 r[10].w, r[17].xyzx, r[17].xyzx
2473: ge r[10].w, r[10].w, l(9.99999975e-05)
2480: if r[10].w
2483: mul_sat r[10].w, r[14].w, l(50)
2490: mul r[4].xyz, r[4].xyzx, r[10].wwww
2497: ld_structured r[19].zw, r[7].w, l(68), t[5][5].xxxy
2507: imad r[20].xy, r[7].wwww, l(35,35,0,0), l(2,4,0,0)
2522: sample_l r[10].w, r[19].xyxx, t[24][r[20].x].yzwx, s[3][19], l(0)
2537: add r[11].w, -r[19].z, r[19].w
2545: mad r[21].y, r[10].w, r[11].w, r[19].z
2554: sample_l r[10].w, r[19].xyxx, t[24][r[20].y].yzwx, s[3][19], l(0)
2569: ld_structured r[7].w, r[7].w, l(0), t[4][4].xxxx
2579: ushr r[11].w, r[7].w, l(24)
2586: ubfe r[19].xy, l(8,8,0,0), l(16,8,0,0), r[7].wwww
2601: utof r[20].x, r[11].w
2606: utof r[20].yz, r[19].xxyx
2611: mul r[19].xyz, r[10].wwww, r[20].xyzx
2618: mul r[19].xyz, r[19].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
2628: dp3 r[7].w, r[19].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
2638: dp3 r[21].x, r[13].xyzx, -r[14].xyzx
2646: sample_l r[14].xyz, r[21].xyxx, t[12][12].xyzw, s[1][6], l(0)
2659: mad r[14].xyw, r[19].xyxz, r[14].xxxx, r[14].yyyy
2668: add r[14].xyw, -r[14].xyxw, l(1,1,0,1)
2679: mul r[14].xyw, r[14].xyxw, cb[1][1][15].wwww
2688: mul r[14].xyz, r[14].zzzz, r[14].xywx
2695: mul r[4].xyz, r[4].xyzx, r[14].xyzx
2702: mul r[14].xyz, r[12].yyyy, cb[0][0][3].xyzx
2711: mad r[14].xyz, cb[0][0][2].xyzx, r[12].xxxx, r[14].xyzx
2722: mad r[12].xyz, cb[0][0][4].xyzx, r[12].zzzz, r[14].xyzx
2733: add r[12].xyz, r[12].xyzx, cb[0][0][5].xyzx
2742: mul r[14].xyz, r[13].yyyy, cb[0][0][3].xyzx
2751: mad r[13].xyw, cb[0][0][2].xyxz, r[13].xxxx, r[14].xyxz
2762: mad r[13].xyz, cb[0][0][4].xyzx, r[13].zzzz, r[13].xywx
2773: add r[11].xyz, r[11].xyzx, l(150,75,75,0)
2783: mul r[11].xyz, r[11].xyzx, l(0.0533333346,0.0533333346,0.0533333346,0)
2793: round_ni r[11].xyz, r[11].xyzx
2798: max r[11].xyz, r[11].xyzx, l(0,0,0,0)
2808: min r[11].xyz, r[11].xyzx, l(15,7,11,0)
2818: ftoi r[11].xyz, r[11].xyzx
2823: imad r[6].x, r[11].x, l(12), r[11].z
2832: ishl r[6].y, r[11].y, l(8)
2839: ld r[10].w, r[6].xyzz, t[17][22].yzwx
2847: dp3 r[11].x, -r[12].xyzx, -r[12].xyzx
2856: mad r[9].xyz, r[13].xyzx, l(0.0700000003,0.0700000003,0.0700000003,0), r[12].xyzx
2868: rsq r[11].x, r[11].x
2873: mul r[11].xyz, r[11].xxxx, -r[12].xyzx
2881: add r[11].w, -r[7].w, l(1)
2889: mov r[14].xyz, l(0,0,0,0)
2897: mov r[13].w, l(0)
2902: mov r[14].w, r[10].w
2907: loop 
2908: ine r[15].w, r[14].w, l(9.18340949e-41)
2915: ilt r[16].w, r[13].w, l(256)
2922: and r[15].w, r[15].w, r[16].w
2929: breakc r[15].w
2932: ld_structured r[19].xyzw, r[14].w, l(0), t[7][7].xyzw
2942: ld_structured r[20].xyzw, r[14].w, l(16), t[7][7].xyzw
2952: ld_structured r[21].xyzw, r[14].w, l(36), t[7][7].xyzw
2962: ld_structured r[22].xyzw, r[14].w, l(52), t[7][7].xyzw
2972: ld_structured r[15].w, r[14].w, l(144), t[7][7].xxxx
2982: ld_structured r[16].w, r[14].w, l(156), t[7][7].xxxx
2992: add r[19].xyz, -r[12].xyzx, r[19].xyzx
3000: dp3 r[17].w, r[19].xyzx, r[19].xyzx
3007: rsq r[18].w, r[17].w
3012: mul r[23].xyz, r[18].wwww, r[19].xyzx
3019: sqrt r[23].w, r[17].w
3024: add r[24].x, r[20].w, -r[23].w
3032: mul r[20].w, r[20].w, l(0.200000003)
3039: div_sat r[20].w, r[24].x, r[20].w
3046: mad r[24].x, r[20].w, l(-2), l(3)
3055: mul r[20].w, r[20].w, r[20].w
3062: mul r[20].w, r[20].w, r[24].x
3069: mad r[15].w, r[15].w, r[15].w, r[17].w
3078: max r[15].w, r[15].w, l(9.99999975e-05)
3085: mul r[15].w, r[15].w, l(3.14159274)
3092: rcp r[15].w, r[15].w
3097: add r[15].w, -r[21].x, r[15].w
3105: max r[15].w, r[15].w, l(0)
3112: mul r[15].w, r[20].w, r[15].w
3119: mul r[20].xyz, r[15].wwww, r[20].xyzx
3126: and r[24].xy, r[16].wwww, l(4,2,0,0)
3136: mov r[25].x, r[21].w
3141: mov r[25].yz, r[22].xxyx
3146: dp3 r[15].w, -r[23].xyzx, r[25].xyzx
3154: mad_sat r[15].w, r[15].w, r[21].y, r[21].z
3163: mul r[15].w, r[15].w, r[15].w
3170: mul r[21].xyz, r[15].wwww, r[20].xyzx
3177: movc r[20].xyz, r[24].xxxx, r[21].xyzx, r[20].xyzx
3186: if r[24].y
3189: ld_structured r[21].xy, r[14].w, l(68), t[7][7].xyxx
3199: ld_structured r[24].xyz, r[14].w, l(80), t[7][7].xyzx
3209: ld_structured r[25].xyz, r[14].w, l(96), t[7][7].xyzx
3219: ld_structured r[26].xyw, r[14].w, l(112), t[7][7].xyxz
3229: ld_structured r[27].xyzw, r[14].w, l(128), t[7][7].xyzw
3239: mov r[28].x, r[22].w
3244: mov r[28].y, r[24].x
3249: mov r[28].z, r[25].x
3254: mov r[28].w, r[26].x
3259: dp4 r[28].x, r[28].xyzw, r[9].xyzw
3266: mov r[29].x, r[21].x
3271: mov r[29].y, r[24].y
3276: mov r[29].z, r[25].y
3281: mov r[29].w, r[26].y
3286: dp4 r[28].y, r[29].xyzw, r[9].xyzw
3293: mov r[26].x, r[21].y
3298: mov r[26].y, r[24].z
3303: mov r[26].z, r[25].z
3308: dp4 r[28].z, r[26].xyzw, r[9].xyzw
3315: dp3 r[15].w, r[28].xyzx, r[28].xyzx
3322: rsq r[15].w, r[15].w
3327: mul r[21].xyz, r[15].wwww, r[28].xyzx
3334: lt r[24].xyzw, abs(r[21].yzxz), abs(r[21].xxyy)
3343: lt r[22].xyw, l(0,0,0,0), r[21].xyxz
3353: and r[24].xy, r[24].ywyy, r[24].xzxx
3360: movc r[22].xyw, r[22].xyxw, l(0,2,0,4), l(1,3,0,5)
3375: movc r[15].w, r[24].y, r[22].y, r[22].w
3384: movc r[15].w, r[24].x, r[22].x, r[15].w
3393: if r[15].w
3396: ine r[16].w, r[15].w, l(1)
3403: if r[16].w
3406: div r[24].xyz, r[21].xxyx, r[21].yzzy
3413: div r[16].w, -r[21].z, r[21].y
3421: mad r[25].y, r[16].w, l(-0.5), l(0.5)
3430: div r[21].xw, r[21].xxxz, -r[21].yyyy
3438: mad r[21].xw, r[21].xxxw, l(0.5,0,0,-0.5), l(0.5,0,0,0.5)
3453: ieq r[26].xyz, r[15].wwww, l(2,3,4,0)
3463: mad r[24].xyz, r[24].xyzx, l(0.5,0.5,-0.5,0), l(0.5,0.5,0.5,0)
3478: div r[16].w, r[21].y, -r[21].z
3486: mad r[24].w, r[16].w, l(-0.5), l(0.5)
3495: movc r[21].yz, r[26].zzzz, r[24].yyzy, r[24].yywy
3504: movc r[21].xy, r[26].yyyy, r[21].xwxx, r[21].yzyy
3513: mov r[25].x, r[24].x
3518: movc r[22].xy, r[26].xxxx, r[25].xyxx, r[21].xyxx
3527: else 
3528: div r[24].xy, r[21].zyzz, -r[21].xxxx
3536: mad r[22].xy, r[24].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
3551: endif 
3552: else 
3553: div r[16].w, -r[21].z, r[21].x
3561: mad r[22].x, r[16].w, l(0.5), l(0.5)
3570: div r[16].w, r[21].y, r[21].x
3577: mad r[22].y, r[16].w, l(-0.5), l(0.5)
3586: endif 
3587: add r[21].xy, r[22].xyxx, l(-0.5,-0.5,0,0)
3597: mad r[21].yz, r[21].xxyx, r[19].wwww, l(0,0.5,0.5,0)
3609: utof r[15].w, r[15].w
3614: mul r[15].w, r[15].w, l(0.166666672)
3621: mad r[21].x, r[21].y, l(0.166666672), r[15].w
3630: mad r[21].xy, r[21].xzxx, r[27].zwzz, r[27].xyxx
3639: sample_l r[21].xy, r[21].xyxx, t[6][6].xyzw, s[1][6], l(0)
3652: add r[15].w, r[23].w, l(-0.0500000007)
3659: mul r[16].w, r[22].z, r[15].w
3666: mad r[17].w, -r[21].x, r[21].x, r[21].y
3676: max r[17].w, r[17].w, l(9.99999975e-06)
3683: mad r[15].w, -r[15].w, r[22].z, r[21].x
3693: mad r[15].w, r[15].w, r[15].w, r[17].w
3702: div r[15].w, r[17].w, r[15].w
3709: min r[15].w, r[15].w, l(1)
3716: lt r[16].w, r[16].w, r[21].x
3723: movc r[15].w, r[16].w, l(1), r[15].w
3732: mul r[20].xyz, r[15].wwww, r[20].xyzx
3739: endif 
3740: mad r[19].xyz, r[19].xyzx, r[18].wwww, r[11].xyzx
3749: dp3 r[15].w, r[19].xyzx, r[19].xyzx
3756: rsq r[15].w, r[15].w
3761: mul r[19].xyz, r[15].wwww, r[19].xyzx
3768: dp3 r[15].w, r[11].xyzx, r[19].xyzx
3775: dp3_sat r[16].w, r[13].xyzx, r[23].xyzx
3782: add r[15].w, -abs(r[15].w), l(1)
3790: max r[15].w, r[15].w, l(0)
3797: mul r[17].w, r[15].w, r[15].w
3804: mul r[17].w, r[17].w, r[17].w
3811: mul r[15].w, r[15].w, r[17].w
3818: mad r[15].w, r[11].w, r[15].w, r[7].w
3827: add_sat r[15].w, -r[15].w, l(1)
3835: mul r[15].w, r[16].w, r[15].w
3842: mul r[15].w, r[15].w, l(0.318309873)
3849: mad r[14].xyz, r[15].wwww, r[20].xyzx, r[14].xyzx
3858: iadd r[13].w, r[13].w, l(1)
3865: iadd r[6].w, r[6].y, r[13].w
3872: ld r[14].w, r[6].xwzz, t[17][22].yzwx
3880: endloop 
3881: ld r[6].w, r[6].xyzz, t[18][23].yzwx
3889: mul r[9].xyz, r[12].zzzz, r[13].xyzx
3896: mad r[8].xyz, r[9].xyzx, l(0.000300000014,0.000300000014,0.000300000014,0), r[12].xyzx
3908: mov r[19].xzw, r[6].xxzz
3913: mov r[9].xyz, r[14].xyzx
3918: mov r[6].x, l(0)
3923: mov r[10].w, r[6].w
3928: loop 
3929: ine r[13].w, r[10].w, l(9.18340949e-41)
3936: ilt r[14].w, r[6].x, l(256)
3943: and r[13].w, r[13].w, r[14].w
3950: breakc r[13].w
3953: iadd r[6].x, r[6].x, l(1)
3960: iadd r[19].y, r[6].x, r[6].y
3967: ld r[13].w, r[19].xyzw, t[18][23].yzwx
3975: ld_structured r[20].xyzw, r[10].w, l(68), t[11][11].xyzw
3985: ld_structured r[21].xyz, r[10].w, l(84), t[11][11].xyzx
3995: ld_structured r[22].xyzw, r[10].w, l(96), t[11][11].xyzw
4005: ld_structured r[23].xyzw, r[10].w, l(112), t[11][11].xyzw
4015: ld_structured r[24].xyzw, r[10].w, l(128), t[11][11].xyzw
4025: mov r[25].x, r[20].w
4030: mov r[25].y, r[22].x
4035: mov r[25].z, r[23].x
4040: mov r[25].w, r[24].x
4045: dp4 r[25].x, r[25].xyzw, r[8].xyzw
4052: mov r[26].x, r[21].x
4057: mov r[26].y, r[22].y
4062: mov r[26].z, r[23].y
4067: mov r[26].w, r[24].y
4072: dp4 r[25].y, r[26].xyzw, r[8].xyzw
4079: mov r[26].x, r[21].y
4084: mov r[26].y, r[22].z
4089: mov r[26].z, r[23].z
4094: mov r[26].w, r[24].z
4099: dp4 r[25].z, r[26].xyzw, r[8].xyzw
4106: mov r[24].x, r[21].z
4111: mov r[24].y, r[22].w
4116: mov r[24].z, r[23].w
4121: dp4 r[14].w, r[24].xyzw, r[8].xyzw
4128: rcp r[15].w, r[14].w
4133: mul r[21].xyz, r[15].wwww, r[25].xyzx
4140: mad r[22].xyz, r[21].xyzx, l(0.5,0.5,1,0), l(0.5,0.5,0,0)
4155: mov_sat r[23].xyz, r[22].xyzx
4160: eq r[22].xyz, r[22].xyzx, r[23].xyzx
4167: and r[15].w, r[22].y, r[22].x
4174: and r[15].w, r[22].z, r[15].w
4181: if r[15].w
4184: ld_structured r[22].xyzw, r[10].w, l(0), t[11][11].xyzw
4194: ld_structured r[15].w, r[10].w, l(156), t[11][11].xxxx
4204: ld_structured r[16].w, r[10].w, l(200), t[11][11].xxxx
4214: ld_structured r[23].xyzw, r[10].w, l(288), t[11][11].xyzw
4224: ld_structured r[17].w, r[10].w, l(304), t[11][11].xxxx
4234: mad r[24].xy, r[21].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
4249: and r[24].zw, r[17].wwww, l(0,0,2,768)
4259: if r[24].z
4262: ld_structured r[26].xyzw, r[10].w, l(272), t[11][11].xyzw
4272: mad r[25].xy, r[24].xyxx, r[26].zwzz, r[26].xyxx
4281: sample_l r[25].xy, r[25].xyxx, t[9][9].xyzw, s[1][6], l(0)
4294: add r[26].xyz, -r[12].xyzx, r[22].xyzx
4302: dp3 r[18].w, r[26].xyzx, r[26].xyzx
4309: sqrt r[18].w, r[18].w
4314: add r[18].w, -r[22].w, r[18].w
4322: mul r[19].y, r[15].w, r[18].w
4329: mad r[20].w, -r[25].x, r[25].x, r[25].y
4339: max r[20].w, r[20].w, l(9.99999975e-06)
4346: mad r[18].w, -r[18].w, r[15].w, r[25].x
4356: mad r[18].w, r[18].w, r[18].w, r[20].w
4365: div r[18].w, r[20].w, r[18].w
4372: min r[18].w, r[18].w, l(1)
4379: lt r[19].y, r[19].y, r[25].x
4386: movc r[18].w, r[19].y, l(1), r[18].w
4395: else 
4396: mov r[18].w, l(1)
4401: endif 
4402: ieq r[25].xy, r[24].wwww, l(256,512,0,0)
4412: if r[25].x
4415: mad r[25].xw, r[24].xxxy, r[23].zzzw, r[23].xxxy
4424: sample_l r[19].y, r[25].xwxx, t[8][8].yxzw, s[1][6], l(0)
4437: ge r[19].y, r[19].y, r[21].z
4444: and r[19].y, r[19].y, l(1)
4451: else 
4452: ieq r[20].w, r[24].w, l(512)
4459: if r[20].w
4462: mad r[21].zw, r[24].xxxy, r[23].zzzw, r[23].xxxy
4471: sample_l r[21].zw, r[21].zwzz, t[9][9].zwxy, s[1][6], l(0)
4484: add r[23].xyz, -r[12].xyzx, r[22].xyzx
4492: dp3 r[20].w, r[23].xyzx, r[23].xyzx
4499: sqrt r[20].w, r[20].w
4504: add r[20].w, -r[22].w, r[20].w
4512: mul r[22].w, r[15].w, r[20].w
4519: mad r[21].w, -r[21].z, r[21].z, r[21].w
4529: max r[21].w, r[21].w, l(9.99999975e-06)
4536: mad r[15].w, -r[20].w, r[15].w, r[21].z
4546: mad r[15].w, r[15].w, r[15].w, r[21].w
4555: div r[15].w, r[21].w, r[15].w
4562: min r[15].w, r[15].w, l(1)
4569: lt r[20].w, r[22].w, r[21].z
4576: movc r[19].y, r[20].w, l(1), r[15].w
4585: else 
4586: mov r[19].y, l(1)
4591: endif 
4592: endif 
4593: ine r[15].w, r[24].z, l(0)
4600: and r[15].w, r[15].w, r[25].y
4607: add r[20].w, -r[18].w, r[19].y
4615: mad r[20].w, r[16].w, r[20].w, r[18].w
4624: mad r[19].y, r[19].y, r[16].w, l(1)
4633: add r[16].w, -r[16].w, r[19].y
4641: min r[16].w, r[16].w, r[18].w
4648: movc r[15].w, r[15].w, r[20].w, r[16].w
4657: ne r[16].w, r[15].w, l(0)
4664: if r[16].w
4667: ld_structured r[23].xyz, r[10].w, l(16), t[11][11].xyzx
4677: ld_structured r[26].xyzw, r[10].w, l(48), t[11][11].xyzw
4687: add r[16].w, -r[25].z, r[26].w
4695: mul r[18].w, r[26].w, l(0.200000003)
4702: div_sat r[16].w, r[16].w, r[18].w
4709: mad r[18].w, r[16].w, l(-2), l(3)
4718: mul r[16].w, r[16].w, r[16].w
4725: mul r[16].w, r[16].w, r[18].w
4732: add r[23].xyz, -r[12].xyzx, r[23].xyzx
4740: dp3 r[18].w, r[23].xyzx, r[23].xyzx
4747: mul r[19].y, r[20].y, r[20].y
4754: mad r[18].w, r[20].z, r[18].w, r[19].y
4763: max r[18].w, r[18].w, l(0.25)
4770: mul r[18].w, r[18].w, l(3.14159274)
4777: rcp r[18].w, r[18].w
4782: add r[18].w, -r[20].x, r[18].w
4790: max r[18].w, r[18].w, l(0)
4797: mul r[16].w, r[16].w, r[18].w
4804: mul r[20].xyz, r[16].wwww, r[26].xyzx
4811: and r[16].w, r[17].w, l(1)
4818: if r[16].w
4821: ld_structured r[23].xyzw, r[10].w, l(256), t[11][11].xyzw
4831: and r[16].w, r[17].w, l(1.43492963e-42)
4838: add r[21].zw, r[21].xxxy, r[21].xxxy
4845: mad r[17].w, r[21].x, r[21].x, l(1)
4854: mad r[17].w, r[21].y, r[21].y, r[17].w
4863: div r[21].xy, r[21].zwzz, r[17].wwww
4870: mad r[21].xy, r[21].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
4885: movc r[21].xy, r[16].wwww, r[21].xyxx, r[24].xyxx
4894: mad r[21].xy, r[21].xyxx, r[23].zwzz, r[23].xyxx
4903: sample_l r[21].xyz, r[21].xyxx, t[10][10].xyzw, s[2][11], l(0)
4916: mul r[20].xyz, r[20].xyzx, r[21].xyzx
4923: endif 
4924: ge r[14].w, r[14].w, r[25].z
4931: and r[14].w, r[14].w, l(1)
4938: mul r[14].w, r[14].w, r[15].w
4945: mul r[20].xyz, r[14].wwww, r[20].xyzx
4952: add r[21].xyz, -r[12].xyzx, r[22].xyzx
4960: dp3 r[14].w, r[21].xyzx, r[21].xyzx
4967: rsq r[14].w, r[14].w
4972: mul r[22].xyz, r[14].wwww, r[21].xyzx
4979: mad r[21].xyz, r[21].xyzx, r[14].wwww, r[11].xyzx
4988: dp3 r[14].w, r[21].xyzx, r[21].xyzx
4995: rsq r[14].w, r[14].w
5000: mul r[21].xyz, r[14].wwww, r[21].xyzx
5007: dp3 r[14].w, r[11].xyzx, r[21].xyzx
5014: dp3_sat r[15].w, r[13].xyzx, r[22].xyzx
5021: add r[14].w, -abs(r[14].w), l(1)
5029: max r[14].w, r[14].w, l(0)
5036: mul r[16].w, r[14].w, r[14].w
5043: mul r[16].w, r[16].w, r[16].w
5050: mul r[14].w, r[14].w, r[16].w
5057: mad r[14].w, r[11].w, r[14].w, r[7].w
5066: add_sat r[14].w, -r[14].w, l(1)
5074: mul r[14].w, r[15].w, r[14].w
5081: mul r[14].w, r[14].w, l(0.318309873)
5088: mad r[9].xyz, r[14].wwww, r[20].xyzx, r[9].xyzx
5097: endif 
5098: endif 
5099: mov r[10].w, r[13].w
5104: endloop 
5105: mad r[4].xyz, r[4].xyzx, l(0.318309873,0.318309873,0.318309873,0), r[9].xyzx
5117: mul r[15].xyz, r[17].xyzx, r[4].xyzx
5124: else 
5125: mov r[15].xyz, l(0,0,0,0)
5133: endif 
5134: endif 
5135: mad r[10].xyz, r[3].xyzx, r[15].xyzx, r[10].xyzx
5144: iadd r[1].z, r[1].z, l(1)
5151: endloop 
5152: add r[0].xy, -cb[1][1][17].yzyy, cb[1][1][17].xwxx
5164: add r[0].z, cb[1][1][17].y, cb[1][1][17].x
5175: add r[0].z, -r[0].x, r[0].z
5183: add r[0].x, -r[0].x, r[0].w
5191: div r[0].z, l(1,1,1,1), r[0].z
5201: mul_sat r[0].x, r[0].z, r[0].x
5208: mad r[0].z, r[0].x, l(-2), l(3)
5217: mul r[0].x, r[0].x, r[0].x
5224: mul r[0].x, r[0].x, r[0].z
5231: mad r[0].x, r[0].x, r[0].y, cb[1][1][17].z
5242: mul r[0].xyz, r[0].xxxx, r[10].xyzx
5249: itof r[0].w, cb[4][4][0].z
5256: div r[2].xyz, r[0].xyzx, r[0].wwww
5263: div r[0].x, r[5].w, r[0].w
5270: ld_uav_typed r[0].y, r[1].xyyy, type30[2][2].yxzw
5278: add r[0].x, r[0].x, r[0].y
5285: store_uav_typed type30[2][2].xyzw, r[1].xyyy, r[0].xxxx
5293: ld_uav_typed r[0].xyzw, r[1].xyyy, type30[0][0].xyzw
5301: mov r[2].w, l(1)
5306: add r[0].xyzw, r[0].xyzw, r[2].xyzw
5313: store_uav_typed type30[0][0].xyzw, r[1].xyyy, r[0].xyzw
5321: ld_uav_typed r[0].x, r[1].xyyy, type30[1][1].xyzw
5329: add r[0].x, r[0].x, l(1)
5336: store_uav_typed type30[1][1].xyzw, r[1].xyyy, r[0].xxxx
5344: ret 
5345: else 
5346: ld_uav_typed r[0].x, r[1].xyyy, type30[2][2].xyzw
5354: add r[0].x, r[0].x, l(65504)
5361: store_uav_typed type30[2][2].xyzw, r[1].xyyy, r[0].xxxx
5369: endif 
5370: ret 