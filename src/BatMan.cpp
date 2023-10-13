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
*/

uint16_t WakeUp[2] = {0x2ad4, 0x0000};
uint16_t Unmute[2] = {0x21f2, 0x0000};
uint16_t Snap[2] = {0x2BFB, 0x0000};
uint16_t reqTemp = 0x0E1B;//Temps returned in words 1 and 5
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

uint16_t Voltage[8][15] = {
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
uint16_t Volts5v [4] = {0};
uint16_t ChipV   [4] = {0};



void BATMan::loop()
{
    LoopTimer1--;
    if(LoopTimer1==0)//Do the loop every 500ms.
    {
    LoopTimer1 = 5;
    WakeUP();//send wake up 4 times for 4 bmb boards
    Generic_Send_Once(Unmute, 2);//unmute
    GetAuxData(auxA, 0x4D);//Read Aux A.Contains 5v reg voltage in word 1
   // GetAuxData(auxB, 0x4E);//Read Aux B. Temps in Aux B
    GetAuxData(cfgR, 0x50);//Read Cfg
    Generic_Send_Once(Snap, 1);//Take a snapshot of the cell voltages
    delay(100);
    Generic_Send_Once(Snap, 1);//Take a snapshot of the cell voltages
    GetData(statR, 0x4F);//Read status reg
    GetData(statR, 0x4F);//Read status reg
    GetData(readA, 0x47);//Read A. Contains Cell voltage measurements
    GetData(readB, 0x48);//Read B. Contains Cell voltage measurements
    GetData(readC, 0x49);//Read C. Contains Cell voltage measurements
    GetData(readD, 0x4A);//Read D. Contains Cell voltage measurements
    GetData(readE, 0x4B);//Read E. Contains Cell voltage measurements
    GetAuxData(readF, 0x4C);//Read F. Contains chip total V in word 1.
    GetTempData();//Request temps
    upDateVolts();
    upDateTemps();
    }

}

void BATMan::GetData(uint16_t Request[2], uint8_t ReqID)
{
  DigIo::BatCS.Clear();
  receive1 = spi_xfer(SPI1, Request[0]);  // do a transfer
  receive2 = spi_xfer(SPI1, Request[1]);  // do a transfer

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

      default:
        // statements
        break;
    }

 // }

  delay(7500);
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

   delay(200);
}

void BATMan::GetAuxData (uint16_t Request[2], uint8_t ReqID)
{
  padding=0x0000;
  DigIo::BatCS.Clear();
  receive1 = spi_xfer(SPI1, Request[0]);  // do a transfer
  receive2 = spi_xfer(SPI1, Request[1]);  // do a transfer
  for (count3 = 0; count3 < 32; count3 ++)
  {
    receive1 = spi_xfer(SPI1, padding);  // do a transfer
    if(receive1 != 0xFFFF)
    {
        if(count3==1 && ReqID==0x4D) Volts5v[0]=receive1;//5v reg voltage in word 1 of Aux A
        if(count3==1 && ReqID==0x4C) ChipV[0]=receive1;//Chip total voltage in word 1 of Main F
        if(count3==5 && ReqID==0x4C) ChipV[1]=receive1;//Chip total voltage in word 1 of Main F
    }
  }
   DigIo::BatCS.Set();

   delay(200);
}



void BATMan::WakeUP()
{
  for (count1 = 0; count1 <= 4; count1++)
  {
    DigIo::BatCS.Clear();
    receive1 = spi_xfer(SPI1, WakeUp[0]);  // do a transfer
    //receive2 = spi_xfer(SPI1, WakeUp[1]);  // do a transfer
    DigIo::BatCS.Set();
   // delayMicroseconds(20);
   delay(2000);
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
  //delayMicroseconds(20);
  delay(200);
}

void BATMan::upDateVolts(void)
{
for (int h = 0; h <= 14; h++)
{
Param::SetFloat((Param::PARAM_NUM)(Param::u0 + h), (Voltage[0][h]));
Param::SetFloat((Param::PARAM_NUM)(Param::u13 + h),(Voltage[1][h]));
Param::SetFloat((Param::PARAM_NUM)(Param::u26 + h),(Voltage[2][h]));
Param::SetFloat((Param::PARAM_NUM)(Param::u39 + h),(Voltage[3][h]));
Param::SetFloat((Param::PARAM_NUM)(Param::u55 + h),(Voltage[4][h]));
Param::SetFloat((Param::PARAM_NUM)(Param::u65 + h),(Voltage[5][h]));
Param::SetFloat((Param::PARAM_NUM)(Param::u78 + h),(Voltage[6][h]));
Param::SetFloat((Param::PARAM_NUM)(Param::u91 + h),(Voltage[7][h]));
}
Param::SetInt(Param::Reg5V,(rev16(Volts5v[0]))/12.5);
Param::SetFloat(Param::ChipV1,(rev16(ChipV[0]))*0.001280);
Param::SetFloat(Param::ChipV2,((ChipV[1]))*0.001280);
}

void BATMan::upDateTemps(void)
{
for (int g = 0; g < 8; g++)
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
Param::SetFloat((Param::PARAM_NUM)(Param::t0 + g), tempval2);
}
temp1=Param::GetFloat(Param::t0);
temp2=Param::GetFloat(Param::t1);
Param::SetFloat(Param::temp,MAX(temp1,temp2));
Param::SetFloat(Param::udc,(Param::GetFloat(Param::ChipV1)+Param::GetFloat(Param::ChipV2)));
}


