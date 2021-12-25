// ==================================== [bitOperation.h] =============================
/*
*	This library is designed to manipulate the single bits in a register on a
*	microcontroller.
*
*	Author: Tobias Brächter
*	Last update: 2019-05-13
*
*/

#ifndef _BIT_OPERATION_H_
#define _BIT_OPERATION_H_

#include <stdint.h>

// sets the given bit in the given register to the given value
void setBit(volatile uint8_t *reg, uint8_t bit, uint8_t value);

// toggles the given bit in the given register
void toggleBit(volatile uint8_t *reg, uint8_t bit);

// returns the value of the given bit in the given register
uint8_t readBit(volatile uint8_t *reg, uint8_t bit);

#endif