#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "nrf.h"
#include "exti.h"
#include "timer.h"
#include "usbserial.h"
#include "queue.h"
#include "nRF24L01.h"

char* myitoa(int val, int base)
{
	static char buf[32];

	memset(buf, 0, 32);
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789ABCDEF"[val % base];
	return &buf[i+1];
}

void vMainNrfDump(void)
{
	uint8_t buf[5];
	uNrfReadRegister(CONFIG, (uint8_t *)&buf, 1);
	vUsbserialWrite("CONFIG: ");
	if (!buf[0])
		vUsbserialWrite("0");
	else
		vUsbserialWrite(myitoa(buf[0], 16));
	vUsbserialWrite("\r\n");
	uNrfReadRegister(STATUS, (uint8_t *)&buf, 1);
	vUsbserialWrite("STATUS: ");
	if (!buf[0])
		vUsbserialWrite("0");
	else
		vUsbserialWrite(myitoa(buf[0], 16));
	vUsbserialWrite("\r\n");
}

void InitAll(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	/* Configure LED */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	vUsbserialInit();
	vTimerInit();
	vExtiInit();
	vNrfHwInit();
	vNrfInit(1, (uint8_t *)"serv1");
	// this should be latest to let all other modules init EXTI
	vExtiStart();
}

uint8_t handle_cmd(uint8_t *buf)
{
	if (strlen((const char *) buf) > 5 &&
			memcmp((const char *) buf, (const char *) "status", 6) == 0)
	{
		vMainNrfDump();
		return 1;
	}
	return 0;
}


int main(void)
{
	uint32_t j, t;
	uint8_t state=1;
	uint8_t buf[256];
	uint8_t payload_size;

	memset(buf, 0, 256);

	InitAll();
	//vMainNrfDump();

	GPIO_SetBits(GPIOB, GPIO_Pin_1);

	j = 0;
	while (1)
	{
		if (uUsbSerialDataAvailable())
		{
			j += uUsbSerialRead(&buf[j], 256 - j);
			if (buf[j-1]=='\r')		// this is wrong!!! need to scan whole string
			{
				buf[j-1]=0;
				vUsbserialWrite("Got: ");
				vUsbserialWrite((char *)buf);
				vUsbserialWrite("\r\n");
				j=0;

				state=!state;
				if (state)
					GPIO_SetBits(GPIOB, GPIO_Pin_1);
				else
					GPIO_ResetBits(GPIOB, GPIO_Pin_1);

				handle_cmd(buf);

				memset(buf, 0, 256);
			}
		}

		if (!uNrfIsSending() && uNrfIsPayloadReceived())
        {
			vUsbserialWrite("Got ping request...");
			payload_size = uNrfGetPayloadSize();
			if (payload_size==4)
				uNrfGetPayload((uint8_t *)&t, 4);

			vNrfSend((uint8_t *)"main1", (uint8_t *)&t, 4);
			while (uNrfIsSending()){};

			vUsbserialWrite("reply send!\r\n");
			state=!state;
			if (state)
				GPIO_SetBits(GPIOB, GPIO_Pin_1);
			else
				GPIO_ResetBits(GPIOB, GPIO_Pin_1);
        }
	}
}
