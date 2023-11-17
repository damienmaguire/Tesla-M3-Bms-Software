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
#ifndef CAN_Common_h
#define CAN_Common_h

#include <stdint.h>
#include "my_fp.h"
#include "stm32_can.h"
#include "canhardware.h"
#include "params.h"
#include "digio.h"


class CAN_Common
{
public:
    static  void Task100Ms();
    static  void StandardCanFrames();//CanHardware* c);
    static void HandleCan(uint32_t data[2]);
    static void SetCan(CanHardware* x);

private:



protected:
};

#endif
