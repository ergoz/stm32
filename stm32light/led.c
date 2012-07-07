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
#include "led.h"
#include "timer.h"

/******************************************************************************
 * Function declarations
 *****************************************************************************/

void vLedOff(void);
void vLedSetDuty(LedState_t state, int32_t duty);

/******************************************************************************
 * Global variables
 *****************************************************************************/

static LedState_t g_lastState=LedState_Off;
static int32_t g_lastDuty=0;

/******************************************************************************
 * Function definitions
 *****************************************************************************/

void vLedInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// Enable PORTA Periph clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void vLedSetState(LedState_t state, int32_t duty)
{
	int32_t d=0;

	if (g_lastState != LedState_Off)
	{
		for (d=g_lastDuty; d>0; d--)
		{
			vLedSetDuty(g_lastState, d);
			vTimerDelayMs(5);
		}
	}

	if (state == LedState_Off)
	{
		vLedOff();
		//vLedSetDuty(LedState_Off, 0);
	}
	else
	{
		for (d=0; d<duty; d++)
		{
			vLedSetDuty(state, d);
			vTimerDelayMs(5);
		}
		vLedSetDuty(state, duty);
	}

	g_lastState=state;
	g_lastDuty=duty;
}

void vLedSetDuty(LedState_t state, int32_t duty)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
//	InterruptStatus_t status;

//	status = Interrupt_saveAndDisable();

	TIM_Cmd(TIM2, DISABLE);

    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = 400;
    TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t) (SystemCoreClock / 24000000) - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* PWM1 Mode configuration: Channel1 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = duty<<2;
    if (TIM_OCInitStructure.TIM_Pulse>TIM_TimeBaseStructure.TIM_Period)
    	TIM_OCInitStructure.TIM_Pulse=TIM_TimeBaseStructure.TIM_Period-1;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    if (state==LedState_White)
    {
		TIM_OC1Init(TIM2, &TIM_OCInitStructure);
		TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
		TIM_OC2Init(TIM2, &TIM_OCInitStructure);
		TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
		TIM_OC3Init(TIM2, &TIM_OCInitStructure);
		TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
    }
    else if (state==LedState_Green)
    {
        TIM_OC1Init(TIM2, &TIM_OCInitStructure);
        TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
    }
    else if (state==LedState_Red)
    {
        TIM_OC2Init(TIM2, &TIM_OCInitStructure);
        TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
    }
    else if (state==LedState_Blue)
    {
        TIM_OC3Init(TIM2, &TIM_OCInitStructure);
        TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
    }

    TIM_ARRPreloadConfig(TIM2, ENABLE);

    /* TIM3 enable counter */
    TIM_Cmd(TIM2, ENABLE);

//    Interrupt_restore(status);
}

void vLedOff(void)
{
	TIM_Cmd(TIM2, DISABLE);
}
