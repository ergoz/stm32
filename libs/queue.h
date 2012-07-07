/**************************************************************************//**
 * @file     queue.h
 * @brief    STM32 generic queue func definitions header file
 * @version  v1.0
 * @date     04.06.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/
#ifndef __QUEUE_H__
#define __QUEUE_H__

/******************************************************************************
 * Includes
 *****************************************************************************/

#include <inttypes.h>

/******************************************************************************
 * Defines and Macros
 *****************************************************************************/

/** Maximum number of queued transmit messages (must be a power of 2: 2^N) */
#define MAXIMUM_QUEUE_SIZE   1024

/******************************************************************************
 * Types declarations
 *****************************************************************************/

typedef struct
{
    volatile uint32_t ReadIndex;
    volatile uint32_t WriteIndex;
    volatile uint8_t Queue[MAXIMUM_QUEUE_SIZE];
} Queue_t, *pQueue_t;

/******************************************************************************
 * Function declarations
 *****************************************************************************/

uint32_t uQueueWrite(pQueue_t pQueue, uint8_t *pData, uint32_t length);
uint32_t uQueueRead(pQueue_t pQueue, uint8_t *pData, uint32_t length);
uint32_t uQueueWriteByte(pQueue_t pQueue, uint8_t data);
uint8_t uQueueReadByte(pQueue_t pQueue);
uint32_t uQueueGetBytesToRead(pQueue_t pQueue);
inline uint32_t uQueueCalculateIndex(uint32_t index, uint32_t offset);

#endif
