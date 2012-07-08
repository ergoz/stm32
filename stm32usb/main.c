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
	vNrfInit(1, (uint8_t *)"main1");
	vDht22Init();
	// this should be latest to let all other modules init EXTI
	vExtiStart();
}

void NrfPacketTest(uint8_t *target, uint32_t count)
{
	uint32_t lost, t, k, total;
	uint8_t payload_size;
	uint8_t name[6];
	uint8_t buf[32];

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
		memset(buf, 0, 32);
		t = uTimerGetMs();
		memcpy(buf, &t, 4);

		vNrfSend((uint8_t *) name, (uint8_t *) buf, 32);

		while (uNrfIsSending()) {};

		while ((uTimerGetMs() - t) < 100)
		{
			if (!uNrfIsSending() && uNrfIsPayloadReceived())
			{
				payload_size = uNrfGetPayloadSize();
				if (payload_size==32)
					uNrfGetPayload((uint8_t *) buf, 32);

				memcpy(&t, buf, 4);
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

					memcpy(&t, buf+4, 4);
					vUsbserialWrite("Lux: ");
					if (!t)
						vUsbserialWrite("0");
					else
						vUsbserialWrite(myitoa(t, 10));
					vUsbserialWrite("\r\n");
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

void vDhtTest(void)
{
	vDht22Start();
	//vTimerDelayMs(2000);
	while (uDht22Measuring()){};

	if (uDht22CheckCrc())
	{
		vUsbserialWrite("Temp: ");
		vUsbserialWrite(myitoa(uDht22GetTemp() / 10, 10));
		vUsbserialWrite(".");
		vUsbserialWrite(myitoa(uDht22GetTemp() % 10, 10));
		vUsbserialWrite("\r\n");

		vUsbserialWrite("Humidity: ");
		vUsbserialWrite(myitoa(uDht22GetHumidity() / 10, 10));
		vUsbserialWrite(".");
		vUsbserialWrite(myitoa(uDht22GetHumidity() % 10, 10));
		vUsbserialWrite("\r\n");
	}
	else
		vUsbserialWrite("CRC check failed!\r\n");
}

uint8_t handle_cmd(uint8_t *buf)
{
	uint8_t cnt;

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
		for (cnt=0; cnt<10; cnt++)
		{
			vDhtTest();
			vTimerDelayMs(2000);
		}

		return 1;
	}

	return 0;
}

extern volatile uint32_t pkt_cnt;

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
			if (payload_size==4)
				uNrfGetPayload((uint8_t *)&t, 4);
			t = uTimerGetMs() - t;
			vUsbserialWrite("Received packet: ");
			if (!t)
				vUsbserialWrite("0");
			else
				vUsbserialWrite(myitoa(t, 10));
			vUsbserialWrite("\r\n");
		}
	}
}
