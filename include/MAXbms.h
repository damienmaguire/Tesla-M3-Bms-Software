#ifndef MAXBMS_h
#define MAXBMS_h

/*  This library supports MAX comms with the max chip modules and BMS slaves
    Ripped off from :
*/
#include <libopencm3/stm32/spi.h>
#include <stdint.h>
#include "params.h"
#include "digio.h"

#define numberOfSlaves 1

class MAXbms
{

public:
    static void MaxStart();
    static void Task10Ms();
    static void Task100Ms();

private:
    //const uint8_t numberOfSlaves = 1;

    static void daisyChainInit();
    static uint8_t receiveBufferError();
    static void transmitQueue();
    static void clearBuffers();
    static void SpiTest();
    static void readAllSlaves(uint8_t dataRegister, bool setupDone);
    static void readAddressedSlave(uint8_t dataRegister, uint8_t address, bool setupDone);
    static void writeAllSlaves(uint8_t dataRegister, uint16_t data, bool setupDone);
    static void writeAddressedSlave(uint8_t dataRegister, uint16_t data, uint8_t address, bool setupDone);
    static void readData();
    static void storeCellVoltage(uint8_t dataRegister, uint8_t readRegisterData[]);
    static void storeCellTemperature(uint8_t dataRegister, uint8_t readRegisterData[]);
    static void storeDieTemperature(uint8_t readRegisterData[]);
    static void checkDataReady(uint8_t readRegisterData[]);
    static uint8_t calculatePEC(uint8_t byteList[], uint8_t numberOfBytes);
    static void setupSlaves();
    static void measureCellData();
    static uint8_t lowByte(int data);
    static uint8_t highByte(int data);
};


#endif /* MAXBMS_h */
