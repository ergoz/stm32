/**************************************************************************//**
 * @file     queue.c
 * @brief    STM32 generic queue func source file
 * @version  v1.0
 * @date     04.06.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/

#include "inttypes.h"
#include "queue.h"
#include "interrupt.h"

/******************************************************************************
 * Function definitions
 *****************************************************************************/

uint32_t uQueueWrite(pQueue_t pQueue, uint8_t *pData, uint32_t length)
{
    uint32_t i;
    const uint32_t space = (  (pQueue->ReadIndex - pQueue->WriteIndex - 1)
                            & (MAXIMUM_QUEUE_SIZE - 1));

    if (0 == space)
    {
        return 0;
    }

    length = (length > space) ? space : length;

    for (i = 0; i < length; i++)
    {
    	pQueue->Queue[pQueue->WriteIndex] = pData[i];
        pQueue->WriteIndex=uQueueCalculateIndex(pQueue->WriteIndex, 1);
    }

    return length;
}

void vQueueSkip(pQueue_t pQueue, uint32_t length)
{
	InterruptStatus_t status;
	uint32_t j, read_index;

	status = Interrupt_saveAndDisable();
	read_index = pQueue->ReadIndex;
	Interrupt_restore(status);

	j=uQueueGetBytesToRead(pQueue);
	if (length<j)
		j=length;

	pQueue->ReadIndex=uQueueCalculateIndex(read_index, j);
}

uint32_t uQueueRead(pQueue_t pQueue, uint8_t *pData, uint32_t length)
{
	InterruptStatus_t status;
	uint32_t i, j, read_index, write_index;

	status = Interrupt_saveAndDisable();
	read_index = pQueue->ReadIndex;
	write_index = pQueue->WriteIndex;
	Interrupt_restore(status);

	j=0;
	for (i = read_index; i != write_index; i = uQueueCalculateIndex(i, 1))
	{
		pData[j++]=pQueue->Queue[i];

		length--;
		if (!length)	// buffer is over
			break;
	}

	pQueue->ReadIndex=uQueueCalculateIndex(read_index, j);

	return j;
}

uint32_t uQueueWriteByte(pQueue_t pQueue, uint8_t data)
{
    const uint32_t space = (  (pQueue->ReadIndex - pQueue->WriteIndex - 1)
                            & (MAXIMUM_QUEUE_SIZE - 1));

    if (0 == space)
    {
        return 0;
    }

   	pQueue->Queue[pQueue->WriteIndex] = data;
    pQueue->WriteIndex=uQueueCalculateIndex(pQueue->WriteIndex, 1);

    return 1;
}

uint8_t uQueueReadByte(pQueue_t pQueue)
{
	InterruptStatus_t status;
	uint32_t read_index, write_index;
	uint8_t res=0;

	status = Interrupt_saveAndDisable();
	read_index = pQueue->ReadIndex;
	write_index = pQueue->WriteIndex;
	Interrupt_restore(status);

	if (read_index!=write_index)
	{
		res=pQueue->Queue[read_index];
		pQueue->ReadIndex=uQueueCalculateIndex(read_index, 1);
	}

	return res;
}


uint32_t uQueueGetBytesToRead(pQueue_t pQueue)
{
	InterruptStatus_t status;
	uint32_t read_index, write_index;

	status = Interrupt_saveAndDisable();
	read_index = pQueue->ReadIndex;
	write_index = pQueue->WriteIndex;
	Interrupt_restore(status);

	if (read_index==write_index)
		return 0;

	if (read_index>write_index)
		return (MAXIMUM_QUEUE_SIZE - read_index) + write_index;

	return (write_index - read_index);
}

inline uint32_t uQueueCalculateIndex(uint32_t index, uint32_t offset)
{
	return ((index + offset) & (MAXIMUM_QUEUE_SIZE - 1));
}
