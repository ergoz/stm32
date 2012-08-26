/**************************************************************************//**
 * @file     mpl115a1.h
 * @brief    STM32 MPL115A1 func definitions header file
 * @version  v1.0
 * @date     27.08.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/
#ifndef __MPL115A1_H__
#define __MPL115A1_H__

/******************************************************************************
 * Includes
 *****************************************************************************/

#include <inttypes.h>

/******************************************************************************
 * Enumeration declarations
 *****************************************************************************/

/******************************************************************************
 * Function pointer (call-back) declarations
 *****************************************************************************/

/******************************************************************************
 * Function declarations
 *****************************************************************************/

void vMpl115a1Init(void);
uint32_t uMpl115a1ReadPressure(void);

#endif
