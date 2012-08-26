/**************************************************************************//**
 * @file     mirf.h
 * @brief    STM32 NRF24L01 func definitions header file
 * @version  v1.0
 * @date     07.06.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/
#ifndef __NRF_H__
#define __NRF_H__

/******************************************************************************
 * Includes
 *****************************************************************************/

#include <inttypes.h>

/******************************************************************************
 * Defines and Macros
 *****************************************************************************/

/******************************************************************************
 * Types declarations
 *****************************************************************************/

/******************************************************************************
 * Function declarations
 *****************************************************************************/

void vNrfHwInit(void);
void vNrfInit(uint8_t channel, uint8_t * rx_addr);

void vNrfSend(uint8_t * addr, uint8_t *data, uint8_t len);
uint8_t uNrfIsSending(void);
uint8_t uNrfGetLastTxStatus(void);

uint8_t uNrfIsPayloadReceived(void);
uint8_t uNrfGetPayloadSize(void);
uint8_t uNrfGetPayload(uint8_t *data, uint8_t len);
void vNrfSkipPayload(uint8_t len);

uint8_t uNrfReadRegister(uint8_t reg, uint8_t *data, uint8_t len);

#endif
