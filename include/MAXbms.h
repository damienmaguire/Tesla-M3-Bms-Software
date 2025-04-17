#ifndef MAXBMS_h
#define MAXBMS_h

/*  This library supports MAX comms with the max chip modules and BMS slaves
    Ripped off from :
*/
#include <libopencm3/stm32/spi.h>
#include <stdint.h>
#include "params.h"
#include "digio.h"


class MAXbms
{

public:
    static void MaxStart();
    static void Task10Ms();
    static void Task100Ms();

private:
    static void daisyChainInit();
    static uint8_t receiveBufferError();
    static void transmitQueue();
    static void clearBuffers();

};


#endif /* MAXBMS_h */
