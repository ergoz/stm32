/**************************************************************************//**
 * @file     usbserial.c
 * @brief    USB Virtual COM-port (CDC) func source file
 * @version  v1.0
 * @date     24.06.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/

#include "inttypes.h"

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "misc.h"
#include "queue.h"
#include "interrupt.h"
#include "exti.h"
#include "timer.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

#define DHT22_GPIO	GPIOA
#define DHT22_PIN	GPIO_Pin_15
#define DHT_H()		GPIO_SetBits(DHT22_GPIO, DHT22_PIN)
#define DHT_L()		GPIO_ResetBits(DHT22_GPIO, DHT22_PIN)


/******************************************************************************
 * Global variables
 *****************************************************************************/

static volatile uint8_t g_DhtRaw[5];
static volatile uint8_t g_DhtMeasured=0;
static volatile uint8_t g_DhtMeasuring=0;

/******************************************************************************
 * Function declarations
 *****************************************************************************/

static void vDhtCbState(ExtiLineState_t state);

/******************************************************************************
 * Function definitions
 *****************************************************************************/

void vDht22Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	// add int exti handler
	vExtiAddCb(GPIO_PortSourceGPIOA, GPIO_PinSource15, vDhtCbState);

	GPIO_InitStructure.GPIO_Pin = DHT22_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT22_GPIO, &GPIO_InitStructure);
}

static void vDhtCbState(ExtiLineState_t state)
{
	static uint32_t timeUs=0;
	static uint8_t cnt=0;
	uint32_t t;

	t=uTimerGetUs();

	if (g_DhtMeasuring)
	{
		if (state == LineState_Low) // last was low
		{
			if (cnt)
			{
				g_DhtRaw[ (cnt-1) / 8] <<= 1;
				if ((t - timeUs) > 50) // 1
					g_DhtRaw[(cnt-1) / 8] |= 1;
			}

			cnt++;

			if (cnt >= 41) {
				g_DhtMeasuring = 0;
				cnt = 0;
			}
		}
	} else
		cnt = 0;

	timeUs=t;
}

uint8_t uDht22CheckCrc(void)
{
	uint8_t crc;
	crc=g_DhtRaw[0]+g_DhtRaw[1]+g_DhtRaw[2]+g_DhtRaw[3];
	return crc==g_DhtRaw[4];
}

void vDht22Start(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

    // configure pin
	GPIO_InitStructure.GPIO_Pin = DHT22_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(DHT22_GPIO, &GPIO_InitStructure);

	DHT_L();
	vTimerDelayUs(500);
	DHT_H();
	vTimerDelayUs(40);

	// switch to input
	GPIO_InitStructure.GPIO_Pin = DHT22_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT22_GPIO, &GPIO_InitStructure);

    g_DhtMeasuring = 1;
    memset(&g_DhtRaw, 5, 0);
}

uint8_t uDht22Measuring(void)
{
	return g_DhtMeasuring;
}

uint16_t uDht22GetTemp(void)
{
	uint16_t res=0;
	res=g_DhtRaw[2];
	res<<=8;
	res|=g_DhtRaw[3];
	return res;
}

uint16_t uDht22GetHumidity(void)
{
	uint16_t res=0;
	res=g_DhtRaw[0];
	res<<=8;
	res|=g_DhtRaw[1];
	return res;
}
