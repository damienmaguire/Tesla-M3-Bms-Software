/*
 *
 * Copyright (C) 2023 Tom de Bree
 *                      Damien Maguire <info@evbmw.com>
 * Yes I'm really writing software now........run.....run away.......
 *
 * Based on info from https://github.com/jsphuebner/FlyingAdcBms
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
#include "BMSUtil.h"

//voltage to state of charge                0%    10%   20%   30%   40%   50%   60%   70%   80%   90%   100%
uint16_t voltageToSoc[] =       { 3300, 3400, 3450, 3500, 3560, 3600, 3700, 3800, 4000, 4100, 4200 };

uint16_t TempSOC =0; // SOC for internal code use
uint32_t NoCurCounter = 0;//
uint32_t NoCurRun = 20;//100ms counts before SOC will be determined based on voltage
float NoCurLim = 1.0; //current limit under which Voltage based
float asDiff = 0;//Ampsecond change since last SOC update

void BMSUtil::UpdateSOC()
{
    TempSOC = Param::GetInt(Param::soc);

    if(ABS(Param::GetFloat(Param::idc)) < NoCurLim)
    {
        NoCurCounter++;
    }
    else
    {
        NoCurCounter=0;
    }

    if(NoCurCounter > NoCurRun)
    {
        TempSOC=EstimateSocFromVoltage();
    }
    else
    {
        TempSOC = TempSOC + (100 * asDiff / (3600 * Param::GetInt(Param::nomcap)));
    }

    Param::SetInt(Param::soc,TempSOC);
}

int BMSUtil::EstimateSocFromVoltage()
{
    float lowestVoltage = Param::GetFloat(Param::umin);
    int n = sizeof(voltageToSoc) / sizeof(voltageToSoc[0]);

    for (int i = 0; i < n; i++)
    {
        if (lowestVoltage < voltageToSoc[i])
        {
            if (i == 0) return 0;

            float soc = i * 10;
            float lutDiff = voltageToSoc[i] - voltageToSoc[i - 1];
            float valDiff = voltageToSoc[i] - lowestVoltage;
            //interpolate
            soc -= (valDiff / lutDiff) * 10;
            return soc;
        }
    }
    return 100;
}
