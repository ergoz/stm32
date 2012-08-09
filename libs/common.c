/**************************************************************************//**
 * @file     common.c
 * @brief    STM32 Common func definitions source file
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
#include "common.h"

/******************************************************************************
 * Function declarations
 *****************************************************************************/

/******************************************************************************
 * Global variables
 *****************************************************************************/

/******************************************************************************
 * Function definitions
 *****************************************************************************/

char* myitoa(int val, int32_t base)
{
	static char buf[32];

	memset(buf, 0, 32);
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789ABCDEF"[val % base];
	return &buf[i+1];
}

int32_t myatoi(char *buf, int32_t len)
{
	int32_t res=0, cnt;
	int8_t b;

	for (cnt=0; cnt<len; cnt++)
	{
		b=buf[cnt];
		b-=0x30;
		if (b>9)
			b-=7;
		res<<=4;
		res+=b;
	}

	return res;
}
