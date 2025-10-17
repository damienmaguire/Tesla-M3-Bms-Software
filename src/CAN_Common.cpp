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
#include "CAN_Common.h"

static CanHardware* c;

uint8_t loopcnt =0;

void CAN_Common::SetCan(CanHardware* x)
{
    c = x;
}

void CAN_Common::Task100Ms()
{
    loopcnt++;

    if(loopcnt > 4)
    {
        loopcnt = 0;
        StandardCanFrames();
    }
}

void CAN_Common::StandardCanFrames()//CanHardware* c)
{
    uint16_t temp= 0;
    uint8_t bytes[8];
    /*
    //Examples of how to send data on CAN
    uint32_t canData[8] = { 0xA0B0C0D, 0};
    c->Send(0x280, canData, 8); //Send on CAN1

    uint8_t bytes[8];
    bytes[0]=0x05;
    bytes[1]=0x00;
    bytes[2]=0x01;
    bytes[3]=0x10;
    bytes[4]=0x00;
    bytes[5]=0x00;
    bytes[6]=0x00;
    bytes[7]=0x69;
    c->Send(0x380, bytes, 8); //Send on CAN1
    */


    temp = Param::GetFloat(Param::chargeVlim)*10;

    bytes[0]=temp & 0x00FF;
    bytes[1]=temp >> 8;

    temp = Param::GetFloat(Param::chargelim);


    bytes[2]=temp & 0x00FF;
    bytes[3]=temp >> 8;

    temp = Param::GetFloat(Param::dischargelim);

    bytes[4]=temp & 0x00FF;
    bytes[5]=temp >> 8;

    temp = Param::GetFloat(Param::dischargeVlim)*10;

    bytes[6]=temp & 0x00FF;
    bytes[7]=temp >> 8;

    c->Send(0x351, bytes, 8); //Send on CAN1


    temp= Param::GetInt(Param::soc);


    bytes[0]=temp & 0x00FF;
    bytes[1]=0x00;
    bytes[2]=0x64; //100% SOH fixed for now
    bytes[3]=0x00;

    temp = temp *100;

    bytes[4]=temp & 0x00FF;
    bytes[5]=temp >> 8;
    bytes[6]=0x00;
    bytes[7]=0x00;

    c->Send(0x355, bytes, 8); //Send on CAN1

    temp = (Param::GetFloat(Param::udc)*100);

    bytes[0]=temp & 0x00FF;
    bytes[1]=temp >> 8;

    temp = (Param::GetInt(Param::idc)*10);

    bytes[2]=temp & 0x00FF;
    bytes[3]=temp >> 8;

    temp = (Param::GetFloat(Param::TempMax)*10);

    bytes[4]=temp & 0x00FF;
    bytes[5]=temp >> 8;
    bytes[6]=0x00;
    bytes[7]=0x00;

    c->Send(0x356, bytes, 8); //Send on CAN1

    //BMS Name//

    bytes[0]='Z';
    bytes[1]='O';
    bytes[2]='M';
    bytes[3]='-';
    bytes[4]='T';
    bytes[5]='B';
    bytes[6]='M';
    bytes[7]='S';

    c->Send(0x35E, bytes, 8); //Send on CAN1

    //BMS Manufacturer//

    bytes[0]='O';
    bytes[1]='P';
    bytes[2]='E';
    bytes[3]='N';
    bytes[4]='-';
    bytes[5]='I';
    bytes[6]='N';
    bytes[7]='V';

    c->Send(0x370, bytes, 8); //Send on CAN1


    temp = Param::GetFloat(Param::umin);

    bytes[0]=temp & 0x00FF;
    bytes[1]=temp >> 8;

    temp = Param::GetFloat(Param::umax);


    bytes[2]=temp & 0x00FF;
    bytes[3]=temp >> 8;

    temp = (Param::GetFloat(Param::TempMin)+273.15);

    bytes[4]=temp & 0x00FF;
    bytes[5]=temp >> 8;

    temp = (Param::GetFloat(Param::TempMax)+273.15);

    bytes[6]=temp & 0x00FF;
    bytes[7]=temp >> 8;

    c->Send(0x373, bytes, 8); //Send on CAN1

    /*

        outMsg.id = 0x35A;
        outMsg.len = 8;
        outMsg.buf[0] = alarm[0];
        outMsg.buf[1] = alarm[1];
        outMsg.buf[2] = alarm[2];
        outMsg.buf[3] = alarm[3];
        outMsg.buf[4] = alarm[4];
        outMsg.buf[5] = alarm[5];
        outMsg.buf[6] = alarm[6];
        outMsg.buf[7] = alarm[7];
        Can.SendCan(outMsg, Bus);

        delay(1);


        delay(1);

        outMsg.id = 0x379;  //Installed capacity
        outMsg.len = 8;
        outMsg.buf[0] = lowByte(uint16_t(settings.Pstrings * settings.CAP));
        outMsg.buf[1] = highByte(uint16_t(settings.Pstrings * settings.CAP));
        outMsg.buf[2] = 0x00;
        outMsg.buf[3] = 0x00;
        outMsg.buf[4] = BMS.getstatus();
        outMsg.buf[5] = VCUStatus;
        outMsg.buf[6] = HVBus;
        outMsg.buf[7] = DriveStatus;
        Can.SendCan(outMsg, Bus);

        delay(1);

        outMsg.id = 0x372;
        outMsg.len = 8;
        outMsg.buf[0] = lowByte(BMS.getNumModules());
        outMsg.buf[1] = highByte(BMS.getNumModules());
        outMsg.buf[2] = 0x00;
        outMsg.buf[3] = 0x00;
        outMsg.buf[4] = 0x00;
        outMsg.buf[5] = 0x00;
        outMsg.buf[6] = 0x00;
        outMsg.buf[7] = 0x00;
        Can.SendCan(outMsg, Bus);
        */
}

void CAN_Common::HandleCan(uint32_t data[2])
{
    //uint8_t* bytes = (uint8_t*)data;

}
