/**************************************************************************//**
 * @file     led.h
 * @brief    STM32 timer func definitions header file
 * @version  v1.0
 * @date     13.05.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/
#ifndef __TIMER_H__
#define __TIMER_H__

/******************************************************************************
 * Includes
 *****************************************************************************/

#include <inttypes.h>

void vTimerInit(void);
void vTimerReset(void);
uint32_t uTimerGetMs(void);
uint32_t uTimerGetUs(void);
void vTimerDelayMs(uint32_t delayTimeMs);
void vTimerDelayUs(uint32_t delayTimeUs);

#endif
