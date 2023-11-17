#ifndef BATMan_h
#define BATMan_h

/*  This library supports SPI communication for the Tesla Model 3 BMB (battery managment boards) "Batman" chip
    Model 3 packs are comprised of 2 short modules with 23 cells and 2 long modules with 25 cells for a total of 96 cells.
    A short bmb will only report 23 voltage values where as a long will report 25.
*/
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/rtc.h>
#include <stdint.h>
#include "params.h"
#include "digio.h"
#include "my_math.h"


class BATMan
{

public:
    static      void BatStart();
    static		void loop();



private:
    static      void StateMachine();
    static      void IdleWake();
    static		void GetData(uint8_t ReqID);
    static		void WakeUP();
    static		void Generic_Send_Once(uint16_t Command[], uint8_t len);
    static      void delay(int16_t FLASH_DELAY)
    {
        int i;
        for (i = 0; i < FLASH_DELAY; i++)       /* Wait a bit. */
            __asm__("nop");
    }
    static       void GetTempData();
    static       void upDateCellVolts(void);
    static       void upDateAuxVolts(void);
    static       void upDateTemps(void);
    static       void WriteCfg();
    static       uint16_t rev16(uint16_t x)
    {
        return (x << 8) | (x >>8);
    }
    static       uint8_t calcCRC(uint8_t *inData, uint8_t Length);
};

#endif /* BATMan_h */
