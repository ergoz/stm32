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

//void SPI1_IRQHandler(void);

/******************************************************************************
 * Function definitions
 *****************************************************************************/

void vSpiInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;
//    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* SPI1 IRQ Channel configuration */
//    NVIC_InitStructure.NVIC_IRQChannel = SPI1_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);

    /* Configure SPI1 pins: SCK, MISO and MOSI -------------------------------*/
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    SPI_I2S_DeInit(SPI1);
    SPI_Cmd(SPI1, DISABLE);

    /* SPI1 Configuration ----------------------------------------------------*/
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);


    /* Enable the SPI1 TxE interrupt */
    //SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_TXE, ENABLE);

    /* Enable the SPI1 RxNE interrupt */
    //SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_RXNE, ENABLE);

    SPI_SSOutputCmd(SPI1, ENABLE);
    /* Enable SPI1 */
    SPI_Cmd(SPI1, ENABLE);
}

//void SPI1_IRQHandler(void)
//{
//    /* Check the interrupt source */
//    if (SPI_I2S_GetITStatus(SPI1, SPI_I2S_IT_TXE) == SET)
//    {
//        /* Send a data from I2S1  */
//        //SPI_I2S_SendData(SPI3, I2S3_Buffer_Tx[TxIdx++]);
//    }
//
//    /* Check the end of buffer transfer */
////    if (RxIdx == 32)
////    {
////        /* Disable the I2S3 TXE interrupt to end the communication */
////        SPI_I2S_ITConfig(SPI1, SPI_I2S_IT_TXE, DISABLE);
////    }
//}

void vSpiReadWriteBufDma(uint8_t *src, uint8_t *dst, uint32_t len)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_InitTypeDef DMA_InitStructure;
	DMA_DeInit(DMA1_Channel2);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &SPI1->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) dst;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = len;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel2, &DMA_InitStructure);

	DMA_DeInit(DMA1_Channel3);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &SPI1->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) src;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);

	/* Enable SPI_MASTER DMA Tx request */
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
	/* Enable SPI_MASTER DMA Rx request */
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);

	DMA_Cmd(DMA1_Channel2, ENABLE);
	DMA_Cmd(DMA1_Channel3, ENABLE);

	while (!DMA_GetFlagStatus(DMA1_FLAG_TC2))
		;
	while (!DMA_GetFlagStatus(DMA1_FLAG_TC3))
		;

	DMA_Cmd(DMA1_Channel2, DISABLE);
	DMA_Cmd(DMA1_Channel3, DISABLE);
}

uint8_t uSpiReadWriteByte(uint8_t b)
{
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
		;

	/* Send byte through the SPI1 peripheral */
	SPI_I2S_SendData(SPI1, b);

	/* Wait to receive a byte */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)
		;

	/* Return the byte read from the SPI bus */
	return SPI_I2S_ReceiveData(SPI1);
}

void uSpiReadWriteBuf(uint8_t *src, uint8_t *dst, uint32_t len)
{
	uint8_t i;

	for(i=0; i<len; i++)
		dst[i] = uSpiReadWriteByte(src[i]);
}

void uSpiWriteBuf(uint8_t *buf, uint32_t len)
{
	uint8_t i;

	for(i=0; i<len; i++)
		uSpiReadWriteByte(*buf++);
}
