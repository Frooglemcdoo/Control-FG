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
237: dcl_input raw:0x20032
239: dcl_temps raw:0x1e
241: dcl_thread_group raw:0x8,0x10,0x1
245: mov r[0].xy, type32.xyxx
249: mov r[0].w, l(0)
254: ld r[1].x, r[0].xyww, t[19][24].xyzw
262: ine r[1].x, r[1].x, l(9.18326936e-41)
269: if r[1].x
272: utof r[1].xy, type32.xyxx
276: add r[1].xy, r[1].xyxx, l(0.5,0.5,0,0)
286: mul r[1].xy, r[1].xyxx, cb[0][0][1].zwzz
295: ld r[1].z, r[0].xyww, t[3][3].yzxw
303: ld r[1].w, r[0].xyww, t[2][2].yzwx
311: mad r[1].xy, r[1].xyxx, l(2,-2,0,0), l(-1,1,0,0)
326: mul r[2].xyzw, r[1].yyyy, cb[0][0][15].xyzw
335: mad r[2].xyzw, cb[0][0][14].xyzw, r[1].xxxx, r[2].xyzw
346: mad r[2].xyzw, cb[0][0][16].xyzw, r[1].zzzz, r[2].xyzw
357: add r[2].xyzw, r[2].xyzw, cb[0][0][17].xyzw
366: rcp r[2].w, r[2].w
371: mul r[3].xyz, r[2].wwww, r[2].xyzx
378: dp3 r[3].w, r[3].xyzx, r[3].xyzx
385: rsq r[3].w, r[3].w
390: mul r[3].xyz, r[3].wwww, r[3].xyzx
397: mul r[4].xyzw, r[1].yyyy, cb[0][0][23].xyzw
406: mad r[4].xyzw, cb[0][0][22].xyzw, r[1].xxxx, r[4].xyzw
417: mad r[4].xyzw, cb[0][0][24].xyzw, r[1].zzzz, r[4].xyzw
428: add r[4].xyzw, r[4].xyzw, cb[0][0][25].xyzw
437: rcp r[1].x, r[4].w
442: mul r[5].xyz, r[1].xxxx, r[4].xyzx
449: mad r[1].xyz, r[4].xyzx, r[1].xxxx, -cb[0][0][54].xyzx
461: dp3 r[3].w, r[1].xyzx, r[1].xyzx
468: rsq r[3].w, r[3].w
473: mul r[1].xyz, r[1].xyzx, r[3].wwww
480: ld r[4].xyzw, r[0].xyww, t[0][0].xzyw
488: mad r[3].w, r[4].w, l(255), l(0.5)
497: ftou r[3].w, r[3].w
502: and r[3].w, r[3].w, l(254)
509: utof r[6].z, r[3].w
514: mov r[6].xy, r[4].xzxx
519: mad r[6].xyz, r[6].xyzx, l(255,255,1,0), l(0.5,0.5,0.5,0)
534: round_ni r[6].xyz, r[6].xyzx
539: ftoi r[6].xyz, r[6].xyzx
544: ubfe r[7].xyzw, l(3,4,3,4), l(1,4,1,4), r[6].zzzz
559: bfi r[6].xyzw, l(29,28,29,28), l(3,4,3,4), r[6].xyxy, r[7].xyzw
576: itof r[6].xyzw, r[6].xyzw
581: mad r[6].xyzw, r[6].xyzw, l(0.00097703957,0.000488400517,0.00097703957,0.000488400517), l(-1,-1,-1,-1)
596: mul r[6].xyzw, r[6].xyzw, l(1.29999995,1.29999995,2.5999999,2.5999999)
606: dp2 r[3].w, r[6].xyxx, r[6].xyxx
613: add r[6].xy, r[3].wwww, l(1,-1,0,0)
623: div r[6].xyz, r[6].zwyz, r[6].xxxx
630: mul r[7].xyz, r[6].yyyy, cb[0][0][7].xyzx
639: mad r[7].xyz, cb[0][0][6].xyzx, r[6].xxxx, r[7].xyzx
650: mad r[7].xyz, cb[0][0][8].xyzx, r[6].zzzz, r[7].xyzx
661: ld r[8].xyz, r[0].xyww, t[1][1].yzwx
669: mul r[4].zw, r[8].yyyz, l(0,0,255,255)
679: ftou r[4].zw, r[4].zzzw
684: ishl r[3].w, r[4].z, l(8)
691: or r[3].w, r[4].w, r[3].w
698: dp3 r[4].x, r[6].xyzx, -r[3].xyzx
706: ld_structured r[3].x, r[3].w, l(0), t[4][4].xxxx
716: ushr r[3].y, r[3].x, l(24)
723: ubfe r[3].xz, l(8,0,8,0), l(16,0,8,0), r[3].xxxx
738: utof r[6].xyz, r[3].yxzy
743: mul r[3].xyz, r[6].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
753: ld_structured r[3].w, r[3].w, l(0), t[4][4].xxxx
763: and r[3].w, r[3].w, l(255)
770: dp3 r[4].z, r[3].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
780: add r[4].w, r[4].z, l(9.99999997e-07)
787: rcp r[4].w, r[4].w
792: mul r[4].z, r[4].z, l(10000)
799: min r[4].z, r[4].z, l(1)
806: mad r[6].xyz, r[3].xyzx, r[4].wwww, l(-1,-1,-1,0)
818: mad r[6].xyz, r[4].zzzz, r[6].xyzx, l(1,1,1,0)
830: mul r[6].xyz, r[8].xxxx, r[6].xyzx
837: movc r[3].xyz, r[3].wwww, r[3].xyzx, r[6].xyzx
846: mul r[1].xyz, r[1].wwww, r[1].xyzx
853: mad r[1].xyz, -r[1].xyzx, l(0.00100000005,0.00100000005,0.00100000005,0), r[5].xyzx
866: mul r[6].xyz, r[1].wwww, r[7].xyzx
873: mad r[1].xyz, r[6].xyzx, l(0.00100000005,0.00100000005,0.00100000005,0), r[1].xyzx
885: sample_l r[4].xyz, r[4].xyxx, t[12][12].xyzw, s[1][6], l(0)
898: mul r[3].w, cb[1][1][15].w, l(0.318309873)
907: mad r[3].xyz, r[3].xyzx, r[4].xxxx, r[4].yyyy
916: add r[3].xyz, -r[3].xyzx, l(1,1,1,0)
927: max r[3].xyz, r[3].xyzx, l(0,0,0,0)
937: mul r[3].xyz, r[4].zzzz, r[3].xyzx
944: mov r[4].w, l(1)
949: mov r[6].z, l(0)
954: mov r[8].w, l(1)
959: mov r[9].w, l(1)
964: mov r[10].xyz, l(0,0,0,0)
972: mov r[5].w, l(0)
977: mov r[0].z, l(0)
982: loop 
983: ige r[7].w, r[0].z, cb[4][4][0].z
992: breakc r[7].w
995: ld r[7].w, r[0].xyzw, t[19][24].yzwx
1003: ld r[11].xyzw, r[0].xyzw, t[21][26].xyzw
1011: mul r[12].xyz, r[11].yyyy, cb[0][0][7].xyzx
1020: mad r[12].xyz, cb[0][0][6].xyzx, r[11].xxxx, r[12].xyzx
1031: mad r[12].xyz, cb[0][0][8].xyzx, r[11].zzzz, r[12].xyzx
1042: add r[12].xyz, r[12].xyzx, cb[0][0][9].xyzx
1051: ld r[13].xyzw, r[0].xyzw, t[20][25].xyzw
1059: add r[14].xyz, -r[1].xyzx, r[12].xyzx
1067: dp3 r[10].w, r[14].xyzx, r[14].xyzx
1074: rsq r[10].w, r[10].w
1079: mul r[14].xyz, r[10].wwww, r[14].xyzx
1086: ieq r[10].w, r[7].w, l(9.18340949e-41)
1093: mad r[15].xyz, cb[4][4][0].yyyy, r[14].xyzx, r[5].xyzx
1104: movc r[4].xyz, r[10].wwww, r[15].xyzx, r[12].xyzx
1113: movc r[15].xyz, r[10].wwww, r[7].xyzx, r[13].xyzx
1122: mov r[16].xyz, l(0,0,0,0)
1130: mov r[12].w, l(0)
1135: mov r[14].w, l(0)
1140: loop 
1141: uge r[15].w, r[14].w, cb[2][2][0].w
1150: breakc r[15].w
1153: ishl r[15].w, r[14].w, l(2)
1160: dp4 r[17].x, cb[3][3][r[15].w].xyzw, r[4].xyzw
1170: dp4 r[17].y, cb[3][3][1+r[15].w].xyzw, r[4].xyzw
1181: dp4 r[17].z, cb[3][3][2+r[15].w].xyzw, r[4].xyzw
1192: ge r[18].xyz, r[17].xyzx, l(0,0,0,0)
1202: and r[16].w, r[18].y, r[18].x
1209: and r[16].w, r[18].z, r[16].w
1216: utof r[18].xyz, cb[3][3][3+r[15].w].xyzx
1225: lt r[19].xyz, r[17].xyzx, r[18].xyzx
1232: and r[17].w, r[19].y, r[19].x
1239: and r[17].w, r[19].z, r[17].w
1246: and r[16].w, r[16].w, r[17].w
1253: if r[16].w
1256: ld_structured r[19].xyz, r[14].w, l(0), t[13][13].xyzx
1266: ld_structured r[20].xyz, r[14].w, l(12), t[13][13].xyzx
1276: ld_structured r[21].xyzw, r[14].w, l(24), t[13][13].xyzw
1286: ld_structured r[22].xyzw, r[14].w, l(40), t[13][13].xyzw
1296: ld_structured r[23].xyz, r[14].w, l(56), t[13][13].xyzx
1306: mul_sat r[16].w, r[17].x, r[21].w
1313: add r[18].xyz, -r[17].xyzx, r[18].xyzx
1321: mul_sat r[17].w, r[21].w, r[18].x
1328: mul r[16].w, r[16].w, r[17].w
1335: mul_sat r[18].xw, r[17].yyyz, r[22].xxxy
1342: mul r[16].w, r[16].w, r[18].x
1349: mul_sat r[18].xy, r[22].xyxx, r[18].yzyy
1356: mul r[16].w, r[16].w, r[18].x
1363: mul r[16].w, r[18].w, r[16].w
1370: mul r[16].w, r[18].y, r[16].w
1377: ine r[17].w, r[23].x, l(nan)
1384: if r[17].w
1387: round_ni r[18].xyz, r[17].xyzx
1392: add r[17].xyz, r[17].xyzx, -r[18].xyzx
1400: ftou r[18].xyz, r[18].xyzx
1405: imad r[17].w, r[18].z, cb[3][3][3+r[15].w].y, r[18].y
1418: imad r[15].w, r[17].w, cb[3][3][3+r[15].w].x, r[18].x
1431: iadd r[15].w, r[22].z, r[15].w
1438: ld_structured r[15].w, r[15].w, l(0), t[14][14].xxxx
1448: mov r[18].xyz, l(0,0,0,0)
1456: mov r[22].xyz, r[17].xyzx
1461: mov r[17].w, r[15].w
1466: mov r[18].w, r[23].y
1471: loop 
1472: and r[19].w, r[17].w, l(-0)
1479: ine r[19].w, r[19].w, l(0)
1486: ge r[20].w, l(9.99999975e-06), r[18].w
1493: or r[19].w, r[19].w, r[20].w
1500: if r[19].w
1503: and r[19].w, r[17].w, l(nan)
1510: iadd r[19].w, r[23].x, r[19].w
1517: and r[24].x, r[19].w, l(127)
1524: ubfe r[24].y, l(7), l(7), r[19].w
1533: ushr r[24].z, r[19].w, l(14)
1540: ishl r[24].xyz, r[24].xyzx, l(2,2,2,0)
1550: utof r[24].xyz, r[24].xyzx
1555: mad r[24].xyz, r[22].xyzx, l(3,3,3,0), r[24].xyzx
1567: add r[18].xyz, r[24].xyzx, l(0.5,0.5,0.5,0)
1577: break 
1578: endif 
1579: iadd r[19].w, r[22].w, r[17].w
1586: ld_structured r[24].xyzw, r[19].w, l(0), t[15][15].xyzw
1596: mul r[25].xyz, r[22].xyzx, l(3,3,3,0)
1606: round_ni r[25].xyz, r[25].xyzx
1611: ftou r[26].xyz, r[25].xyzx
1616: utof r[26].xz, r[26].zzxz
1621: imul null, r[19].w, r[26].y, l(3)
1629: utof r[19].w, r[19].w
1634: mad r[19].w, r[26].x, l(9), r[19].w
1643: add r[19].w, r[26].z, r[19].w
1650: ftou r[19].w, r[19].w
1655: ishl r[20].w, l(1), r[19].w
1662: and r[21].w, r[20].w, r[24].x
1669: if r[21].w
1672: iadd r[21].w, r[23].x, r[17].w
1679: and r[26].x, r[21].w, l(127)
1686: ubfe r[26].y, l(7), l(7), r[21].w
1695: ushr r[26].z, r[21].w, l(14)
1702: ishl r[26].xyz, r[26].xyzx, l(2,2,2,0)
1712: utof r[26].xyz, r[26].xyzx
1717: mad r[26].xyz, r[22].xyzx, l(3,3,3,0), r[26].xyzx
1729: add r[18].xyz, r[26].xyzx, l(0.5,0.5,0.5,0)
1739: break 
1740: endif 
1741: mad r[22].xyz, r[22].xyzx, l(3,3,3,0), -r[25].xyzx
1754: and r[20].w, r[20].w, r[24].y
1761: ubfe r[21].w, r[19].w, l(0), r[24].y
1770: countbits r[21].w, r[21].w
1775: iadd r[21].w, r[21].w, r[24].w
1782: or r[21].w, r[21].w, l(-0)
1789: not r[23].w, r[24].y
1794: and r[23].w, r[23].w, r[24].x
1801: ubfe r[19].w, r[19].w, l(0), r[23].w
1810: countbits r[19].w, r[19].w
1815: iadd r[19].w, r[19].w, r[24].z
1822: movc r[17].w, r[20].w, r[21].w, r[19].w
1831: mul r[18].w, r[18].w, l(0.333333343)
1838: mov r[18].xyz, l(0,0,0,0)
1846: endloop 
1847: else 
1848: mov r[18].xyz, l(0,0,0,0)
1856: endif 
1857: mul r[17].xyz, r[18].xyzx, cb[2][2][0].xyzx
1866: mov r[22].x, r[19].x
1871: mov r[22].y, r[20].x
1876: mov r[22].z, r[21].x
1881: dp3 r[22].x, r[22].xyzx, r[15].xyzx
1888: mov r[24].x, r[19].y
1893: mov r[24].y, r[20].y
1898: mov r[24].z, r[21].y
1903: dp3 r[22].y, r[24].xyzx, r[15].xyzx
1910: mov r[21].x, r[19].z
1915: mov r[21].y, r[20].z
1920: dp3 r[22].z, r[21].xyzx, r[15].xyzx
1927: mov_sat r[19].xyz, r[22].xyzx
1932: mul r[19].xyz, r[19].xyzx, r[19].xyzx
1939: mov_sat r[22].xyz, -r[22].xyzx
1945: mul r[20].xyz, r[22].xyzx, r[22].xyzx
1952: lt r[21].xyz, l(0,0,0,0), r[19].xyzx
1962: if r[21].x
1965: sample_l r[22].xyz, r[17].xyzx, t[16][16].xyzw, s[1][6], l(0)
1978: mul r[22].xyz, r[19].xxxx, r[22].xyzx
1985: else 
1986: mov r[22].xyz, l(0,0,0,0)
1994: endif 
1995: lt r[24].xyz, l(0,0,0,0), r[20].xyzx
2005: if r[24].x
2008: sample_l r[25].xyz, r[17].xyzx, t[16][17].xyzw, s[1][6], l(0)
2021: mad r[22].xyz, r[25].xyzx, r[20].xxxx, r[22].xyzx
2030: endif 
2031: if r[21].y
2034: sample_l r[21].xyw, r[17].xyzx, t[16][18].xywz, s[1][6], l(0)
2047: mad r[22].xyz, r[21].xywx, r[19].yyyy, r[22].xyzx
2056: endif 
2057: if r[24].y
2060: sample_l r[19].xyw, r[17].xyzx, t[16][19].xywz, s[1][6], l(0)
2073: mad r[22].xyz, r[19].xywx, r[20].yyyy, r[22].xyzx
2082: endif 
2083: if r[21].z
2086: sample_l r[19].xyw, r[17].xyzx, t[16][20].xywz, s[1][6], l(0)
2099: mad r[22].xyz, r[19].xywx, r[19].zzzz, r[22].xyzx
2108: endif 
2109: if r[24].z
2112: sample_l r[17].xyz, r[17].xyzx, t[16][21].xyzw, s[1][6], l(0)
2125: mad r[22].xyz, r[17].xyzx, r[20].zzzz, r[22].xyzx
2134: endif 
2135: ieq r[15].w, r[23].x, l(nan)
2142: movc r[17].xyz, r[15].wwww, l(1,0.349999994,0,0), r[22].xyzx
2154: mad r[15].w, r[16].w, l(-2), l(3)
2163: mul r[17].w, r[16].w, r[16].w
2170: mul r[15].w, r[15].w, r[17].w
2177: add r[17].w, r[23].z, l(-1)
2184: mad r[15].w, r[15].w, r[17].w, l(1)
2193: mul r[17].w, r[15].w, r[16].w
2200: mad r[16].xyz, r[17].xyzx, r[17].wwww, r[16].xyzx
2209: mad r[12].w, r[16].w, r[15].w, r[12].w
2218: endif 
2219: iadd r[14].w, r[14].w, l(1)
2226: endloop 
2227: lt r[4].x, l(0), r[12].w
2234: div r[15].xyz, r[16].xyzx, r[12].wwww
2241: movc r[4].xyz, r[4].xxxx, r[15].xyzx, r[16].xyzx
2250: add r[4].xyz, r[4].xyzx, cb[2][2][1].xyzx
2259: mad r[15].xyz, r[2].xyzx, r[2].wwww, -r[11].xyzx
2269: dp3 r[14].w, r[15].xyzx, r[15].xyzx
2276: sqrt r[14].w, r[14].w
2281: movc r[14].w, r[10].w, cb[4][4][0].x, r[14].w
2292: add r[5].w, r[5].w, r[14].w
2299: if r[10].w
2302: mul r[15].xyz, r[3].wwww, r[4].xyzx
2309: else 
2310: ld_structured r[17].xyz, r[7].w, l(52), t[5][5].xyzx
2320: imul null, r[10].w, r[7].w, l(35)
2328: mov r[19].x, r[13].w
2333: mov r[19].y, r[11].w
2338: sample_l r[20].xyz, r[19].xyxx, t[22][r[10].w].xyzw, s[0][5], l(0)
2353: mul r[17].xyz, r[17].xyzx, r[20].xyzx
2360: dp3 r[10].w, r[17].xyzx, r[17].xyzx
2367: ge r[10].w, r[10].w, l(9.99999975e-05)
2374: if r[10].w
2377: mul_sat r[10].w, r[14].w, l(50)
2384: mul r[4].xyz, r[4].xyzx, r[10].wwww
2391: ld_structured r[19].zw, r[7].w, l(68), t[5][5].xxxy
2401: imad r[20].xy, r[7].wwww, l(35,35,0,0), l(2,4,0,0)
2416: sample_l r[10].w, r[19].xyxx, t[22][r[20].x].yzwx, s[3][19], l(0)
2431: add r[11].w, -r[19].z, r[19].w
2439: mad r[21].y, r[10].w, r[11].w, r[19].z
2448: sample_l r[10].w, r[19].xyxx, t[22][r[20].y].yzwx, s[3][19], l(0)
2463: ld_structured r[7].w, r[7].w, l(0), t[4][4].xxxx
2473: ushr r[11].w, r[7].w, l(24)
2480: ubfe r[19].xy, l(8,8,0,0), l(16,8,0,0), r[7].wwww
2495: utof r[20].x, r[11].w
2500: utof r[20].yz, r[19].xxyx
2505: mul r[19].xyz, r[10].wwww, r[20].xyzx
2512: mul r[19].xyz, r[19].xyzx, l(0.00392156886,0.00392156886,0.00392156886,0)
2522: dp3 r[7].w, r[19].xyzx, l(0.212599993,0.715200007,0.0722000003,0)
2532: dp3 r[21].x, r[13].xyzx, -r[14].xyzx
2540: sample_l r[14].xyz, r[21].xyxx, t[12][12].xyzw, s[1][6], l(0)
2553: mad r[14].xyw, r[19].xyxz, r[14].xxxx, r[14].yyyy
2562: add r[14].xyw, -r[14].xyxw, l(1,1,0,1)
2573: mul r[14].xyw, r[14].xyxw, cb[1][1][15].wwww
2582: mul r[14].xyz, r[14].zzzz, r[14].xywx
2589: mul r[4].xyz, r[4].xyzx, r[14].xyzx
2596: mul r[14].xyz, r[12].yyyy, cb[0][0][3].xyzx
2605: mad r[14].xyz, cb[0][0][2].xyzx, r[12].xxxx, r[14].xyzx
2616: mad r[12].xyz, cb[0][0][4].xyzx, r[12].zzzz, r[14].xyzx
2627: add r[12].xyz, r[12].xyzx, cb[0][0][5].xyzx
2636: mul r[14].xyz, r[13].yyyy, cb[0][0][3].xyzx
2645: mad r[13].xyw, cb[0][0][2].xyxz, r[13].xxxx, r[14].xyxz
2656: mad r[13].xyz, cb[0][0][4].xyzx, r[13].zzzz, r[13].xywx
2667: add r[11].xyz, r[11].xyzx, l(150,75,75,0)
2677: mul r[11].xyz, r[11].xyzx, l(0.0533333346,0.0533333346,0.0533333346,0)
2687: round_ni r[11].xyz, r[11].xyzx
2692: max r[11].xyz, r[11].xyzx, l(0,0,0,0)
2702: min r[11].xyz, r[11].xyzx, l(15,7,11,0)
2712: ftoi r[11].xyz, r[11].xyzx
2717: imad r[6].x, r[11].x, l(12), r[11].z
2726: ishl r[6].y, r[11].y, l(8)
2733: ld r[10].w, r[6].xyzz, t[17][22].yzwx
2741: dp3 r[11].x, -r[12].xyzx, -r[12].xyzx
2750: mad r[9].xyz, r[13].xyzx, l(0.0700000003,0.0700000003,0.0700000003,0), r[12].xyzx
2762: rsq r[11].x, r[11].x
2767: mul r[11].xyz, r[11].xxxx, -r[12].xyzx
2775: add r[11].w, -r[7].w, l(1)
2783: mov r[14].xyz, l(0,0,0,0)
2791: mov r[13].w, l(0)
2796: mov r[14].w, r[10].w
2801: loop 
2802: ine r[15].w, r[14].w, l(9.18340949e-41)
2809: ilt r[16].w, r[13].w, l(256)
2816: and r[15].w, r[15].w, r[16].w
2823: breakc r[15].w
2826: ld_structured r[19].xyzw, r[14].w, l(0), t[7][7].xyzw
2836: ld_structured r[20].xyzw, r[14].w, l(16), t[7][7].xyzw
2846: ld_structured r[21].xyzw, r[14].w, l(36), t[7][7].xyzw
2856: ld_structured r[22].xyzw, r[14].w, l(52), t[7][7].xyzw
2866: ld_structured r[15].w, r[14].w, l(144), t[7][7].xxxx
2876: ld_structured r[16].w, r[14].w, l(156), t[7][7].xxxx
2886: add r[19].xyz, -r[12].xyzx, r[19].xyzx
2894: dp3 r[17].w, r[19].xyzx, r[19].xyzx
2901: rsq r[18].w, r[17].w
2906: mul r[23].xyz, r[18].wwww, r[19].xyzx
2913: sqrt r[23].w, r[17].w
2918: add r[24].x, r[20].w, -r[23].w
2926: mul r[20].w, r[20].w, l(0.200000003)
2933: div_sat r[20].w, r[24].x, r[20].w
2940: mad r[24].x, r[20].w, l(-2), l(3)
2949: mul r[20].w, r[20].w, r[20].w
2956: mul r[20].w, r[20].w, r[24].x
2963: mad r[15].w, r[15].w, r[15].w, r[17].w
2972: max r[15].w, r[15].w, l(9.99999975e-05)
2979: mul r[15].w, r[15].w, l(3.14159274)
2986: rcp r[15].w, r[15].w
2991: add r[15].w, -r[21].x, r[15].w
2999: max r[15].w, r[15].w, l(0)
3006: mul r[15].w, r[20].w, r[15].w
3013: mul r[20].xyz, r[15].wwww, r[20].xyzx
3020: and r[24].xy, r[16].wwww, l(4,2,0,0)
3030: mov r[25].x, r[21].w
3035: mov r[25].yz, r[22].xxyx
3040: dp3 r[15].w, -r[23].xyzx, r[25].xyzx
3048: mad_sat r[15].w, r[15].w, r[21].y, r[21].z
3057: mul r[15].w, r[15].w, r[15].w
3064: mul r[21].xyz, r[15].wwww, r[20].xyzx
3071: movc r[20].xyz, r[24].xxxx, r[21].xyzx, r[20].xyzx
3080: if r[24].y
3083: ld_structured r[21].xy, r[14].w, l(68), t[7][7].xyxx
3093: ld_structured r[24].xyz, r[14].w, l(80), t[7][7].xyzx
3103: ld_structured r[25].xyz, r[14].w, l(96), t[7][7].xyzx
3113: ld_structured r[26].xyw, r[14].w, l(112), t[7][7].xyxz
3123: ld_structured r[27].xyzw, r[14].w, l(128), t[7][7].xyzw
3133: mov r[28].x, r[22].w
3138: mov r[28].y, r[24].x
3143: mov r[28].z, r[25].x
3148: mov r[28].w, r[26].x
3153: dp4 r[28].x, r[28].xyzw, r[9].xyzw
3160: mov r[29].x, r[21].x
3165: mov r[29].y, r[24].y
3170: mov r[29].z, r[25].y
3175: mov r[29].w, r[26].y
3180: dp4 r[28].y, r[29].xyzw, r[9].xyzw
3187: mov r[26].x, r[21].y
3192: mov r[26].y, r[24].z
3197: mov r[26].z, r[25].z
3202: dp4 r[28].z, r[26].xyzw, r[9].xyzw
3209: dp3 r[15].w, r[28].xyzx, r[28].xyzx
3216: rsq r[15].w, r[15].w
3221: mul r[21].xyz, r[15].wwww, r[28].xyzx
3228: lt r[24].xyzw, abs(r[21].yzxz), abs(r[21].xxyy)
3237: lt r[22].xyw, l(0,0,0,0), r[21].xyxz
3247: and r[24].xy, r[24].ywyy, r[24].xzxx
3254: movc r[22].xyw, r[22].xyxw, l(0,2,0,4), l(1,3,0,5)
3269: movc r[15].w, r[24].y, r[22].y, r[22].w
3278: movc r[15].w, r[24].x, r[22].x, r[15].w
3287: if r[15].w
3290: ine r[16].w, r[15].w, l(1)
3297: if r[16].w
3300: div r[24].xyz, r[21].xxyx, r[21].yzzy
3307: div r[16].w, -r[21].z, r[21].y
3315: mad r[25].y, r[16].w, l(-0.5), l(0.5)
3324: div r[21].xw, r[21].xxxz, -r[21].yyyy
3332: mad r[21].xw, r[21].xxxw, l(0.5,0,0,-0.5), l(0.5,0,0,0.5)
3347: ieq r[26].xyz, r[15].wwww, l(2,3,4,0)
3357: mad r[24].xyz, r[24].xyzx, l(0.5,0.5,-0.5,0), l(0.5,0.5,0.5,0)
3372: div r[16].w, r[21].y, -r[21].z
3380: mad r[24].w, r[16].w, l(-0.5), l(0.5)
3389: movc r[21].yz, r[26].zzzz, r[24].yyzy, r[24].yywy
3398: movc r[21].xy, r[26].yyyy, r[21].xwxx, r[21].yzyy
3407: mov r[25].x, r[24].x
3412: movc r[22].xy, r[26].xxxx, r[25].xyxx, r[21].xyxx
3421: else 
3422: div r[24].xy, r[21].zyzz, -r[21].xxxx
3430: mad r[22].xy, r[24].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
3445: endif 
3446: else 
3447: div r[16].w, -r[21].z, r[21].x
3455: mad r[22].x, r[16].w, l(0.5), l(0.5)
3464: div r[16].w, r[21].y, r[21].x
3471: mad r[22].y, r[16].w, l(-0.5), l(0.5)
3480: endif 
3481: add r[21].xy, r[22].xyxx, l(-0.5,-0.5,0,0)
3491: mad r[21].yz, r[21].xxyx, r[19].wwww, l(0,0.5,0.5,0)
3503: utof r[15].w, r[15].w
3508: mul r[15].w, r[15].w, l(0.166666672)
3515: mad r[21].x, r[21].y, l(0.166666672), r[15].w
3524: mad r[21].xy, r[21].xzxx, r[27].zwzz, r[27].xyxx
3533: sample_l r[21].xy, r[21].xyxx, t[6][6].xyzw, s[1][6], l(0)
3546: add r[15].w, r[23].w, l(-0.0500000007)
3553: mul r[16].w, r[22].z, r[15].w
3560: mad r[17].w, -r[21].x, r[21].x, r[21].y
3570: max r[17].w, r[17].w, l(9.99999975e-06)
3577: mad r[15].w, -r[15].w, r[22].z, r[21].x
3587: mad r[15].w, r[15].w, r[15].w, r[17].w
3596: div r[15].w, r[17].w, r[15].w
3603: min r[15].w, r[15].w, l(1)
3610: lt r[16].w, r[16].w, r[21].x
3617: movc r[15].w, r[16].w, l(1), r[15].w
3626: mul r[20].xyz, r[15].wwww, r[20].xyzx
3633: endif 
3634: mad r[19].xyz, r[19].xyzx, r[18].wwww, r[11].xyzx
3643: dp3 r[15].w, r[19].xyzx, r[19].xyzx
3650: rsq r[15].w, r[15].w
3655: mul r[19].xyz, r[15].wwww, r[19].xyzx
3662: dp3 r[15].w, r[11].xyzx, r[19].xyzx
3669: dp3_sat r[16].w, r[13].xyzx, r[23].xyzx
3676: add r[15].w, -abs(r[15].w), l(1)
3684: max r[15].w, r[15].w, l(0)
3691: mul r[17].w, r[15].w, r[15].w
3698: mul r[17].w, r[17].w, r[17].w
3705: mul r[15].w, r[15].w, r[17].w
3712: mad r[15].w, r[11].w, r[15].w, r[7].w
3721: add_sat r[15].w, -r[15].w, l(1)
3729: mul r[15].w, r[16].w, r[15].w
3736: mul r[15].w, r[15].w, l(0.318309873)
3743: mad r[14].xyz, r[15].wwww, r[20].xyzx, r[14].xyzx
3752: iadd r[13].w, r[13].w, l(1)
3759: iadd r[6].w, r[6].y, r[13].w
3766: ld r[14].w, r[6].xwzz, t[17][22].yzwx
3774: endloop 
3775: ld r[6].w, r[6].xyzz, t[18][23].yzwx
3783: mul r[9].xyz, r[12].zzzz, r[13].xyzx
3790: mad r[8].xyz, r[9].xyzx, l(0.000300000014,0.000300000014,0.000300000014,0), r[12].xyzx
3802: mov r[19].xzw, r[6].xxzz
3807: mov r[9].xyz, r[14].xyzx
3812: mov r[6].x, l(0)
3817: mov r[10].w, r[6].w
3822: loop 
3823: ine r[13].w, r[10].w, l(9.18340949e-41)
3830: ilt r[14].w, r[6].x, l(256)
3837: and r[13].w, r[13].w, r[14].w
3844: breakc r[13].w
3847: iadd r[6].x, r[6].x, l(1)
3854: iadd r[19].y, r[6].x, r[6].y
3861: ld r[13].w, r[19].xyzw, t[18][23].yzwx
3869: ld_structured r[20].xyzw, r[10].w, l(68), t[11][11].xyzw
3879: ld_structured r[21].xyz, r[10].w, l(84), t[11][11].xyzx
3889: ld_structured r[22].xyzw, r[10].w, l(96), t[11][11].xyzw
3899: ld_structured r[23].xyzw, r[10].w, l(112), t[11][11].xyzw
3909: ld_structured r[24].xyzw, r[10].w, l(128), t[11][11].xyzw
3919: mov r[25].x, r[20].w
3924: mov r[25].y, r[22].x
3929: mov r[25].z, r[23].x
3934: mov r[25].w, r[24].x
3939: dp4 r[25].x, r[25].xyzw, r[8].xyzw
3946: mov r[26].x, r[21].x
3951: mov r[26].y, r[22].y
3956: mov r[26].z, r[23].y
3961: mov r[26].w, r[24].y
3966: dp4 r[25].y, r[26].xyzw, r[8].xyzw
3973: mov r[26].x, r[21].y
3978: mov r[26].y, r[22].z
3983: mov r[26].z, r[23].z
3988: mov r[26].w, r[24].z
3993: dp4 r[25].z, r[26].xyzw, r[8].xyzw
4000: mov r[24].x, r[21].z
4005: mov r[24].y, r[22].w
4010: mov r[24].z, r[23].w
4015: dp4 r[14].w, r[24].xyzw, r[8].xyzw
4022: rcp r[15].w, r[14].w
4027: mul r[21].xyz, r[15].wwww, r[25].xyzx
4034: mad r[22].xyz, r[21].xyzx, l(0.5,0.5,1,0), l(0.5,0.5,0,0)
4049: mov_sat r[23].xyz, r[22].xyzx
4054: eq r[22].xyz, r[22].xyzx, r[23].xyzx
4061: and r[15].w, r[22].y, r[22].x
4068: and r[15].w, r[22].z, r[15].w
4075: if r[15].w
4078: ld_structured r[22].xyzw, r[10].w, l(0), t[11][11].xyzw
4088: ld_structured r[15].w, r[10].w, l(156), t[11][11].xxxx
4098: ld_structured r[16].w, r[10].w, l(200), t[11][11].xxxx
4108: ld_structured r[23].xyzw, r[10].w, l(288), t[11][11].xyzw
4118: ld_structured r[17].w, r[10].w, l(304), t[11][11].xxxx
4128: mad r[24].xy, r[21].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
4143: and r[24].zw, r[17].wwww, l(0,0,2,768)
4153: if r[24].z
4156: ld_structured r[26].xyzw, r[10].w, l(272), t[11][11].xyzw
4166: mad r[25].xy, r[24].xyxx, r[26].zwzz, r[26].xyxx
4175: sample_l r[25].xy, r[25].xyxx, t[9][9].xyzw, s[1][6], l(0)
4188: add r[26].xyz, -r[12].xyzx, r[22].xyzx
4196: dp3 r[18].w, r[26].xyzx, r[26].xyzx
4203: sqrt r[18].w, r[18].w
4208: add r[18].w, -r[22].w, r[18].w
4216: mul r[19].y, r[15].w, r[18].w
4223: mad r[20].w, -r[25].x, r[25].x, r[25].y
4233: max r[20].w, r[20].w, l(9.99999975e-06)
4240: mad r[18].w, -r[18].w, r[15].w, r[25].x
4250: mad r[18].w, r[18].w, r[18].w, r[20].w
4259: div r[18].w, r[20].w, r[18].w
4266: min r[18].w, r[18].w, l(1)
4273: lt r[19].y, r[19].y, r[25].x
4280: movc r[18].w, r[19].y, l(1), r[18].w
4289: else 
4290: mov r[18].w, l(1)
4295: endif 
4296: ieq r[25].xy, r[24].wwww, l(256,512,0,0)
4306: if r[25].x
4309: mad r[25].xw, r[24].xxxy, r[23].zzzw, r[23].xxxy
4318: sample_l r[19].y, r[25].xwxx, t[8][8].yxzw, s[1][6], l(0)
4331: ge r[19].y, r[19].y, r[21].z
4338: and r[19].y, r[19].y, l(1)
4345: else 
4346: ieq r[20].w, r[24].w, l(512)
4353: if r[20].w
4356: mad r[21].zw, r[24].xxxy, r[23].zzzw, r[23].xxxy
4365: sample_l r[21].zw, r[21].zwzz, t[9][9].zwxy, s[1][6], l(0)
4378: add r[23].xyz, -r[12].xyzx, r[22].xyzx
4386: dp3 r[20].w, r[23].xyzx, r[23].xyzx
4393: sqrt r[20].w, r[20].w
4398: add r[20].w, -r[22].w, r[20].w
4406: mul r[22].w, r[15].w, r[20].w
4413: mad r[21].w, -r[21].z, r[21].z, r[21].w
4423: max r[21].w, r[21].w, l(9.99999975e-06)
4430: mad r[15].w, -r[20].w, r[15].w, r[21].z
4440: mad r[15].w, r[15].w, r[15].w, r[21].w
4449: div r[15].w, r[21].w, r[15].w
4456: min r[15].w, r[15].w, l(1)
4463: lt r[20].w, r[22].w, r[21].z
4470: movc r[19].y, r[20].w, l(1), r[15].w
4479: else 
4480: mov r[19].y, l(1)
4485: endif 
4486: endif 
4487: ine r[15].w, r[24].z, l(0)
4494: and r[15].w, r[15].w, r[25].y
4501: add r[20].w, -r[18].w, r[19].y
4509: mad r[20].w, r[16].w, r[20].w, r[18].w
4518: mad r[19].y, r[19].y, r[16].w, l(1)
4527: add r[16].w, -r[16].w, r[19].y
4535: min r[16].w, r[16].w, r[18].w
4542: movc r[15].w, r[15].w, r[20].w, r[16].w
4551: ne r[16].w, r[15].w, l(0)
4558: if r[16].w
4561: ld_structured r[23].xyz, r[10].w, l(16), t[11][11].xyzx
4571: ld_structured r[26].xyzw, r[10].w, l(48), t[11][11].xyzw
4581: add r[16].w, -r[25].z, r[26].w
4589: mul r[18].w, r[26].w, l(0.200000003)
4596: div_sat r[16].w, r[16].w, r[18].w
4603: mad r[18].w, r[16].w, l(-2), l(3)
4612: mul r[16].w, r[16].w, r[16].w
4619: mul r[16].w, r[16].w, r[18].w
4626: add r[23].xyz, -r[12].xyzx, r[23].xyzx
4634: dp3 r[18].w, r[23].xyzx, r[23].xyzx
4641: mul r[19].y, r[20].y, r[20].y
4648: mad r[18].w, r[20].z, r[18].w, r[19].y
4657: max r[18].w, r[18].w, l(0.25)
4664: mul r[18].w, r[18].w, l(3.14159274)
4671: rcp r[18].w, r[18].w
4676: add r[18].w, -r[20].x, r[18].w
4684: max r[18].w, r[18].w, l(0)
4691: mul r[16].w, r[16].w, r[18].w
4698: mul r[20].xyz, r[16].wwww, r[26].xyzx
4705: and r[16].w, r[17].w, l(1)
4712: if r[16].w
4715: ld_structured r[23].xyzw, r[10].w, l(256), t[11][11].xyzw
4725: and r[16].w, r[17].w, l(1.43492963e-42)
4732: add r[21].zw, r[21].xxxy, r[21].xxxy
4739: mad r[17].w, r[21].x, r[21].x, l(1)
4748: mad r[17].w, r[21].y, r[21].y, r[17].w
4757: div r[21].xy, r[21].zwzz, r[17].wwww
4764: mad r[21].xy, r[21].xyxx, l(0.5,-0.5,0,0), l(0.5,0.5,0,0)
4779: movc r[21].xy, r[16].wwww, r[21].xyxx, r[24].xyxx
4788: mad r[21].xy, r[21].xyxx, r[23].zwzz, r[23].xyxx
4797: sample_l r[21].xyz, r[21].xyxx, t[10][10].xyzw, s[2][11], l(0)
4810: mul r[20].xyz, r[20].xyzx, r[21].xyzx
4817: endif 
4818: ge r[14].w, r[14].w, r[25].z
4825: and r[14].w, r[14].w, l(1)
4832: mul r[14].w, r[14].w, r[15].w
4839: mul r[20].xyz, r[14].wwww, r[20].xyzx
4846: add r[21].xyz, -r[12].xyzx, r[22].xyzx
4854: dp3 r[14].w, r[21].xyzx, r[21].xyzx
4861: rsq r[14].w, r[14].w
4866: mul r[22].xyz, r[14].wwww, r[21].xyzx
4873: mad r[21].xyz, r[21].xyzx, r[14].wwww, r[11].xyzx
4882: dp3 r[14].w, r[21].xyzx, r[21].xyzx
4889: rsq r[14].w, r[14].w
4894: mul r[21].xyz, r[14].wwww, r[21].xyzx
4901: dp3 r[14].w, r[11].xyzx, r[21].xyzx
4908: dp3_sat r[15].w, r[13].xyzx, r[22].xyzx
4915: add r[14].w, -abs(r[14].w), l(1)
4923: max r[14].w, r[14].w, l(0)
4930: mul r[16].w, r[14].w, r[14].w
4937: mul r[16].w, r[16].w, r[16].w
4944: mul r[14].w, r[14].w, r[16].w
4951: mad r[14].w, r[11].w, r[14].w, r[7].w
4960: add_sat r[14].w, -r[14].w, l(1)
4968: mul r[14].w, r[15].w, r[14].w
4975: mul r[14].w, r[14].w, l(0.318309873)
4982: mad r[9].xyz, r[14].wwww, r[20].xyzx, r[9].xyzx
4991: endif 
4992: endif 
4993: mov r[10].w, r[13].w
4998: endloop 
4999: mad r[4].xyz, r[4].xyzx, l(0.318309873,0.318309873,0.318309873,0), r[9].xyzx
5011: mul r[15].xyz, r[17].xyzx, r[4].xyzx
5018: else 
5019: mov r[15].xyz, l(0,0,0,0)
5027: endif 
5028: endif 
5029: mad r[10].xyz, r[3].xyzx, r[15].xyzx, r[10].xyzx
5038: iadd r[0].z, r[0].z, l(1)
5045: endloop 
5046: add r[0].xy, -cb[1][1][17].yzyy, cb[1][1][17].xwxx
5058: add r[0].z, cb[1][1][17].y, cb[1][1][17].x
5069: add r[0].z, -r[0].x, r[0].z
5077: add r[0].x, -r[0].x, r[1].w
5085: div r[0].z, l(1,1,1,1), r[0].z
5095: mul_sat r[0].x, r[0].z, r[0].x
5102: mad r[0].z, r[0].x, l(-2), l(3)
5111: mul r[0].x, r[0].x, r[0].x
5118: mul r[0].x, r[0].x, r[0].z
5125: mad r[0].x, r[0].x, r[0].y, cb[1][1][17].z
5136: mul r[0].xyz, r[0].xxxx, r[10].xyzx
5143: itof r[0].w, cb[4][4][0].z
5150: div r[1].xyz, r[0].xyzx, r[0].wwww
5157: div r[0].x, r[5].w, r[0].w
5164: mov r[1].w, l(1)
5169: else 
5170: mov r[1].xyzw, l(0,0,0,0)
5178: mov r[0].x, l(65504)
5183: endif 
5184: store_uav_typed type30[1][1].xyzw, type32.xyyy, r[0].xxxx
5191: store_uav_typed type30[0][0].xyzw, type32.xyyy, r[1].xyzw
5198: ret 