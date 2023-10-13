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
#define VER 0.01.A

/* Entries must be ordered as follows:
   1. Saveable parameters (id != 0)
   2. Temporary parameters (id = 0)
   3. Display values
 */
//Next param id (increase when adding new parameter!): 8
//Next value Id: 2049
/*              category     name         unit       min     max     default id */
#define PARAM_LIST \
    PARAM_ENTRY(CAT_BMS,     type,        TYPES,     0,      1,      0,      1   ) \
    PARAM_ENTRY(CAT_BMS,     numbmbs,     "",        1,      4,      1,      2   ) \
    PARAM_ENTRY(CAT_BMS,     balance,     OFFON,     0,      1,      0,      3   ) \
    PARAM_ENTRY(CAT_BMS,     nomcap,      "Ah",      0,      1000,   100,    4   ) \
    PARAM_ENTRY(CAT_SENS,    idcgain,     "dig/A",   -1000,  1000,   10,     5   ) \
    PARAM_ENTRY(CAT_SENS,    idcofs,      "dig",    -4095,   4095,   0,      6   ) \
    PARAM_ENTRY(CAT_SENS,    idcmode,     IDCMODES,  0,      3,      0,      7   ) \
    VALUE_ENTRY(opmode,      OPMODES,2000 ) \
    VALUE_ENTRY(version,     VERSTR, 2001 ) \
    VALUE_ENTRY(soc,         "As",   2002 ) \
    VALUE_ENTRY(chargelim,   "A",    2003 ) \
    VALUE_ENTRY(dischargelim,"A",    2004 ) \
    VALUE_ENTRY(deltaV,      "mV",   2005 ) \
    VALUE_ENTRY(udc,         "V",    2006 ) \
    VALUE_ENTRY(idc,         "A",    2007 ) \
    VALUE_ENTRY(temp,        "°C",   2008 ) \
    VALUE_ENTRY(uavg,        "mV",   2009 ) \
    VALUE_ENTRY(umin,        "mV",   2010 ) \
    VALUE_ENTRY(umax,        "mV",   2011 ) \
    VALUE_ENTRY(u0,          "mV",   2012 ) \
    VALUE_ENTRY(u1,          "mV",   2013 ) \
    VALUE_ENTRY(u2,          "mV",   2014 ) \
    VALUE_ENTRY(u3,          "mV",   2015 ) \
    VALUE_ENTRY(u4,          "mV",   2016 ) \
    VALUE_ENTRY(u5,          "mV",   2017 ) \
    VALUE_ENTRY(u6,          "mV",   2018 ) \
    VALUE_ENTRY(u7,          "mV",   2019 ) \
    VALUE_ENTRY(u8,          "mV",   2020 ) \
    VALUE_ENTRY(u9,          "mV",   2021 ) \
    VALUE_ENTRY(u10,         "mV",   2022 ) \
    VALUE_ENTRY(u11,         "mV",   2023 ) \
    VALUE_ENTRY(u12,         "mV",   2024 ) \
    VALUE_ENTRY(u13,         "mV",   2025 ) \
    VALUE_ENTRY(u14,         "mV",   2026 ) \
    VALUE_ENTRY(u15,         "mV",   2027 ) \
    VALUE_ENTRY(u16,         "mV",   2028 ) \
    VALUE_ENTRY(u17,         "mV",   2029 ) \
    VALUE_ENTRY(u18,         "mV",   2030 ) \
    VALUE_ENTRY(u19,         "mV",   2031 ) \
    VALUE_ENTRY(u20,         "mV",   2032 ) \
    VALUE_ENTRY(u21,         "mV",   2033 ) \
    VALUE_ENTRY(u22,         "mV",   2035 ) \
    VALUE_ENTRY(u23,         "mV",   2036 ) \
    VALUE_ENTRY(u24,         "mV",   2037 ) \
    VALUE_ENTRY(u25,         "mV",   2038 ) \
    VALUE_ENTRY(t0,          "°C",   2040 ) \
    VALUE_ENTRY(t1,          "°C",   2041 ) \
    VALUE_ENTRY(t2,          "°C",   2042 ) \
    VALUE_ENTRY(t3,          "°C",   2043 ) \
    VALUE_ENTRY(t4,          "°C",   2044 ) \
    VALUE_ENTRY(t5,          "°C",   2045 ) \
    VALUE_ENTRY(t6,          "°C",   2046 ) \
    VALUE_ENTRY(t7,          "°C",   2047 ) \
    VALUE_ENTRY(cpuload,     "%",    2048 )


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
