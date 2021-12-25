// ==================================== [serialCommunication_windows.c] =============================
/*
*	This library offers some functions to communicate with a device via a serial interface.
*
*	Usage:
*	At first, you have to declare the interface this library should use by calling the function
*	"se_setPort(...)", as well as calling the function "se_init()".
*	After that, you can repeatedly open a port, send and receive data and close the port again.
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
*	Last update: 2019-05-17
*
*/

#include <stdio.h>

#define COLUMNS 8 // divide portlisting output into columns

int se_portOpen; 

int se_listPorts(void)
{
	printf("Fehler! Funktion derzeit nicht implementiert.\n");
	return 0;
}

int se_isPortOpen(void)
{
	return se_portOpen;
}

int se_openPort(char *port)
{
	printf("Fehler! Funktion derzeit nicht implementiert.\n");
	return 0;
}

int se_closePort(void)
{
	printf("Fehler! Funktion derzeit nicht implementiert.\n");
	return 0;
}

int se_put(char value)
{
	printf("Fehler! Funktion derzeit nicht implementiert.\n");
	return 0;
}

int se_putN(char *values, int amount)
{
	printf("Fehler! Funktion derzeit nicht implementiert.\n");
	return 0;
	int returnValue = 0;
	for(int i=0; i < amount; i++)
		returnValue = se_put(values[i]);
	return returnValue;
}

int se_get(char *value)
{
	printf("Fehler! Funktion derzeit nicht implementiert.\n");
	return 0;
}

int se_getN(char *values, int amount)
{
	printf("Fehler! Funktion derzeit nicht implementiert.\n");
	return 0;
	for(int i=0; i < amount; i++)
	{
		if(!se_get(values+i))
			return 0;
	}
	return 1;
}
