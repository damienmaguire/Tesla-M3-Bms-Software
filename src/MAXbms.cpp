#include "MAXbms.h"

static uint8_t dataReady = 0;

// errorByte MASK
const uint8_t initError = 0x01;
const uint8_t mismatchBefore = 0x02;
const uint8_t mismatchAfter = 0x04;
const uint8_t receiveBuffer = 0x08;

// BMS parameters
const uint8_t numberOfSlaves = 1;
const uint8_t numberOfCellsPerSlave = 6;



void MAXbms::MaxStart()
{
    dataReady = 0x02;  // Reset data ready
    daisyChainInit();
}

void MAXbms::Task10Ms()
{
    /*
    DigIo::BatCS.Clear();
    for (int cnt = 0; cnt < 30; cnt++)
    {
        spi_xfer(SPI1, cnt);  // do a transfer
    }
    DigIo::BatCS.Set();
    */
}

void MAXbms::Task100Ms()
{

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
    }
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState, 14);

    // 15, Check for receive buffer errors
    errorByte |= receiveBufferError();
    Param::SetInt(Param::LoopState, 15);

    //SPI.endTransaction();

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
    uint8_t check = 0x01;
    while (check & 0x01)  // If RX_Status[0] (RX_Empty_Status) is false, continue. If true, then repeat transaction until false.
    {
        DigIo::BatCS.Clear();
        spi_xfer(SPI1,0x01);          // Read RX_Status register
        check = spi_xfer(SPI1,0x01);  // Read RX_Status register
        DigIo::BatCS.Set();

        Param::SetInt(Param::LoopState,7);

        if (watchdogTimer > 100)  // Watchdog timer of 10ms
        {
            watchdogTimer++;
            return;
        }
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
    uint8_t check = 0;

    // Start transmitting the loaded sequence from the transmit queue
    DigIo::BatCS.Clear();
    spi_xfer(SPI1,0xB0);  // WR_NXT_LD_Q SPI command byte (write the next load queue)
    DigIo::BatCS.Set();
    Param::SetInt(Param::LoopState, 12);

    // Check if a message has been received into the receive buffer
    while (!(check &= 0x12))  // If RX_Status[1] is true, continue. If false, then repeat transaction until true
    {
        // Poll RX_Stop_Status bit
        DigIo::BatCS.Clear();
        spi_xfer(SPI1,0x01);          // Read RX_Status register
        check = spi_xfer(SPI1,0x01);  // Read RX_Status register
        DigIo::BatCS.Set();
        Param::SetInt(Param::LoopState,13);

        if (watchdogTimer > 100)  // Watchdog timer of 10ms
        {
            //Serial.println("transmitQueue WD timeout");
            return;

        }
    }
    //delay(1);  // Needed to work, unknown why?
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
