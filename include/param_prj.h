/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file contains all parameters used in your project
 * See main.cpp on how to access them.
 * If a parameters unit is of format "0=Choice, 1=AnotherChoice" etc.
 * It will be displayed as a dropdown in the web interface
 * If it is a spot value, the decimal is translated to the name, i.e. 0 becomes "Choice"
 * If the enum values are powers of two, they will be displayed as flags, example
 * "0=None, 1=Flag1, 2=Flag2, 4=Flag3, 8=Flag4" and the value is 5.
 * It means that Flag1 and Flag3 are active -> Display "Flag1 | Flag3"
 *
 * Every parameter/value has a unique ID that must never change. This is used when loading parameters
 * from flash, so even across firmware versions saved parameters in flash can always be mapped
 * back to our list here. If a new value is added, it will receive its default value
 * because it will not be found in flash.
 * The unique ID is also used in the CAN module, to be able to recover the CAN map
 * no matter which firmware version saved it to flash.
 * Make sure to keep track of your ids and avoid duplicates. Also don't re-assign
 * IDs from deleted parameters because you will end up loading some random value
 * into your new parameter!
 * IDs are 16 bit, so 65535 is the maximum
 */

//Define a version string of your firmware here
#define VER 0.03.A

/* Entries must be ordered as follows:
   1. Saveable parameters (id != 0)
   2. Temporary parameters (id = 0)
   3. Display values
 */
//Next param id (increase when adding new parameter!): 8
//Next value Id: 2126
/*              category     name         unit       min     max     default id */
#define PARAM_LIST \
    PARAM_ENTRY(CAT_BMS,     type,        TYPES,     0,      1,      0,      1   ) \
    PARAM_ENTRY(CAT_BMS,     numbmbs,     "",        1,      4,      1,      2   ) \
    PARAM_ENTRY(CAT_BMS,     balance,     OFFON,     0,      1,      0,      3   ) \
    PARAM_ENTRY(CAT_BMS,     nomcap,      "Ah",      0,      1000,   100,    4   ) \
    PARAM_ENTRY(CAT_BMS,    CellVmax,     "mV",      3000, 4200,   4150,      5   ) \
    PARAM_ENTRY(CAT_BMS,    CellVmin,     "mV",      2800, 3500,   3200,      6   ) \
    PARAM_ENTRY(CAT_SENS,    idcgain,     "dig/A",   -1000,  1000,   10,     7  ) \
    PARAM_ENTRY(CAT_SENS,    idcofs,      "dig",    -4095,   4095,   0,      8   ) \
    PARAM_ENTRY(CAT_SENS,    idcmode,     IDCMODES,  0,      3,      0,      9  ) \
    VALUE_ENTRY(opmode,      OPMODES,2000 ) \
    VALUE_ENTRY(version,     VERSTR, 2001 ) \
    VALUE_ENTRY(soc,         "%",   2002 ) \
    VALUE_ENTRY(chargelim,   "A",    2003 ) \
    VALUE_ENTRY(dischargelim,"A",    2004 ) \
    VALUE_ENTRY(chargeVlim,  "V",    2157 ) \
    VALUE_ENTRY(dischargeVlim,"V",   2158 ) \
    VALUE_ENTRY(deltaV,      "mV",   2005 ) \
    VALUE_ENTRY(udc,         "V",    2006 ) \
    VALUE_ENTRY(idc,         "A",    2007 ) \
    VALUE_ENTRY(TempMax,     "°C",   2008 ) \
    VALUE_ENTRY(TempMin,     "°C",   2156 ) \
    VALUE_ENTRY(uavg,        "mV",   2009 ) \
    VALUE_ENTRY(umin,        "mV",   2010 ) \
    VALUE_ENTRY(CellMin,       "",   2129 ) \
    VALUE_ENTRY(umax,        "mV",   2011 ) \
    VALUE_ENTRY(CellMax,       "",   2130 ) \
    VALUE_ENTRY(u1,          "mV",   2012 ) \
    VALUE_ENTRY(u2,          "mV",   2013 ) \
    VALUE_ENTRY(u3,          "mV",   2014 ) \
    VALUE_ENTRY(u4,          "mV",   2015 ) \
    VALUE_ENTRY(u5,          "mV",   2016 ) \
    VALUE_ENTRY(u6,          "mV",   2017 ) \
    VALUE_ENTRY(u7,          "mV",   2018 ) \
    VALUE_ENTRY(u8,          "mV",   2019 ) \
    VALUE_ENTRY(u9,          "mV",   2020 ) \
    VALUE_ENTRY(u10,         "mV",   2021 ) \
    VALUE_ENTRY(u11,         "mV",   2022 ) \
    VALUE_ENTRY(u12,         "mV",   2023 ) \
    VALUE_ENTRY(u13,         "mV",   2024 ) \
    VALUE_ENTRY(u14,         "mV",   2025 ) \
    VALUE_ENTRY(u15,         "mV",   2026 ) \
    VALUE_ENTRY(u16,         "mV",   2027 ) \
    VALUE_ENTRY(u17,         "mV",   2028 ) \
    VALUE_ENTRY(u18,         "mV",   2029 ) \
    VALUE_ENTRY(u19,         "mV",   2030 ) \
    VALUE_ENTRY(u20,         "mV",   2031 ) \
    VALUE_ENTRY(u21,         "mV",   2032 ) \
    VALUE_ENTRY(u22,         "mV",   2033 ) \
    VALUE_ENTRY(u23,         "mV",   2035 ) \
    VALUE_ENTRY(u24,         "mV",   2036 ) \
    VALUE_ENTRY(u25,         "mV",   2037 ) \
    VALUE_ENTRY(u26,         "mV",   2038 ) \
    VALUE_ENTRY(u27,         "mV",   2039 ) \
    VALUE_ENTRY(u28,         "mV",   2040 ) \
    VALUE_ENTRY(u29,         "mV",   2041 ) \
    VALUE_ENTRY(u30,         "mV",   2042 ) \
    VALUE_ENTRY(u31,         "mV",   2043 ) \
    VALUE_ENTRY(u32,         "mV",   2044 ) \
    VALUE_ENTRY(u33,         "mV",   2045 ) \
    VALUE_ENTRY(u34,         "mV",   2046 ) \
    VALUE_ENTRY(u35,         "mV",   2047 ) \
    VALUE_ENTRY(u36,         "mV",   2048 ) \
    VALUE_ENTRY(u37,         "mV",   2049 ) \
    VALUE_ENTRY(u38,         "mV",   2050 ) \
    VALUE_ENTRY(u39,         "mV",   2051 ) \
    VALUE_ENTRY(u40,         "mV",   2052 ) \
    VALUE_ENTRY(u41,         "mV",   2053 ) \
    VALUE_ENTRY(u42,         "mV",   2054 ) \
    VALUE_ENTRY(u43,         "mV",   2055 ) \
    VALUE_ENTRY(u44,         "mV",   2056 ) \
    VALUE_ENTRY(u45,         "mV",   2057 ) \
    VALUE_ENTRY(u46,         "mV",   2058 ) \
    VALUE_ENTRY(u47,         "mV",   2059 ) \
    VALUE_ENTRY(u48,         "mV",   2060 ) \
    VALUE_ENTRY(u49,         "mV",   2061 ) \
    VALUE_ENTRY(u50,         "mV",   2062 ) \
    VALUE_ENTRY(u51,         "mV",   2063 ) \
    VALUE_ENTRY(u52,         "mV",   2064 ) \
    VALUE_ENTRY(u53,         "mV",   2065 ) \
    VALUE_ENTRY(u54,         "mV",   2066 ) \
    VALUE_ENTRY(u55,         "mV",   2067 ) \
    VALUE_ENTRY(u56,         "mV",   2068 ) \
    VALUE_ENTRY(u57,         "mV",   2069 ) \
    VALUE_ENTRY(u58,         "mV",   2070 ) \
    VALUE_ENTRY(u59,         "mV",   2071 ) \
    VALUE_ENTRY(u60,         "mV",   2072 ) \
    VALUE_ENTRY(u61,         "mV",   2073 ) \
    VALUE_ENTRY(u62,         "mV",   2074 ) \
    VALUE_ENTRY(u63,         "mV",   2075 ) \
    VALUE_ENTRY(u64,         "mV",   2076 ) \
    VALUE_ENTRY(u65,         "mV",   2077 ) \
    VALUE_ENTRY(u66,         "mV",   2078 ) \
    VALUE_ENTRY(u67,         "mV",   2079 ) \
    VALUE_ENTRY(u68,         "mV",   2080 ) \
    VALUE_ENTRY(u69,         "mV",   2081 ) \
    VALUE_ENTRY(u70,         "mV",   2082 ) \
    VALUE_ENTRY(u71,         "mV",   2083 ) \
    VALUE_ENTRY(u72,         "mV",   2084 ) \
    VALUE_ENTRY(u73,         "mV",   2085 ) \
    VALUE_ENTRY(u74,         "mV",   2086 ) \
    VALUE_ENTRY(u75,         "mV",   2087 ) \
    VALUE_ENTRY(u76,         "mV",   2088 ) \
    VALUE_ENTRY(u77,         "mV",   2089 ) \
    VALUE_ENTRY(u78,         "mV",   2090 ) \
    VALUE_ENTRY(u79,         "mV",   2091 ) \
    VALUE_ENTRY(u80,         "mV",   2092 ) \
    VALUE_ENTRY(u81,         "mV",   2093 ) \
    VALUE_ENTRY(u82,         "mV",   2094 ) \
    VALUE_ENTRY(u83,         "mV",   2095 ) \
    VALUE_ENTRY(u84,         "mV",   2096 ) \
    VALUE_ENTRY(u85,         "mV",   2097 ) \
    VALUE_ENTRY(u86,         "mV",   2098 ) \
    VALUE_ENTRY(u87,         "mV",   2099 ) \
    VALUE_ENTRY(u88,         "mV",   2100 ) \
    VALUE_ENTRY(u89,         "mV",   2101 ) \
    VALUE_ENTRY(u90,         "mV",   2102 ) \
    VALUE_ENTRY(u91,         "mV",   2103 ) \
    VALUE_ENTRY(u92,         "mV",   2104 ) \
    VALUE_ENTRY(u93,         "mV",   2105 ) \
    VALUE_ENTRY(u94,         "mV",   2106 ) \
    VALUE_ENTRY(u95,         "mV",   2107 ) \
    VALUE_ENTRY(u96,         "mV",   2108 ) \
    VALUE_ENTRY(u97,         "mV",   2109 ) \
    VALUE_ENTRY(u98,         "mV",   2110 ) \
    VALUE_ENTRY(u99,         "mV",   2111 ) \
    VALUE_ENTRY(u100,        "mV",   2112 ) \
    VALUE_ENTRY(u101,        "mV",   2113 ) \
    VALUE_ENTRY(Cellt0_0,    "°C",   2140 ) \
    VALUE_ENTRY(Cellt0_1,    "°C",   2141 ) \
    VALUE_ENTRY(Cellt1_0,    "°C",   2142 ) \
    VALUE_ENTRY(Cellt1_1,    "°C",   2143 ) \
    VALUE_ENTRY(Cellt2_0,    "°C",   2144 ) \
    VALUE_ENTRY(Cellt2_1,    "°C",   2145 ) \
    VALUE_ENTRY(Cellt3_0,    "°C",   2146 ) \
    VALUE_ENTRY(Cellt3_1,    "°C",   2147 ) \
    VALUE_ENTRY(Cellt4_0,    "°C",   2148 ) \
    VALUE_ENTRY(Cellt4_1,    "°C",   2149 ) \
    VALUE_ENTRY(Cellt5_0,    "°C",   2150 ) \
    VALUE_ENTRY(Cellt5_1,    "°C",   2151 ) \
    VALUE_ENTRY(Cellt6_0,    "°C",   2152 ) \
    VALUE_ENTRY(Cellt6_1,    "°C",   2153 ) \
    VALUE_ENTRY(Cellt7_0,    "°C",   2154 ) \
    VALUE_ENTRY(Cellt7_1,    "°C",   2155 ) \
    VALUE_ENTRY(Chipt0,      "°C",   2114 ) \
    VALUE_ENTRY(Chipt1,      "°C",   2115 ) \
    VALUE_ENTRY(Chipt2,      "°C",   2116 ) \
    VALUE_ENTRY(Chipt3,      "°C",   2117 ) \
    VALUE_ENTRY(Chipt4,      "°C",   2118 ) \
    VALUE_ENTRY(Chipt5,      "°C",   2119 ) \
    VALUE_ENTRY(Chipt6,      "°C",   2120 ) \
    VALUE_ENTRY(Chipt7,      "°C",   2121 ) \
    VALUE_ENTRY(Chip1_5V,    "mV",   2123 ) \
    VALUE_ENTRY(Chip2_5V,    "mV",   2124 ) \
    VALUE_ENTRY(ChipV1,       "V",   2125 ) \
    VALUE_ENTRY(ChipV2,       "V",   2126 ) \
    VALUE_ENTRY(ChipV3,       "V",   2132 ) \
    VALUE_ENTRY(ChipV4,       "V",   2133 ) \
    VALUE_ENTRY(ChipV5,       "V",   2134 ) \
    VALUE_ENTRY(ChipV6,       "V",   2135 ) \
    VALUE_ENTRY(ChipV7,       "V",   2136 ) \
    VALUE_ENTRY(ChipV8,       "V",   2137 ) \
    VALUE_ENTRY(CellsPresent,  "",   2128 ) \
    VALUE_ENTRY(CellsBalancing,  "",   2160 ) \
    VALUE_ENTRY(LoopCnt,      "",    2127 ) \
    VALUE_ENTRY(LoopState,    "",    2131 ) \
    VALUE_ENTRY(cpuload,     "%",    2122 )


/***** Enum String definitions *****/
#define OPMODES      "0=Off, 1=Run, 2=RunBalance"
#define OFFON        "0=Off, 1=On"
#define BAL          "0=None, 1=Discharge"
#define IDCMODES     "0=Off, 1=AdcSingle, 2=IsaCan"
#define TYPES        "0=Model_3, 1=Model_S"
#define CAT_BMS      "BMS"
#define CAT_SENS     "Sensor setup"
#define CAT_COMM     "Communication"

#define VERSTR STRINGIFY(4=VER)

/***** enums ******/

enum
{
    IDC_OFF, IDC_SINGLE, IDC_ISACAN
};


enum _modes
{
    MOD_OFF = 0,
    MOD_RUN,
    MOD_LAST
};

//Generated enum-string for possible errors
extern const char* errorListString;
