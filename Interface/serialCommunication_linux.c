// ==================================== [serialCommunication_linux.c] =============================
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
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

#define COLUMNS 8 // divide portlisting output into columns
#define PORTBUFFER 100 // to store the port

struct termios se_tio, se_tioOld;
int se_tty_fd;
int se_flags;
int se_portOpen; 

int se_listPorts(void)
{
	DIR *pDir;
	struct dirent *pFile;
	pDir = opendir("/dev/");
	if(pDir == NULL)
		printf("Fehler! Konnte Verzeichnis nicht lesen.\n");
	else
	{
		int index = 0;
		while((pFile = readdir(pDir)))
		{
			if(!strncmp(pFile->d_name, "tty", 3))
			{
				printf("/dev/%s\t",pFile->d_name);
				index++;
				if(index && !(index%COLUMNS))
					printf("\n");
			}
		}
		if(index && (index%COLUMNS))
			printf("\n");
		return 1;
	}
	return 0;
}

int se_isPortOpen(void)
{
	return se_portOpen;
}

int se_openPort(char *port)
{
	if(!port[0])
	{
		printf("Fehler! Kein Port angegeben.\n");
		return 0;
	}
	
	// try to open port
	se_tty_fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
	if(se_tty_fd > 0)
	{
		tcgetattr(se_tty_fd, &se_tioOld);
		tcgetattr(se_tty_fd, &se_tio);
		se_tio.c_iflag = 0;
		se_tio.c_oflag = 0;
		se_tio.c_cflag = (CS8 | CREAD | CLOCAL);
		se_tio.c_lflag = 0;
		se_tio.c_cc[VMIN] = 1;
		se_tio.c_cc[VTIME] = 1;
		cfsetospeed(&se_tio, B19200);
		cfsetispeed(&se_tio, B19200);
		tcsetattr(se_tty_fd,TCSANOW,&se_tio);
		se_portOpen = 1;
		return 1;
	}
	printf("Fehler! Port konnte nicht reserviert werden.\n");
	return 0;
}

int se_closePort(void)
{
	if(se_isPortOpen())
	{
		tcsetattr(se_tty_fd, TCSANOW, &se_tioOld);
		close(se_tty_fd);
		se_tty_fd = 0;
		se_portOpen = 0;
		return 1;
	}
	printf("Fehler! Es ist kein Port reserviert.\n");
	return 0;
}

int se_put(char value)
{
	if(se_isPortOpen())
	{
		char cache = value;
		write(se_tty_fd, &cache, 1);
		return 1;
	}
	return 0;
}

int se_putN(char *values, int amount)
{
	int returnValue = 0;
	for(int i=0; i < amount; i++)
		returnValue = se_put(values[i]);
	return returnValue;
}

int se_get(char *value)
{
	if(se_isPortOpen())
	{
		char cache;
		if(read(se_tty_fd, &cache, 1) > 0)
		{
			*value = cache;
			return 1;
		}
	}
	return 0;
}

int se_getN(char *values, int amount)
{
	for(int i=0; i < amount; i++)
	{
		if(!se_get(values+i))
			return 0;
	}
	return 1;
}
