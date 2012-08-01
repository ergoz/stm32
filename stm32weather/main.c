#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "nrf.h"
#include "dht22.h"
#include "exti.h"
#include "timer.h"
#include "queue.h"
#include "nRF24L01.h"
#include "packet.h"

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
	vNrfInit(1, (uint8_t *)"main1");
	vDht22Init();
	vIrledInit();
	// this should be latest to let all other modules init EXTI
	vExtiStart();
}

int main(void)
{
	Packet_t pkt;
	uint8_t payload_size;
	const uint8_t cmd[]={0x80, 0x4d, 0x75, 0x8f, 0xce, 0x88, 0xf};

	InitAll();

	while(1)
    {
		vIrledSend(cmd, sizeof(cmd)*8);
		vTimerDelayMs(1000);

		if (!uNrfIsSending() && uNrfIsPayloadReceived())
		{
			payload_size = uNrfGetPayloadSize();
			if (payload_size==sizeof(Packet_t))
			{
				uNrfGetPayload((uint8_t *)&pkt, sizeof(Packet_t));

				switch(GET_PACKET_CMD(pkt.cmd))
				{
				case PACKET_AIR:
				{
					break;
				}
				case PACKET_DHT22:
				{
					break;
				}
				};
			}
		}
    }
}
