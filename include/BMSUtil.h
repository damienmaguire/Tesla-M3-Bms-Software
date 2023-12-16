/*
 *
 * Copyright (C) 2023 Tom de Bree
 *                      Damien Maguire <info@evbmw.com>
 * Yes I'm really writing software now........run.....run away.......
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
#ifndef BMSUtil_h
#define BMSUtil_h
#include "my_math.h"
#include <stdint.h>
#include "my_fp.h"
#include "params.h"


class BMSUtil
{
public:
    static void UpdateSOC();
    static  int EstimateSocFromVoltage();

private:



protected:
};

#endif
