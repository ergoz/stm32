/**************************************************************************//**
 * @file     irled.c
 * @brief    STM32 ir led func definitions source file
 * @version  v1.0
 * @date     30.07.2012
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
#include "stm32f10x_exti.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "inttypes.h"
#include "interrupt.h"
#include "motion.h"
#include "exti.h"
#include "timer.h"

/******************************************************************************
 * Function declarations
 *****************************************************************************/

void vIrledSetupTimer(void);

/******************************************************************************
 * Global variables
 *****************************************************************************/

/******************************************************************************
 * Function definitions
 *****************************************************************************/

void vIrledInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// Enable PORTA Periph clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    vIrledSetupTimer();
}

void vIrledOn(void)
{
    /* TIM3 enable counter */
    TIM_Cmd(TIM2, ENABLE);
}

void vIrledSetupTimer(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	TIM_Cmd(TIM2, DISABLE);

    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = 270;
    TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t) (SystemCoreClock / 10000000) - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* PWM1 Mode configuration: Channel1 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 90;	// duty
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM2, ENABLE);
}

void vIrledOff(void)
{
	TIM_Cmd(TIM2, DISABLE);
}

void vIrledSend(uint8_t *pBuf, uint32_t len)
{
	uint32_t i;

	// preambula
	vIrledOn();
	vTimerDelayMs(3);
	vIrledOff();
	vTimerDelayMs(9);

	for (i=0; i<len; i++)
	{
		vIrledOn();
		vTimerDelayUs(500);
		vIrledOff();
		vTimerDelayUs(500);
		if (pBuf[i>>3] & (1<<(7 - (i&7))))
			vTimerDelayUs(1000);
	}
	vIrledOn();
	vTimerDelayUs(500);
	vIrledOff();
}
