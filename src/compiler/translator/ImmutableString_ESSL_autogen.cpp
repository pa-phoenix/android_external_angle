// GENERATED FILE - DO NOT EDIT.
// Generated by gen_builtin_symbols.py using data from builtin_variables.json and
// builtin_function_declarations.txt.
//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ImmutableString_ESSL_autogen.cpp: Wrapper for static or pool allocated char arrays, that are
// guaranteed to be valid and unchanged for the duration of the compilation. Implements
// mangledNameHash using perfect hash function from gen_builtin_symbols.py

#include "compiler/translator/ImmutableString.h"

std::ostream &operator<<(std::ostream &os, const sh::ImmutableString &str)
{
    return os.write(str.data(), str.length());
}

#if defined(_MSC_VER)
#    pragma warning(disable : 4309)  // truncation of constant value
#endif

namespace
{

constexpr int mangledkT1[] = {33,   1446, 1611, 1780, 1507, 937,  983,  546,  92,  1884,
                              1735, 1433, 510,  683,  1381, 1792, 856,  1603, 990, 821,
                              537,  88,   1300, 451,  195,  1506, 1410, 159,  346, 186,
                              1746, 1061, 651,  1878, 961,  1349, 1389};
constexpr int mangledkT2[] = {922,  1311, 1706, 1401, 1291, 1445, 1607, 152, 1095, 948,
                              1236, 332,  611,  1126, 1453, 449,  160,  754, 23,   1299,
                              1345, 1028, 897,  734,  273,  563,  160,  231, 440,  1001,
                              1509, 1657, 1545, 646,  294,  1125, 1260};
constexpr int mangledkG[]  = {
    0,    0,    0,    0,    0,    0,    1280, 0,    0,    0,    0,    1657, 1176, 1694, 0,    0,
    1026, 0,    857,  0,    0,    207,  0,    0,    0,    0,    1118, 0,    0,    0,    139,  0,
    0,    0,    655,  1864, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    1178, 193,  0,    0,    321,  0,    0,    0,    0,    173,  233,
    174,  603,  153,  0,    0,    0,    0,    0,    0,    769,  337,  0,    1718, 0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    467,  1625, 1858, 0,    0,    0,    1124,
    0,    0,    0,    0,    1324, 0,    0,    294,  0,    1066, 0,    0,    1805, 0,    1770, 1318,
    638,  0,    0,    712,  0,    0,    0,    0,    845,  0,    0,    983,  0,    509,  0,    0,
    0,    0,    0,    0,    1628, 1366, 0,    1265, 0,    0,    0,    0,    0,    0,    944,  0,
    0,    0,    0,    1674, 0,    0,    262,  1387, 534,  0,    0,    0,    0,    0,    232,  0,
    0,    0,    0,    424,  0,    97,   203,  225,  0,    463,  0,    1068, 1811, 98,   0,    317,
    1126, 0,    206,  0,    0,    0,    0,    0,    1716, 1188, 104,  1304, 1729, 0,    0,    0,
    0,    0,    0,    779,  0,    0,    0,    1303, 0,    0,    873,  0,    713,  0,    0,    142,
    0,    0,    0,    0,    0,    0,    0,    987,  0,    0,    1670, 588,  0,    0,    0,    0,
    0,    0,    160,  0,    418,  831,  360,  0,    1372, 0,    0,    1766, 0,    0,    863,  0,
    0,    0,    1033, 0,    0,    1102, 0,    927,  359,  78,   309,  0,    56,   0,    61,   0,
    0,    0,    0,    1286, 0,    0,    0,    133,  168,  0,    0,    1669, 0,    135,  310,  55,
    763,  0,    767,  1268, 1247, 0,    1677, 0,    800,  470,  0,    0,    0,    221,  0,    0,
    0,    0,    0,    0,    0,    0,    323,  174,  1846, 206,  0,    518,  0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    1649, 0,    0,    189,  0,    1454, 1850, 0,    310,  1145,
    1500, 729,  0,    239,  1236, 0,    0,    0,    0,    16,   515,  1811, 0,    315,  0,    0,
    0,    0,    171,  0,    0,    0,    824,  0,    0,    0,    0,    0,    379,  1010, 0,    0,
    1346, 0,    1677, 1048, 0,    0,    244,  0,    0,    0,    1581, 1545, 1564, 821,  1774, 570,
    0,    0,    0,    0,    764,  0,    0,    0,    0,    836,  0,    0,    0,    235,  0,    747,
    0,    0,    0,    304,  0,    0,    1023, 0,    1199, 1303, 0,    0,    0,    1600, 492,  1571,
    0,    531,  0,    1092, 1576, 0,    0,    906,  0,    743,  0,    0,    384,  291,  0,    6,
    0,    950,  1422, 0,    0,    0,    105,  781,  0,    0,    69,   0,    0,    0,    881,  0,
    0,    1547, 683,  0,    0,    1732, 1710, 0,    847,  0,    0,    0,    0,    527,  1405, 0,
    0,    0,    0,    175,  0,    662,  0,    43,   0,    1258, 0,    0,    1267, 0,    0,    0,
    0,    1600, 1663, 0,    0,    0,    149,  0,    773,  989,  0,    625,  0,    593,  1295, 0,
    10,   0,    1600, 0,    0,    0,    0,    502,  764,  0,    847,  0,    0,    770,  1772, 0,
    12,   0,    0,    251,  0,    0,    0,    0,    25,   0,    650,  0,    1439, 0,    660,  0,
    1799, 0,    1325, 0,    965,  852,  660,  0,    905,  52,   1344, 634,  0,    178,  14,   363,
    0,    835,  0,    0,    0,    677,  536,  0,    64,   0,    0,    0,    0,    0,    1191, 0,
    0,    0,    0,    0,    0,    634,  1839, 0,    199,  874,  1789, 275,  0,    0,    0,    195,
    0,    0,    0,    0,    0,    0,    0,    1151, 0,    0,    0,    630,  0,    145,  948,  215,
    1876, 588,  1088, 0,    1059, 721,  0,    0,    825,  1114, 0,    0,    761,  0,    0,    0,
    91,   0,    0,    104,  0,    262,  0,    0,    0,    0,    0,    0,    293,  0,    0,    0,
    10,   101,  163,  124,  0,    0,    1845, 1110, 1491, 454,  0,    0,    0,    186,  534,  0,
    1214, 0,    0,    392,  873,  0,    0,    368,  1262, 0,    1313, 1569, 558,  22,   0,    0,
    226,  123,  0,    0,    0,    418,  1723, 701,  962,  969,  1807, 529,  1474, 770,  0,    0,
    0,    1109, 0,    0,    0,    0,    1816, 0,    0,    1577, 1110, 823,  0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    783,  905,  1580, 0,    1083, 0,    11,   686,  0,    956,
    0,    1644, 330,  0,    0,    0,    0,    0,    0,    0,    0,    687,  0,    467,  79,   421,
    0,    0,    490,  0,    797,  0,    126,  0,    824,  0,    0,    715,  0,    589,  0,    113,
    0,    131,  1593, 0,    327,  593,  1603, 1152, 0,    1767, 892,  0,    147,  194,  0,    0,
    0,    908,  636,  825,  1866, 1685, 459,  0,    132,  897,  1325, 0,    1873, 0,    141,  0,
    839,  918,  888,  186,  0,    884,  666,  0,    0,    237,  0,    0,    0,    1778, 1113, 1357,
    0,    775,  0,    151,  0,    0,    955,  200,  0,    746,  281,  468,  665,  0,    579,  1282,
    879,  0,    1094, 895,  1309, 418,  74,   166,  0,    0,    0,    0,    443,  1779, 0,    0,
    0,    0,    110,  451,  780,  969,  0,    0,    1632, 0,    0,    413,  0,    0,    724,  0,
    1784, 0,    0,    373,  0,    106,  693,  894,  1053, 1874, 750,  0,    0,    0,    1145, 6,
    281,  383,  0,    0,    7,    1342, 619,  1577, 343,  913,  420,  0,    0,    0,    0,    696,
    506,  0,    0,    925,  900,  0,    860,  1265, 521,  609,  381,  0,    1474, 463,  697,  0,
    0,    990,  0,    431,  0,    404,  0,    0,    0,    0,    0,    1654, 0,    594,  0,    0,
    0,    1861, 268,  0,    0,    811,  0,    1706, 0,    0,    0,    0,    897,  635,  0,    70,
    0,    0,    1195, 568,  587,  477,  189,  891,  776,  0,    0,    1537, 0,    0,    191,  1116,
    969,  0,    148,  0,    944,  752,  0,    779,  0,    0,    1786, 14,   854,  1631, 0,    909,
    1058, 92,   0,    0,    331,  0,    0,    829,  0,    0,    388,  992,  750,  1519, 0,    0,
    0,    1578, 0,    241,  908,  0,    0,    137,  721,  0,    0,    947,  678,  0,    1766, 1352,
    831,  0,    547,  1745, 699,  0,    1281, 0,    0,    894,  0,    574,  192,  1270, 0,    754,
    0,    1819, 0,    0,    20,   871,  1746, 0,    886,  1308, 1628, 0,    794,  0,    63,   1413,
    1826, 0,    0,    1740, 0,    1441, 0,    375,  1054, 436,  326,  1649, 0,    0,    0,    0,
    0,    0,    831,  0,    263,  0,    716,  929,  921,  0,    1394, 0,    0,    475,  232,  1843,
    515,  0,    0,    794,  0,    0,    1624, 0,    696,  981,  207,  870,  210,  1827, 0,    0,
    0,    1218, 0,    539,  0,    77,   0,    600,  502,  1514, 840,  1413, 892,  910,  0,    0,
    371,  0,    1865, 260,  72,   380,  0,    575,  150,  1723, 0,    931,  809,  1015, 0,    567,
    1805, 0,    557,  337,  295,  125,  573,  0,    1430, 730,  144,  0,    0,    0,    0,    0,
    1147, 0,    260,  996,  0,    0,    0,    0,    780,  0,    285,  908,  0,    0,    352,  0,
    1203, 0,    0,    1324, 0,    0,    1736, 562,  0,    0,    1150, 1596, 252,  0,    1095, 0,
    0,    1250, 591,  1854, 408,  1271, 0,    1835, 762,  0,    0,    0,    152,  315,  524,  1127,
    328,  257,  0,    1751, 177,  0,    1841, 874,  887,  425,  0,    0,    220,  0,    819,  381,
    0,    0,    0,    1694, 1697, 0,    992,  16,   244,  1768, 0,    0,    746,  0,    1512, 0,
    23,   0,    0,    1021, 0,    993,  0,    923,  0,    0,    1799, 1772, 731,  1489, 914,  1233,
    0,    1254, 442,  155,  0,    0,    552,  0,    0,    1224, 287,  876,  0,    0,    0,    677,
    0,    357,  327,  483,  1495, 643,  1827, 453,  1491, 0,    456,  0,    924,  0,    1428, 0,
    0,    460,  694,  0,    0,    1053, 136,  0,    0,    0,    34,   1732, 433,  118,  1577, 378,
    1702, 351,  727,  1877, 628,  649,  1356, 129,  174,  1817, 253,  1264, 0,    626,  0,    1651,
    125,  271,  0,    458,  0,    952,  0,    1853, 861,  0,    229,  1295, 0,    195,  0,    1869,
    250,  1480, 0,    0,    792,  1324, 1147, 1854, 0,    0,    124,  384,  394,  0,    1539, 830,
    28,   46,   990,  0,    949,  0,    0,    0,    356,  0,    695,  0,    143,  340,  1262, 0,
    1,    0,    1810, 438,  0,    0,    1850, 0,    13,   0,    0,    244,  1710, 0,    0,    0,
    0,    943,  0,    0,    790,  1812, 598,  0,    0,    1150, 71,   1375, 48,   978,  354,  0,
    0,    184,  42,   0,    578,  0,    589,  0,    0,    707,  0,    689,  1676, 0,    565,  0,
    723,  0,    0,    561,  0,    311,  82,   629,  0,    0,    35,   139,  274,  1228, 0,    780,
    0,    0,    0,    145,  1026, 0,    242,  588,  0,    1388, 538,  302,  336,  544,  0,    0,
    1429, 234,  477,  0,    1381, 403,  12,   113,  0,    20,   0,    0,    0,    1814, 1475, 22,
    177,  1217, 180,  1857, 804,  0,    62,   503,  0,    834,  788,  0,    1619, 570,  349,  100,
    272,  537,  236,  1222, 0,    470,  1514, 1213, 1288, 997,  938,  0,    768,  0,    0,    675,
    757,  0,    1333, 571,  0,    181,  0,    1282, 1205, 0,    0,    1878, 998,  0,    983,  0,
    365,  793,  26,   176,  104,  722,  160,  666,  0,    111,  0,    880,  0,    1321, 533,  0,
    492,  0,    0,    21,   0,    574,  1179, 847,  832,  532,  281,  1178, 0,    633,  0,    1123,
    0,    0,    1502, 1740, 1875, 209,  0,    541,  0,    774,  101,  0,    0,    0,    880,  732,
    0,    441,  0,    0,    451,  595,  975,  0,    170,  0,    156,  0,    314,  191,  497,  0,
    0,    1784, 1679, 1821, 283,  231,  629,  0,    892,  0,    890,  1555, 0,    50,   0,    3,
    0,    0,    0,    1870, 268,  709,  0,    361,  0,    1388, 1325, 0,    102,  483,  779,  992,
    937,  0,    0,    977,  0,    0,    689,  239,  0,    1812, 116,  357,  0,    967,  714,  1291,
    540,  0,    297,  190,  719,  913,  564,  853,  948,  70,   0,    764,  0,    0,    1743, 264,
    0,    544,  263,  1328, 0,    0,    514,  1686, 1239, 933,  468,  663,  445,  0,    0,    828,
    0,    1337, 0,    84,   0,    1799, 0,    0,    1545, 693,  0,    1311, 613,  0,    457,  0,
    0,    1825, 419,  564,  763,  0,    739,  0,    0,    47,   720,  497,  563,  0,    580,  1879,
    775,  0,    409,  1849, 837,  1262, 0,    945,  0,    687,  300,  0,    265,  84,   9,    0,
    0,    0,    1156, 611,  531,  0,    565,  0,    108,  0,    261,  1599, 1250, 1868, 153,  1809,
    0,    0,    949,  0,    0,    912,  303,  0,    0,    475,  316,  1614, 0,    0,    345,  1660,
    1768, 1863, 1844, 1880, 883,  1311, 1331, 1212, 978,  996,  746,  953,  1065, 0,    0,    1380,
    263,  1322, 549,  376,  0,    1556, 627,  700,  262,  0,    0,    841,  97,   219,  1545, 113,
    0,    1446, 0,    1656, 0,    280,  1030, 833,  960,  277,  1733, 1726, 1305, 0,    0,    167,
    226,  0,    133,  0,    0,    1324, 657,  1860, 818,  1456, 826,  752,  0,    0,    0,    0,
    0,    884,  293,  0,    0,    212,  0,    472,  680,  1348, 238,  0,    327,  646,  0,    0,
    1649, 224,  98,   0,    310,  0,    494,  193,  1352, 431,  207,  0,    0,    385,  0,    1346,
    0,    499,  503,  869,  0,    0,    1790, 1688, 289,  385,  279,  0,    18,   0,    1075, 0,
    597,  0,    0,    0,    0,    0,    0,    0,    0,    0,    1309, 0,    0,    796,  0,    1754,
    0,    1216, 0,    886,  0,    0,    641,  1779, 0,    870,  660,  1869, 667,  872,  1404, 1401,
    2,    0,    1172, 498,  1348, 0,    0,    0,    1858, 520,  1579, 75,   0,    1709, 1261, 1860,
    1646, 177,  0,    44,   1701, 0,    948,  0,    0,    328,  0,    172,  1816, 505,  0,    0,
    0,    0,    402,  0,    0,    0,    755,  1642, 1810, 224,  1834, 738,  0,    0,    0,    1716,
    1565, 0,    1677, 485,  1766, 1771, 0,    762,  994,  1756, 0,    0,    0,    389,  0,    83,
    127,  905,  0,    156,  0,    823,  866,  902,  481,  1807, 1885, 0,    74,   520,  320,  0};

int MangledHashG(const char *key, const int *T)
{
    int sum = 0;

    for (int i = 0; key[i] != '\0'; i++)
    {
        sum += T[i] * key[i];
        sum %= 1888;
    }
    return mangledkG[sum];
}

int MangledPerfectHash(const char *key)
{
    if (strlen(key) > 37)
        return 0;

    return (MangledHashG(key, mangledkT1) + MangledHashG(key, mangledkT2)) % 1888;
}

constexpr int unmangledkT1[] = {125, 134, 144, 142, 7,   31,  31,  182, 204, 204, 167, 65, 77,
                                132, 129, 162, 50,  103, 127, 240, 136, 183, 181, 82,  65, 136};
constexpr int unmangledkT2[] = {229, 104, 57, 237, 84, 219, 208, 217, 243, 216, 172, 34, 112,
                                93,  122, 69, 89,  42, 13,  36,  33,  103, 48,  226, 43, 99};
constexpr int unmangledkG[]  = {
    0,   65,  0,   0,   0,   0,   0,   0,   183, 207, 0,   83,  0,   68,  0,   0,   0,   0,   0,
    215, 0,   57,  0,   0,   0,   0,   96,  0,   0,   206, 16,  242, 0,   191, 0,   0,   12,  60,
    195, 189, 0,   0,   163, 0,   0,   6,   0,   75,  0,   0,   177, 67,  0,   203, 0,   0,   0,
    224, 28,  241, 8,   238, 25,  160, 94,  3,   34,  0,   0,   36,  22,  0,   70,  0,   75,  0,
    210, 0,   87,  0,   189, 86,  0,   0,   0,   0,   0,   0,   0,   205, 0,   60,  0,   0,   0,
    196, 158, 219, 90,  71,  0,   0,   139, 82,  147, 90,  0,   0,   141, 10,  126, 57,  91,  161,
    80,  18,  40,  78,  203, 106, 0,   81,  225, 77,  190, 225, 76,  0,   223, 0,   94,  41,  0,
    0,   175, 38,  0,   0,   174, 77,  0,   119, 150, 69,  14,  98,  88,  12,  0,   0,   144, 49,
    0,   181, 123, 0,   3,   33,  0,   0,   141, 108, 30,  201, 7,   214, 176, 94,  231, 0,   128,
    160, 0,   157, 142, 52,  0,   30,  0,   43,  10,  26,  81,  29,  214, 201, 220, 142, 0,   176,
    0,   0,   53,  0,   178, 23,  0,   99,  0,   142, 43,  39,  140, 175, 19,  31,  160, 66,  103,
    105, 0,   157, 66,  99,  0,   91,  124, 83,  0,   44,  154, 55,  102, 0,   238, 92,  0,   0,
    82,  131, 114, 0,   0,   79,  125, 0,   68,  0,   117, 146, 57,  0,   117, 53,  118, 35};

int UnmangledHashG(const char *key, const int *T)
{
    int sum = 0;

    for (int i = 0; key[i] != '\0'; i++)
    {
        sum += T[i] * key[i];
        sum %= 246;
    }
    return unmangledkG[sum];
}

int UnmangledPerfectHash(const char *key)
{
    if (strlen(key) > 26)
        return 0;

    return (UnmangledHashG(key, unmangledkT1) + UnmangledHashG(key, unmangledkT2)) % 246;
}

}  // namespace

namespace sh
{

template <>
const size_t ImmutableString::FowlerNollVoHash<4>::kFnvPrime = 16777619u;

template <>
const size_t ImmutableString::FowlerNollVoHash<4>::kFnvOffsetBasis = 0x811c9dc5u;

template <>
const size_t ImmutableString::FowlerNollVoHash<8>::kFnvPrime =
    static_cast<size_t>(1099511628211ull);

template <>
const size_t ImmutableString::FowlerNollVoHash<8>::kFnvOffsetBasis =
    static_cast<size_t>(0xcbf29ce484222325ull);

uint32_t ImmutableString::mangledNameHash() const
{
    return MangledPerfectHash(data());
}

uint32_t ImmutableString::unmangledNameHash() const
{
    return UnmangledPerfectHash(data());
}

}  // namespace sh
