/**************************************************************************//**
 * @file     led.h
 * @brief    STM32 led func definitions header file
 * @version  v1.0
 * @date     13.05.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/
#ifndef __LED_H__
#define __LED_H__

/******************************************************************************
 * Includes
 *****************************************************************************/

#include <inttypes.h>

typedef enum
{
    LedState_Off = 0,
    LedState_Red,
    LedState_Green,
    LedState_Blue,
    LedState_White
} LedState_t;

void vLedInit(void);
void vLedSetState(LedState_t state, int32_t duty);

#endif
