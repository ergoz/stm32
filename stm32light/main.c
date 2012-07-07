#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "motion.h"
#include "led.h"
#include "misc.h"
#include "light.h"
#include "timer.h"
#include "exti.h"
#include "nrf.h"
#include "interrupt.h"
#include "stdio.h"
#include "stdlib.h"
#include "nRF24L01.h"

// global vars

volatile ExtiLineState_t g_currentMotion = LineState_Low;
volatile ExtiLineState_t g_currentButton = LineState_High;
volatile ExtiLineState_t g_currentLight = LineState_High;

// func def

void InitAll( void);
void MainLoop(void);

static void vMotCbState(ExtiLineState_t state);
static void vBtnCbState(ExtiLineState_t state);
static void vLtCbState(ExtiLineState_t state);

// func decl

void vMainNrfDump(void)
{
	uint8_t buf[5];
	uNrfReadRegister(CONFIG, (uint8_t *)&buf, 1);
	memset(buf, 0, 5);
	uNrfReadRegister(STATUS, (uint8_t *)&buf, 1);
	memset(buf, 0, 5);
}


void MainLoop(void)
{
    //InterruptStatus_t status;
	ExtiLineState_t currentMotion = LineState_Low;
	ExtiLineState_t currentButton = LineState_High;
	ExtiLineState_t currentLight = LineState_High;
	uint32_t timeMs=0, lastTime=0, lastBtnTime=0;
	int32_t motionEnabled=0, ledActive=0, t;
	uint8_t val=0, payload_size=0;

    while(1)
    {
    	timeMs = uTimerGetMs();

        //status = Interrupt_saveAndDisable();
        currentMotion=g_currentMotion;
        g_currentMotion = LineState_Low;
        currentButton=g_currentButton;
        g_currentButton = LineState_High;
        currentLight=g_currentLight;
        g_currentLight = LineState_High;
        //Interrupt_restore(status);

        if (!uNrfIsSending() && uNrfIsPayloadReceived())
        {
        	payload_size=uNrfGetPayloadSize();
        	if (payload_size==4)
        	{
				uNrfGetPayload((uint8_t *)&t, 4);
				vNrfSend((uint8_t *)"main1", (uint8_t *)&t, 4);
//				while (uNrfIsSending()) {};
        	}
//			vLedSetState(LedState_Green, 100);
//			vTimerDelayMs(10);
//			vLedSetState(LedState_Off, 0);
        }

        if (uNrfIsSending())
        	vMainNrfDump();

        if (currentLight == LineState_Low)	// light is triggered
        {
        	vLightReadByte(&val, 0);
        	if (val==1 && !ledActive)		// light int is active and led not lighting
        	{
        		if (uLightReadLux()>20)		// light is high
        		{
        			if (motionEnabled)		// if all is enabled - disable it
        			{
						vLedSetState(LedState_Off, 0);
						ledActive=0;
						motionEnabled=0;
						lastTime=0;
        			}
        		}
        		else						// light should be low
        		{
        			if (!motionEnabled)		// if not enabled - enable it
        			{
						vLedSetState(LedState_White, 100);
						ledActive=1;
						motionEnabled=1;
						lastTime=timeMs;
        			}
        		}
        	}
        }

        if (currentMotion == LineState_High)
        {
        	if (motionEnabled)
        	{
        		lastTime=timeMs;

        		if (!ledActive)
        		{
        			vLedSetState(LedState_White, 100);
        			ledActive=1;
        		}
        	}
        	//currentMotion = MotionState_Off;
        }

        if (currentButton == LineState_Low)
        {
        	if ((timeMs - lastBtnTime)>100) // delay
        	{
        		if (!motionEnabled)
        		{
        			vLedSetState(LedState_Green, 100);
        			vTimerDelayMs(50);
        			vLedSetState(LedState_Off, 0);
        			motionEnabled=1;
        		}
        		else
        		{
        			vLedSetState(LedState_Off, 0);
        			vLedSetState(LedState_Red, 100);
        			vTimerDelayMs(50);
        			vLedSetState(LedState_Off, 0);
        			motionEnabled=0;
        			ledActive=0;
        			lastTime=0;
        		}
        	}

        	lastBtnTime = timeMs;
        	//currentButton = ButtonState_Off;
        }

        if (ledActive)
        {
        	if ((timeMs-lastTime) > (30 * 1000)) // timeout
        	{
        		ledActive=!ledActive;
        		vLedSetState(LedState_Off, 0);
        	}
        }
    }
}


int main(void)
{
//	uint8_t buf[16], i, v;
	uint32_t t=0;

	InitAll();

//	for (i=0; i<7; i++)
//	{
//		memset(buf, 0, 16);
//		vLightReadByte(&v, i);
//		sprintf(buf, "%d: %d\n", i, v);
//		SH_SendString(buf);
//		vTimerDelayMs(1000);
//	}
//
//	while (1)
//	{
//		memset(buf, 0, 16);
//		sprintf(buf, "Lux: %d\n", uLightReadLux());
//		SH_SendString(buf);
//		vTimerDelayMs(1000);
//	}

	vLedSetState(LedState_Red, 100);
	vTimerDelayMs(500);
	vLedSetState(LedState_Green, 100);
	vTimerDelayMs(500);
	vLedSetState(LedState_Blue, 100);
	vTimerDelayMs(500);
	vLedSetState(LedState_Off, 0);

	vNrfSend((uint8_t *) "main1", (uint8_t *) &t, 4);
	while (uNrfIsSending())
		vMainNrfDump();

	while (1)
	{
		MainLoop();
	}

	// never should be there
	return 0;
}

void InitAll( void)
{
	// set config for NVIC
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);

	vExtiInit();
	vTimerInit();

	vLightInit();
	vLedInit();
	vMotionInit();

	vMotionCbState(vMotCbState);
	vButtonCbState(vBtnCbState);
	vLightCbState(vLtCbState);

	vNrfHwInit();
	vNrfInit(1, (uint8_t *)"litt1");

	vExtiStart();
}

static void vMotCbState(ExtiLineState_t state)
{
	g_currentMotion=state;
}

static void vBtnCbState(ExtiLineState_t state)
{
	g_currentButton=state;
}

static void vLtCbState(ExtiLineState_t state)
{
	g_currentLight=state;
}
