/**************************************************************************//**
 * @file     motion.c
 * @brief    STM32 motion func definitions source file
 * @version  v1.0
 * @date     28.02.2012
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
#include "misc.h"
#include "inttypes.h"
#include "interrupt.h"
#include "motion.h"
#include "exti.h"

/******************************************************************************
 * Function declarations
 *****************************************************************************/

/******************************************************************************
 * Global variables
 *****************************************************************************/

/******************************************************************************
 * Function definitions
 *****************************************************************************/

void vMotionCbState(vExtiCbState_t pCb)
{
	vExtiAddCb(GPIO_PortSourceGPIOB, GPIO_PinSource8, pCb);
}

void vButtonCbState(vExtiCbState_t pCb)
{
	vExtiAddCb(GPIO_PortSourceGPIOB, GPIO_PinSource11, pCb);
}

void vMotionInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	// enable port B
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE);

	// Configure PB.8 as Input (this is IR motion sensor)
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; // pull-down, default is 0
	GPIO_Init( GPIOB, &GPIO_InitStructure);

	// Configure PB.11 as Input (this is our button)
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // pull-up, default is 1
	GPIO_Init( GPIOB, &GPIO_InitStructure);
}

void vMotionBackground(void)
{
}
