/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/iwdg.h>
#include "stm32_can.h"
#include "canmap.h"
#include "cansdo.h"
//#include "canobd2.h"
#include "terminal.h"
#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "hwinit.h"
#include "anain.h"
#include "param_save.h"
#include "my_math.h"
#include "errormessage.h"
#include "printf.h"
#include "stm32scheduler.h"
#include "terminalcommands.h"
#include "BatMan.h"
#include "ModelS.h"
#include "CAN_Common.h"

#define PRINT_JSON 0

extern "C" void __cxa_pure_virtual()
{
    while (1);
}

static Stm32Scheduler* scheduler;
static CanHardware* can;
static CanMap* canMap;

//sample 100ms task
static void Ms100Task(void)
{
    DigIo::led_out.Toggle();
    iwdg_reset();
    float cpuLoad = scheduler->GetCpuLoad();
    Param::SetFloat(Param::cpuload, cpuLoad / 10);

    BATMan::loop();

    CAN_Common::Task100Ms();
}

//sample 10 ms task
static void Ms10Task(void)
{
    //Set timestamp of error message
    ErrorMessage::SetTime(rtc_get_counter_val());


}

/** This function is called when the user changes a parameter */
void Param::Change(Param::PARAM_NUM paramNum)
{
    switch (paramNum)
    {
    default:
        //Handle general parameter changes here. Add paramNum labels for handling specific parameters
        break;
    }
}

static void HandleClear()//Must add the ids to be received here as this set the filters.
{
    can->RegisterUserMessage(0x100);

}

static bool CanCallback(uint32_t id, uint32_t data[2])//, uint8_t dlc)//Here we decide what to to with the received ids. e.g. call a function in another class etc.
{

    switch (id)
    {
    case 0x100:
        CAN_Common::HandleCan(data);//can also pass the id and dlc if required to do further work downstream.
        break;
    default:

        break;
    }
    return false;

}

//Whichever timer(s) you use for the scheduler, you have to
//implement their ISRs here and call into the respective scheduler
extern "C" void tim2_isr(void)
{
    scheduler->Run();
}

extern "C" int main(void)
{
    extern const TERM_CMD termCmds[];

    clock_setup(); //Must always come first
    rtc_setup();
    ANA_IN_CONFIGURE(ANA_IN_LIST);
    DIG_IO_CONFIGURE(DIG_IO_LIST);
    gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, AFIO_MAPR_CAN1_REMAP_PORTB);//Remap CAN pins to Portb alt funcs.
    AnaIn::Start(); //Starts background ADC conversion via DMA
    write_bootloader_pininit(); //Instructs boot loader to initialize certain pins

    nvic_setup(); //Set up some interrupts
    parm_load(); //Load stored parameters
    spi1_setup();// SPI1 for Model 3 BMB modules
    usart1_setup();//Usart 1 for Model S / X slaves

    Stm32Scheduler s(TIM2); //We never exit main so it's ok to put it on stack
    scheduler = &s;

    //Initialize CAN1, including interrupts. Clock must be enabled in clock_setup()
    Stm32Can c(CAN1, CanHardware::Baud500,true);
    FunctionPointerCallback cb(CanCallback, HandleClear);

//store a pointer for easier access
    can = &c;
    //c.SetNodeId(2);
    c.AddCallback(&cb);
    CanMap cm(&c);
    CanSdo sdo(&c, &cm);
    TerminalCommands::SetCanMap(&cm);
    HandleClear();
    sdo.SetNodeId(2);

    canMap = &cm;

    CAN_Common::SetCan(&c);

    Terminal t(USART3, termCmds);
    TerminalCommands::SetCanMap(canMap);

    BATMan::BatStart();

    s.AddTask(Ms10Task, 10);
    s.AddTask(Ms100Task, 100);

    Param::SetInt(Param::version, 4);
    Param::Change(Param::PARAM_LAST); //Call callback one for general parameter propagation

    while(1)
    {
        char c = 0;
        t.Run();
        if (sdo.GetPrintRequest() == PRINT_JSON)
        {
            TerminalCommands::PrintParamsJson(&sdo, &c);
        }
    }

    return 0;
}

