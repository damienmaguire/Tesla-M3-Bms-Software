#include "BatMan.h"

/*
This library supports SPI communication for the Tesla Model 3 BMB (battery managment boards) "Batman" chip
Model 3 packs are comprised of 2 short modules with 23 cells and 2 long modules with 25 cells for a total of 96 cells.
A short bmb will only report 23 voltage values where as a long will report 25.

Most of these are guesses based on the LTC6813 datasheet and spi captures. Command bytes are of course different for MuskChip
https://www.analog.com/media/en/technical-documentation/data-sheets/ltc6813-1.pdf
Wake up = 0x2AD4
Unmute = 0x21F2
Snapshot = 0x2BFB
Read A = 0x4700
Read B = 0x4800
Read C = 0x4900
Read D = 0x4A00
Read E = 0x4B00
Read F = 0x4C00
Read Aux A = 0x4D00
Read Aux B = 0x4E00
Read Aux B No Tag = 0x0E00
Read Status Reg = 0x4F00
Read Config = 0x5000

Write Config = 0x11

Major Code Contributors:
Tom de Bree - Volt Influx
Damien Mcguire - EV Bmw
*/

uint16_t WakeUp[2] = {0x2ad4, 0x0000};
uint16_t Mute[2] = {0x20dd, 0x0000};
uint16_t Unmute[2] = {0x21f2, 0x0000};
uint16_t Snap[2] = {0x2BFB, 0x0000};
uint16_t reqTemp = 0x0E1B;//Temps returned in words 1 and 5
/* //no longer needed due to PEC calc
uint16_t readA[2] = {0x4700, 0x7000};
uint16_t readB[2] = {0x4800, 0x3400};
uint16_t readC[2] = {0x4900, 0xdd00};
uint16_t readD[2] = {0x4a00, 0xc900};
uint16_t readE[2] = {0x4b00, 0x2000};
uint16_t readF[2] = {0x4c00, 0xe100};
uint16_t auxA[2] = {0x4D00, 0x0800};
uint16_t auxB[2] = {0x4E00, 0x3300};//3300
uint16_t statR[2] = {0x4F00, 0xF500};
uint16_t cfgR[2] = {0x5000, 0x9400};
*/
uint16_t padding = 0x0000;
uint16_t receive1 = 0;
uint16_t receive2 = 0;
float tempval1 = 0;
float tempval2 = 0;
float temp1 = 0;
float temp2 = 0;
uint8_t  Fluffer[72];
uint8_t count1 = 0;
uint8_t count2 = 0;
uint8_t count3 = 0;
uint8_t LoopTimer1 = 5;

uint16_t Voltage[8][15] =
{
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};


uint16_t Temps   [8] = {0};
uint16_t Temp1   [8] = {0};
uint16_t Temp2   [8] = {0};
uint16_t Volts5v [8] = {0};
uint16_t ChipV   [8] = {0};
uint16_t Cfg [8][2] =
{
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0},
    {0,0}
};

/*
UVAR16 TSOLO   : 4;
UVAR16 RAND    : 1;
UVAR16 FILT    : 3;  // LTC831_FILT_E
UVAR16 DCT0    : 4;  // 0 = off, 0.5,1
UVAR16 TEMP_OW : 1;
UVAR16 SPARE   : 1;
UVAR16 TRY     : 1;
UVAR16 MOD_DIS : 1;
UVAR8  DCC8_1;
UVAR8  DCC16_9;
*/

uint16_t crcTable2f[256] =
{
    0x00, 0x2F, 0x5E, 0x71, 0xBC, 0x93, 0xE2, 0xCD, 0x57, 0x78, 0x09, 0x26, 0xEB, 0xC4, 0xB5, 0x9A,
    0xAE, 0x81, 0xF0, 0xDF, 0x12, 0x3D, 0x4C, 0x63, 0xF9, 0xD6, 0xA7, 0x88, 0x45, 0x6A, 0x1B, 0x34,
    0x73, 0x5C, 0x2D, 0x02, 0xCF, 0xE0, 0x91, 0xBE, 0x24, 0x0B, 0x7A, 0x55, 0x98, 0xB7, 0xC6, 0xE9,
    0xDD, 0xF2, 0x83, 0xAC, 0x61, 0x4E, 0x3F, 0x10, 0x8A, 0xA5, 0xD4, 0xFB, 0x36, 0x19, 0x68, 0x47,
    0xE6, 0xC9, 0xB8, 0x97, 0x5A, 0x75, 0x04, 0x2B, 0xB1, 0x9E, 0xEF, 0xC0, 0x0D, 0x22, 0x53, 0x7C,
    0x48, 0x67, 0x16, 0x39, 0xF4, 0xDB, 0xAA, 0x85, 0x1F, 0x30, 0x41, 0x6E, 0xA3, 0x8C, 0xFD, 0xD2,
    0x95, 0xBA, 0xCB, 0xE4, 0x29, 0x06, 0x77, 0x58, 0xC2, 0xED, 0x9C, 0xB3, 0x7E, 0x51, 0x20, 0x0F,
    0x3B, 0x14, 0x65, 0x4A, 0x87, 0xA8, 0xD9, 0xF6, 0x6C, 0x43, 0x32, 0x1D, 0xD0, 0xFF, 0x8E, 0xA1,
    0xE3, 0xCC, 0xBD, 0x92, 0x5F, 0x70, 0x01, 0x2E, 0xB4, 0x9B, 0xEA, 0xC5, 0x08, 0x27, 0x56, 0x79,
    0x4D, 0x62, 0x13, 0x3C, 0xF1, 0xDE, 0xAF, 0x80, 0x1A, 0x35, 0x44, 0x6B, 0xA6, 0x89, 0xF8, 0xD7,
    0x90, 0xBF, 0xCE, 0xE1, 0x2C, 0x03, 0x72, 0x5D, 0xC7, 0xE8, 0x99, 0xB6, 0x7B, 0x54, 0x25, 0x0A,
    0x3E, 0x11, 0x60, 0x4F, 0x82, 0xAD, 0xDC, 0xF3, 0x69, 0x46, 0x37, 0x18, 0xD5, 0xFA, 0x8B, 0xA4,
    0x05, 0x2A, 0x5B, 0x74, 0xB9, 0x96, 0xE7, 0xC8, 0x52, 0x7D, 0x0C, 0x23, 0xEE, 0xC1, 0xB0, 0x9F,
    0xAB, 0x84, 0xF5, 0xDA, 0x17, 0x38, 0x49, 0x66, 0xFC, 0xD3, 0xA2, 0x8D, 0x40, 0x6F, 0x1E, 0x31,
    0x76, 0x59, 0x28, 0x07, 0xCA, 0xE5, 0x94, 0xBB, 0x21, 0x0E, 0x7F, 0x50, 0x9D, 0xB2, 0xC3, 0xEC,
    0xD8, 0xF7, 0x86, 0xA9, 0x64, 0x4B, 0x3A, 0x15, 0x8F, 0xA0, 0xD1, 0xFE, 0x33, 0x1C, 0x6D, 0x42
};


//Tom Magic....
bool BalanceFlage = false;
bool BmbTimeout = true;
uint16_t LoopState = 0;
uint16_t LoopRanCnt =0;
uint8_t WakeCnt = 0;
uint8_t WaitCnt = 0;
uint8_t IdleCnt = 0;
uint8_t ChipNum =0;
float CellVMax = 0;
float CellVMin = 5000;
float TempMax = 0;
float TempMin = 1000;
uint16_t SendDelay = 1000;
uint32_t lasttime = 0;

void BATMan::BatStart()
{
    ChipNum = Param::GetInt(Param::numbmbs)*2;
}

void BATMan::loop() //runs every 100ms
{
    StateMachine();
}

void BATMan::StateMachine()
{
    switch (LoopState)
    {
    case 0: //first state check if there is time out of commms requiring full wake
    {
        if(BmbTimeout == true)
        {
            WakeUP();//send wake up 4 times for 4 bmb boards
            LoopState++;
        }
        else
        {
            LoopState++;
        }
        break;
    }

    case 1:
    {
        IdleWake();//unmute
        GetData(0x4D);//Read Aux A.Contains 5v reg voltage in word 1
        GetData(0x50);//Read Cfg
        LoopState++;
        break;
    }

    case 2:
    {
        IdleWake();//unmute
        delay(SendDelay);
        Generic_Send_Once(Snap, 1);//Take a snapshot of the cell voltages
        LoopState++;
        break;
    }

    case 3:
    {
        IdleWake();//unmute
        delay(SendDelay);
        Generic_Send_Once(Snap, 1);//Take a snapshot of the cell voltages
        LoopState++;
        break;
    }

    case 4:
    {
        IdleWake();//unmute
        delay(SendDelay);
        GetData(0x4F);//Read status reg
        delay(SendDelay);
        GetData(0x4F);//Read status reg
        delay(SendDelay);
        GetData(0x47);//Read A. Contains Cell voltage measurements
        delay(SendDelay);
        GetData(0x48);//Read B. Contains Cell voltage measurements
        delay(SendDelay);
        GetData(0x49);//Read C. Contains Cell voltage measurements
        delay(SendDelay);
        GetData(0x4A);//Read D. Contains Cell voltage measurements

        LoopState++;
        break;
    }

    case 5:
    {
        IdleWake();//unmute
        delay(SendDelay);
        GetData(0x4F);//Read status reg
        delay(SendDelay);
        GetData(0x4F);//Read status reg
        delay(SendDelay);
        GetData(0x4B);//Read E. Contains Cell voltage measurements
        delay(SendDelay);
        GetData(0x4C);//Read F. Contains chip total V in word 1.
        delay(SendDelay);
        GetData(0x4D);//Read F. Contains chip total V in word 1.
        delay(SendDelay);
        GetTempData();//Request temps

        //WriteCfg();

        LoopState++;
        break;
    }

    case 6: //first state check if there is time out of commms requiring full wake
    {
        WakeUP();//send wake up 4 times for 4 bmb boards
        WriteCfg();
        delay(SendDelay);
        WakeUP();//send wake up 4 times for 4 bmb boards
        WriteCfg();
        delay(SendDelay);
        WakeUP();//send wake up 4 times for 4 bmb boards
        LoopState++;
        break;
    }

    case 7:
    {

        upDateTemps();
        upDateCellVolts();
        upDateAuxVolts();

        LoopState++;
        break;
    }

    case 8: //Waiting State
    {
        if(IdleCnt < 20)
        {
            IdleCnt++;
        }
        else
        {
            LoopState = 0;
            LoopRanCnt++;
            Param::SetInt(Param::LoopCnt, LoopRanCnt);
        }
        break;
    }

    default: //Should not get here
    {
        break;
    }

    }

    Param::SetInt(Param::LoopState, LoopState);
}


void BATMan::IdleWake()
{
    if(BalanceFlage == true)
    {
        Generic_Send_Once(Mute, 2);//mute need to do more when balancing to dig into (Primen_CMD)
    }
    else
    {
        Generic_Send_Once(Unmute, 2);//unmute

    }
}

void BATMan::GetData(uint8_t ReqID)
{
    uint8_t tempData[2] = {0};
    uint16_t ReqData[2] = {0};

    tempData[0] = ReqID;

    ReqData[0] = ReqID << 8;
    ReqData[1] = (calcCRC(tempData, 2))<<8;



    DigIo::BatCS.Clear();

    receive1 = spi_xfer(SPI1, ReqData[0]);  // do a transfer
    receive2 = spi_xfer(SPI1, ReqData[1]);  // do a transfer

    //receive1 = spi_xfer(SPI1, Request[0]);  // do a transfer
    // receive2 = spi_xfer(SPI1, Request[1]);  // do a transfer

    for (count2 = 0; count2 <= 72; count2 = count2 + 2)
    {
        receive1 = spi_xfer(SPI1, padding);  // do a transfer
        Fluffer[count2] = receive1 >> 8;
        Fluffer[count2 + 1] = receive1 & 0xFF;
    }
    DigIo::BatCS.Set();

    uint16_t tempvol = 0;

    switch (ReqID)
    {
    case 0x47:
        for (int h = 0; h <= 8; h++)
        {
            for (int g = 0; g <= 2; g++)
            {
                tempvol = Fluffer[1 + (h * 9) + (g * 2)] * 256 + Fluffer [0 + (h * 9) + (g * 2)];
                if (tempvol != 0xffff)
                {
                    Voltage[h][g] = tempvol / 12.5;
                }
            }
        }
        break;

    case 0x48:
        for (int h = 0; h <= 8; h++)
        {
            for (int g = 3; g <= 5; g++)
            {
                tempvol = Fluffer[1 + (h * 9) + ((g - 3) * 2)] * 256 + Fluffer [0 + (h * 9) + ((g - 3) * 2)];
                if (tempvol != 0xffff)
                {
                    Voltage[h][g] = tempvol / 12.5;
                }
            }
        }
        break;

    case 0x49:
        for (int h = 0; h <= 8; h++)
        {
            for (int g = 6; g <= 8; g++)
            {
                tempvol = Fluffer[1 + (h * 9) + ((g - 6) * 2)] * 256 + Fluffer [0 + (h * 9) + ((g - 6) * 2)];
                if (tempvol != 0xffff)
                {
                    Voltage[h][g] = tempvol / 12.5 ;
                }
            }
        }
        break;


    case 0x4A:
        for (int h = 0; h <= 8; h++)
        {
            for (int g = 9; g <= 11; g++)
            {
                tempvol = Fluffer[1 + (h * 9) + ((g - 9) * 2)] * 256 + Fluffer [0 + (h * 9) + ((g - 9) * 2)];
                if (tempvol != 0xffff)
                {
                    Voltage[h][g] = tempvol / 12.5;
                }
            }
        }
        break;

    case 0x4B:
        for (int h = 0; h <= 8; h++)
        {
            for (int g = 12; g <= 14; g++)
            {
                tempvol = Fluffer[1 + (h * 9) + ((g - 12) * 2)] * 256 + Fluffer [0 + (h * 9) + ((g - 12) * 2)];
                if (tempvol != 0xffff)
                {
                    Voltage[h][g] = tempvol / 12.5;
                }
            }
        }
        break;

    case 0x4C:
        for (int h = 0; h <= 8; h++)
        {
            tempvol = Fluffer[3 + (h * 7)] * 256 + Fluffer [2 + (h * 7)];
            if (tempvol != 0xffff)
            {
                ChipV[h] = tempvol;
            }
        }
        break;


    case 0x4D:
        for (int h = 0; h < 8; h++)
        {
            tempvol = Fluffer[1 + (h * 9)] * 256 + Fluffer [0 + (h * 9)];
            if (tempvol != 0xffff)
            {
                Temp1[h] = tempvol;
            }


            tempvol = Fluffer[3 + (h * 9)] * 256 + Fluffer [2 + (h * 9)];
            if (tempvol != 0xffff)
            {
                if(h == 0 || h == 3 || h == 5 || h == 7)
                {
                    Volts5v[h] = rev16(tempvol);
                }
                else
                {
                    Volts5v[h] = tempvol;
                }
            }

            tempvol = Fluffer[5 + (h * 9)] * 256 + Fluffer [4 + (h * 9)];
            if (tempvol != 0xffff)
            {
                Temp2[h] = tempvol;
            }
        }
        break;

    case 0x50:
        for (int h = 0; h < 8; h++)
        {
            tempvol = Fluffer[0 + (h * 7)] * 256 + Fluffer [1 + (h * 7)];
            if (tempvol != 0xffff)
            {
                Cfg[h][0] = tempvol;
            }


            tempvol = Fluffer[2 + (h * 7)] * 256 + Fluffer [3 + (h * 7)];
            if (tempvol != 0xffff)
            {
                Cfg[h][1] =  tempvol;
            }
        }
        break;


    default:
        // statements
        break;
    }

}


void BATMan::WriteCfg()
{
    // CMD(one byte) PEC(one byte)
    uint8_t tempData[6] = {0};

    //uint8_t DCC16_9 = 0;
    //uint8_t DCC8_1 = 0;
    uint16_t cfgwrt [24] = {0};

    cfgwrt[0]= 0x112F;        //CMD

    tempData[0]=0xF3;
    tempData[1]=0x00;
    tempData[2]=0x00;
    tempData[3]=0x00;
    tempData[4]=0x38;
    tempData[5] = (calcCRC(tempData, 5));

    Param::SetFloat(Param::idc,tempData[5]);

    for (int h = 0; h < 7; h++)//write the 8 BMB registers
    {
        cfgwrt[1+h*3] = 0xF300;
        cfgwrt[2+h*3] = 0x0000;
        cfgwrt[3+h*3] = 0x38DC;//Contains the PEC and other shit
    }


    DigIo::BatCS.Clear();

    for (int cnt = 0; cnt < 24; cnt++)
    {
        receive1 = spi_xfer(SPI1, cfgwrt[cnt]);  // do a transfer
    }
    DigIo::BatCS.Set();
}


void BATMan::GetTempData ()  //request
{
    padding=0x0000;
    DigIo::BatCS.Clear();
    receive1 = spi_xfer(SPI1, reqTemp);  // do a transfer
    for (count3 = 0; count3 < 32; count3 ++)
    {
        receive1 = spi_xfer(SPI1, padding);  // do a transfer
        if(receive1 != 0xFFFF)
        {
            if(count3==1) Temps[0]=receive1;//temperature 1
            if(count3==5) Temps[1]=receive1;//temperature 2
            if(count3==9) Temps[2]=receive1;//temperature 3
            if(count3==13) Temps[3]=receive1;//temperature 4
            if(count3==17) Temps[4]=receive1;//temperature 5
            if(count3==21) Temps[5]=receive1;//temperature 6
            if(count3==25) Temps[6]=receive1;//temperature 7
            if(count3==29) Temps[7]=receive1;//temperature 8
        }
    }
    DigIo::BatCS.Set();

    //Param::SetFloat(Param::temp,69);

    //delay(200);
}

void BATMan::WakeUP()
{
    for (count1 = 0; count1 <= 4; count1++)
    {
        DigIo::BatCS.Clear();
        receive1 = spi_xfer(SPI1, WakeUp[0]);  // do a transfer
        DigIo::BatCS.Set();
    }
}

void BATMan::Generic_Send_Once(uint16_t Command[], uint8_t len)
{
    DigIo::BatCS.Clear();
    for (int h = 0; h < len; h++)
    {
        receive1 = spi_xfer(SPI1, Command[h]);  // do a transfer
    }
    DigIo::BatCS.Set();

}

void BATMan::upDateCellVolts(void)
{
    uint8_t Xr = 0;
    uint8_t Yc = 0;
    uint8_t h = 0;
    while (h <= 100)
    {
        if(Voltage[Xr][Yc] > 0) //Check actual measurement present
        {
            if (CellVMax< Voltage[Xr][Yc])
            {
                CellVMax =  Voltage[Xr][Yc];
                Param::SetInt(Param::CellMax, h+1);
            }
            if (CellVMin > Voltage[Xr][Yc])
            {
                CellVMin =  Voltage[Xr][Yc];
                Param::SetInt(Param::CellMin, h+1);
            }
            Param::SetFloat((Param::PARAM_NUM)(Param::u1 + h), (Voltage[Xr][Yc]));
            h++; //next cell along
            Yc++; //next cell along
        }
        else
        {
            Yc =0; //reset Cell colum
            Xr++; //next BMB
        }
        if(Xr == ChipNum)
        {
            Param::SetFloat(Param::umax,CellVMax);
            Param::SetFloat(Param::umin,CellVMin);
            Param::SetFloat(Param::deltaV,CellVMax-CellVMin);
            Param::SetInt(Param::CellsPresent,h);
            h = 100;
            break;
        }
    }
}

void BATMan::upDateAuxVolts(void)
{
    Param::SetInt(Param::Chip1_5V,(rev16(Volts5v[0]))/12.5);
    Param::SetInt(Param::Chip2_5V,((Volts5v[1]))/12.5);
    Param::SetInt(Param::soc,((Volts5v[2])));

    Param::SetFloat(Param::udc,0);

    if(Param::GetInt(Param::numbmbs) >= 1)
    {
        Param::SetFloat(Param::ChipV1,ChipV[0]*0.001280);
        Param::SetFloat(Param::ChipV2,ChipV[1]*0.001280);
        Param::SetFloat(Param::udc,(Param::GetFloat(Param::ChipV1)+Param::GetFloat(Param::ChipV2)));
    }
    if(Param::GetInt(Param::numbmbs) >= 2)
    {
        Param::SetFloat(Param::ChipV3,ChipV[2]*0.001280);
        Param::SetFloat(Param::ChipV4,ChipV[3]*0.001280);
        Param::SetFloat(Param::udc,(Param::GetFloat(Param::udc)+Param::GetFloat(Param::ChipV3)+Param::GetFloat(Param::ChipV4)));
    }
    if(Param::GetInt(Param::numbmbs) >= 3)
    {
        Param::SetFloat(Param::ChipV5,ChipV[4]*0.001280);
        Param::SetFloat(Param::ChipV6,ChipV[5]*0.001280);
        Param::SetFloat(Param::udc,(Param::GetFloat(Param::udc)+Param::GetFloat(Param::ChipV5)+Param::GetFloat(Param::ChipV6)));
    }
    if(Param::GetInt(Param::numbmbs) == 4)
    {
        Param::SetFloat(Param::ChipV7,ChipV[6]*0.001280);
        Param::SetFloat(Param::ChipV8,ChipV[7]*0.001280);
        Param::SetFloat(Param::udc,(Param::GetFloat(Param::udc)+Param::GetFloat(Param::ChipV7)+Param::GetFloat(Param::ChipV8)));
    }

    Param::SetInt(Param::uavg,(Param::GetFloat(Param::udc)/Param::GetInt(Param::CellsPresent)*1000));

    //Set Charge and discharge voltage limits !!! Update with configrable
    Param::SetFloat(Param::chargeVlim,(Param::GetInt(Param::CellVmax)*0.001*Param::GetInt(Param::CellsPresent)));
    Param::SetFloat(Param::dischargeVlim,(Param::GetInt(Param::CellVmin)*0.001*Param::GetInt(Param::CellsPresent)));
}

void BATMan::upDateTemps(void)
{

    TempMax = 0;
    TempMin= 100;

    for (int g = 0; g < ChipNum; g++)
    {
        tempval1=rev16(Temps[g]);//bytes swapped in the 16 bit words
        if (tempval1==0)
        {
            tempval2=0;
        }
        else if (tempval1 >= (1131))
        {
            tempval1 = tempval1-1131;
            tempval2 = tempval1/10;
        }

        else
        {
            tempval1 = 1131-tempval1;
            tempval2 = tempval1/10;
        }
        Param::SetFloat((Param::PARAM_NUM)(Param::Chipt0 + g), tempval2);

        Temp1[g] = ((Temp1[g])*0.01)-40;
        Temp2[g] = ((Temp2[g])*0.01)-40;

        if(TempMax < Temp1[g])
        {
            TempMax = Temp1[g];
        }
        if(TempMax < Temp2[g])
        {
            TempMax = Temp2[g];
        }

        if(TempMin > Temp1[g])
        {
            TempMin = Temp1[g];
        }

        if(TempMin > Temp2[g])
        {
            TempMin = Temp2[g];
        }

        Param::SetFloat((Param::PARAM_NUM)(Param::Cellt0_0 + g*2),Temp1[g]);
        Param::SetFloat((Param::PARAM_NUM)(Param::Cellt0_1 + g*2),Temp2[g]);
    }
    Param::SetFloat(Param::TempMax,TempMax);
    Param::SetFloat(Param::TempMin,TempMin);

    Param::SetFloat(Param::chargelim,Cfg[0][0]);
    Param::SetFloat(Param::dischargelim,Cfg[0][1]);

}


uint8_t BATMan::calcCRC(uint8_t *inData, uint8_t Length)
{
    uint8_t CRC8 =  0x10;
    uint8_t crc_temp = 0;
    for (uint8_t i = 0; i<Length; i++)
    {
        crc_temp = CRC8 ^ inData[i];
        CRC8 = crcTable2f[crc_temp];
    }
    return(CRC8);
}
