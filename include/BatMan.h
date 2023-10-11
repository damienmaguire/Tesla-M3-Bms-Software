#ifndef BATMan_h
#define BATMan_h

/*  This library supports SPI communication for the Tesla Model 3 BMB (battery managment boards) "Batman" chip

*/
#include <libopencm3/stm32/spi.h>
#include <stdint.h>
#include "params.h"
#include "digio.h"


class BATMan
{

public:
       static		void loop();



private:
       static		void GetData(uint16_t Request[2], uint8_t ReqID);
       static		void WakeUP();
       static		void Generic_Send_Once(uint16_t Command[], uint8_t len);
       static       void delay(int16_t FLASH_DELAY)
                         {
                           int i;
                           for (i = 0; i < FLASH_DELAY; i++)       /* Wait a bit. */
                           __asm__("nop");
                         }
       static       void GetTempData();
       static       void upDateVolts(void);
       static       void upDateTemps(void);
};

#endif /* BATMan_h */
