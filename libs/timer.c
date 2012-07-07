/**************************************************************************//**
 * @file     led.c
 * @brief    STM32 led func definitions source file
 * @version  v1.0
 * @date     13.05.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "inttypes.h"
#include "interrupt.h"
#include "timer.h"

/******************************************************************************
 * Defines and Macros
 *****************************************************************************/

/** SysTick timer interrupt (overflow) frequency in Hz */
#define SYSTICK_INTERRUPT_FREQUENCY             100000

/** Setup SysTick Timer (24 bit) for 0.125 s interrupts (4 Hz blink signal)
 *  72 MHz / DIV = 8 Hz ==> DIV = 9000000 */
#define SYSTICK_RELOAD_VALUE                    ( 72000000 / SYSTICK_INTERRUPT_FREQUENCY)

/******************************************************************************
 * Global variables
 *****************************************************************************/

static volatile uint32_t g_SysTickOverflowCnt;

/******************************************************************************
 * Function definitions
 *****************************************************************************/

void vTimerInit(void)
{
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
    SysTick_Config(SYSTICK_RELOAD_VALUE);

    g_SysTickOverflowCnt = 0;
}

void vTimerReset(void)
{
    InterruptStatus_t status;

    /* Begin of critical section */
    status = Interrupt_saveAndDisable();

    SysTick->VAL = 0;
    g_SysTickOverflowCnt = 0;

    /* End of critical section */
    Interrupt_restore(status);
}

uint32_t uTimerGetMs(void)
{
	return g_SysTickOverflowCnt/(SYSTICK_INTERRUPT_FREQUENCY/1000);
}

uint32_t uTimerGetUs(void)
{
	return g_SysTickOverflowCnt*(1000000/SYSTICK_INTERRUPT_FREQUENCY);
}

void vTimerDelayMs(uint32_t delayTimeMs)
{
	uint32_t timeMs=uTimerGetMs();

	while ((uTimerGetMs()-timeMs)<delayTimeMs)
	{
		__NOP();
	}
}

void vTimerDelayUs(uint32_t delayTimeUs)
{
	uint32_t timeUs=uTimerGetUs();

	while ((uTimerGetUs()-timeUs)<delayTimeUs)
	{
		__NOP();
	}
}


void SysTick_Handler(void)
{
//    const uint32_t maxOverflows = 0xFFFFFFFFUL;
//
//    if (maxOverflows == g_SysTickOverflowCnt)
//    {
//        //Communication_transmitStatusMessage(StatusMessage_Warning, "HW: Timestamp overflow (305.4 h elapsed)");
//    }

    g_SysTickOverflowCnt++;
}

