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

#define cycletime 92 //in 100ms. Measurement is 800ms + (cycletime x 100ms)
//float BalHys = 20; //mV balance limit - Made dynamic

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

uint16_t CellBalCmd[8]= {0, 0, 0, 0, 0, 0, 0, 0};

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

// CRC table for Batman data
#define crcPolyBmData 0x025b

static const uint16_t crc14table[256] =
{
    0x0000, 0x025b, 0x04b6, 0x06ed, 0x096c, 0x0b37, 0x0dda, 0x0f81,
    0x12d8, 0x1083, 0x166e, 0x1435, 0x1bb4, 0x19ef, 0x1f02, 0x1d59,
    0x25b0, 0x27eb, 0x2106, 0x235d, 0x2cdc, 0x2e87, 0x286a, 0x2a31,
    0x3768, 0x3533, 0x33de, 0x3185, 0x3e04, 0x3c5f, 0x3ab2, 0x38e9,
    0x093b, 0x0b60, 0x0d8d, 0x0fd6, 0x0057, 0x020c, 0x04e1, 0x06ba,
    0x1be3, 0x19b8, 0x1f55, 0x1d0e, 0x128f, 0x10d4, 0x1639, 0x1462,
    0x2c8b, 0x2ed0, 0x283d, 0x2a66, 0x25e7, 0x27bc, 0x2151, 0x230a,
    0x3e53, 0x3c08, 0x3ae5, 0x38be, 0x373f, 0x3564, 0x3389, 0x31d2,
    0x1276, 0x102d, 0x16c0, 0x149b, 0x1b1a, 0x1941, 0x1fac, 0x1df7,
    0x00ae, 0x02f5, 0x0418, 0x0643, 0x09c2, 0x0b99, 0x0d74, 0x0f2f,
    0x37c6, 0x359d, 0x3370, 0x312b, 0x3eaa, 0x3cf1, 0x3a1c, 0x3847,
    0x251e, 0x2745, 0x21a8, 0x23f3, 0x2c72, 0x2e29, 0x28c4, 0x2a9f,
    0x1b4d, 0x1916, 0x1ffb, 0x1da0, 0x1221, 0x107a, 0x1697, 0x14cc,
    0x0995, 0x0bce, 0x0d23, 0x0f78, 0x00f9, 0x02a2, 0x044f, 0x0614,
    0x3efd, 0x3ca6, 0x3a4b, 0x3810, 0x3791, 0x35ca, 0x3327, 0x317c,
    0x2c25, 0x2e7e, 0x2893, 0x2ac8, 0x2549, 0x2712, 0x21ff, 0x23a4,
    0x24ec, 0x26b7, 0x205a, 0x2201, 0x2d80, 0x2fdb, 0x2936, 0x2b6d,
    0x3634, 0x346f, 0x3282, 0x30d9, 0x3f58, 0x3d03, 0x3bee, 0x39b5,
    0x015c, 0x0307, 0x05ea, 0x07b1, 0x0830, 0x0a6b, 0x0c86, 0x0edd,
    0x1384, 0x11df, 0x1732, 0x1569, 0x1ae8, 0x18b3, 0x1e5e, 0x1c05,
    0x2dd7, 0x2f8c, 0x2961, 0x2b3a, 0x24bb, 0x26e0, 0x200d, 0x2256,
    0x3f0f, 0x3d54, 0x3bb9, 0x39e2, 0x3663, 0x3438, 0x32d5, 0x308e,
    0x0867, 0x0a3c, 0x0cd1, 0x0e8a, 0x010b, 0x0350, 0x05bd, 0x07e6,
    0x1abf, 0x18e4, 0x1e09, 0x1c52, 0x13d3, 0x1188, 0x1765, 0x153e,
    0x369a, 0x34c1, 0x322c, 0x3077, 0x3ff6, 0x3dad, 0x3b40, 0x391b,
    0x2442, 0x2619, 0x20f4, 0x22af, 0x2d2e, 0x2f75, 0x2998, 0x2bc3,
    0x132a, 0x1171, 0x179c, 0x15c7, 0x1a46, 0x181d, 0x1ef0, 0x1cab,
    0x01f2, 0x03a9, 0x0544, 0x071f, 0x089e, 0x0ac5, 0x0c28, 0x0e73,
    0x3fa1, 0x3dfa, 0x3b17, 0x394c, 0x36cd, 0x3496, 0x327b, 0x3020,
    0x2d79, 0x2f22, 0x29cf, 0x2b94, 0x2415, 0x264e, 0x20a3, 0x22f8,
    0x1a11, 0x184a, 0x1ea7, 0x1cfc, 0x137d, 0x1126, 0x17cb, 0x1590,
    0x08c9, 0x0a92, 0x0c7f, 0x0e24, 0x01a5, 0x03fe, 0x0513, 0x0748
};

const uint8_t utilTopN[9] = { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };


//Tom Magic....
bool BalanceFlag = false;
bool BmbTimeout = true;
uint16_t LoopState = 0;
uint16_t LoopRanCnt =0;
uint8_t WakeCnt = 0;
uint8_t WaitCnt = 0;
uint16_t IdleCnt = 0;
uint8_t ChipNum =0;
float CellVMax = 0;
float CellVMin = 5000;
float TempMax = 0;
float TempMin = 1000;
uint16_t SendDelay = 1000;
uint32_t lasttime = 0;
bool BalEven = false;

float Cell1start, Cell2start = 0;

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
        //GetData(0x50);//Read Cfg
        WriteCfg();
        GetData(0x50);//Read Cfg
        Generic_Send_Once(Unmute, 2);//unmute
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
        IdleCnt++;



        if(IdleCnt & 0x01 && (IdleCnt < (Param::GetFloat(Param::VmInterval)*10)-8 - 20))//subtract minimum time for a discharge timer + run every other loop
        {
            WakeUP();
            WriteCfg();
        }



        if(IdleCnt > (Param::GetFloat(Param::VmInterval)*10)-8)//VmInterval [in S] x 10 is loops to run - 8 loops for measuring
        {
            LoopState = 0;
            IdleCnt = 0;
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
    if(BalanceFlag == true)
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
    uint16_t cfgwrt [25] = {0};

    cfgwrt[0]= 0x112F;        //CMD

    for (int h = 0; h < 8; h++)//write the 8 BMB registers
    {

        tempData[0]=0xF3;
        tempData[1]=0x00;
        // Note can not be adjacent cells
        //first copy all cells we want to balance
        tempData[2]=CellBalCmd[7-h] & 0x00FF; //balancing 8-1
        tempData[3]=(CellBalCmd[7-h] & 0xFF00)>>8; //balancing  16-9

        //now alternate between even and odd using AND 0x55 or 0xAA

        if(BalEven == false)
        {
            tempData[2] = tempData[2] & 0xAA;
            tempData[3] = tempData[3] & 0xAA;
        }
        else
        {
            tempData[2] = tempData[2] & 0x55;
            tempData[3] = tempData[3] & 0x55;
        }

        uint16_t payPec =0x0010;

        crc14_bytes(4,tempData,&payPec);
        crc14_bits(2,2,&payPec);

        cfgwrt[1+h*3] = tempData[1] + (tempData[0] << 8);
        cfgwrt[2+h*3] = tempData[3] + (tempData[2] << 8);
        cfgwrt[3+h*3] = payPec;//Contains the PEC and other shit

    }

    DigIo::BatCS.Clear();

    for (int cnt = 0; cnt < 25; cnt++)
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
    uint8_t Xr = 0; //BMB number
    uint8_t Yc = 0; //Cell voltage register number
    uint8_t hc = 0; //Cells present per chip
    uint8_t h = 0; //Spot value index
    uint16_t CellBalancing = 0;
    bool AllowBalancing = false;
    float OldUmin = Param::GetFloat(Param::umin);
    float BalHys = 200; //mV set high

    BalanceFlag = false;
    CellVMax = 0;
    CellVMin = 5000;

    for(uint8_t L =0; L < 8; L++)
    {
        CellBalCmd[L] = 0;
    }

    //Determine if we should be balancing

    if(Param::GetFloat(Param::BallVthres) < Param::GetFloat(Param::umax))//Only balance if highest cell is above Balance Threshold
    {
        AllowBalancing = true;
        //Do magic on the hyst on balancing adjustment
        float dV = Param::GetFloat(Param::deltaV); //extract deltaV

        if(dV > 32)
        {
            BalHys = 16;
        }
        else if(dV > 16)
        {
            BalHys = 8;
        }
        else if(dV > 8)
        {
            BalHys = 4;
        }
        else if(dV > 4)
        {
            BalHys = 2;
        }

        if(BalEven == false)//Flip between balancing odd and even cells
        {
            BalEven = true;
        }
        else
        {
            BalEven = false;
        }

    }

    while (h <= 100)
    {
        if(Yc < 14) //Check actual measurement present
        {
            if(Voltage[Xr][Yc] > 10) //Check actual measurement present
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
                //section to do balancing setup
                if(Param::GetInt(Param::balance) && AllowBalancing == true) // Check if balancing flag is set
                {
                    if((OldUmin + BalHys) < Voltage[Xr][Yc])//USE OLD umin
                    {
                        CellBalCmd[Xr] = CellBalCmd[Xr]+(0x01 << Yc);//populate balancing command register
                        CellBalancing++;
                        BalanceFlag = true;
                    }
                }
                //
                h++; //next cell spot value along
                hc++; //one more cell present
            }

            Yc++; //next cell along
        }
        else
        {
            Param::SetInt((Param::PARAM_NUM)(Param::Chip1Cells+Xr),hc);
            Yc = 0; //reset Cell colum
            hc = 0; //reset cell count per chip
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

    Param::SetInt(Param::CellsBalancing, CellBalancing);
}

void BATMan::upDateAuxVolts(void)
{
    Param::SetInt(Param::Chip1_5V,(rev16(Volts5v[0]))/12.5);
    Param::SetInt(Param::Chip2_5V,((Volts5v[1]))/12.5);
    //Param::SetInt(Param::soc,((Volts5v[2])));

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

void BATMan::crc14_bytes( uint8_t len_B, uint8_t *bytes, uint16_t *crcP )
{
    uint8_t pos, idx;

    for ( idx = 0; idx < len_B; idx++ )
    {
        pos = (uint8_t)((*crcP >> 6) ^ bytes[idx]);

        *crcP = (uint16_t)( (0x3fff & (*crcP << 8)) ^ (uint16_t)(crc14table[pos]));
    }
}

void BATMan::crc14_bits( uint8_t len_b,uint8_t inB, uint16_t *crcP )
{
    inB = inB & utilTopN[len_b];   // Mask out the bite we don't care about

    *crcP ^= (uint16_t)((inB) << 6); /* move byte into MSB of 14bit CRC */

    while( len_b-- )
    {
        if ((*crcP & 0x2000) != 0) /* test for MSB = bit 13 */
        {
            *crcP = (uint16_t)((*crcP << 1) ^ crcPolyBmData);
        }
        else
        {
            *crcP = (uint16_t)( *crcP << 1);
        }
    }
    *crcP &= 0x3fff;
}
