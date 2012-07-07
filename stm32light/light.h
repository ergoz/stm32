/**************************************************************************//**
 * @file     light.h
 * @brief    STM32 i2c light sensor func definitions header file
 * @version  v1.0
 * @date     25.05.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/
#ifndef __LIGHT_H__
#define __LIGHT_H__

/******************************************************************************
 * Defines
 *****************************************************************************/

#define MAX44009_INT_STATUS			0
#define MAX44009_INT_ENABLE			1
#define MAX44009_INT_CONFIG			2
#define MAX44009_LUX_HIGH			3
#define MAX44009_LUX_LOW			4
#define MAX44009_UPPER_THRESHOLD	5
#define MAX44009_LOWER_THRESHOLD	6
#define MAX44009_THRESHOLD_TIMER	7

/******************************************************************************
 * Includes
 *****************************************************************************/

#include <inttypes.h>
#include <exti.h>



void vLightInit(void);
void vLightWriteByte(uint8_t *pBuffer, u8 writeAddr);
void vLightReadByte(uint8_t *pBuffer, u8 readAddr);
uint32_t uLightReadLux();
void vLightCbState(vExtiCbState_t pCb);

#endif
