/******************************************************************************
 * @file     packet.h
 * @brief    Communication packet definitions header file
 * @version  v1.0
 * @date     30.07.2012
 *
 * @note
 * Copyright (C) 2012 kab
 *
 *****************************************************************************/

#ifndef PACKET_H_
#define PACKET_H_

/******************************************************************************
 * Includes
 *****************************************************************************/

#include <inttypes.h>

/******************************************************************************
 * Type definitions
 *****************************************************************************/

#define PACKET_ASK		0x00
#define PACKET_REPLY	0x80

#define IS_PACKET_ASK(x)	((x&PACKET_REPLY)!=PACKET_REPLY)
#define IS_PACKET_REPLY(x)	((x&PACKET_REPLY)==PACKET_REPLY)

#define PACKET_PING		0x00
#define PACKET_DHT22	0x01
#define PACKET_AIR		0x02
#define PACKET_CMD_MASK	0x7F

#define GET_PACKET_CMD(x) 	(x&PACKET_CMD_MASK)

typedef union
{
	uint8_t		status;		// status to ASK command, 1 - success, 0 - failure
	uint32_t	dht22[2];	// temp + humidity from dht22
	uint32_t	pressure;	// pressure from Freescale
	uint32_t	temp;		// temperature from max6635
	uint8_t		ir[7];		// Samsung air conditioneer command
} packet_data;

typedef struct
{
	uint8_t		cmd;
	packet_data	data;
} Packet_t, *pPacket_t;

/******************************************************************************
 * Function definitions
 *****************************************************************************/

#endif /* PACKET_H_ */
