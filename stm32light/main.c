#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "nrf.h"
#include "led.h"
#include "light.h"
#include "motion.h"
#include "exti.h"
#include "timer.h"
#include "usbserial.h"
#include "queue.h"
#include "nRF24L01.h"
#include "packet.h"

#define NRF_OWN_ADDR	"litt1"


// global vars

volatile ExtiLineState_t g_currentMotion = LineState_Low;
volatile ExtiLineState_t g_currentButton = LineState_High;
volatile ExtiLineState_t g_currentLight = LineState_High;

// func def

void InitAll(void);
static void vMotCbState(ExtiLineState_t state);
static void vBtnCbState(ExtiLineState_t state);
static void vLtCbState(ExtiLineState_t state);
void vMainNrfDump(void);

// func decl

void vMainNrfDump(void) {
	uint8_t buf[5];
	uNrfReadRegister(CONFIG, (uint8_t *) &buf, 1);
	memset(buf, 0, 5);
	uNrfReadRegister(STATUS, (uint8_t *) &buf, 1);
	memset(buf, 0, 5);
}

static void vMotCbState(ExtiLineState_t state) {
	g_currentMotion = state;
}

static void vBtnCbState(ExtiLineState_t state) {
	g_currentButton = state;
}

static void vLtCbState(ExtiLineState_t state) {
	g_currentLight = state;
}

void InitAll(void) {
	GPIO_InitTypeDef GPIO_InitStructure;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	// onboard led
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	/* Configure LED */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	vTimerInit();
	vExtiInit();
	vLightInit();
	vLedInit();
	vMotionInit();

	vMotionCbState(vMotCbState);
	vButtonCbState(vBtnCbState);
	vLightCbState(vLtCbState);

	vNrfHwInit();
	vNrfInit(0, (uint8_t *)NRF_OWN_ADDR);
	// this should be latest to let all other modules init EXTI
	vExtiStart();
}

int main(void) {
	ExtiLineState_t currentMotion = LineState_Low;
	ExtiLineState_t currentButton = LineState_High;
	ExtiLineState_t currentLight = LineState_High;

//	uint8_t state = 1;
	uint8_t payload_size;
	uint8_t val=0;

//	uint8_t buf[32];

	uint32_t timeMs=0, lastTime=0, lastBtnTime=0;
	int32_t motionEnabled=0, ledActive=0;

	Packet_t pkt;
	uint8_t sender[5];


	InitAll();

	vLedSetState(LedState_Red, 100);
	vTimerDelayMs(500);
	vLedSetState(LedState_Green, 100);
	vTimerDelayMs(500);
	vLedSetState(LedState_Blue, 100);
	vTimerDelayMs(500);
	vLedSetState(LedState_Off, 0);

	//	GPIO_SetBits(GPIOB, GPIO_Pin_1);

	while (1)
	{
    	timeMs = uTimerGetMs();

        currentMotion=g_currentMotion;
        g_currentMotion = LineState_Low;
        currentButton=g_currentButton;
        g_currentButton = LineState_High;
        currentLight=g_currentLight;
        g_currentLight = LineState_High;

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
						pkt.status = 1;
						break;
					}
					case PACKET_LUX:
					{
						pkt.data.lux=uLightReadLux();
						pkt.status = 1;
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

		if (currentLight == LineState_Low) // light is triggered
		{
			vLightReadByte(&val, MAX44009_INT_STATUS);
			if (val == 1 && !ledActive) // light int is active and led not lighting
			{
				if (uLightReadLux() > 20) // light is high
				{
					if (motionEnabled) // if all is enabled - disable it
					{
						vLedSetState(LedState_Off, 0);
						ledActive = 0;
						motionEnabled = 0;
						lastTime = 0;
					}
				} else // light should be low
				{
					if (!motionEnabled) // if not enabled - enable it
					{
						vLedSetState(LedState_White, 100);
						ledActive = 1;
						motionEnabled = 1;
						lastTime = timeMs;
					}
				}
			}
		}

		if (currentMotion == LineState_High)
		{
			if (motionEnabled)
			{
				lastTime = timeMs;

				if (!ledActive)
				{
					vLedSetState(LedState_White, 100);
					ledActive = 1;
				}
			}
			//currentMotion = MotionState_Off;
		}

		if (currentButton == LineState_Low)
		{
			if ((timeMs - lastBtnTime) > 100) // delay
			{
				if (!motionEnabled)
				{
					vLedSetState(LedState_Green, 100);
					vTimerDelayMs(50);
					vLedSetState(LedState_Off, 0);
					motionEnabled = 1;
				}
				else
				{
					vLedSetState(LedState_Off, 0);
					vLedSetState(LedState_Red, 100);
					vTimerDelayMs(50);
					vLedSetState(LedState_Off, 0);
					motionEnabled = 0;
					ledActive = 0;
					lastTime = 0;
				}
			}

			lastBtnTime = timeMs;
			//currentButton = ButtonState_Off;
		}

		if (ledActive)
		{
			if ((timeMs - lastTime) > (45 * 1000)) // timeout
			{
				ledActive = !ledActive;
				vLedSetState(LedState_Off, 0);
			}
		}
	}
}
