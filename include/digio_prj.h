#ifndef PinMode_PRJ_H_INCLUDED
#define PinMode_PRJ_H_INCLUDED

#include "hwdefs.h"

/* Here you specify generic IO pins, i.e. digital input or outputs.
 * Inputs can be floating (INPUT_FLT), have a 30k pull-up (INPUT_PU)
 * or pull-down (INPUT_PD) or be an output (OUTPUT)
*/

#define DIG_IO_LIST \
    DIG_IO_ENTRY(led_out,     GPIOC, GPIO13, PinMode::OUTPUT)      \
    DIG_IO_ENTRY(batm_en,     GPIOB, GPIO12, PinMode::OUTPUT)      \
    DIG_IO_ENTRY(out1,        GPIOA, GPIO2, PinMode::OUTPUT)      \
    DIG_IO_ENTRY(out2,        GPIOA, GPIO3, PinMode::OUTPUT)      \
    DIG_IO_ENTRY(BatCS,       GPIOA, GPIO4, PinMode::OUTPUT)      \
    DIG_IO_ENTRY(in1,         GPIOB, GPIO3, PinMode::INPUT_FLT)      \
    DIG_IO_ENTRY(in2,         GPIOB, GPIO4, PinMode::INPUT_FLT)      \

#endif // PinMode_PRJ_H_INCLUDED
