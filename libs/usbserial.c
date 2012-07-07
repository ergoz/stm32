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
#include "stm32f10x_spi.h"
#include "misc.h"

#include "usb_lib.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "usb_pwr.h"
#include "usb_istr.h"

#include "queue.h"
#include "interrupt.h"

#include <stdint.h>
#include <stdio.h>

/******************************************************************************
 * Global variables
 *****************************************************************************/

static Queue_t g_UsbQueue;

/******************************************************************************
 * Function declarations
 *****************************************************************************/
void USB_LP_CAN1_RX0_IRQHandler(void);
void vUsbserialReceiveCB(uint8_t *pData, uint16_t length);

/******************************************************************************
 * Function definitions
 *****************************************************************************/

void vUsbserialInit(void)
{
	Set_System();
	Set_USBClock();
	USB_Interrupts_Config();
	USB_Init();
	USB_Receive_Data_Cb(vUsbserialReceiveCB);
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
  USB_Istr();
}

void vUsbserialReceiveCB(uint8_t *pData, uint16_t length)
{
	uQueueWrite(&g_UsbQueue, pData, length);
}

void vUsbserialWrite(char *str)
{
	uint32_t i;

	for (i=0;i<strlen(str);i++)
		USB_Send_Data(str[i]);
}

uint32_t uUsbSerialDataAvailable(void)
{
	return uQueueGetBytesToRead(&g_UsbQueue);
}

uint32_t uUsbSerialRead(uint8_t *buf, uint32_t len)
{
	return uQueueRead(&g_UsbQueue, buf, len);
}
