#include "stm32f10x.h"

#include "usb_lib.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "usb_pwr.h"

#include "usb_istr.h"

#include <stdint.h>
#include <stdio.h>

void USB_LP_CAN1_RX0_IRQHandler(void)
{
  USB_Istr();
}

int main(void)
{

	uint32_t i;

	Set_System();
	Set_USBClock();
	USB_Interrupts_Config();
	USB_Init();

	while (1)
	{
		//printf("Hello world!\r\n");
		for (i=0; i<0x000fffff; i++);
	}
}
