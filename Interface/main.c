// ==================================== [main.c (kombiinstrument Interface)] =============================
/*
*	This program is used to program the controller of the kombiinstrument project.
*
*	For further information, read the "readme.txt" of the kombiinstrument project.
*
*	Author: Tobias Brächter
*	Last update: 2019-10-06
*
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "serialCommunication.h"
#include "kombiData.h"

#define INPUT_BUFFER 150
#define COM_BUFFER 30

#define SERIAL_READ_TIMEOUT 2

int loadingScript;
int exitProgram;

void handleInput(void);
void resetData(void);

// data handling functions
void cm_loadFile(void); // load kombiData from file
void cm_saveFile(void); // save kombiData to file
void cm_loadData(void); // load kombiData to the controller
void cm_getData(void); // load kombiData from the controller
void cm_saveData(void); // save kombiData in the controller permanent
int cm_readAnswer(char *buffer, int length); // try to read answer with given length
int cm_readStatus(char *buffer); // try to read status from the controller
void cm_loadScript(void); // load a command script
void cm_breakpoint(void); // edit a breakpoint
void cm_listBreakpoints(void); // list all breakpoints
void cm_dimmer(void); // edit a dimmer
void cm_listDimmer(void); // list all dimmers
void cm_hysteresis(void);// edit hysteresis parameters
void cm_listHysteresis(void); // list hysteresis parameters
void cm_starter(void); // edit the starter parameters
void cm_listStarter(void); // list the starter parameters
void cm_filter(void); // edit the filter parameter
void cm_listFilter(void); // list the filter parameter
void cm_plot(void); // plot the kombiData

//variables
char inputBuffer[INPUT_BUFFER]; // buffer for reading complete line
char command[COM_BUFFER]; // buffer for command line command

kombiData kdActive, kdCache;
char *pkdActive, *pkdCache;

int main(void)
{
	resetData();
	pkdActive = (char *) &kdActive;
	pkdCache = (char *) &kdCache;

	int ioIndex = 0;

	printf("Willkommen im Kombiinstrument-Interface. Um Hilfe zu erhalten, gib \"help\" ein.\n");
	while(!exitProgram)
	{
		if(scanf("%c", inputBuffer + ioIndex) == 1)
		{
			ioIndex++;
			if(inputBuffer[ioIndex-1] == '\n' || ioIndex >= INPUT_BUFFER)
			{
				if(inputBuffer[ioIndex-1] == '\n')
					inputBuffer[ioIndex-1] = 0;
				handleInput();
				ioIndex = 0;
			}
		}
	}
	if(se_isPortOpen())
	{
		printf("Reservierter Port wird freigegeben...\n");
		se_closePort();
	}
	printf("Programm wird beendet...\n");
	return 0;
}

void handleInput(void)
{
	// read first word of input-string end check vor valid command...
	sscanf(inputBuffer, "%s", command);
	if(!strcmp(command, "help"))
	{
		printf("Folgende Befehle sind vorhanden:\n");
		printf("-> help - Listet die vorhandenen Befehle auf.\n");
		printf("-> exit - Beendet das Programm.\n");
		printf("-> listports - Listet die im System vorhandenen seriellen Schnittstellen auf.\n");
		printf("-> openport <PORT> - Reserviert <PORT> als Kommunikationsport.\n");
		printf("-> closeport - Gibt den reservierten Port wieder frei.\n");
		printf("-> loadfile <filename> - Importiert die Daten aus der angegebenen Datei.\n");
		printf("-> savefile <filename> - Exportiert die Daten in die angegebene Datei.\n");
		printf("-> loaddata - Exportiert die aktuellen Daten in das Kombiinstrument.\n");
		printf("-> getdata - Importiert die aktuellen Daten aus dem Kombiinstrument.\n");
		printf("-> savedata - Speichert die aktuellen Daten im Kombiinstrument dauerhaft.\n");
		printf("-> loadscript <filename> - Importiert ein Befehls-Skript.\n");
		printf("-> breakpoint <ID> <rpm> <red> <green> <blue> - Manipuliert die entsprechenden Daten.\n");
		printf("-> listbreakpoints - Listet die Daten der breakpoints auf.\n");
		printf("-> dimmer <ID> <rpmLow> <rpmHigh> <tRise> <tHigh> <tFall> <tLow> - Manipuliert die entsprechenden Daten.\n");
		printf("-> listdimmer - Listet die Daten der dimmer auf.\n");
		printf("-> hysteresis <breakHyst> <dimHyst> - Stellt die Hysterese-Parameter ein.\n");
		printf("-> listhysteresis - Listet die Hysterese-Parameter auf.\n");
		printf("-> starter <rpmOn> <rpmOff> - Stellt die Grenzen der Starterfreigabe ein.\n");
		printf("-> liststarter - Listet die Daten des Starters auf.\n");
		printf("-> filter <time> - Stellt die Drehzahlwechselrate ein (1/100us).\n");
		printf("-> listfilter - Listet die Daten des Filters auf.\n");
		printf("-> clearall - Setzt alle in kombiData gespeicherten Parameter auf null.\n");
		printf("-> listall - Listet alle in kombiData gespeicherten Parameter auf.\n");
		printf("-> plot - Stellt den Farbverlauf entsprechend der aktuellen Daten in einer Grafik dar.\n");
		printf("-> Um genauere Anweisungen zur Verwendung des Programms zu erhalten, siehe in der readme.txt nach.\n");
	}
	else if(!strcmp(command, "exit"))
	{
		exitProgram = 1;
	}
	else if(!strcmp(command, "listports"))
	{
		se_listPorts();
	}
	else if(!strcmp(command, "openport"))
	{
		if(se_isPortOpen())
			printf("Fehler! Es ist bereits ein Port reserviert.\n");
		else if(sscanf(inputBuffer, "openport %s", command) == 1)
		{
			if(se_openPort(command))
				printf("Der Port \"%s\" wurde erfolgreich reserviert.\n", command);
		}
		else
			printf("Fehler! Leerer Port nicht zugelassen.\n");
	}
	else if(!strcmp(command, "closeport"))
	{
		if(se_closePort())
			printf("Port wurde erfolgreich freigegeben.\n");
	}
	else if(!strcmp(command, "loadfile"))
		cm_loadFile();
	else if(!strcmp(command, "savefile"))
		cm_saveFile();
	else if(!strcmp(command, "loaddata"))
		cm_loadData();
	else if(!strcmp(command, "getdata"))
		cm_getData();
	else if(!strcmp(command, "savedata"))
		cm_saveData();
	else if(!strcmp(command, "loadscript"))
		if(!loadingScript) // prevent recursive script-calling
			cm_loadScript();
		else
			printf("Netter Versuch ;-)\n");
	else if(!strcmp(command, "breakpoint"))
		cm_breakpoint();
	else if(!strcmp(command, "listbreakpoints"))
		cm_listBreakpoints();
	else if(!strcmp(command, "dimmer"))
		cm_dimmer();
	else if(!strcmp(command, "listdimmer"))
		cm_listDimmer();
	else if(!strcmp(command, "hysteresis"))
		cm_hysteresis();
	else if(!strcmp(command, "listhysteresis"))
		cm_listHysteresis();
	else if(!strcmp(command, "starter"))
		cm_starter();
	else if(!strcmp(command, "liststarter"))
		cm_listStarter();
	else if(!strcmp(command, "filter"))
		cm_filter();
	else if(!strcmp(command, "listfilter"))
		cm_listFilter();
	else if(!strcmp(command, "clearall"))
	{
		printf("Setze alle Werte auf null...");
		for(int i=0; i < sizeof(kombiData); i++)
			pkdActive[i] = 0;
		printf("fertig.\n");
	}
	else if(!strcmp(command, "listall"))
	{
		cm_listBreakpoints();
		cm_listDimmer();
		cm_listHysteresis();
		cm_listStarter();
		cm_listFilter();
	}
	else if(!strcmp(command, "plot"))
		cm_plot();
	else if(!command[0])
		printf("Fehler! Leere Befehle sind nicht zugelassen.\n");
	else
	{
		printf("Unbekannter Befehl! Um Hilfe zu erhalten, gib \"help\" ein.\n");
	}
	for(int i=0; i < INPUT_BUFFER; i++)
		inputBuffer[i] = 0;
}

void resetData(void)
{
	kdActive.breakActive = 0;
	kdActive.dimActive = 0;
	kdActive.dimEnabled = 1;
}

void cm_loadFile(void)
{
	char pathBuffer[INPUT_BUFFER];
	for(int i=9; i<INPUT_BUFFER; i++) // copy input-buffer without the command
		pathBuffer[i-9] = inputBuffer[i];
	if(!pathBuffer[0])
	{
		printf("Fehler! Du musst einen Dateinamen angeben!\n");
	}
	else
	{
		FILE *inputFile = fopen(pathBuffer, "rb");
		if(inputFile > 0) // check for file
		{
			int tooShort = 0; // is file smaller than kombiData?
			for(int i=0; i < sizeof(kombiData); i++) // read data
			{
				if(fscanf(inputFile, "%c", pkdCache+i) == EOF)
				{
					tooShort = 1;
					break;
				}
			}
			if(tooShort)
				printf("Fehler! Falsches Dateiformat.\n");
			else
			{
				char checkEnd, cache;
				checkEnd = fscanf(inputFile, "%c", &cache); // only accept files, which are exactly as long as kombiData
				if(checkEnd == EOF)
				{
					for(int i=0; i < sizeof(kombiData); i++) // read data is valid, copy it to active data
						pkdActive[i] = pkdCache[i];
					resetData();
					printf("Daten erfolgreich geladen.\n");
				}
				else
					printf("Fehler! Falsches Dateiformat.\n");
			}
			fclose(inputFile);
		}
		else
			printf("Fehler! Datei wurde nicht gefunden!\n");
	}
}

void cm_saveFile(void)
{
	char pathBuffer[INPUT_BUFFER];
	for(int i=9; i<INPUT_BUFFER; i++) // copy input-buffer without the command
		pathBuffer[i-9] = inputBuffer[i];
	if(!pathBuffer[0])
	{
		printf("Fehler! Du musst einen Dateinamen angeben!\n");
	}
	else
	{
		int writeFile = 0;
		FILE *outputFile = fopen(pathBuffer, "r"); // first, check if a file for the given path already exists
		if(outputFile > 0)
		{
			fclose(outputFile);
			printf("Die Datei ist bereits vorhanden! Ersetzen? (y/n)");
			char answer;
			while(scanf("%c", &answer) != 1);
			if(answer == 'y') // everything other than y is handled as a no...
				writeFile = 1;
			while(scanf("%c", &answer) != 1 && answer != '\n'); // clear current line from input stream
		}
		else
			writeFile = 1;

		if(writeFile)
		{
			outputFile = fopen(pathBuffer, "w+");
			if(outputFile > 0) // user could have no write permission...
			{
				for(int i=0; i < sizeof(kombiData); i++)
					fprintf(outputFile, "%c", pkdActive[i]);
				fclose(outputFile);
				printf("Erfolgreich gespeichert!\n");
			}
			else
				printf("Fehler! Schreiben der Datei fehlgeschlagen.\n");
		}
		else
			printf("Der Speichervorgang wurde abgebrochen.\n");
	}
}

void cm_loadData(void)
{
	if(se_isPortOpen())
	{
		char cacheBuffer[INPUT_BUFFER];
		se_put('l');
		for(int i=0; i < sizeof(kombiData); i++)
			se_put(pkdActive[i]);
		se_put('e');
		if(cm_readStatus(cacheBuffer))
		{
			printf("Daten erfolgreich vermittelt.\n");
			printf("Lese vermittelte Daten...\n");
			time_t beginning = time(NULL);
			while(difftime(time(NULL), beginning) < SERIAL_READ_TIMEOUT);
			se_putN("ge",2);
			if(cm_readAnswer(cacheBuffer, sizeof(kombiData)+2))
			{
				if(cacheBuffer[0] != 'd')
					printf("Fehler! Kombiinstrument sendete unerwartete Antwort.\n");
				else
				{
					int dataCorrect = 1;
					for(int i=0; i < sizeof(kombiData); i++)
						if(cacheBuffer[i+1] != pkdActive[i])
							dataCorrect = 0;
					if(!dataCorrect)
						printf("Fehler! Gesendete und empfange Daten sind nicht identisch.\n");
					else
					{
						printf("Vermittelte Daten korrekt. Aktiviere Daten...\n");
						se_putN("te",2);
						if(cm_readStatus(cacheBuffer))
							printf("Daten erfolgreich aktiviert.\n");
						else
							printf("Daten konnten nicht aktiviert werden.\n");
					}
				}
			}
		}
	}
	else
		printf("Fehler! Es ist kein Port reserviert.\n");
}

void cm_getData(void)
{
	if(se_isPortOpen())
	{
		printf("Fordere Daten an...\n");
		char cacheBuffer[INPUT_BUFFER];
		se_putN("ge",2);
		if(cm_readAnswer(cacheBuffer, sizeof(kombiData)+2))
		{
			if(cacheBuffer[0] != 'd')
				printf("Fehler! Kombiinstrument sendete unerwartete Antwort.\n");
			else
			{
				for(int i=0; i < sizeof(kombiData); i++) //sizeof(kombiData); i++)
					pkdActive[i] = cacheBuffer[i+1];
				resetData();
				printf("Daten erfolgreich empfangen.\n");
			}
		}
	}
	else
		printf("Fehler! Es ist kein Port reserviert.\n");
}

void cm_saveData(void)
{
	if(se_isPortOpen())
	{
		printf("Speichere Daten dauerhaft...\n");
		char cacheBuffer[INPUT_BUFFER];
		se_putN("se",2);
		if(cm_readStatus(cacheBuffer))
			printf("Daten erfolgreich gespeichert.\n");
		else
			printf("Daten konnten nicht gespeichert werden.\n");
	}
	else
		printf("Fehler! Es ist kein Port reserviert.\n");
}

int cm_readAnswer(char *buffer, int length)
{
	int index = 0;
	time_t beginning = time(NULL);
	while(difftime(time(NULL), beginning) < SERIAL_READ_TIMEOUT && index < length)
		if(se_get(buffer+index))
			index++;
	buffer[index] = 0;
	if(index == 0)
		printf("Fehler! Das Kombiinstrument antwortet nicht.\n");
	else if(index < length)
		printf("Fehler! Zu wenig Daten empfangen.\n");
	else if(buffer[length-1] != 'e')
		printf("Fehler! Terminator fehlt.\n");
	else
		return 1;
	return 0;
}

int cm_readStatus(char *buffer)
{
	if(cm_readAnswer(buffer, 3))
	{
		if(buffer[0] != 's')
			printf("Fehler! Kombiinstrument sendete unerwartete Antwort.\n");
		else if(buffer[1] == STATUS_UNKNOWN)
			printf("Fehler! Kombiinstrument kennt Befehl nicht.\n");
		else if(buffer[1] == STATUS_INVALID)
			printf("Fehler! Befehl wurde falsch vermittelt.\n");
		else if(buffer[1] != STATUS_OK)
			printf("Fehler! Unbekannter Status.\n");
		else
			return 1;
	}
	return 0;
}

void cm_loadScript(void)
{
	loadingScript = 1;
	char pathBuffer[INPUT_BUFFER];
	for(int i=11; i<INPUT_BUFFER; i++) // copy input-buffer without the command
		pathBuffer[i-11] = inputBuffer[i];
	if(!pathBuffer[0])
	{
		printf("Fehler! Du musst einen Dateinamen angeben!\n");
	}
	else
	{
		FILE *inputFile = fopen(pathBuffer, "rb");
		if(inputFile > 0) // check for file
		{
			//clear input buffer for handling incoming commands
			for(int i=0; i < INPUT_BUFFER; i++)
				inputBuffer[i] = 0;
			int index = 0;
			while(fscanf(inputFile, "%c", inputBuffer+index) != EOF)
			{
				// read scriptfile line-based, prevent buffer-overflow
				if(inputBuffer[index] == '\n' || (index + 1) == INPUT_BUFFER)
				{
					if(inputBuffer[index] == '\n')
						inputBuffer[index] = 0;
					printf("--> %s\n", inputBuffer);
					handleInput();
					index = 0;
				}
				else
					index++;
			}
			// the last line propably isn't finished by '\n'
			if(inputBuffer[0] != 0)
			{
				printf("--> %s\n", inputBuffer);
				handleInput();
			}
			fclose(inputFile);
		}
		else
			printf("Fehler! Datei wurde nicht gefunden!\n");
	}
	loadingScript = 0;
}

void cm_breakpoint(void)
{
	unsigned int id=0, rpm=0, dutyRed=0, dutyGre=0, dutyBlu=0, variables=0;
	variables = sscanf(inputBuffer, "breakpoint %u %u %u %u %u", &id, &rpm, &dutyRed, &dutyGre, &dutyBlu);
	if(variables == 5)
	{
		if(id >= NUM_BREAK)
			printf("Fehler! Nicht erlaubte ID.\n");
		else
		{
			if(rpm > 65535) // catch overflow (rpm is uint16_t on the controller)
				rpm = 65535;
			if(dutyRed > 100) // the duty cycles in the controller are from 0 to 100
				dutyRed = 100;
			if(dutyGre > 100)
				dutyGre = 100;
			if(dutyBlu > 100)
				dutyBlu = 100;

			kdActive.breakpoints[id].rpm = rpm;
			kdActive.breakpoints[id].dutyRed = dutyRed;
			kdActive.breakpoints[id].dutyGre = dutyGre;
			kdActive.breakpoints[id].dutyBlu = dutyBlu;
			printf("Breakpoint mit ID %u erfolgreich angepasst.\n", id);
		}
	}
	else
	{
		printf("Fehler! Nicht zugelassene Eingabe.\n");
		printf("-> Richtige Anwendung: \"breakpoint <ID> <RPM> <RED> <GREEN> <BLUE>\"\n");
		printf("-> Alle Parameter werden als Ganzzahlen vorrausgesetzt.\n");
	}
}

void cm_listBreakpoints(void)
{
	printf("============[breakpoints]========\n");
	printf("<ID> <rpm> <red> <green> <blue>\n");
	for(int i=0; i < NUM_BREAK; i++)
		printf("%3d %6u %5u %5u %5u\n", i, kdActive.breakpoints[i].rpm, kdActive.breakpoints[i].dutyRed, kdActive.breakpoints[i].dutyGre, kdActive.breakpoints[i].dutyBlu);
	printf("---------------------------------\n");
}

void cm_dimmer(void)
{
	unsigned int id=0, rpmLow=0, rpmHigh=0, tRise=0, tHigh=0, tFall=0, tLow=0, variables=0;
	variables = sscanf(inputBuffer, "dimmer %u %u %u %u %u %u %u", &id, &rpmLow, &rpmHigh, &tRise, &tHigh, &tFall, &tLow);
	if(variables == 7)
	{
		if(id >= NUM_DIM)
			printf("Fehler! Nicht erlaubte ID.\n");
		else
		{
			if(rpmLow > 65535) // target variable is uint16_t
				rpmLow = 65535;
			if(rpmHigh > 65535)
				rpmHigh = 65535;
			if(tRise > 65535)
				tRise = 65535;
			if(tHigh > 65535)
				tHigh = 65535;
			if(tFall > 65535)
				tFall = 65535;
			if(tLow > 65535)
				tLow = 65535;

			kdActive.dimmers[id].rpmLow = rpmLow;
			kdActive.dimmers[id].rpmHigh = rpmHigh;
			kdActive.dimmers[id].tRise = tRise;
			kdActive.dimmers[id].tHigh = tHigh;
			kdActive.dimmers[id].tFall = tFall;
			kdActive.dimmers[id].tLow = tLow;

			printf("Dimmer mit ID %u erfolgreich angepasst.\n", id);
		}
	}
	else
	{
		printf("Fehler! Nicht zugelassene Eingabe.\n");
		printf("-> Richtige Anwendung: \"dimmer <ID> <rpmLow> <rpmHigh> <tRise> <tHigh> <tFall> <tLow>\"\n");
		printf("-> Alle Parameter werden als Ganzzahlen vorrausgesetzt.\n");
	}
}

void cm_listDimmer(void)
{
	printf("==========================[dimmer]=====================\n");
	printf("<ID> <rpmLow> <rpmHigh> <tRise> <tHigh> <tFall> <tLow>\n");
	for(int i=0; i < NUM_DIM; i++)
		printf("%3d %7u %9u %8u %7u %7u %6u\n", i, kdActive.dimmers[i].rpmLow, kdActive.dimmers[i].rpmHigh, kdActive.dimmers[i].tRise, kdActive.dimmers[i].tHigh, kdActive.dimmers[i].tFall, kdActive.dimmers[i].tLow);
	printf("-------------------------------------------------------\n");
}

void cm_hysteresis(void)
{
	unsigned int breakHyst=0, dimHyst=0, variables=0;
	variables = sscanf(inputBuffer, "hysteresis %u %u", &breakHyst, &dimHyst);
	if(variables == 2)
	{
		if(breakHyst > 65535) // target variable is uint16_t
			breakHyst = 65535;
		if(dimHyst > 65535)
			dimHyst = 65535;

		kdActive.breakHyst = breakHyst;
		kdActive.dimHyst = dimHyst;
		printf("Hysterese-Parameter erfolgreich angepasst.\n");
	}
	else
	{
		printf("Fehler! Nicht zugelassene Eingabe.\n");
		printf("-> Richtige Anwendung: \"hysteresis <breakHyst> <dimHyst>\"\n");
		printf("-> Alle Parameter werden als Ganzzahlen vorrausgesetzt.\n");
	}
}

void cm_listHysteresis(void)
{
	printf("==========[Hysterese]=======\n");
	printf("breakpoint: %u dimmer: %u\n", kdActive.breakHyst, kdActive.dimHyst);
	printf("----------------------------\n");
}

void cm_starter(void)
{
	unsigned int rpmStarterOn=0, rpmStarterOff=0, variables=0;
	variables = sscanf(inputBuffer, "starter %u %u", &rpmStarterOn, &rpmStarterOff);
	if(variables == 2)
	{
		if(rpmStarterOn > 65535) // target variable is uint16_t
			rpmStarterOn = 65535;
		if(rpmStarterOff > 65535)
			rpmStarterOff = 65535;

		kdActive.rpmStarterOn = rpmStarterOn;
		kdActive.rpmStarterOff = rpmStarterOff;
		printf("Starter-Parameter erfolgreich angepasst.\n");
	}
	else
	{
		printf("Fehler! Nicht zugelassene Eingabe.\n");
		printf("-> Richtige Anwendung: \"starter <rpmOn> <rpmOff>\"\n");
		printf("-> Alle Parameter werden als Ganzzahlen vorrausgesetzt.\n");
	}
}

void cm_listStarter(void)
{
	printf("==============[starter]=============\n");
	printf("rpmStarterOn: %u rpmStarterOff: %u\n", kdActive.rpmStarterOn, kdActive.rpmStarterOff);
	printf("------------------------------------\n");
}

void cm_filter(void)
{
	unsigned int filter=0, variables=0;
	variables = sscanf(inputBuffer, "filter %u", &filter);
	if(variables == 1)
	{
		kdActive.filter = filter;
		printf("Filter-Parameter erfolgreich angepasst.\n");
	}
	else
	{
		printf("Fehler! Nicht zugelassene Eingabe.\n");
		printf("-> Richtige Anwendung: \"filter <time>\"\n");
		printf("-> Drehzahlwechselrate in 100us.\n");
		printf("-> Alle Parameter werden als Ganzzahlen vorrausgesetzt.\n");
	}
}

void cm_listFilter(void)
{
	printf("==============[filter]=============\n");
	printf("time: %uus\n", kdActive.filter * 100);
	printf("------------------------------------\n");
}

void cm_plot(void)
{
	printf("Diese Funktion ist leider noch nicht implementiert.\n");
}
