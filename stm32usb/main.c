#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "nrf.h"
#include "dht22.h"
#include "exti.h"
#include "timer.h"
#include "usbserial.h"
#include "queue.h"
#include "nRF24L01.h"
#include "packet.h"
#include "common.h"

#define NRF_OWN_ADDR	"main1"

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
	vNrfInit(1, (uint8_t *)NRF_OWN_ADDR);
	vDht22Init();
	// this should be latest to let all other modules init EXTI
	vExtiStart();
}

void NrfPacketTest(uint8_t *target, uint32_t count)
{
	uint32_t lost, t, k, total;
	uint8_t payload_size;
	uint8_t name[6];
	Packet_t pkt;

	memset(name, 0, 6);
	memcpy(name, target, 5);
	total=0;

	if (count==1)
		vUsbserialWrite("Ping: ");
	else
		vUsbserialWrite("Packet test with node: ");
	vUsbserialWrite(name);
	vUsbserialWrite("\r\n");
	vUsbserialWrite("Begin: ");
	lost = 0;
	for (k = 0; k < count; k++)
	{
		pkt.cmd=PACKET_PING;
		memcpy(pkt.sender, NRF_OWN_ADDR, 5);
		t = uTimerGetMs();
		pkt.data.ping = t;

		vNrfSend((uint8_t *) name, (uint8_t *) &pkt, sizeof(Packet_t));

		while (uNrfIsSending()) {};

		while ((uTimerGetMs() - t) < 100)
		{
			if (!uNrfIsSending() && uNrfIsPayloadReceived())
			{
				payload_size = uNrfGetPayloadSize();
				if (payload_size==sizeof(Packet_t))
					uNrfGetPayload((uint8_t *) &pkt, sizeof(Packet_t));

				memcpy(&t, &pkt.data.ping, 4);
				t = uTimerGetMs() - t;
				total+=t;

				vUsbserialWrite(".");
				if (count==1)
				{
					vUsbserialWrite("response: ");
					if (!t)
						vUsbserialWrite("0");
					else
						vUsbserialWrite(myitoa(t, 10));
					vUsbserialWrite(" ms\r\n");
				}
				t = 0;
				break;
			}
		}
		if (t)
		{
			if (count==1)
				vUsbserialWrite(" lost!\r\n");
			else
				vUsbserialWrite("X");
			lost++;
		}
		//vTimerDelayMs(1);
	}
	if (count!=1)
	{
		vUsbserialWrite("End\r\nTotal lost packets: ");
		if (!lost)
			vUsbserialWrite("0");
		else
			vUsbserialWrite(myitoa(lost, 10));
		vUsbserialWrite("\r\n");

		vUsbserialWrite("Average response time: ");
		if (!total)
			vUsbserialWrite("0");
		else
			vUsbserialWrite(myitoa(total/count, 10));
		vUsbserialWrite("\r\n");

	}
}

void vSendIr(uint8_t *buf)
{
	Packet_t pkt;
	uint8_t cnt;
	uint8_t ir[7];
	int32_t i;

	for (cnt=0; cnt<7; cnt++)
	{
		pkt.data.ir[cnt]=(uint8_t)myatoi(buf+cnt*2, 2);
	}

	// 80 4D 75 8F CE 88 0F // 23 + fan auto
	// 80 41 F0 00 06 86 00 // off
	// 80 43 75 8F C1 A8 0F // on



	pkt.cmd=PACKET_AIR;

//	pkt.data.ir[0]=0x80;
//	pkt.data.ir[1]=0x4d;
//	pkt.data.ir[2]=0x75;
//	pkt.data.ir[3]=0x8F;
//	pkt.data.ir[4]=0xce;
//	pkt.data.ir[5]=0x88;
//	pkt.data.ir[6]=0xF;
	memcpy(pkt.sender, NRF_OWN_ADDR, 5);
	vNrfSend((uint8_t *) "temp1", (uint8_t *) &pkt, sizeof(Packet_t));
}


void vDhtTest(void)
{
	Packet_t pkt;

	pkt.cmd=PACKET_DHT22;
	memcpy(pkt.sender, NRF_OWN_ADDR, 5);
	vNrfSend((uint8_t *) "temp1", (uint8_t *) &pkt, sizeof(Packet_t));
}

uint8_t handle_cmd(uint8_t *buf)
{
	if (strlen((const char *) buf) > 5 &&
			memcmp((const char *) buf, (const char *) "status", 6) == 0)
	{
		vMainNrfDump();
		return 1;
	}

	if (strlen((const char *)buf)>9 &&
			memcmp((const char *)buf, (const char *)"test", 4)==0)
	{
		NrfPacketTest(&buf[5], 500);
		return 1;
	}

	if (strlen((const char *)buf)>9 &&
			memcmp((const char *)buf, (const char *)"ping", 4)==0)
	{
		NrfPacketTest(&buf[5], 1);
		return 1;
	}

	if (strlen((const char *)buf)>4 &&
			memcmp((const char *)buf, (const char *)"dht22", 5)==0)
	{
		vDhtTest();
		return 1;
	}

	if (strlen((const char *)buf)>17 &&
			memcmp((const char *)buf, (const char *)"air", 3)==0)
	{
		vSendIr(buf+4);
		return 1;
	}

	return 0;
}

//extern volatile uint32_t pkt_cnt;

int main(void)
{
	uint32_t j;
	uint8_t state=1;
	uint8_t buf[256], name[6];
	uint8_t payload_size;
	Packet_t pkt;

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
				vUsbserialWrite("Command: ");
				vUsbserialWrite((char *)buf);
				vUsbserialWrite("\r\n");
				j=0;

				state=!state;
				if (state)
					GPIO_SetBits(GPIOB, GPIO_Pin_1);
				else
					GPIO_ResetBits(GPIOB, GPIO_Pin_1);

				if (!handle_cmd(buf))
					vUsbserialWrite("Unknown command...\r\n");

				memset(buf, 0, 256);
			}
		}

		if (!uNrfIsSending() && uNrfIsPayloadReceived())
		{
			payload_size = uNrfGetPayloadSize();
			if (payload_size==sizeof(Packet_t))
				uNrfGetPayload((uint8_t *)&pkt, sizeof(Packet_t));

			vUsbserialWrite("Received packet from:");
			memset(name, 0, 6);
			memcpy(name, pkt.sender, 5);
			vUsbserialWrite(name);
			vUsbserialWrite("\r\n");

			if ((pkt.cmd&PACKET_REPLY)==PACKET_REPLY)
			{
				switch (pkt.cmd&PACKET_CMD_MASK)
				{
				case PACKET_DHT22:
				{
					if (pkt.status==1)
					{
						vUsbserialWrite("Temp: ");
						vUsbserialWrite(myitoa(pkt.data.dht22[0] / 10, 10));
						vUsbserialWrite(".");
						vUsbserialWrite(myitoa(pkt.data.dht22[0] % 10, 10));
						vUsbserialWrite("\r\n");

						vUsbserialWrite("Humidity: ");
						vUsbserialWrite(myitoa(pkt.data.dht22[1] / 10, 10));
						vUsbserialWrite(".");
						vUsbserialWrite(myitoa(pkt.data.dht22[1] % 10, 10));
						vUsbserialWrite("\r\n");
					}
					else
						vUsbserialWrite("DHT22 measuring failed!\r\n");
					break;
				}
				default:
				{
					vUsbserialWrite("Packet type: ");
					vUsbserialWrite(myitoa(pkt.cmd, 10));
					vUsbserialWrite("\r\n");
					break;
				}
				}

			}
		}
	}
}
