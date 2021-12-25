// ==================================== [charBuffer.h] =============================
/*
*	This library implements a lightweight circular buffer for chars. The main usage is
*	to handle the chars, which have to be transmitted or were received on a
*	microcontroller via UART.
*
*	Usage:
*	To use the char-buffer, you have to declare a variable that stores the struct
*	"cb_charBuffer" as well as a char-array, which will contain the chars. After
*	calling "cb_initBuffer(...)" with the right parameters, you can use the other
*	functions.
*
*	CAUTION: There is no error handling for errors caused through missing cb_charBuffer
*				or buffer-array. You have to make sure that the buffer is initialized
*				correctly.
*
*	Author: Tobias Brächter
*	Last update: 2019-05-13
*
*/

#ifndef _CHAR_BUFFER_H_
#define _CHAR_BUFFER_H_

#include <stdint.h>

// Struct to store the needed data for the buffer
typedef struct
{
	uint8_t *buffer;
	uint8_t size;
	uint8_t read;
	uint8_t write;
	uint8_t stored;
}
cb_charBuffer;

// Used to initialize the buffer. This function must be called before any other function.
//	The struct cb_charBuffer und the buffer-array have to be declared by the user.
void cb_initBuffer(cb_charBuffer *this, uint8_t *buffer, uint8_t size);

// Deletes all data stored in the buffer.
void cb_clearBuffer(cb_charBuffer *this);

// Returns the amount of chars stored in the buffer.
uint8_t cb_hasNext(cb_charBuffer *this);

// Inserts a char into the buffer.
//	If the buffer is full, the action will be ignored.
void cb_put(cb_charBuffer *this, uint8_t value);

// Inserts <amount> chars into the buffer from the given buffer.
// Chars are only inserted, while the buffer is full. All remaining
// chars will be ignored.
void cb_putN(cb_charBuffer *this, uint8_t *values, uint8_t amount);

// Inserts a string from the given buffer into the buffer.
// The chars are inserted until the zero-terminator is found or the
//	buffer is full.
void cb_putString(cb_charBuffer *this, uint8_t *values);

// Returns the next available char. The char will remain in the buffer.
// If there is no char, the function will return zero.
uint8_t cb_getNext(cb_charBuffer *this);

// Returns the next available char with <offset>. The char will remain in the buffer.
// If there is no char, the function will return zero.
uint8_t cb_getNextOff(cb_charBuffer *this, uint8_t offset);

// Copies the next <amount> chars from the buffer to the given buffer.
// If there aren't enough chars in the buffer, the given buffer will be
//	filled up with zeros.
void cb_getNextN(cb_charBuffer *this, uint8_t *values, uint8_t amount);

// Deletes the next available char in the buffer.
// If there is no next char, the action will be ignored.
void cb_delete(cb_charBuffer *this);

// Deletes the next <amount> chars in the buffer. If there aren't as mouch as <amount> chars,
//	the buffer will be cleared.
void cb_deleteN(cb_charBuffer *this, uint8_t amount);

#endif
