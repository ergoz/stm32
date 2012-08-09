/**************************************************************************//**
 * @file     tim4.c
 * @brief    STM32 tim4 func definitions source file
 * @version  v1.0
 * @date     09.08.2012
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
#include "tim4.h"

/******************************************************************************
 * Function declarations
 *****************************************************************************/

void TIM4_IRQHandler();

/******************************************************************************
 * Global variables
 *****************************************************************************/

static volatile vTim4Cb_t g_Tim4Cb=0;

/******************************************************************************
 * Function definitions
 *****************************************************************************/

void TIM4_IRQHandler()
{
  if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
  {
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	if (g_Tim4Cb)
		g_Tim4Cb();
  }
}

void vTim4Init(uint16_t uPrescaler, uint16_t uPeriod)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	TIM_Cmd(TIM4, DISABLE);

    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = uPeriod; // 12000 / 1200 = 10 sec
    TIM_TimeBaseStructure.TIM_Prescaler = uPrescaler-1; // 72mzh/60khz = 1200
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

    TIM_ARRPreloadConfig(TIM4, ENABLE);

    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

    NVIC_EnableIRQ(TIM4_IRQn);
}

void vTim4SetCb(vTim4Cb_t pCb)
{
	g_Tim4Cb=pCb;
}

void vTim4Start(void)
{
    /* TIM4 enable counter */
    TIM_Cmd(TIM4, ENABLE);
}

void vTim4Stop(void)
{
	TIM_Cmd(TIM4, DISABLE);
}
