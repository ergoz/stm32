/**************************************************************************//**
 * @file     irled.h
 * @brief    STM32 IR led func definitions header file
 * @version  v1.0
 * @date     30.07.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/
#ifndef __IRLED_H__
#define __IRLED_H__

/******************************************************************************
 * Includes
 *****************************************************************************/

#include <inttypes.h>
#include <exti.h>

/******************************************************************************
 * Enumeration declarations
 *****************************************************************************/

/******************************************************************************
 * Function pointer (call-back) declarations
 *****************************************************************************/

/******************************************************************************
 * Function declarations
 *****************************************************************************/

void vIrledInit(void);
void vIrledOn(void);
void vIrledOff(void);
void vIrledSend(uint8_t *pBuf, uint32_t len);

#endif