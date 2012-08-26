#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_iwdg.h"
#include "misc.h"
#include "nrf.h"
#include "exti.h"
#include "timer.h"
#include "usbserial.h"
#include "queue.h"
#include "nRF24L01.h"
#include "packet.h"
#include "common.h"

#define NRF_OWN_ADDR	"serv1"

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

void vMainIWDGInit(void)
{
    /* IWDG timeout equal to 350ms (the timeout may varies due to LSI frequency
     dispersion) -------------------------------------------------------------*/
    /* Enable write access to IWDG_PR and IWDG_RLR registers */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    /* IWDG counter clock: 32KHz(LSI) / 256 = 1KHz */
    IWDG_SetPrescaler(IWDG_Prescaler_256);

    /* Set counter reload value to 1563 */
    IWDG_SetReload(1563);

    /* Reload IWDG counter */
    IWDG_ReloadCounter();

    /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
    IWDG_Enable();
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
	vNrfInit(0, (uint8_t *)NRF_OWN_ADDR);
	vMainIWDGInit();

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

void vSendIr(uint8_t *addr, uint8_t *buf)
{
	Packet_t pkt;
	uint8_t cnt;
//	uint8_t ir[7];
//	int32_t i;

	for (cnt=0; cnt<7; cnt++)
	{
		pkt.data.ir[cnt]=(uint8_t)myatoi(buf+cnt*2, 2);
	}

	pkt.cmd=PACKET_AIR;

	memcpy(pkt.sender, NRF_OWN_ADDR, 5);
	vNrfSend(addr, (uint8_t *) &pkt, sizeof(Packet_t));
}


void vDhtSend(uint8_t *addr)
{
	Packet_t pkt;

	pkt.cmd=PACKET_DHT22;
	memcpy(pkt.sender, NRF_OWN_ADDR, 5);
	vNrfSend(addr, (uint8_t *) &pkt, sizeof(Packet_t));
}

void vLuxSend(uint8_t *addr)
{
	Packet_t pkt;

	pkt.cmd=PACKET_LUX;
	memcpy(pkt.sender, NRF_OWN_ADDR, 5);
	vNrfSend(addr, (uint8_t *) &pkt, sizeof(Packet_t));
}

void vPressureSend(uint8_t *addr)
{
	Packet_t pkt;

	pkt.cmd=PACKET_PRESSURE;
	memcpy(pkt.sender, NRF_OWN_ADDR, 5);
	vNrfSend(addr, (uint8_t *) &pkt, sizeof(Packet_t));
}


uint8_t handle_cmd(uint8_t *buf)
{
	uint8_t *p;

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

	if (strlen((const char *)buf)>10 &&
			memcmp((const char *)buf, (const char *)"send", 4)==0)
	{
		p=buf+5;
		buf+=11;

		if (strlen((const char *)buf)>17 &&
				memcmp((const char *)buf, (const char *)"air", 3)==0)
		{
			vSendIr(p, buf+4);
			return 1;
		}

		if (strlen((const char *)buf)>4 &&
				memcmp((const char *)buf, (const char *)"dht22", 5)==0)
		{
			vDhtSend(p);
			return 1;
		}

		if (strlen((const char *)buf)>7 &&
				memcmp((const char *)buf, (const char *)"pressure", 8)==0)
		{
			vPressureSend(p);
			return 1;
		}

		if (strlen((const char *)buf)>2 &&
				memcmp((const char *)buf, (const char *)"lux", 3)==0)
		{
			vLuxSend(p);
			return 1;
		}

	}

	return 0;
}

//extern volatile uint32_t pkt_cnt;

int main(void)
{
	uint32_t j;
	uint8_t state=1;
	uint8_t buf[256], sender[6];
	uint8_t payload_size;
	Packet_t pkt;

	memset(buf, 0, 256);

	InitAll();
	//vMainNrfDump();

	//GPIO_SetBits(GPIOB, GPIO_Pin_1);

	j = 0;
	while (1)
	{
		IWDG_ReloadCounter();
		if (uUsbSerialDataAvailable())
		{
			j += uUsbSerialRead(&buf[j], 256 - j);
			if (buf[j-1]=='\r')		// this is wrong!!! need to scan whole string
			{
				buf[j-1]=0;
//				vUsbserialWrite("Command: ");
//				vUsbserialWrite((char *)buf);
//				vUsbserialWrite("\r\n");
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
			{
				uNrfGetPayload((uint8_t *)&pkt, sizeof(Packet_t));

				vUsbserialWrite("Received packet from: ");
				memset(sender, 0, 6);
				memcpy(sender, pkt.sender, 5);
				vUsbserialWrite(sender);
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
								if (pkt.data.dht22[0] % 10 != 0)
									vUsbserialWrite(myitoa(pkt.data.dht22[0] % 10, 10));
								else
									vUsbserialWrite("0");
								vUsbserialWrite("\r\n");

								vUsbserialWrite("Humidity: ");
								vUsbserialWrite(myitoa(pkt.data.dht22[1] / 10, 10));
								vUsbserialWrite(".");
								if (pkt.data.dht22[1] % 10 != 0)
									vUsbserialWrite(myitoa(pkt.data.dht22[1] % 10, 10));
								else
									vUsbserialWrite("0");
								vUsbserialWrite("\r\n");
							}
							else
								vUsbserialWrite("DHT22 measuring failed!\r\n");
							break;
						}
						case PACKET_LUX:
						{
							if (pkt.status==1)
							{
								vUsbserialWrite("Lux: ");
								vUsbserialWrite(myitoa(pkt.data.lux, 10));
								vUsbserialWrite("\r\n");
							}
							break;
						}
						case PACKET_PRESSURE:
						{
							if (pkt.status==1)
							{
								vUsbserialWrite("Pressure: ");
								vUsbserialWrite(myitoa(pkt.data.pressure, 10));
								vUsbserialWrite("\r\n");
							}
							break;
						}
						default:
						{
							vUsbserialWrite("Packet type: ");
							vUsbserialWrite(myitoa(pkt.cmd&PACKET_CMD_MASK, 10));
							vUsbserialWrite("\r\n");
							vUsbserialWrite("Status: ");
							if (pkt.status==1)
								vUsbserialWrite("Ok\r\n");
							else
								vUsbserialWrite("Failed\r\n");
							break;
						}
					}

				}
				else // ask
				{
					memcpy(pkt.sender, NRF_OWN_ADDR, 5);
					pkt.cmd |= PACKET_REPLY;

					switch (pkt.cmd & PACKET_CMD_MASK)
					{
						case PACKET_PING:
						{
							pkt.status = 1;
							break;
						}
						default:
						{
							pkt.status = 0;
							break;
						}
					}
					vNrfSend((uint8_t *) sender, (uint8_t *) &pkt, sizeof(Packet_t));
				}
			}
			else
				vNrfSkipPayload(payload_size);
		}
	}
}
