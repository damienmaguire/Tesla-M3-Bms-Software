#include "BatMan.h"

uint16_t WakeUp[2] = {0x2ad4, 0x0000};
uint16_t Com[2] = {0x21f2, 0x4d00};
uint16_t Com2[2] = {0x2BFB, 0x0000};
uint16_t send5 = 0x0008;
uint16_t reqTemp = 0x0E1B;
uint16_t sendX[2] = {0x0000, 0x0000}; //place holder array for random messages
uint16_t req47[2] = {0x4700, 0x7000};
uint16_t req48[2] = {0x4800, 0x3400} ;
uint16_t req49[2] = {0x4900, 0xdd00};
uint16_t req4a[2] = {0x4a00, 0xc900};
uint16_t req4b[2] = {0x4b00, 0x2000};
uint16_t req4c[2] = {0x4c00, 0xe100};
uint16_t req4d[2] = {0x4D00, 0x0800};
uint16_t req4e[2] = {0x4E00, 0x3300};
uint16_t req4f[2] = {0x4F00, 0xF500};
uint16_t req50[2] = {0x5000, 0x9400};
uint16_t padding = 0x0000;
uint16_t Request_A = 0x0000;
uint16_t Request_B = 0x0000;
uint16_t receive1 = 0;
uint16_t receive2 = 0;
float tempval1 = 0;
float tempval2 = 0;
uint8_t  Fluffer[72];
uint16_t FlufferT[24][2];
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

uint16_t Temps [4][6] = {
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0},
};



void BATMan::loop()
{
    LoopTimer1--;
    if(LoopTimer1==0)//Do the loop every 500ms.
    {
    LoopTimer1 = 5;
    WakeUP();//send wake up 4 times for 4 bmb boards
    WakeUP();
    WakeUP();
    WakeUP();
    Generic_Send_Once(Com, 2);//unmute
    WakeUP();
    GetData(req4d, 0x4D);//Read Aux A
    GetData(req4e, 0x4E);//Read Aux B
    GetData(req4e, 0x4E);//Read Aux B
    GetData(req50, 0x50);//Read Cfg
    Generic_Send_Once(Com2, 1);//Take a snapshot of the cell voltages
    delay(100);
    Generic_Send_Once(Com2, 1);//Take a snapshot of the cell voltages
    GetData(req4f, 0x4F);//Read status
    GetData(req4f, 0x4F);//Read status
    GetData(req47, 0x47);//Read A
    GetData(req48, 0x48);//Read B
    GetData(req49, 0x49);//Read C
    GetData(req4a, 0x4A);//Read D
    GetData(req4b, 0x4B);//Read E
    GetData(req4c, 0x4C);//Read F
    Generic_Send_Once(Com, 2);//Snapshot
    delay(100);
    Generic_Send_Once(Com, 2);//Snapshot
    GetTempData();
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

//  if (Fluffer != 0xffff)
 // {
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
  DigIo::BatCS.Clear();
  receive1 = spi_xfer(SPI1, reqTemp);  // do a transfer
  count1=0;
  count2=0;
  for (count3 = 0; count3 < 32; count3 ++)
  {
    receive1 = spi_xfer(SPI1, padding);  // do a transfer
    count1++;
    if (count1<4)
     {
      //Fluffer[count2][0] =receive1;
      FlufferT[count2][0] = receive1 >> 8;
      FlufferT[count2][1] = receive1 & 0xFF;
      count2++;
     }
    else count1=0;
  }
   DigIo::BatCS.Set();
   uint16_t tempval = 0;
  if (FlufferT[0][1] != 0xff)
  {
    for (int p = 0; p < 4; p++)   //pack
    {
      for (int c = 0; c < 6; c++)  //sens
        {
           if (FlufferT[p*6+c][1] != 0xff)
            {
              //tempvol = 0xC412;
              //tempvol = Fluffer[p*6+c][0];
              tempval = FlufferT[p*6+c][1] * 256 + FlufferT [p*6+c] [0];
              Temps[p][c] = tempval;
            }
        }
    }
  }

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
  delay(2000);
}

void BATMan::upDateVolts(void)
{
for (int h = 1; h <= 12; h++)
{
Param::SetFloat((Param::PARAM_NUM)(Param::u0 + h), Voltage[0][h]);
Param::SetFloat((Param::PARAM_NUM)(Param::u12 + h), Voltage[1][h]);
}


}

void BATMan::upDateTemps(void)
{
for (int g = 0; g < 2; g++)
{
tempval1=Temps[0][g*3+1];
         if (tempval1 >= (1131))
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


}


