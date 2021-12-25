// ==================================== [serialCommunication.h] =============================
/*
*	This library offers some functions to communicate with a device via a serial interface.
*
*	Usage:
*	At first, you have to open a port with the function "openPort(...)".
*	After that, you can send and receive data with the different functions provided.
*	If you have finished all your communication, you can close the port again by calling "closePort()".
*
*	HINT (Linux):
*	On linux, you have to have the correct rights to use the serial interfaces. For example, you
*	can obtain them by starting the program as root, or adding the user, which starts the program,
*	to the group "dialout".
*
*	CAUTION:
*	Opening a port will set the stream-readings to non-blocking. This means, that functions reading
*	data from streams will not wait until a char is received but rather just return zero. You should
*	design you program to properly handle this behaviour, if you plan on leaving ports open.
*
*	Author: Tobias Brächter
*	Last update: 2019-05-14
*
*/

#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

// All functions return 1 on success, zero on failure.
// Error messages will be printed directly via printf(...).

// Lists the available serial ports.
// On Linux, this command will list all devices from /dev/ beginning with "tty".
int se_listPorts(void);

// returns wether a port is opened or not
int se_isPortOpen(void);

// Opens the communication port.
int se_openPort(char *port);

// Closes the communication port.
int se_closePort(void);

// Sends one char to the opened port.
int se_put(char value);

// Sends <amount> chars from the given buffer to the opened port.
int se_putN(char *values, int amount);

// Reads one char from the opened port.
int se_get(char *value);

// Reads <amount> chars from the opened port and copies them to the given buffer.
int se_getN(char *values, int amount);

#endif
