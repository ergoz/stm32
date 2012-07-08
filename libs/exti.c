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
#include "exti.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

#define MAX_NUMS_OF_EXTI 16

/******************************************************************************
 * Types definitions
 *****************************************************************************/

typedef struct
{
	uint8_t port;
	vExtiCbState_t cb;
	FunctionalState state;
} ExtiConfig_t;

/******************************************************************************
 * Function declarations
 *****************************************************************************/

void EXTI0_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);
void vExtiHandleInt(uint8_t line);
/******************************************************************************
 * Global variables
 *****************************************************************************/

/** Call-back array for int/events */
static volatile ExtiConfig_t g_ExtiConfig[MAX_NUMS_OF_EXTI];

/******************************************************************************
 * Function definitions
 *****************************************************************************/

void vExtiInit(void)
{
	memset(&g_ExtiConfig, 0, sizeof(ExtiConfig_t)*MAX_NUMS_OF_EXTI);
}

void vExtiAddCb(uint8_t port, uint8_t line, vExtiCbState_t pCb)
{
	g_ExtiConfig[line].port=port;
	g_ExtiConfig[line].cb=pCb;
	g_ExtiConfig[line].state=ENABLE;
}

void vExtiChangeState(uint8_t line, FunctionalState state)
{
	g_ExtiConfig[line].state=state;
}

void vExtiStart(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	uint32_t uExtiLines=0;
	uint8_t exti=0;

	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    for (exti=0;exti<MAX_NUMS_OF_EXTI;exti++)
	{
		if ((g_ExtiConfig[exti].state==ENABLE) && (g_ExtiConfig[exti].cb!=0))
		{
			uExtiLines|=(1<<exti);	// set line for exti

			if (exti>9)
				NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
			else if (exti>4)
				NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
			else if (exti==4)
				NVIC_InitStructure.NVIC_IRQChannel = EXTI4_IRQn;
			else if (exti==3)
				NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
			else if (exti==2)
				NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
			else if (exti==1)
				NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
			else if (exti==0)
				NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
		    // Enable the EXTIxxxx Interrupt
		    NVIC_Init(&NVIC_InitStructure);

		    // Connect EXTI LineXX to port
		    GPIO_EXTILineConfig(g_ExtiConfig[exti].port, exti);
		}
	}


    if (uExtiLines)	// if any of exti set
    {
		// Configure EXTI Lines to generate an interrupt on rising or falling edge
		EXTI_InitStructure.EXTI_Line = uExtiLines;
		EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
		EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
		EXTI_InitStructure.EXTI_LineCmd = ENABLE;
		EXTI_Init(&EXTI_InitStructure);
    }
}

void vExtiStop(void)
{
	// tbd
}

// Handle interrupt
void vExtiHandleInt(uint8_t line)
{
	GPIO_TypeDef *pGPIO=0;

	if (g_ExtiConfig[line].state==ENABLE) // if interrupt handler enabled
	{
		if (g_ExtiConfig[line].cb!=0)		// and handler is set
		{
			switch(g_ExtiConfig[line].port)
			{
			case GPIO_PortSourceGPIOA:
				pGPIO = GPIOA;
				break;
			case GPIO_PortSourceGPIOB:
				pGPIO = GPIOB;
				break;
			case GPIO_PortSourceGPIOC:
				pGPIO = GPIOC;
				break;
			default:
				break;
			}

			if (pGPIO)
			{
				if (GPIO_ReadInputDataBit(pGPIO, (1 << line)) == 1)
					g_ExtiConfig[line].cb(LineState_High);
				else
					g_ExtiConfig[line].cb(LineState_Low);
			}
		}
	}
}

//**************************************************************************
//
//This function handles External line 0 interrupt request.
//
//**************************************************************************
void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
    vExtiHandleInt(0);
}

//**************************************************************************
//
//This function handles External lines 10 to 15 interrupt request.
//
//**************************************************************************
void EXTI15_10_IRQHandler(void)
{
	uint8_t line=0;

    if (EXTI_GetITStatus(EXTI_Line10) != RESET)
    {
    	line=10;
        EXTI_ClearITPendingBit(EXTI_Line10);
    }
    else if (EXTI_GetITStatus(EXTI_Line11) != RESET)
    {
    	line=11;
        EXTI_ClearITPendingBit(EXTI_Line11);
    }
    else if (EXTI_GetITStatus(EXTI_Line12) != RESET)
    {
    	line=12;
        EXTI_ClearITPendingBit(EXTI_Line12);
    }
    else if (EXTI_GetITStatus(EXTI_Line13) != RESET)
    {
    	line=13;
        EXTI_ClearITPendingBit(EXTI_Line13);
    }
    else if (EXTI_GetITStatus(EXTI_Line14) != RESET)
    {
    	line=14;
        EXTI_ClearITPendingBit(EXTI_Line14);
    }
    else if (EXTI_GetITStatus(EXTI_Line15) != RESET)
    {
    	line=15;
        EXTI_ClearITPendingBit(EXTI_Line15);
    }

    vExtiHandleInt(line);
}

//**************************************************************************
//
//This function handles External lines 9 to 5 interrupt request.
//
//**************************************************************************
void EXTI9_5_IRQHandler(void)
{
	uint8_t line=0;

    if (EXTI_GetITStatus(EXTI_Line5) != RESET)
    {
    	line=5;
        EXTI_ClearITPendingBit(EXTI_Line5);
    }
    else if (EXTI_GetITStatus(EXTI_Line6) != RESET)
    {
    	line=6;
        EXTI_ClearITPendingBit(EXTI_Line6);
    }
    else if (EXTI_GetITStatus(EXTI_Line7) != RESET)
    {
    	line=7;
        EXTI_ClearITPendingBit(EXTI_Line7);
    }
    else if (EXTI_GetITStatus(EXTI_Line8) != RESET)
    {
    	line=8;
        EXTI_ClearITPendingBit(EXTI_Line8);
    }
    else if (EXTI_GetITStatus(EXTI_Line9) != RESET)
    {
    	line=9;
        EXTI_ClearITPendingBit(EXTI_Line9);
    }

    vExtiHandleInt(line);
}

