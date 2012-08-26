/**************************************************************************//**
 * @file     spi2.c
 * @brief    STM32 SPI2 generic func source file
 * @version  v1.0
 * @date     27.08.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 ******************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/

#include "inttypes.h"
#include "spi2.h"
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_dma.h"
#include "misc.h"

/******************************************************************************
 * Function definitions
 *****************************************************************************/

void vSpi2Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* Configure SPI2 pins: SCK, MISO and MOSI -------------------------------*/
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    SPI_I2S_DeInit(SPI2);
    SPI_Cmd(SPI2, DISABLE);

    /* SPI2 Configuration ----------------------------------------------------*/
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStructure);


    SPI_SSOutputCmd(SPI2, ENABLE);
    /* Enable SPI2 */
    SPI_Cmd(SPI2, ENABLE);
}

uint8_t uSpi2ReadWriteByte(uint8_t b)
{
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
		;

	/* Send byte through the SPI2 peripheral */
	SPI_I2S_SendData(SPI2, b);

	/* Wait to receive a byte */
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET)
		;

	/* Return the byte read from the SPI2 bus */
	return SPI_I2S_ReceiveData(SPI2);
}

void uSpi2ReadWriteBuf(uint8_t *src, uint8_t *dst, uint32_t len)
{
	uint8_t i;

	for(i=0; i<len; i++)
		dst[i] = uSpi2ReadWriteByte(src[i]);
}

void uSpi2WriteBuf(uint8_t *buf, uint32_t len)
{
	uint8_t i;

	for(i=0; i<len; i++)
		uSpi2ReadWriteByte(*buf++);
}
