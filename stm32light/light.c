/**************************************************************************//**
 * @file     light.c
 * @brief    STM32 light i2c sensor func definitions source file
 * @version  v1.0
 * @date     25.05.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_exti.h"
#include "misc.h"
#include "inttypes.h"
#include "interrupt.h"
#include "timer.h"
#include "light.h"
//#include "semihosting.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

#define I2C_LIGHT			I2C1
#define I2C_LIGHT_CLK		RCC_APB1Periph_I2C1
#define I2C_LIGHT_GPIO		GPIOB
#define I2C_LIGHT_GPIO_CLK	RCC_APB2Periph_GPIOB
#define I2C_LIGHT_INT		GPIO_Pin_5
#define I2C_LIGHT_SCL		GPIO_Pin_6
#define I2C_LIGHT_SDA		GPIO_Pin_7

#define I2C_SPEED			100000
#define I2C_LIGHT_ADDRESS	0x94


/******************************************************************************
 * Function declarations
 *****************************************************************************/

void vLightHardwareInit(void);
void vLightIntInit(void);
uint32_t vLightVal2Lux(uint8_t val1, uint8_t val2);
void vLightLux2Val(uint32_t lux, uint8_t *pVal1, uint8_t *pVal2);
void TestI2CAccess();

/******************************************************************************
 * Global variables
 *****************************************************************************/

/******************************************************************************
 * Function definitions
 *****************************************************************************/

uint32_t uLightReadLux()
{
	uint8_t val1, val2;

	vLightReadByte((u8*)&val1, MAX44009_LUX_HIGH);
	vLightReadByte((u8*)&val2, MAX44009_LUX_LOW);

	//return (1 << ((val1>>4)&0xf)) * (((val1&0xf)<<4) | (val2&0xf)) * 45/1000;
	return vLightVal2Lux(val1, val2);
}

void vLightCbState(vExtiCbState_t pCb)
{
	vExtiAddCb(GPIO_PortSourceGPIOB, GPIO_PinSource5, pCb);
}

void vLightInit(void)
{
	//SH_SendString("I2C init\n");
	vLightHardwareInit();
	vLightIntInit();
}

uint32_t vLightVal2Lux(uint8_t val1, uint8_t val2)
{
	return (1 << ((val1>>4)&0xf)) * (((val1&0xf)<<4) | (val2&0xf)) * 45/1000;
}

void vLightLux2Val(uint32_t lux, uint8_t *pVal1, uint8_t *pVal2)
{
	uint32_t base=0;
	uint8_t temp=0;

	base=lux * 1000 / 45;

	while (base>255)
	{
		temp++;
		base=base>>1;
	}

	*pVal1=(uint8_t) (((temp & 0xf)<<4) | ((base>>4)&0xf));
	*pVal2=(uint8_t) (base&0xf);
}

void vLightIntInit(void)
{
	uint8_t val1=0, val2=0;

	// upper limit: 20 lux
	vLightLux2Val(20, &val1, &val2);
	vLightWriteByte((u8*)&val1, MAX44009_UPPER_THRESHOLD);

	// lower limit: 8 lux
	vLightLux2Val(8, &val1, &val2);
	vLightWriteByte((u8*)&val1, MAX44009_LOWER_THRESHOLD);

	// time
	val1=20; // 2 sec
	vLightWriteByte((u8*)&val1, MAX44009_THRESHOLD_TIMER);

	// int enable
	val1=1;  // enable
	vLightWriteByte((u8*)&val1, MAX44009_INT_ENABLE);

	// clean current status if any
	vLightReadByte(&val1, MAX44009_INT_STATUS);
}

void vLightHardwareInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef I2C_InitStructure;
//	NVIC_InitTypeDef NVIC_InitStructure;
//	EXTI_InitTypeDef EXTI_InitStructure;

	/* I2C Periph clock enable */
    RCC_APB1PeriphClockCmd(I2C_LIGHT_CLK, ENABLE);
	// Enable PORTB Periph clock
	RCC_APB2PeriphClockCmd(I2C_LIGHT_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);

	/* Configure I2C pins */

	GPIO_InitStructure.GPIO_Pin = I2C_LIGHT_SCL | I2C_LIGHT_SDA;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(I2C_LIGHT_GPIO, &GPIO_InitStructure);

    /* I2C configuration */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = I2C_LIGHT_ADDRESS;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;

    /* Apply I2C configuration after enabling it */
    I2C_Init(I2C_LIGHT, &I2C_InitStructure);

    /* I2C Peripheral Enable */
    I2C_Cmd(I2C_LIGHT, ENABLE);
    //TestI2CAccess();

    // configure interrupt pin
	GPIO_InitStructure.GPIO_Pin = I2C_LIGHT_INT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // pull-up, default is 1
    GPIO_Init(I2C_LIGHT_GPIO, &GPIO_InitStructure);
}

void vLightWriteByte(uint8_t *pBuffer, u8 writeAddr)
{
    /* Send STRAT condition */
    I2C_GenerateSTART(I2C_LIGHT, ENABLE);

    /* Test on EV5 and clear it */
    while(!I2C_CheckEvent(I2C_LIGHT, I2C_EVENT_MASTER_MODE_SELECT));

    /* Send EEPROM address for write */
    I2C_Send7bitAddress(I2C_LIGHT, I2C_LIGHT_ADDRESS, I2C_Direction_Transmitter);

    /* Test on EV6 and clear it */
    while(!I2C_CheckEvent(I2C_LIGHT, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

    /* Send the MAX44009 internal address to write to */
    I2C_SendData(I2C_LIGHT, writeAddr);

    /* Test on EV8 and clear it */
    while(!I2C_CheckEvent(I2C_LIGHT, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

	/* Send the current byte */
	I2C_SendData(I2C_LIGHT, *pBuffer);

	/* Test on EV8 and clear it */
	while (!I2C_CheckEvent(I2C_LIGHT, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    /* Send STOP condition */
    I2C_GenerateSTOP(I2C_LIGHT, ENABLE);
}

void vLightReadByte(uint8_t *pBuffer, u8 readAddr)
{
	/* While the bus is busy */
	while (I2C_GetFlagStatus(I2C_LIGHT, I2C_FLAG_BUSY));

	/* Send START condition */
	I2C_GenerateSTART(I2C_LIGHT, ENABLE);

	/* Test on EV5 and clear it */
	while (!I2C_CheckEvent(I2C_LIGHT, I2C_EVENT_MASTER_MODE_SELECT));

	/* Send MAX44009 address for write */
	I2C_Send7bitAddress(I2C_LIGHT, I2C_LIGHT_ADDRESS, I2C_Direction_Transmitter);

	/* Test on EV6 and clear it */
	while (!I2C_CheckEvent(I2C_LIGHT, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	/* Clear EV6 by setting again the PE bit */
	I2C_Cmd(I2C_LIGHT, ENABLE);

	/* Send the MAX44009's internal address to read from */
	I2C_SendData(I2C_LIGHT, readAddr);

	/* Test on EV8 and clear it */
	while (!I2C_CheckEvent(I2C_LIGHT, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

	/* Send STRAT condition a second time */
	I2C_GenerateSTART(I2C_LIGHT, ENABLE);

	/* Test on EV5 and clear it */
	while (!I2C_CheckEvent(I2C_LIGHT, I2C_EVENT_MASTER_MODE_SELECT));

	/* Send MAX44009 address for read */
	I2C_Send7bitAddress(I2C_LIGHT, I2C_LIGHT_ADDRESS, I2C_Direction_Receiver);

	/* Test on EV6 and clear it */
	while (!I2C_CheckEvent(I2C_LIGHT, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

	/* Disable Acknowledgement */
	I2C_AcknowledgeConfig(I2C_LIGHT, DISABLE);

	/* Test on EV7 and clear it */
	while (!I2C_CheckEvent(I2C_LIGHT, I2C_EVENT_MASTER_BYTE_RECEIVED));

	/* Read a byte from the MAX44009 */
	*pBuffer = I2C_ReceiveData(I2C_LIGHT);

	/* Enable Acknowledgement to be ready for another reception */
	I2C_AcknowledgeConfig(I2C_LIGHT, ENABLE);

	/* Send STOP Condition */
	I2C_GenerateSTOP(I2C_LIGHT, ENABLE);
}

void TestI2CAccess()
{
#define _I2C_SR2_BUSY           1

#define PIN6                            6
#define PIN7                            7
#define CR_PIN6                         (PIN6 << 2)
#define CR_PIN7                         (PIN7 << 2)

#define CR_MODE6                        CR_PIN6
#define CR_MODE7                        CR_PIN7

#define CR_CNF6                         (CR_PIN6 + 2)
#define CR_CNF7                         (CR_PIN7 + 2)

   if(!(I2C1->SR2 & (1 << _I2C_SR2_BUSY))) // провер€ю, зан€та ли шина I2C (взведЄн ли флаг BUSY)
      return; // если нет - выходим
   // а если взведЄн - рулю ситуацию
   GPIOB->BSRR |= (1 << PIN7) | (1 << PIN6); // выставл€ю SDA и SCL в 1
   GPIOB->CRL &= ~(0xC << CR_PIN6) & ~(0xF << CR_PIN7); // настраиваю порты: 6 - выход(SCL), 7 - вход(SDA)
   GPIOB->CRL |= (0x3 << CR_MODE6) | (0x2 << CR_CNF7);

   while(1) // здесь мы можем быть максимум 9 раз (крайний случай, смотри в pdf)
   {// дЄргаю SCL (тактирую Slave)
      GPIOB->BRR |= 1 << PIN6; // SCL = 0
      vTimerDelayMs(1);
      GPIOB->BSRR |= 1 << PIN6; // SCL = 1
      vTimerDelayMs(1);
      if(GPIOB->IDR & (1 << PIN7)) // смотрю, отпустил ли Slave SDA (SDA == 1 ?)
      {// если да - настраиваю выводы на выход и делаю Stop состо€ние
         GPIOB->BRR |= 1 << PIN6; // SCL = 0
         vTimerDelayMs(1);
         GPIOB->BRR |= 1 << PIN7; // SDA = 0
         vTimerDelayMs(1);

         GPIOB->CRL &= ~(0xC << CR_PIN7); // выход
         GPIOB->CRL |= (0x3 << CR_MODE7);

         GPIOB->BSRR |= 1 << PIN6; // SCL = 1
         vTimerDelayMs(1);
         GPIOB->BSRR |= 1 << PIN7; // SDA = 1
         break; // выходим из цикла
      }
   }
   // возвращаю настройки порта (аппаратный I2C)
   GPIOB->CRL |=  (0x3 << CR_MODE6) | (0x3 << CR_CNF6) | (0x3 << CR_MODE7) | (0x3 << CR_CNF7);
   // после этого шина I2C свободна и готова к работе
}
