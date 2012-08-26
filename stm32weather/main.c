#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_iwdg.h"
#include "misc.h"
#include "nrf.h"
#include "dht22.h"
#include "exti.h"
#include "timer.h"
#include "queue.h"
#include "nRF24L01.h"
#include "packet.h"
#include "irled.h"
#include "mpl115a1.h"
//#include "usbserial.h"
#include "tim4.h"
#include "common.h"

#define NRF_OWN_ADDR	"temp1"

static volatile uint8_t g_MainStartDht=0;

void vMainTim4Cb(void)
{
	g_MainStartDht=1;
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

	//vUsbserialInit();
	vTimerInit();
	vExtiInit();
	vNrfHwInit();
	vNrfInit(0, (uint8_t *)NRF_OWN_ADDR);
	vDht22Init();
	vIrledInit();
	vMpl115a1Init();

	vTim4Init(60000, 12000);
	vTim4SetCb(&vMainTim4Cb);
	vMainIWDGInit();

	// this should be latest to let all other modules init EXTI
	vExtiStart();
}
/*
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
*/
static uint16_t g_dht1=0, g_dht2=0;
static uint8_t g_dht_status=0;

void vMainDhtMeasure(void)
{
	vDht22Start();
	while (uDht22Measuring());

	if (uDht22CheckCrc())
	{
		g_dht1 = uDht22GetTemp();
		g_dht2 = uDht22GetHumidity();
		g_dht_status = 1;

		//vDhtTest();
	} else
		g_dht_status = 0;
}

/*
void vDhtTest(void)
{
	if (g_dht_status)
	{
		vUsbserialWrite("Temp: ");
		vUsbserialWrite(myitoa(g_dht1 / 10, 10));
		vUsbserialWrite(".");
		vUsbserialWrite(myitoa(g_dht1 % 10, 10));
		vUsbserialWrite("\r\n");

		vUsbserialWrite("Humidity: ");
		vUsbserialWrite(myitoa(g_dht2 / 10, 10));
		vUsbserialWrite(".");
		vUsbserialWrite(myitoa(g_dht2 % 10, 10));
		vUsbserialWrite("\r\n");
	}
	else
		vUsbserialWrite("CRC check failed!\r\n");
}

uint8_t handle_cmd(uint8_t *buf)
{
//	uint8_t cnt;

	if (strlen((const char *) buf) > 5 &&
			memcmp((const char *) buf, (const char *) "status", 6) == 0)
	{
		vMainNrfDump();
		return 1;
	}

	if (strlen((const char *)buf)>4 &&
			memcmp((const char *)buf, (const char *)"dht22", 5)==0)
	{
		vDhtTest();
		return 1;
	}

	return 0;
}
*/

int main(void)
{
	Packet_t pkt;
	uint8_t payload_size;
	uint8_t sender[5];
	//const uint8_t cmd[]={0x80, 0x4d, 0x75, 0x8f, 0xce, 0x88, 0xf};
//	uint8_t buf[256];
//	uint32_t j=0;
	uint8_t state=1;

	InitAll();

	vTim4Start();

	while(1)
    {
		IWDG_ReloadCounter();

		if (g_MainStartDht && !uNrfIsSending())
		{
			g_MainStartDht=0;
			vMainDhtMeasure();
			state=!state;
			if (state)
				GPIO_SetBits(GPIOB, GPIO_Pin_1);
			else
				GPIO_ResetBits(GPIOB, GPIO_Pin_1);
		}

//		if (uUsbSerialDataAvailable())
//		{
//			j += uUsbSerialRead(&buf[j], 256 - j);
//			if (buf[j-1]=='\r')		// this is wrong!!! need to scan whole string
//			{
//				buf[j-1]=0;
//				vUsbserialWrite("Command: ");
//				vUsbserialWrite((char *)buf);
//				vUsbserialWrite("\r\n");
//				j=0;
//
//
//				if (!handle_cmd(buf))
//					vUsbserialWrite("Unknown command...\r\n");
//
//				memset(buf, 0, 256);
//			}
//		}

//		vTimerDelayMs(1000);
//		vIrledSend(cmd, sizeof(cmd)*8);
		//vDhtMeasure();

		if (!uNrfIsSending() && uNrfIsPayloadReceived())
		{
			payload_size = uNrfGetPayloadSize();
			if (payload_size==sizeof(Packet_t))
			{
				uNrfGetPayload((uint8_t *)&pkt, sizeof(Packet_t));
				memcpy(sender, pkt.sender, 5);
				memcpy(pkt.sender, NRF_OWN_ADDR, 5);
				pkt.cmd|=PACKET_REPLY;

				switch (GET_PACKET_CMD(pkt.cmd))
				{
					case PACKET_PING:
					{
						//vIrledSend(cmd, sizeof(cmd)*8);
						pkt.status = 1;
						break;
					}
					case PACKET_AIR:
					{
						vIrledSend(pkt.data.ir, sizeof(pkt.data.ir) * 8);
						pkt.status = 1;
						break;
					}
					case PACKET_DHT22:
					{
						pkt.status = g_dht_status;
						pkt.data.dht22[0] = g_dht1;
						pkt.data.dht22[1] = g_dht2;
						break;
					}
					case PACKET_PRESSURE:
					{
						pkt.status = 1;
						pkt.data.pressure = uMpl115a1ReadPressure();
						break;
					}
					default:
					{
						pkt.status = 0;
						break;
					}
				};
				vNrfSend((uint8_t *) sender, (uint8_t *) &pkt, sizeof(Packet_t));
			}
		}
    }
}
