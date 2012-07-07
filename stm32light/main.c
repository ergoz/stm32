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

	vTimerInit();
	vExtiInit();
	vNrfHwInit();
	vNrfInit(1, (uint8_t *)"litt1");
	// this should be latest to let all other modules init EXTI
	vExtiStart();
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
		if (!uNrfIsSending() && uNrfIsPayloadReceived())
        {
			payload_size = uNrfGetPayloadSize();
			if (payload_size==4)
				uNrfGetPayload((uint8_t *)&t, 4);

			vNrfSend((uint8_t *)"main1", (uint8_t *)&t, 4);
			while (uNrfIsSending()){};

			state=!state;
			if (state)
				GPIO_SetBits(GPIOB, GPIO_Pin_1);
			else
				GPIO_ResetBits(GPIOB, GPIO_Pin_1);
        }
	}
}
