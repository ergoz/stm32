/**************************************************************************//**
 * @file     mpl115a1.c
 * @brief    STM32 MPL115A1 func definitions source file
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

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "inttypes.h"
#include "interrupt.h"
#include "spi2.h"
#include "timer.h"

// MPL115A1 register address map
#define PRESH   0x00    // 80
#define PRESL   0x02    // 82
#define TEMPH   0x04    // 84
#define TEMPL   0x06    // 86

#define A0MSB   0x08    // 88
#define A0LSB   0x0A    // 8A
#define B1MSB   0x0C    // 8C
#define B1LSB   0x0E    // 8E
#define B2MSB   0x10    // 90
#define B2LSB   0x12    // 92
#define C12MSB  0x14    // 94
#define C12LSB  0x16    // 96

//SPI Chip Select
#define SPI2_H()	GPIO_SetBits(GPIOB, GPIO_Pin_12)
#define SPI2_L()	GPIO_ResetBits(GPIOB, GPIO_Pin_12)

/******************************************************************************
 * Function declarations
 *****************************************************************************/

/******************************************************************************
 * Global variables
 *****************************************************************************/
static int32_t sia0, sib1, sib2, sic12;

/******************************************************************************
 * Function definitions
 *****************************************************************************/

uint32_t uMplCalcPressure(uint32_t uiPadc, uint32_t uiTadc)
{
    // See Freescale document AN3785 for detailed explanation
    // of this implementation.

    int32_t siPcomp;
//    float decPcomp;
    int32_t lt1, lt2, lt3;
    int32_t si_a1, si_c12x2, si_a1x1, si_y1, si_a2x2;

    // Coefficient 9 equation compensation
    uiPadc = uiPadc >> 6;
    uiTadc = uiTadc >> 6;

    // Step 1 c12x2 = c12 * Tadc
    lt1 = (signed long) sic12;
    lt2 = (signed long) uiTadc;
    lt3 = lt1*lt2;
    si_c12x2 = (signed long)lt3;

    // Step 2 a1 = b1 + c12x2
    lt1 = ((signed long)sib1<<11);
    lt2 = (signed long)si_c12x2;
    lt3 = lt1 + lt2;
    si_a1 = (signed long) lt3>>11;

    // Step 3 a1x1 = a1 * Padc
    lt1 = (signed long)si_a1;
    lt2 = (signed long)uiPadc;
    lt3 = lt1*lt2;
    si_a1x1 = (signed long)(lt3);

    // Step 4 y1 = a0 + a1x1
    lt1 = ((signed long)sia0<<10);
    lt2 = (signed long)si_a1x1;
    lt3 = lt1+lt2;
    si_y1 = ((signed long)lt3>>10);

    // Step 5 a2x2 = b2 * Tadc
    lt1 = (signed long)sib2;
    lt2 = (signed long)uiTadc;
    lt3 = lt1*lt2;
    si_a2x2 = (signed long)(lt3);

    // Step 6 pComp = y1 + a2x2
    lt1 = ((signed long)si_y1<<11);
    lt2 = (signed long)si_a2x2;
    lt3 = lt1+lt2;

    // Fixed point result with rounding
    //siPcomp = ((signed int)lt3>>13);
    siPcomp = lt3>>14;

    // decPcomp is defined as a floating point number
    // Conversion to decimal value from 1023 ADC count value
    // ADC counts are 0 to 1023, pressure is 50 to 115kPa respectively

//    decPcomp = ((65.0/1023.0)*siPcomp)+50;
//	printf("%f\n", decPcomp);
//    return decPcomp;
//    return (((65.0/1023.0)*siPcomp)+50) * (7.50061683);

    return (325*siPcomp+255750) / 682 ;
}

void vMplWriteReg(uint8_t reg)
{
	SPI2_L();
	uSpi2ReadWriteByte(reg);
	uSpi2ReadWriteByte(0);
	SPI2_H();
}


uint8_t uMplReadReg(uint8_t reg)
{
	unsigned char reg_val;
	SPI2_L();
	uSpi2ReadWriteByte(0x80 | reg);
	reg_val = uSpi2ReadWriteByte(0);
	SPI2_H();
	return(reg_val);
}

void vMpl115a1Init(void)
{
	int8_t b;

	vSpi2Init();

	b = uMplReadReg(A0MSB);
	sia0 = (signed int) b << 8;
	b = uMplReadReg(A0LSB);
    sia0 += (signed int) b & 0x00FF;

    b = uMplReadReg(B1MSB);
	sib1 = (signed int) b << 8;
	b = uMplReadReg(B1LSB);
    sib1 += (signed int) b & 0x00FF;

    b = uMplReadReg(B2MSB);
	sib2 = (signed int) b << 8;
	b = uMplReadReg(B2LSB);
    sib2 += (signed int) b & 0x00FF;

    b = uMplReadReg(C12MSB);
	sic12 = (signed int) b << 8;
	b = uMplReadReg(C12LSB);
    sic12 += (signed int) b & 0x00FF;
}

uint32_t uMpl115a1ReadPressure(void)
{
	uint8_t b;
	uint32_t uiPadc, uiTadc;

	vMplWriteReg(0x24);	// start measuring
	vTimerDelayMs(3);	// 3 ms

	b = uMplReadReg(PRESH);
	uiPadc = (uint32_t)b << 8;
	b = uMplReadReg(PRESL);
	uiPadc += (uint32_t)b & 0x00FF;

	b = uMplReadReg(TEMPH);
	uiTadc = (uint32_t)b << 8;
	b = uMplReadReg(TEMPL);
	uiTadc += (uint32_t)b & 0x00FF;

	return uMplCalcPressure(uiPadc, uiTadc);
}
