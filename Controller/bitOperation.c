// ==================================== [bitOperation.c] =============================
/*
*	This library is designed to manipulate the single bits in a register on a
*	microcontroller.
*
*	Author: Tobias Brächter
*	Last update: 2019-05-13
*
*/

#include "bitOperation.h"

void setBit(volatile uint8_t *reg, uint8_t bit, uint8_t value)
{
	if(value == 0)
		*reg &= ~(1 << bit);
	else if(value == 1)
		*reg |= (1 << bit);
}

void toggleBit(volatile uint8_t *reg, uint8_t bit)
{
	setBit(reg, bit, !readBit(reg, bit));
}

uint8_t readBit(volatile uint8_t *reg, uint8_t bit)
{
	return (*reg >> bit) & 1;	
}
