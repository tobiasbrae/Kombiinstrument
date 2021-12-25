// ==================================== [charBuffer.c] =============================
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

#include "charBuffer.h"

void cb_initBuffer(cb_charBuffer *this, uint8_t *buffer, uint8_t size)
{
	this->buffer = buffer;
	this->size = size;
	this->read = 0;
	this->write = 0;
	this->stored = 0;
}

void cb_clearBuffer(cb_charBuffer *this)
{
	this->read = 0;
	this->write = 0;
	this->stored = 0;
}

uint8_t cb_hasNext(cb_charBuffer *this)
{
	return this->stored;
}

void cb_put(cb_charBuffer *this, uint8_t value)
{
	uint8_t next = (this->write+1)%(this->size);
	if(next != this->read)
	{
		this->buffer[this->write] = value;
		this->write = next;
		this->stored = this->stored+1;
	}
}

void cb_putN(cb_charBuffer *this, uint8_t *values, uint8_t amount)
{
	for(uint8_t i = 0; i < amount; i++)
		cb_put(this, values[i]);
}

void cb_putString(cb_charBuffer *this, uint8_t *values)
{
	uint8_t i = 0;
	while(values[i])
		cb_put(this, values[i++]);
}

uint8_t cb_getNext(cb_charBuffer *this)
{
	if(this->stored)
		return this->buffer[this->read];
	return 0;
}

uint8_t cb_getNextOff(cb_charBuffer *this, uint8_t offset)
{
	if(offset >= this->stored)
		return 0;
	uint8_t next = (this->read+offset)%(this->size);
	return this->buffer[next];
}

void cb_getNextN(cb_charBuffer *this, uint8_t *values, uint8_t amount)
{
	for(uint8_t i = 0; i < amount; i++)
	{
		uint8_t next = (this->read+i)%(this->size);
		if(i < this->stored)
			values[i] = this->buffer[next];
		else
			values[i] = 0;
	}
}

void cb_delete(cb_charBuffer *this)
{
	if(this->stored)
	{
		this->read = (this->read+1)%(this->size);
		this->stored = this->stored-1;
	}
}

void cb_deleteN(cb_charBuffer *this, uint8_t amount)
{
	for(uint8_t i = 0; i < amount; i++)
		cb_delete(this);
}
