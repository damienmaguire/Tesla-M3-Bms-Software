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
#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/crc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/spi.h>
#include "hwdefs.h"
#include "hwinit.h"
#include "stm32_loader.h"
#include "my_string.h"
#include "params.h"

/**
* Start clocks of all needed peripherals
*/
void clock_setup(void)
{
    RCC_CLOCK_SETUP();

    //The reset value for PRIGROUP (=0) is not actually a defined
    //value. Explicitly set 16 preemtion priorities
    SCB_AIRCR = SCB_AIRCR_VECTKEY | SCB_AIRCR_PRIGROUP_GROUP16_NOSUB;

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOD);
    rcc_periph_clock_enable(RCC_USART3);
    rcc_periph_clock_enable(RCC_USART1);//Model S slaves
    rcc_periph_clock_enable(RCC_TIM2); //Scheduler
    rcc_periph_clock_enable(RCC_DMA1);  //ADC, Encoder and UART receive
    rcc_periph_clock_enable(RCC_ADC1);
    rcc_periph_clock_enable(RCC_CRC);
    rcc_periph_clock_enable(RCC_AFIO); //CAN
    rcc_periph_clock_enable(RCC_CAN1); //CAN
    rcc_periph_clock_enable(RCC_SPI1); //SPI1 for BATMAN!
}

/* Some pins should never be left floating at any time
 * Since the bootloader delays firmware startup by a few 100ms
 * We need to tell it which pins we want to initialize right
 * after startup
 */
void write_bootloader_pininit()
{
    uint32_t flashSize = desig_get_flash_size();
    uint32_t pindefAddr = FLASH_BASE + flashSize * 1024 - PINDEF_BLKNUM * PINDEF_BLKSIZE;
    const struct pincommands* flashCommands = (struct pincommands*)pindefAddr;

    struct pincommands commands;

    memset32((int*)&commands, 0, PINDEF_NUMWORDS);

    //!!! Customize this to match your project !!!
    //Here we specify that PC13 be initialized to ON
    //AND PB1 AND PB2 be initialized to OFF
    commands.pindef[0].port = GPIOC;
    commands.pindef[0].pin = GPIO13;
    commands.pindef[0].inout = PIN_OUT;
    commands.pindef[0].level = 1;
    commands.pindef[1].port = GPIOB;
    commands.pindef[1].pin = GPIO1 | GPIO2;
    commands.pindef[1].inout = PIN_OUT;
    commands.pindef[1].level = 0;

    crc_reset();
    uint32_t crc = crc_calculate_block(((uint32_t*)&commands), PINDEF_NUMWORDS);
    commands.crc = crc;

    if (commands.crc != flashCommands->crc)
    {
        flash_unlock();
        flash_erase_page(pindefAddr);

        //Write flash including crc, therefor <=
        for (uint32_t idx = 0; idx <= PINDEF_NUMWORDS; idx++)
        {
            uint32_t* pData = ((uint32_t*)&commands) + idx;
            flash_program_word(pindefAddr + idx * sizeof(uint32_t), *pData);
        }
        flash_lock();
    }
}

/**
* Enable Timer refresh and break interrupts
*/
void nvic_setup(void)
{
    nvic_enable_irq(NVIC_TIM2_IRQ); //Scheduler
    nvic_set_priority(NVIC_TIM2_IRQ, 0xe << 4); //second lowest priority
}

void rtc_setup()
{
    //Base clock is HSE/128 = 8MHz/128 = 62.5kHz
    //62.5kHz / (624 + 1) = 100Hz
    rtc_auto_awake(RCC_HSE, 624); //10ms tick
    rtc_set_counter_val(0);
}

void spi1_setup()   //spi 1 used for BATMAN!
{
    uint8_t BMStype = Param::GetInt(Param::bmstype); //pull BMS type

    if(BMStype == BMS_M3)
    {
        spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_64, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
                        SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_16BIT, SPI_CR1_MSBFIRST);
        spi_set_standard_mode(SPI1,3);//set mode 3
    }
    else if(BMStype == BMS_MAX)
    {
        spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_32, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
                        SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
        //spi_set_baudrate_prescaler(SPI1,4000000); //SPI1 at 4Mhz
        spi_set_standard_mode(SPI1,0);//set mode 0
    }


    spi_enable_software_slave_management(SPI1);
    //spi_enable_ss_output(SPI1);
    spi_set_nss_high(SPI1);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO7 | GPIO5);//MOSI , CLK
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO6);//MISO
    spi_enable(SPI1);
}

void usart1_setup(void)
{
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
    usart_set_baudrate(USART1, 612500);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX_RX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_enable(USART1);

}

void tim_setup()
{

}



