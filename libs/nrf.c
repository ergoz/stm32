/**************************************************************************//**
 * @file     spi.c
 * @brief    STM32 SPI generic func source file
 * @version  v1.0
 * @date     07.06.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/

#include "inttypes.h"
#include "spi.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_dma.h"
#include "misc.h"
#include "spi.h"
#include "exti.h"
#include "queue.h"
#include "interrupt.h"
#include "nRF24L01.h"
#include "timer.h"
#include "usbserial.h"

//Chip Enable Activates RX or TX mode
#define CE_H()		GPIO_SetBits(GPIOA, GPIO_Pin_8)
#define CE_L()		GPIO_ResetBits(GPIOA, GPIO_Pin_8)

//SPI Chip Select
#define SPI1_H()	GPIO_SetBits(GPIOA, GPIO_Pin_4)
#define SPI1_L()	GPIO_ResetBits(GPIOA, GPIO_Pin_4)

// INT pib
#define NRF_GPIO	GPIOB
#define NRF_INT_PIN	GPIO_Pin_0
#define NRF_CS_PIN	GPIO_Pin_4
#define NRF_CE_PIN	GPIO_Pin_8

#define NRF_ADR_LEN    5   // 5 bytes TX(RX) address width
#define NRF_CONFIG ((1<<EN_CRC) | (0<<CRCO) )

/******************************************************************************
 * Global variables
 *****************************************************************************/

static volatile uint8_t g_uNrfTx=0;		// we transmitting
static volatile uint8_t g_uNrdLastTxStatus=0;
static Queue_t g_NrfQueue;

//static uint8_t g_RxAddr[]={0,0,0,0,0};
//static uint8_t g_ZeroAddr[]={0,0,0,0,0};

/******************************************************************************
 * Function declarations
 *****************************************************************************/

uint8_t uNrfWriteReg(uint8_t reg, uint32_t value);
uint8_t uNrfReadReg(uint8_t reg);
uint8_t uNrfReadBuf(uint8_t reg, uint8_t *pBuf, uint32_t bufSize);
uint8_t uNrfWriteBuf(uint8_t reg, uint8_t *pBuf, uint32_t bufSize);

void vNrfFlushRx(void);
void vNrfFlushTx(void);
void vNrfPowerUpRx(void);
void vNrfPowerUpTx(void);
void vNrfPowerDown(void);
uint8_t uNrfGetFifoStatus(void);
static void vNrfCbState(ExtiLineState_t state);
uint8_t uNrfGetDynamicPayloadSize(void);
void uNrfToggleFeatures(void);
void vNrfSetDynamicPayload(void);

/******************************************************************************
 * Function definitions
 *****************************************************************************/

uint8_t uNrfWriteReg(uint8_t reg, uint32_t value)
{
	uint8_t status;
	SPI1_L();
	//select register
	status = uSpiReadWriteByte(reg | W_REGISTER);
	//write to it the value
	uSpiReadWriteByte(value);
	SPI1_H();
	return(status);
}

uint8_t uNrfReadReg(uint8_t reg)
{
	unsigned char reg_val;
	SPI1_L();
	uSpiReadWriteByte(reg);
	reg_val = uSpiReadWriteByte(0);
	SPI1_H();
	return(reg_val);
}

uint8_t uNrfReadBuf(uint8_t reg, uint8_t *pBuf, uint32_t bufSize)
{
	uint8_t status;
	uint32_t i;
	SPI1_L();
	// Select register to write to and read status byte
	status = uSpiReadWriteByte(reg);
	for(i=0;i<bufSize;i++)
		pBuf[i] = uSpiReadWriteByte(0);
	SPI1_H();
	return(status);
}

uint8_t uNrfWriteBuf(uint8_t reg, uint8_t *pBuf, uint32_t bufSize)
{
	uint8_t status;
	uint32_t i;
	SPI1_L();
	// Select register to write to and read status byte
	status = uSpiReadWriteByte(reg | W_REGISTER);
	for(i=0; i<bufSize; i++) // then write all byte in buffer(*pBuf)
		uSpiReadWriteByte(pBuf[i]);
	SPI1_H();
	return(status);
}

uint8_t uNrfReadRegister(uint8_t reg, uint8_t *data, uint8_t len)
{
	return uNrfReadBuf(reg, data, len);
}

uint8_t uNrfGetFifoStatus(void)
{
	return uNrfReadReg(FIFO_STATUS);
}

void vNrfSend(uint8_t * addr, uint8_t *data, uint8_t len)
 {
	uint8_t status;
	static uint8_t last_addr[NRF_ADR_LEN];

	while (g_uNrfTx)	// this should be never executed code!
	{
		status = uNrfReadReg(STATUS);
		if (status & ( (1 << TX_DS) | (1 << MAX_RT) ) )
		{
			uNrfWriteReg(STATUS, (1 << TX_DS) | (1 << MAX_RT));
			g_uNrfTx=0;
			break;
		}
	}

	if (memcmp(last_addr, addr, 5)!=0)
	{
		memcpy(last_addr, addr, 5);
		uNrfWriteBuf(TX_ADDR, addr, NRF_ADR_LEN);
		uNrfWriteBuf(RX_ADDR_P0, addr, NRF_ADR_LEN);
	}

	CE_L();
	vNrfPowerUpTx();
	vNrfFlushTx();
	uNrfWriteBuf(W_TX_PAYLOAD, data, len);
	CE_H();
//	vTimerDelayUs(20);
//	CE_L();
}

void vNrfPowerUpRx(void)
{
	g_uNrfTx = 0;
	CE_L();
	uNrfWriteReg(CONFIG, NRF_CONFIG | ((1<<PWR_UP) | (1<<PRIM_RX)));
	uNrfWriteReg(EN_RXADDR, (0<<ERX_P0) | (1<<ERX_P1));
	uNrfWriteReg(STATUS, (1 << TX_DS) | (1 << MAX_RT));
	CE_H();
	// need delay!!! but inside interrupt!!
}

void vNrfPowerUpTx(void)
 {
	g_uNrfTx = 1;
	uNrfWriteReg(CONFIG, NRF_CONFIG | (1<<PWR_UP) | (0<<PRIM_RX));
	uNrfWriteReg(EN_RXADDR, (1<<ERX_P0) | (1<<ERX_P1));
}

void vNrfPowerDown(void)
 {
	uNrfWriteReg(CONFIG, NRF_CONFIG);
}

void vNrfFlushTx(void)
{
	SPI1_L();
	uSpiReadWriteByte(FLUSH_TX);
	SPI1_H();
}

void vNrfFlushRx(void)
{
	SPI1_L();
	uSpiReadWriteByte(FLUSH_RX);
	SPI1_H();
}


uint8_t uNrfIsSending(void)
{
//	uint8_t status;

	if (g_uNrfTx)
	{
//		status = uNrfReadReg(STATUS);
//		if (status & ( (1 << TX_DS) | (1 << MAX_RT) ) )
//		{
//			vNrfPowerUpRx();
//			return 0;
//		}
		return 1;
	}
	return 0;
}

uint8_t uNrfIsPayloadReceived(void)
{
    return uQueueGetBytesToRead(&g_NrfQueue);
}

uint8_t uNrfGetPayloadSize(void)
{
	return uQueueReadByte(&g_NrfQueue);
}

uint8_t uNrfGetPayload(uint8_t *data, uint8_t len)
{
	return uQueueRead(&g_NrfQueue, data, len);
}

void vNrfSkipPayload(uint8_t len)
{
	vQueueSkip(&g_NrfQueue, len);
}

void uNrfToggleFeatures(void)
{
	SPI1_L();
	uSpiReadWriteByte(ACTIVATE);
	uSpiReadWriteByte(0x73);
	SPI1_H();
}

void vNrfSetDynamicPayload(void)
{
	uNrfWriteReg(FEATURE, uNrfReadReg(FEATURE) | (1<<EN_DPL));

	if (!uNrfReadReg(FEATURE))
	{
		uNrfToggleFeatures();
		uNrfWriteReg(FEATURE, uNrfReadReg(FEATURE) | (1<<EN_DPL));
	}

	uNrfWriteReg(DYNPD, uNrfReadReg(DYNPD) | (1<<DPL_P1) | (1<<DPL_P0));
}

uint8_t uNrfGetDynamicPayloadSize(void)
{
	return uNrfReadReg(R_RX_PL_WID);
}


static void vNrfCbState(ExtiLineState_t state)
{
	uint8_t status;
	uint8_t rx_buf[32], payload_size;

	if (state==LineState_Low)	// INT active
	{
		status = uNrfReadReg(STATUS);

		if ( status & (1 << RX_DR) )	// new data ready
		{
			while (1)
			{
				// read payload size
				payload_size=uNrfGetDynamicPayloadSize();
				// store payload size
				uQueueWriteByte(&g_NrfQueue, payload_size);
				// read data
				uNrfReadBuf(R_RX_PAYLOAD, rx_buf, payload_size);
				// clear RX bit
				uNrfWriteReg(STATUS, (1 << RX_DR));
				// save result
				uQueueWrite(&g_NrfQueue, rx_buf, payload_size);
				// check: FIFO buffer contain something else?
				if (uNrfGetFifoStatus() & (1 << RX_EMPTY))
					break;
			}
		}
		if ((status & ((1 << TX_DS) | (1 << MAX_RT))))	// data send finished or max_rt
		{
			if (status & (1 << MAX_RT))
				g_uNrdLastTxStatus = 0;
			else
				g_uNrdLastTxStatus = 1;
			vNrfPowerUpRx();
		}
	}
}

uint8_t uNrfGetLastTxStatus(void)
{
	return g_uNrdLastTxStatus;
}

void vNrfHwInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = NRF_CE_PIN | NRF_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // configure interrupt pin
	GPIO_InitStructure.GPIO_Pin = NRF_INT_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // pull-up, default is 1
    GPIO_Init(NRF_GPIO, &GPIO_InitStructure);

    vSpiInit();

    SPI1_H();
	CE_L();

	// add int exti handler
	vExtiAddCb(GPIO_PortSourceGPIOB, GPIO_PinSource0, vNrfCbState);
}

void vNrfInit(uint8_t channel, uint8_t * rx_addr)
{
	//		memcpy(g_RxAddr, rx_addr, NRF_ADR_LEN);
	CE_L();
	uNrfWriteBuf(RX_ADDR_P1, rx_addr, NRF_ADR_LEN);
	CE_H();

	uNrfWriteReg(RF_CH, channel); // Select RF channel

	uNrfWriteReg(RX_PW_P0, 4);
	uNrfWriteReg(RX_PW_P1, 4);

	uNrfWriteReg(SETUP_RETR, 0x1F); // 15 retries
	//	uNrfWriteReg(RF_SETUP, 0xF);		// 2 mbps + max gain + LNA

	vNrfSetDynamicPayload();

	vNrfPowerUpRx();
	vNrfFlushRx();
}
