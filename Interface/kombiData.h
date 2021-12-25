// ==================================== [kombiData.h] =============================
/*
*	This include file declares the needed data structures for the kombiinstrument
*	project as well as constants for transmitting status messages between the controller
*	and the computer.
*
*	For further information, read the "readme.txt" of the kombiinstrument project.
*
*	Author: Tobias Brächter
*	Last update: 2019-10-03
*
*/

#ifndef _KOMBIDATA_H_
#define _KOMBIDATA_H_

#define STATUS_OK '0'
#define STATUS_UNKNOWN '1'
#define STATUS_INVALID '2'

#define SEND_STATUS_OK "s0e"
#define SEND_STATUS_UNKNOWN "s1e"
#define SEND_STATUS_INVALID "s2e"

typedef struct
{
	uint16_t rpm;
	uint8_t dutyRed;
	uint8_t dutyGre;
	uint8_t dutyBlu;
	uint8_t dummy; // needed to avoid padding
}breakpoint;

#define PH_RISE 0
#define PH_HIGH 1
#define PH_FALL 2
#define PH_LOW 3

typedef struct
{
	uint16_t rpmLow;
	uint16_t rpmHigh;
	uint16_t tRise;
	uint16_t tHigh;
	uint16_t tFall;
	uint16_t tLow;
}dimmer;

#define NUM_BREAK 10
#define NUM_DIM 5

typedef struct
{
	breakpoint breakpoints[NUM_BREAK];
	dimmer dimmers[NUM_DIM];
	uint16_t rpmStarterOn;
	uint16_t rpmStarterOff;
	uint8_t breakHyst;
	uint8_t dimHyst;
	uint8_t dimActive;
	uint8_t dimEnabled;
	uint8_t breakActive;
	uint8_t filter;
}kombiData;

#endif
