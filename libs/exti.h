/**************************************************************************//**
 * @file     exti.h
 * @brief    STM32 EXTI func definitions header file
 * @version  v1.0
 * @date     27.05.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/
#ifndef __EXTI_H__
#define __EXTI_H__

/******************************************************************************
 * Includes
 *****************************************************************************/

#include <inttypes.h>

/******************************************************************************
 * Enumeration declarations
 *****************************************************************************/

/** IR motion states */
typedef enum
{
    LineState_Low = 0,
    LineState_High
} ExtiLineState_t;

/******************************************************************************
 * Function pointer (call-back) declarations
 *****************************************************************************/

typedef void (*vExtiCbState_t) (ExtiLineState_t state);

/******************************************************************************
 * Function declarations
 *****************************************************************************/

void vExtiInit(void);
void vExtiAddCb(uint8_t port, uint8_t line, vExtiCbState_t pCb);
void vExtiStart(void);
void vExtiStop(void);
void vExtiChangeState(uint8_t line, FunctionalState state);

#endif
