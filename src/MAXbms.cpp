#include "MAXbms.h"

static uint8_t dataReady = 0;

const int8_t tempMap[49] = { 79, 75, 71, 68, 65, 62, 59, 56, 54, 52, 49, 47, 45, 43, 41, 39, 38, 36, 35, 33, 31, 30, 29, 27,
                             26, 24, 23, 21, 20, 19, 17, 16, 14, 13, 11, 10, 8, 7, 5, 3, 1, -1, -2, -5, -7, -9, -12, -15, -18
                           };

// errorByte MASK
const uint8_t initError = 0x01;
const uint8_t mismatchBefore = 0x02;
const uint8_t mismatchAfter = 0x04;
const uint8_t receiveBuffer = 0x08;
// BMS parameters
const uint8_t numberOfCellsPerSlaveMax = 12;
const uint8_t numberOfTempsPerSlaveMax = 3;
static uint8_t numberOfSlaves = 1;
static uint8_t numberOfCellsPerSlave = 6;
static uint8_t CellsPresent = 0;

// Testing Variables
static uint8_t loop = 0;
static uint8_t loopstate = 0;
static bool SetupComplete = false;

//Data
// Variable declaration
static uint16_t cellVoltage[numberOfSlavesMax * numberOfCellsPerSlaveMax];
static float cellBlockTemp[3 * numberOfSlavesMax];
static int16_t dieTemperature[numberOfSlavesMax];
static bool TempToggle = false;

void MAXbms::MaxStart()
{
    numberOfSlaves = Param::GetInt(Param::numbmbs);
    dataReady = 0x02;  // Reset data ready
    daisyChainInit();
    setupSlaves();
    iwdg_reset();
    SetupComplete = true;
}

void MAXbms::Task10Ms()
{

}

void MAXbms::Task100Ms()
{
    if(loop == 0 && SetupComplete == true)
    {
        measureCellData();
        loopstate = 1;
    }

    if(loop < (Param::GetFloat(Param::VmInterval)*10))//20x100ms
    {
        loop++;
        UpdateStats();
    }
    else
    {
        loop = 0;
        loopstate = 0;
    }

    Param::SetInt(Param::LoopState, loopstate);
}

void MAXbms::SpiTest()
{
    uint8_t check;

    // 1, Enable Keep-Alive mode
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x10);  // Write Configuration 3 register
    spi_xfer(SPI1,0x05);  // Set keep-alive period to 160μs
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState,1);

    // 2, Enable Rx Interrupt flags for RX_Error and RX_Overflow
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x04);  // Write RX_Interrupt_Enable register
    spi_xfer(SPI1,0x88);  // Set the RX_Error_INT_Enable and RX_Overflow_INT_Enable bits
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState,2);

    // 3, Clear receive buffer
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0xE0);  // Clear receive buffer
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState,3);

    // 4, Wake-up UART slave devices
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x0E);  // Write Configuration 2 register
    spi_xfer(SPI1,0x30);  // Enable Transmit Preambles mode
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState,4);

    // 5, Wait for all UART slave devices to wake up
    uint32_t watchdogTimer = 0;
    check = 0;
    while (check != 0x21)  // If RX_Status = 21h, continue. Otherwise, repeat transaction until true or timeout
    {
        DigIo::BatCS.Clear();
        spi_xfer(SPI1,0x01);          // Read RX_Status register
        check = spi_xfer(SPI1,0x01);  // Read RX_Status register
        DigIo::BatCS.Set();
        Param::SetInt(Param::LoopState,5);
        if (watchdogTimer > 1000)  // Watchdog timer of 100ms
        {
            //Serial.println("UARTSlaveDevicesWakeUp WD timeout");
            return;
        }
        watchdogTimer++;
    }

    /*
        //check FMEA Reg
        DigIo::BatCS.Clear();
        spi_xfer(SPI1,0x13);  // Write Configuration 2 register
        spi_xfer(SPI1,0x13);  // Enable Transmit Preambles mode
        DigIo::BatCS.Set();

        //check Model
        DigIo::BatCS.Clear();
        spi_xfer(SPI1,0x15);  // Write Configuration 2 register
        spi_xfer(SPI1,0x15);  // Enable Transmit Preambles mode
        DigIo::BatCS.Set();

        //check Version
        DigIo::BatCS.Clear();
        spi_xfer(SPI1,0x17);  // Write Configuration 2 register
        spi_xfer(SPI1,0x17);  // Enable Transmit Preambles mode
        DigIo::BatCS.Set();
        */

}

/*******************************************
* UART Daisy-Chain Initialization Sequence *
*******************************************/
void MAXbms::daisyChainInit()
{
    uint8_t check;
    uint8_t data[4];
    uint8_t errorByte = 0x00;

    // 1, Enable Keep-Alive mode
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x10);  // Write Configuration 3 register
    spi_xfer(SPI1,0x05);  // Set keep-alive period to 160μs
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState,1);

    // 2, Enable Rx Interrupt flags for RX_Error and RX_Overflow
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x04);  // Write RX_Interrupt_Enable register
    spi_xfer(SPI1,0x88);  // Set the RX_Error_INT_Enable and RX_Overflow_INT_Enable bits
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState,2);

    // 3, Clear receive buffer
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0xE0);  // Clear receive buffer
    DigIo::BatCS.Set();
    // 3.5, Clear transmit buffer
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x20);  // Clear receive buffer
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState,3);

    // 4, Wake-up UART slave devices
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x0E);  // Write Configuration 2 register
    spi_xfer(SPI1,0x30);  // Enable Transmit Preambles mode
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState,4);

    // 5, Wait for all UART slave devices to wake up
    uint32_t watchdogTimer = 0;
    uint16_t check2 = 0;
    while (check2 != 0x21)  // If RX_Status = 21h, continue. Otherwise, repeat transaction until true or timeout
    {
        DigIo::BatCS.Clear();
        spi_xfer(SPI1,0x01);          // Read RX_Status register
        check2 = spi_xfer(SPI1,0x01);  // Read RX_Status register
        DigIo::BatCS.Set();
        if (watchdogTimer > 1000)  // Watchdog timer of 100ms
        {
            //Serial.println("UARTSlaveDevicesWakeUp WD timeout");
            return;
        }
        watchdogTimer++;
    }

    // 6, End of UART slave device wake-up period
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x0E);  // Write Configuration 2 register
    spi_xfer(SPI1,0x10);  // Disable Transmit Preambles mode
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState,6);

    // 7, Wait for null message to be received

    // 8, Clear transmit buffer

    // 9, Clear receive buffer

    clearBuffers();

    // 10, Load the HELLOALL command sequence into the load queue
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0xC0);  // WR_LD_Q SPI command byte (write the load queue)
    spi_xfer(SPI1,0x03);  // Message length
    spi_xfer(SPI1,0x57);  // HELLOALL command byte
    spi_xfer(SPI1,0x00);  // Register address (0x00)
    spi_xfer(SPI1,0x00);  // Initialization address of HELLOALL
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState,10);

    // 11, Verify contents of the load queue
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0xC1);  // RD_LD_Q SPI command byte
    Param::SetInt(Param::LoopState, 11);

    for (int i = 0; i < 4; i++)
    {
        data[i] = spi_xfer(SPI1,0xC1);
    }

    if (!((data[0] == 0x03) && (data[1] == 0x57) && (data[2] == 0x00) && (data[3] == 0x00)))
    {
        errorByte |= initError;
        //spi_xfer(SPI1,0x69);  //Error
    }
    DigIo::BatCS.Set();

    // 12, Transmit HELLOALL sequence
    // 13, Poll RX_Stop_Status bit
    transmitQueue();

    // 14, Read the HELLOALL message that propagated through the daisy-chain and was returned back to the ASCI
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x93);  // RD_NXT_MSG SPI transaction

    for (int i = 0; i < 3; i++)
    {
        data[i] = spi_xfer(SPI1,0x93);
    }

    if (!((data[0] == 0x57) && (data[1] == 0x00) && (data[2] == numberOfSlaves)))
    {
        errorByte |= initError;
        //spi_xfer(SPI1,0x69);  //Error
    }
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState, 14);

    // 15, Check for receive buffer errors
    errorByte |= receiveBufferError();
    Param::SetInt(Param::LoopState, 15);

    //DigIo::BatCS.Set();

    if (errorByte)  // Error
    {
        //Serial.println(errorByte, HEX);
        errorByte &= 0x00;  // Clear errors
        //Serial.println("errorByte cleared");
        daisyChainInit();  // Redo Daisy chain init
    }

}

/*********************************************************************
* Wait for null messages and then clear transmit and receive buffers *
*********************************************************************/
void MAXbms::clearBuffers()
{
    // Wait for null message to be received
    uint32_t watchdogTimer = 0;
    uint8_t check3 = 0x01;
    uint8_t loop3 = 1;
    while (loop3 == 1)  // If RX_Status[0] (RX_Empty_Status) is false, continue. If true, then repeat transaction until false.
    {
        DigIo::BatCS.Clear();
        spi_xfer(SPI1,0x01);          // Read RX_Status register
        check3 = spi_xfer(SPI1,0x01);  // Read RX_Status register
        if(check3 == 0x11)
        {
            loop3 = 0;
        }
        DigIo::BatCS.Set();

        //Param::SetInt(Param::LoopState,7);

        if (watchdogTimer > 100)  // Watchdog timer of 10ms
        {
            return;
        }
        watchdogTimer++;
    }

    // Clear transmit buffer
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x20);  // Clear transmit buffer
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState,8);

    // Clear receive buffer
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0xE0);  // Clear receive buffer
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState,9);
}


/********************************************************
* Start transmitting the queue and check receive buffer *
********************************************************/
void MAXbms::transmitQueue()
{
    uint32_t watchdogTimer = 0;
    uint8_t check4 = 0;
    uint8_t loop4 = 1;

    // Start transmitting the loaded sequence from the transmit queue
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0xB0);  // WR_NXT_LD_Q SPI command byte (write the next load queue)
    DigIo::BatCS.Set();
    //::SetInt(Param::LoopState, 12);

    // Check if a message has been received into the receive buffer
    while (loop4 == 1)  // If RX_Status[1] is true, continue. If false, then repeat transaction until true
    {
        // Poll RX_Stop_Status bit
        DigIo::BatCS.Clear();
        spi_xfer(SPI1,0x01);          // Read RX_Status register
        check4 = spi_xfer(SPI1,0x01);  // Read RX_Status register
        if((check4 & 0x12) > 0 && watchdogTimer > 10)
        {
            loop4 = 0;
        }
        DigIo::BatCS.Set();
        //Param::SetInt(Param::LoopState,13);

        if (watchdogTimer > 100)  // Watchdog timer of 10ms
        {
            //Serial.println("transmitQueue WD timeout");
            return;
        }
        watchdogTimer++;
    }
    watchdogTimer = 0;
    /*
        while (loop4 == 0)
        {
            watchdogTimer++;
            if (watchdogTimer > 10)  // Watchdog timer of 10ms
            {
                return;
            }
        }
        //delay(1);  // Needed to work, unknown why?
        */
}


/*********************************
* Check for receive buffer error *
*********************************/
uint8_t MAXbms::receiveBufferError()
{
    uint8_t check = 0x00;
    uint8_t errorByte = 0x00;

    // Check for receive buffer errors
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x09);  // Read RX_Interrupt_Flags register
    check = spi_xfer(SPI1,0x09);
    DigIo::BatCS.Set();

    if (!(check == 0x00))  // Error
    {
        errorByte |= receiveBuffer;  // Set status byte

        //Serial.print("Error ");
        //Serial.println(check, HEX);

        // Clear INT flag register
        DigIo::BatCS.Clear();
        spi_xfer(SPI1,0x08);  // Write RX_Interrupt_Flags register
        spi_xfer(SPI1,0x00);  // Clear flags
        DigIo::BatCS.Set();
    }

    return errorByte;
}

/************************************
* Reads data from all slave devices *
************************************/
void MAXbms::readAllSlaves(uint8_t dataRegister, bool setupDone)  // Read all slaves
{
    uint8_t command = 0x03;  // READALL
    uint8_t byteList[3] = { command, dataRegister, 0x00 };
    uint8_t PEC = calculatePEC(byteList, 3);
    uint8_t readRegisterData[(2 * numberOfSlaves + 5)];
    uint8_t errorByte = 0x00;
    static uint8_t resendCounter = 0;

    // Load the READALL command sequence into the load queue
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0xC0);  // WR_LD_Q SPI command byte (write the load queue)
    //spi_xfer(SPI1,0x1D); // Message length (5 + 2 x n = 29)
    spi_xfer(SPI1,2 * numberOfSlaves + 5);  // Message length (5 + 2 x n)
    spi_xfer(SPI1,command);                 // Command byte
    spi_xfer(SPI1,dataRegister);            // Register address
    spi_xfer(SPI1,0x00);                    // Data-check byte (seed value = 0x00)
    spi_xfer(SPI1,PEC);                     // PEC byte
    spi_xfer(SPI1,0x00);                    // Alive-counter byte (seed value = 0x00)
    DigIo::BatCS.Set();

    transmitQueue();

    // Read the receive buffer
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x93);  // RD_NXT_MSG SPI command byte

    for (int i = 0; i < (2 * numberOfSlaves + 5); i++)
    {
        readRegisterData[i] = spi_xfer(SPI1,0x93);
    }

    errorByte |= receiveBufferError();

    DigIo::BatCS.Set();

    // Verify that the device register data is received correctly during the READALL sequence
    if (!((readRegisterData[0] == command) && (readRegisterData[1] == dataRegister)))
    {
        errorByte |= mismatchBefore;
    }
    if (setupDone)
    {
        uint8_t checkPEC = calculatePEC(readRegisterData, (2 * numberOfSlaves + 3));
        // Check check-byte, PEC and alive-counter
        if (!(((readRegisterData[(2 * numberOfSlaves + 2)] == 0x00)||(readRegisterData[(2 * numberOfSlaves + 2)] == 0x40)) && (readRegisterData[(2 * numberOfSlaves + 3)] == checkPEC) && (readRegisterData[(2 * numberOfSlaves + 4)] == numberOfSlaves)))
        {
            errorByte |= mismatchAfter;
            if (readRegisterData[(2 * numberOfSlaves + 2)] != 0x00)
            {
                readAllSlaves(0x02, false);          // Read STATUS of all slaves with checks ignored
                writeAllSlaves(0x02, 0x0000, true);  // Clear STATUS register
                //Serial.println("STATUS cleared");
            }
        }
        if((readRegisterData[(2 * numberOfSlaves + 2)] == 0x40))
        {
            Param::SetInt(Param::PecErrCnt,Param::GetInt(Param::PecErrCnt)+1);
        }
    }

    if (errorByte)  // Error
    {
        resendCounter++;
        //Serial.println(errorByte, HEX);
        errorByte &= 0x00;  // Clear errors
        //Seriaprintlnl.("errorByte cleared");

        if (resendCounter > 2)
        {
            ////Serial.println("READALL fail");
            resendCounter = 0;
            //failCounter++;
            return;
        }

        //delay(1);
        readAllSlaves(dataRegister, setupDone);  // Resend READALL
    }
    else                                       // No errors, clear counters and store data received
    {
        resendCounter = 0;  // Reset counter
        //failCounter = 0; // Reset counter

        if ((dataRegister >= 0x20) && (dataRegister <= 0x2B))  // Cell voltage measurements
        {
            storeCellVoltage(dataRegister, readRegisterData);
        }

        if ((dataRegister == 0x2D) || (dataRegister == 0x2E))  // Aux voltage measurements (external temperature sensors)
        {
            storeCellTemperature(dataRegister, readRegisterData);
        }

        if (dataRegister == 0x50)  // Die temperature measurements
        {
            storeDieTemperature(readRegisterData);
        }

        if (dataRegister == 0x13)  // Read SCANCTRL to check if data is ready to be read
        {
            checkDataReady(readRegisterData);
        }

        if (dataRegister == 0x02)  // Read STATUS and print content
        {
            // Print data received
            for (int i = 0; i < (2 * numberOfSlaves + 5); i++)
            {
                //Serial.print(readRegisterData[i], HEX);
                //Serial.print(" ");
            }
            //Serial.println();
        }
    }
}


/*****************************************
* Reads data from addressed slave device *
*****************************************/
void MAXbms::readAddressedSlave(uint8_t dataRegister, uint8_t address, bool setupDone)  // Read addressed slave
{
    uint8_t command = 0x05;  // READDEVICE
    command |= (address << 3);
    uint8_t byteList[3] = { command, dataRegister, 0x00 };
    uint8_t PEC = calculatePEC(byteList, 3);
    uint8_t readRegisterData[7];
    uint8_t errorByte = 0x00;
    static uint8_t resendCounter = 0;
    //static uint8_t failCounter = 0;


    // 1, Load the READDEVICE command sequence into the load queue
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0xC0);          // WR_LD_Q SPI command byte (write the load queue)
    spi_xfer(SPI1,0x07);          // Message length (5 + 2 x n)
    spi_xfer(SPI1,command);       // Command byte
    spi_xfer(SPI1,dataRegister);  // Register address
    spi_xfer(SPI1,0x00);          // Data-check byte (seed value = 0x00)
    spi_xfer(SPI1,PEC);           // PEC byte
    spi_xfer(SPI1,0x00);          // Alive-counter byte (seed value = 0x00)
    DigIo::BatCS.Set();

    transmitQueue();

    // 4, Read the receive buffer
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x93);  // RD_NXT_MSG SPI command byte

    for (int i = 0; i < 7; i++)
    {
        readRegisterData[i] = spi_xfer(SPI1,0x93);
    }

    errorByte |= receiveBufferError();

    DigIo::BatCS.Set();

    // Verify that the device register data is received correctly during the READDEVICE sequence
    if (!((readRegisterData[0] == command) && (readRegisterData[1] == dataRegister)))
    {
        errorByte |= mismatchBefore;
    }
    if (setupDone)
    {
        uint8_t checkPEC = calculatePEC(readRegisterData, 5);
        // Check check-byte, PEC and alive-counter
        if (!((readRegisterData[4] == 0x00) && (readRegisterData[5] == checkPEC) && (readRegisterData[6] == 0x01)))
        {
            errorByte |= mismatchAfter;
            if (readRegisterData[4] != 0x00)
            {
                readAddressedSlave(0x02, address, false);          // Read STATUS of addressed slave with checks ignored
                writeAddressedSlave(0x02, 0x0000, address, true);  // Clear STATUS register
                //Serial.println("STATUS cleared");
            }
        }
    }


    if (errorByte)  // Error
    {
        resendCounter++;
        //Serial.println(errorByte, HEX);
        errorByte &= 0x00;  // Clear errors
        //Serial.println("errorByte cleared");

        if (resendCounter > 2)
        {
            //Serial.println("READEVICE fail");
            resendCounter = 0;
            //failCounter++;
            return;
        }

        //delay(1);
        readAddressedSlave(dataRegister, address, setupDone);  // Resend READDEVICE
    }
    else                                                     // No errors, clear counters and store data received
    {
        resendCounter = 0;  // Reset counter
        //failCounter = 0; // Reset counter

        if (dataRegister == 0x02)  // Read STATUS and print content
        {
            /*
             // Print data received
             for (int i = 0; i < 7; i++) {
               Serial.print(readRegisterData[i], HEX);
               Serial.print(" ");
             }
             Serial.println();
             */
        }
    }
}


/***********************************
* Writes data to all slave devices *
***********************************/
void MAXbms::writeAllSlaves(uint8_t dataRegister, uint16_t data, bool setupDone)
{
    uint8_t command = 0x02;  // WRITEALL
    uint8_t byteList[4] = { command, dataRegister, lowByte(data), highByte(data) };
    uint8_t PEC = calculatePEC(byteList, 4);
    uint8_t readRegisterData[6];
    uint8_t errorByte = 0x00;
    static uint8_t resendCounter = 0;
    //static uint8_t failCounter = 0;

    // 1, Load the WRITEALL command sequence into the load queue
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0xC0);            // WR_LD_Q SPI command byte (write the load queue)
    spi_xfer(SPI1,0x06);            // Message length
    spi_xfer(SPI1,command);         // Command byte
    spi_xfer(SPI1,dataRegister);    // Register address
    spi_xfer(SPI1,lowByte(data));   // LS byte of register data to be written
    spi_xfer(SPI1,highByte(data));  // MS byte of register data to be written
    spi_xfer(SPI1,PEC);             // PEC byte
    spi_xfer(SPI1,0x00);            // Alive-counter byte (seed value = 0x00)
    DigIo::BatCS.Set();

    transmitQueue();

    // 4, Read the receive buffer
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x93);  // RD_NXT_MSG SPI command byte

    for (int i = 0; i < 6; i++)
    {
        readRegisterData[i] = spi_xfer(SPI1,0x93);
    }
    DigIo::BatCS.Set();

    errorByte |= receiveBufferError();

    DigIo::BatCS.Set();

    // Verify that the device register data is what was written during the WRITEALL sequence
    if (!((readRegisterData[0] == command) && (readRegisterData[1] == dataRegister) && (readRegisterData[2] == lowByte(data)) && (readRegisterData[3] == highByte(data)) && (readRegisterData[4] == PEC)))
    {
        errorByte |= mismatchBefore;
    }
    if (setupDone)
    {
        // Check alive-counter
        if (!(readRegisterData[5] == numberOfSlaves))
        {
            errorByte |= mismatchAfter;
        }
    }

    /*// Print data received
    for(int i=0; i<6; i++)
    {
      Serial.print(readRegisterData[i], HEX);
      Serial.print(" ");
    }
    Serial.println();*/

    if (errorByte)  // Error
    {
        resendCounter++;
        //Serial.println(errorByte, HEX);
        errorByte &= 0x00;  // Clear errors
//Serial.println("errorByte cleared");

        if (resendCounter > 2)
        {
            //Serial.println("WRITEALL fail");
            resendCounter = 0;
            //failCounter++;
            return;
        }

        //delay(1);
        writeAllSlaves(dataRegister, data, setupDone);  // Resend WRITEALL
    }
    else                                              // No errors, clear counters
    {
        resendCounter = 0;  // Reset counter
        //failCounter = 0; // Reset counter
    }
}


/****************************************
* Writes data to addressed slave device *
****************************************/
void MAXbms::writeAddressedSlave(uint8_t dataRegister, uint16_t data, uint8_t address, bool setupDone)  // Write addressed slave
{
    uint8_t command = 0x04;  // WRITEDEVICE
    command |= (address << 3);
    uint8_t byteList[4] = { command, dataRegister, lowByte(data), highByte(data) };
    uint8_t PEC = calculatePEC(byteList, 4);
    uint8_t readRegisterData[6];
    uint8_t errorByte = 0x00;
    static uint8_t resendCounter = 0;
    //static uint8_t failCounter = 0;

    // 1, Load the WRITEDEVICE command sequence into the load queue
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0xC0);            // WR_LD_Q SPI command byte (write the load queue)
    spi_xfer(SPI1,0x06);            // Message length
    spi_xfer(SPI1,command);         // Command byte
    spi_xfer(SPI1,dataRegister);    // Register address
    spi_xfer(SPI1,lowByte(data));   // LS byte of register data to be written
    spi_xfer(SPI1,highByte(data));  // MS byte of register data to be written
    spi_xfer(SPI1,PEC);             // PEC byte
    spi_xfer(SPI1,0x00);            // Alive-counter byte (seed value = 0x00)
    DigIo::BatCS.Set();

    transmitQueue();

    // 4, Read the receive buffer
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0x93);  // RD_NXT_MSG SPI command byte

    for (int i = 0; i < 6; i++)
    {
        readRegisterData[i] = spi_xfer(SPI1,0x93);
    }
    DigIo::BatCS.Set();

    errorByte |= receiveBufferError();

    DigIo::BatCS.Set();

    // Verify that the device register data is what was written during the WRITEDEVICE sequence
    if (!((readRegisterData[0] == command) && (readRegisterData[1] == dataRegister) && (readRegisterData[2] == lowByte(data)) && (readRegisterData[3] == highByte(data)) && (readRegisterData[4] == PEC)))
    {
        errorByte |= mismatchBefore;
    }
    if (setupDone)
    {
        // Check alive-counter
        if (!(readRegisterData[5] == 0x01))
        {
            errorByte |= mismatchAfter;
        }
    }


    if (errorByte)  // Error
    {
        resendCounter++;
//Serial.println(errorByte, HEX);
        errorByte &= 0x00;  // Clear errors
        //Serial.println("errorByte cleared");

        if (resendCounter > 2)
        {
            //Serial.println("WRITEDEVICE fail");
            resendCounter = 0;
            //failCounter++;
            return;
        }

        //delay(1);
        writeAddressedSlave(dataRegister, data, address, setupDone);  // Resend WRITEDEVICE
    }
    else                                                            // No errors, clear counters
    {
        resendCounter = 0;  // Reset counter
        //failCounter = 0; // Reset counter
    }
}


/**********************************************
* Reads data from slaves and stores in arrays *
**********************************************/
void MAXbms::readData()
{

    for (int i = 0; i < numberOfCellsPerSlave; i++)
    {
        readAllSlaves((0x20 + i), true);  // Read CELL 1-numberOfCellsPerSlave of all slaves
    }
    //Need to drive GPIO to force logic gates to connect temp sensors GPIO 0x11 set to 0x7001 - NTC1-AIN1 + NTC2-AIN2
    // GPIO 0x11 set to 0x7002 - NTC3-AIN2 + AIN1 - nothing
    readAllSlaves(0x2D, true);  // Read AIN1 of all slaves (Cell temperature)
    readAllSlaves(0x2E, true);  // Read AIN2 of all slaves (Cell temperature)
    readAllSlaves(0x50, true);  // Read DIAG (Die temperature) of all slaves
}

/*********************************************************
* Stores measured cell voltage data in cellVoltage array *
*********************************************************/
void MAXbms::storeCellVoltage(uint8_t dataRegister, uint8_t readRegisterData[])
{
    for (int i = 0; i < numberOfSlaves; i++)
    {
        uint16_t measVoltage = ((readRegisterData[(2 * numberOfSlaves + 1) - i * 2] << 8) + readRegisterData[(2 * numberOfSlaves) - i * 2]);
        measVoltage = (measVoltage >> 2) * (uint32_t)5000 / 0x3FFF;
        cellVoltage[i * numberOfCellsPerSlave + (dataRegister - 0x20)] = measVoltage;
        Param::SetFloat((Param::PARAM_NUM)(Param::u1 + (i * numberOfCellsPerSlave + (dataRegister - 0x20))), measVoltage);
    }
}


/***************************************************************************
* Stores measured temperature sensor voltage data in cellTemperature array *
***************************************************************************/
void MAXbms::storeCellTemperature(uint8_t dataRegister, uint8_t readRegisterData[])
{
    for (int i = 0; i < numberOfSlaves; i++)
    {
        /* Old KIA Code
        //uint16_t beta = 3800;
        uint16_t dataADC = ((readRegisterData[(2 * numberOfSlaves + 1) - i * 2] << 8) + readRegisterData[(2 * numberOfSlaves) - i * 2]);
        dataADC = (dataADC >> 10);  // >> 4 for 12bit value & >> 6 for correct array index size
        //int8_t temperature = beta / (log((float)dataADC / (4095 - dataADC)) + beta / 298.15) - 273;
        if (dataADC < 7)
        {
            dataADC = 7;
        }
        if (dataADC > 55)
        {
            dataADC = 55;
        }
        */

        uint16_t measTemperature = uint16_t(uint16_t(readRegisterData[3 + (i * 2)] << 4) + uint16_t(readRegisterData[2 + (i * 2)]>>4));

        if(TempToggle)
        {
            if (0x2E == dataRegister)//Toggled is NTC3
            {
                cellBlockTemp[i * 3 + 2] = float(2450 - measTemperature)/31;
                //Param::SetFloat(Param::Cellt1_0,cellBlockTemp[i * 3 + 2]);
                //Param::SetFloat(Param::Cellt3_0,measTemperature);
                //Param::SetFloat((Param::PARAM_NUM)(Param::Cellt1_0 + i*2),cellBlockTemp[i * 3 + 2]); // For now use Temp 3 as 2
            }
        }
        else
        {
            if (0x2D == dataRegister)//Not toggled so NTC1
            {
                cellBlockTemp[i * 3] = float(measTemperature - 1680)/32;
                Param::SetFloat((Param::PARAM_NUM)(Param::Cellt0_0 + i*2), cellBlockTemp[i * 3]);
                //Param::SetFloat(Param::Cellt2_0,measTemperature);
                //Param::SetFloat((Param::PARAM_NUM)(Param::Cellt0_0 + i*2),cellBlockTemp[i * 3]); //Not reading correctly right now
            }
            else if (0x2E == dataRegister)//Not toggled so NTC2
            {
                cellBlockTemp[i * 3 + 1] = float(2450 - measTemperature)/31;

                //Param::SetFloat(Param::Cellt2_1,measTemperature);
                //Param::SetFloat((Param::PARAM_NUM)(Param::Cellt0_1 + i*2),cellBlockTemp[i * 3 + 1]); // For now use Temp 2 as1
            }
        }

        if (0x2E == dataRegister)//NTC2 or NTC3
        {
            if(cellBlockTemp[i * 3 + 1]> cellBlockTemp[i * 3 + 2])//if NTC 2 is higher then NTC 3
            {
                Param::SetFloat((Param::PARAM_NUM)(Param::Cellt0_1 + i*2), cellBlockTemp[i * 3 + 1]);
            }
            else
            {
                Param::SetFloat((Param::PARAM_NUM)(Param::Cellt0_1 + i*2), cellBlockTemp[i * 3 + 2]);
            }
        }
    }
}


/***************************************************************
* Stores measured die temperature data in dieTemperature array *
***************************************************************/
void MAXbms::storeDieTemperature(uint8_t readRegisterData[])
{
    for (int i = 0; i < numberOfSlaves; i++)
    {
        uint16_t measTemperature = ((readRegisterData[((2 * numberOfSlaves + 1)) - i * 2] << 8) + readRegisterData[(2 * numberOfSlaves) - i * 2]);
        measTemperature = (measTemperature >> 2) * (uint32_t)230700 / 5029581;
        dieTemperature[i] = measTemperature - (int16_t)273;
        Param::SetFloat((Param::PARAM_NUM)(Param::Chipt0 + i), dieTemperature[i]);
    }
}


/****************************************************
* Check if data is ready to be read from all slaves *
****************************************************/
void MAXbms::checkDataReady(uint8_t readRegisterData[])
{
    dataReady = 0x01;
    for (int i = 0; i < numberOfSlaves; i++)
    {
        if (!(readRegisterData[(2 * numberOfSlaves + 1) - i * 2] & 0x80))
            dataReady = 0x00;
    }
}


/***********************************************************
* PEC Calculation, CRC-8, from MAX17823 datasheet, page 96 *
***********************************************************/
uint8_t MAXbms::calculatePEC(uint8_t byteList[], uint8_t numberOfBytes)
{
    uint8_t CRCByte = 0;
    uint8_t POLY = 0xB2;

    for (int byteCounter = 0; byteCounter < numberOfBytes; byteCounter++)
    {
        CRCByte ^= byteList[byteCounter];

        for (int bitCounter = 0; bitCounter < 8; bitCounter++)
        {
            CRCByte = (CRCByte & 0x01) ? ((CRCByte >> 1) ^ POLY) : (CRCByte >> 1);
        }
    }

    return CRCByte;
}


/******************************************************
* Measures cell voltages and temperatures from slaves *
******************************************************/
void MAXbms::measureCellData()
{
    uint32_t watchdogTimer = 0;

    dataReady = 0x00;

    if(TempToggle)//toggle between one set and the other
    {
        writeAllSlaves(0x11, 0x7001, true); // Set DRV 1 - Cell temp 1 +2
        TempToggle = false; //set to read NTC1+2
    }
    else
    {
        writeAllSlaves(0x11, 0x7002, true); // // Set DRV 1 - Cell temp 3 +4
        TempToggle = true; //set to read NTC3
    }

    //writeAllSlaves(0x13, 0x0001, true); // Set SCANCTRL, SCAN
    writeAllSlaves(0x13, 0x0041, true);  // Set SCANCTRL, SCAN and 32 oversamples

    while (!dataReady)  // Wait for data ready
    {
        readAllSlaves(0x13, true);  // Read SCANCTRL to check when data is ready to be read

        if (watchdogTimer > 150)  // Watchdog timer of 15ms !!!TODO check if this works
        {
            //Serial.println("measureCellData WD timeout");
            return;
        }
        watchdogTimer++;//incremebt
    }

    readData();  // Read and store data
    dataReady = 0x02;
}


/********************
* Slave board setup *
********************/
void MAXbms::setupSlaves()
{
    // Setup
    readAllSlaves(0x01, false);           // Read ADDRESS of all slaves
    readAllSlaves(0x02, false);           // Read STATUS of all slaves
    writeAllSlaves(0x02, 0x0000, false);  // Clear STATUS register
    writeAllSlaves(0x10, 0x0040, false);  // Set DEVCFG1, ALIVECNTEN
    //setupDone = true;

    // Enable measurement
    writeAllSlaves(0x12, 0x3FFF, true);  // Set MEASUREEN, Enable cell voltages 1-12 and AUX1-2
    writeAllSlaves(0x1E, 0x000C, true);  // Set TOPCELL, Top cell for measurement is 12
    writeAllSlaves(0x51, 0x0006, true);  // Set DIAGCFG, Enable Die temperature measurement
    //writeAllSlaves(0x19, 0x003F, true); // Set ACQCFG, 64x6us settling time before AUX1 measurement
    //writeAllSlaves(0x18, 0x1500, true); // Set WATCHDOG, Set watchdg for cell balancing to 5s
    //writeAllSlaves(0x13, 0x0001, true); // Set SCANCTRL, SCAN
    //Switch on THRM voltage
    writeAllSlaves(0x19, 0x033F, true); // Set ACQCFG, Maunal Thrmmode AUX voltage, set measure to max.
}

uint8_t MAXbms::lowByte(int data)
{
    return(data & 0xFF);
}

uint8_t MAXbms::highByte(int data)
{
    return((data >> 8) & 0xFF);
}

void MAXbms::UpdateStats()
{
    uint32_t Vtotal = 0;
    uint16_t MaxVoltTemp = 0;
    uint16_t CellMax = 0;
    uint16_t MinVoltTemp = 9000;
    uint16_t CellMin = 0;
    float MinTempTemp = -90.0;
    float MaxTempTemp = 390.0;
    uint16_t CellCnt = 0;

    for (int h = 0; h < numberOfSlaves; h++) //go through all connected slaves
    {
        for (int g = 0; g < numberOfCellsPerSlaveMax; g++) //go through all voltages
        {
            if(cellVoltage[(h * numberOfCellsPerSlaveMax)+g] > MaxVoltTemp) //check max volt
            {
                MaxVoltTemp = cellVoltage[(h * numberOfCellsPerSlaveMax)+g];
                CellMax = CellCnt + 1;
            }
            if(cellVoltage[(h * numberOfCellsPerSlaveMax)+g] > 1000)//1000mV threshold check cell present
            {
                CellCnt++;
                Vtotal = Vtotal + cellVoltage[(h * numberOfCellsPerSlaveMax)+g];

                if(cellVoltage[(h * numberOfCellsPerSlaveMax)+g] < MinVoltTemp)//check min volt
                {
                    MinVoltTemp = cellVoltage[(h * numberOfCellsPerSlaveMax)+g];
                    CellMin = CellCnt;
                }
            }
        }

        for (int j = 0; j < numberOfTempsPerSlaveMax; j++) //go through all Temps
        {
            if(cellBlockTemp[j + (3 * h)] > MinTempTemp)
            {
                MinTempTemp  = cellBlockTemp[j + (3 * h)];
            }

            if(cellBlockTemp[j + (3 * h)] < MaxTempTemp)
            {
                MaxTempTemp  = cellBlockTemp[j + (3 * h)];
            }
        }

    }

    Param::SetInt(Param::CellMax,CellMax);
    Param::SetInt(Param::CellMin,CellMin);
    Param::SetFloat(Param::umax,MaxVoltTemp);
    Param::SetFloat(Param::umin,MinVoltTemp);
    Param::SetFloat(Param::deltaV,MaxVoltTemp-MinVoltTemp);
    Param::SetInt(Param::CellsPresent,CellCnt);

    Param::SetFloat(Param::udc, float(Vtotal*0.001));

    Param::SetInt(Param::uavg,(Param::GetFloat(Param::udc)/Param::GetInt(Param::CellsPresent)*1000));

    Param::SetFloat(Param::TempMax,MaxTempTemp);
    Param::SetFloat(Param::TempMin,MinTempTemp);

}
