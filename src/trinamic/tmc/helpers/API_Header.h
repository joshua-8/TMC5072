/*******************************************************************************
* Copyright © 2016 TRINAMIC Motion Control GmbH & Co. KG
* (now owned by Analog Devices Inc.),
*
* Copyright © 2023 Analog Devices Inc. All Rights Reserved. This software is
* proprietary & confidential to Analog Devices, Inc. and its licensors.
*******************************************************************************/


#ifndef API_HEADER_H
#define API_HEADER_H

#include "Config.h"
#include "Macros.h"
#include "Constants.h"
#include "Bits.h"
#include "CRC.h"
#include "RegisterAccess.h"
// #include <stdlib.h> //edited
#include "Types.h"

// TODO: Restructure these.
/*
 * Goal: Just give these values here as status back to the IDE when used with EvalSystem.
 * Currently, this is obtained by just leaving out implementation specific error bits here.
 */
typedef enum {
	TMC_ERROR_NONE      = 0x00,
	TMC_ERROR_GENERIC   = 0x01,
	TMC_ERROR_FUNCTION  = 0x02,
	TMC_ERROR_MOTOR     = 0x08,
	TMC_ERROR_VALUE     = 0x10,
	TMC_ERROR_CHIP      = 0x40
} TMCError;

typedef enum {
	TMC_COMM_DEFAULT,
	TMC_COMM_SPI,
	TMC_COMM_UART
} TMC_Comm_Mode;

#endif // API_HEADER_H
