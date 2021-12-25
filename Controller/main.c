// ==================================== [main.c (kombiinstrument)] =============================
/*
*	This programm is desgined to control an RGB-Illumination in dependency of the RPM-Value.
*	Furthermore, it supplies an enable output for the starter switch to prevent starting while
*	the engine is already running.
*	It is designed to run on an Atmel ATmega8 (L) running @8MHz
*
*	Communication via UART (19200 baud/s, 8 data bits, no parity, 1 stop bit)
*
*	For further information, read the "readme.txt" of the kombiinstrument project.
*
*	Author: Tobias Brächter
*	Last update: 2019-10-06
*
*/

// ==================================== [includes] =========================================

#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>

#include "charBuffer.h"
#include "bitOperation.h"
#include "kombiData.h"

// ==================================== [pin configuration] ===============================

// PORTB0 (ICP1)		- Unused
// PORTB1 (OC1A)		- Unused
// PORTB2 (SS/OC1B)		- Unused
// PORTB3 (MOSI/OC2)	- ISP
// PORTB4 (MISO)		- ISP
// PORTB5 (SCK)			- ISP
// PORTB6 (XTAL1)		- Unused
// PORTB7 (XTAL2)		- Unused

// PORTC0 (ADC0)		- LED Channel Blue
// PORTC1 (ADC1)		- LED Channel Red
// PORTC2 (ADC2)		- LED Channel Green
// PORTC3 (ADC3)		- Unused
// PORTC4 (ADC4)		- Unused
// PORTC5 (ADC5)		- Unused
// PORTC6 (RESET)		- ISP

// PORTD0 (RXD)			- UART
// PORTD1 (TXD)			- UART
// PORTD2 (INT0)		- RPM signal input
// PORTD3 (INT1)		- Unused
// PORTD4 (XCK)			- Unused
// PORTD5 (T1)			- Unused
// PORTD6 (AIN0)		- Starter enable output
// PORTD7 (AIN1)		- Starter enable output

#define DDR_LED_RED &DDRC,1
#define DDR_LED_GRE &DDRC,2
#define DDR_LED_BLU &DDRC,0
#define DDR_STARTER1 &DDRD,6
#define DDR_STARTER2 &DDRD,7
#define LED_RED &PORTC,1
#define LED_GRE &PORTC,2
#define LED_BLU &PORTC,0
#define STARTER1 &PORTD,6
#define STARTER2 &PORTD,7

// ==================================== [defines] ==========================================

#define RPM_TO_NUM 300000 // time-base 100us, 2 signals per round, 60s per round -> 300000 cycles between to signals for 1 RPM
#define MIN_RPM 3000 // factor for the lowest accepted rpm
#define PWM_PERIOD 100 // period for pwm signals
#define CHECK_PERIOD 1000 // checking for low rpm, determine running effects...

#define RED 0
#define GRE 1
#define BLU 2

#define NUM_BUFFERS 2
#define INDATA 0
#define OUTDATA 1

#define IO_BUFFER_SIZE 150

#define NUM_COMMANDS 7
#define COM 0
#define NUM 1
#define COM_S 0 // save the data from the cache to the memory
#define COM_R 1 // read the data from the memory into the cache
#define COM_L 2 // load data via UART to the cache
#define COM_G 3 // get the data via UART from the cache (for veryfying data...)
#define COM_T 4 // transfer the data stored in the cache to the active data
#define COM_D 5 // loads some demo data into the cache
#define COM_A 6 // activate echo for unknown commands

#define NUM_TIMERS 4
#define T_PWM 0
#define T_RPM 1
#define T_DIMMER 2
#define T_CHECK 3

#define NUM_DT 4 // DT -> dutycycle
#define DT_RED 0
#define DT_GRE 1
#define DT_BLU 2
#define DT_STARTER 3

// ==================================== [variables] ==========================================

volatile uint32_t timer; // gets incremented via timer-interrupt
uint32_t timers[NUM_TIMERS]; // stores the different timer values
volatile uint16_t newRpm; // stores the new calculated rpm
volatile uint16_t rpm; // stores the current rpm
volatile uint16_t filterStep; // used to count the steps for filtering

cb_charBuffer buffers[NUM_BUFFERS]; // buffers for io-communication
uint8_t ioBuffers[NUM_BUFFERS][IO_BUFFER_SIZE];

uint8_t commands[NUM_COMMANDS][2]; // stores the implemented commands
uint8_t currentCommand; // stores the currently received command

uint8_t dutyCycles[NUM_DT]; // stores the current duty cycles for each channel; gets updated from Buffer with PWM period
uint8_t dutyCyclesBuffer[NUM_DT]; // stores the new calculated duty cycles; buffering prevents flickering

uint8_t isSending; // indicates if the output buffer is currently being emptied

uint8_t sendAnswer; // if true, unknown commands will be sent back

// kombiData
kombiData kdActive, kdCache;
uint8_t *pkdActive; // for loop-based data transfer
uint8_t *pkdCache; // for loop-base data transfer

// breakpoint
uint8_t breakActive; // points to the active breakpoint
double breakSlopes[3]; // slopes for each color
double breakOffset[3]; // offsets for each color
double breakValues[3]; // calculated value for each color

// dimmer
uint8_t dimActive; // points to the active dimmer
uint8_t dimEnabled; // shows if any dimmer is active
uint8_t dimPhase; // current phase of the dimmer
double dimValue; // shows the current dimming value

// ==================================== [function declaration] ==========================================

void initialize(void); // setting the timers, uart, etc.
void createCommands(void); // filling the array of allowed commands
void handleData(void); // checks the received data for commands and executes them
uint8_t hasNextCommand(void); // checks for next valid command
void sendString(char* data); // send a string via uart
void loadFromMemory(void); // loads the data from the EEPROM into the cache
void saveToMemory(void); // saves the data from cache to EEPROM
void loadFromCache(void); // transfers the data from cache to active
void loadDemoData(void); // loads some demo data into the cache
void resetTimer(uint8_t index); // resets the time for the given timer
uint32_t getTimeDiff(uint8_t index); // returns the stored time for the given timer
void determineActiveEffects(void); // checks which breakpoint & dimmer should be active
void calculateBreakpoint(void); // pre-calculates the parameters for the active breakpoint
void calculateEffects(void); // calculates the outcomes of active breakpoint & dimmer
void handlePWM(void); // switches the output ports on and off

// ==================================== [program start] ==========================================

int main(void)
{
	sendAnswer = 0;

	initialize();
	createCommands();
	
	while(1)
	{
		handleData();

		if(getTimeDiff(T_CHECK) > CHECK_PERIOD)
		{
			if(MIN_RPM < getTimeDiff(T_RPM))
				newRpm = 0;
			determineActiveEffects();
			resetTimer(T_CHECK);
		}
		calculateEffects();
	}
}

void initialize(void)
{
	cli(); // disable global interrupts
  
	setBit(&DDRD, 0, 0); // RX0 as input
	setBit(&DDRD, 1, 1); // TX0 as output
	setBit(DDR_LED_RED, 1); // set output-ports
	setBit(DDR_LED_GRE, 1);
	setBit(DDR_LED_BLU, 1);
	setBit(DDR_STARTER1, 1);
	setBit(DDR_STARTER2, 1);

	// timer setup
	OCR2 = 100; // time-base 100us
	setBit(&TCCR2, WGM21, 1); // ctc mode
	setBit(&TCCR2, WGM20, 0);
	setBit(&TIMSK, OCIE2, 1); // enable compare match interrupt
	setBit(&TCCR2, CS22, 0);
	setBit(&TCCR2, CS21, 1);
	setBit(&TCCR2, CS20, 0); // divider 8 -> 1 MHz
	
	// uart setup
	UBRRH = 0;
	UBRRL = 25; // baudrate 19200
	setBit(&UCSRA, U2X, 0); // no double data rate
	setBit(&UCSRB, RXCIE, 1); // enable RX complete interrupt
	setBit(&UCSRB, TXCIE, 1); // enable TX complete interrupt
	UCSRC = (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1); // URSEL needed to write in UCSRC!
	setBit(&UCSRB, UCSZ2, 0); //
	setBit(&UCSRB, TXEN, 1); // Enable TX
	setBit(&UCSRB, RXEN, 1); // Enable RX

	// rpm-interrupt setup
	setBit(&DDRD, 2, 0); // set interrupt pin as input
	setBit(&MCUCR, ISC00, 1); // interrupt on rising edge
	setBit(&MCUCR, ISC01, 1);
	setBit(&GICR, INT0, 1); // enable interrupt on pin INT0
	
	// init io-buffers
	cb_initBuffer(&buffers[INDATA], &ioBuffers[INDATA][0], IO_BUFFER_SIZE);
	cb_initBuffer(&buffers[OUTDATA], &ioBuffers[OUTDATA][0], IO_BUFFER_SIZE);

	// set kombiData-pointers
	pkdActive = (uint8_t *) &kdActive;
	pkdCache = (uint8_t *) &kdCache;

	// load the data stored in EEPROM
	loadFromMemory();
	loadFromCache();
	
	// enable global interrupts
	sei();
}

void createCommands(void)
{
	commands[COM_S][COM] = 's';
	commands[COM_S][NUM] = 2;
	commands[COM_R][COM] = 'r';
	commands[COM_R][NUM] = 2;
	commands[COM_L][COM] = 'l';
	commands[COM_L][NUM] = sizeof(kombiData)+2;
	commands[COM_G][COM] = 'g';
	commands[COM_G][NUM] = 2;
	commands[COM_T][COM] = 't';
	commands[COM_T][NUM] = 2;
	commands[COM_D][COM] = 'd';
	commands[COM_D][NUM] = 2;
	commands[COM_A][COM] = 'a';
	commands[COM_A][NUM] = 3;
}

void handleData(void)
{
	if(hasNextCommand())
	{
		if(currentCommand == COM_S) // save data from cache in EEPROM
		{
			saveToMemory();
			sendString(SEND_STATUS_OK);
		}
		else if(currentCommand == COM_R) // read data from EEPROM to cache
		{
			loadFromMemory();
			sendString(SEND_STATUS_OK);
		}
		else if(currentCommand == COM_L) // load data into cache via UART
		{
			for(uint8_t i=0; i < sizeof(kombiData); i++)
				pkdCache[i] = cb_getNextOff(&buffers[INDATA], i+1);
			sendString(SEND_STATUS_OK);
		}
		else if(currentCommand == COM_G) // get data from cache via UART
		{
			cb_put(&buffers[OUTDATA], 'd');
			for(uint8_t i=0; i < sizeof(kombiData); i++)
				cb_put(&buffers[OUTDATA], pkdCache[i]);
			sendString("e");
		}
		else if(currentCommand == COM_T) // transfer data from cache to active
		{
			loadFromCache();
			sendString(SEND_STATUS_OK);
		}
		else if(currentCommand == COM_D) // activate demodata
		{
			loadDemoData();
			sendString(SEND_STATUS_OK);
		}
		else if(currentCommand == COM_A) // activate answers on unknown commands
		{
			if(cb_getNextOff(&buffers[INDATA], 1) == '1')
				sendAnswer = 1;
			else
				sendAnswer = 0;
			sendString(SEND_STATUS_OK);
		}
		cb_deleteN(&buffers[INDATA], commands[currentCommand][NUM]); // clear the buffer after input is computed
	}
}

uint8_t hasNextCommand(void)
{
	if(cb_hasNext(&buffers[INDATA]))
	{
		for(currentCommand = 0; currentCommand < NUM_COMMANDS; currentCommand++) // loop through defined commands
			if(commands[currentCommand][COM] == cb_getNext(&buffers[INDATA]))
				break;
		if(currentCommand < NUM_COMMANDS)
		{
			if(cb_hasNext(&buffers[INDATA]) >= commands[currentCommand][NUM]) // check if already enough chars are available
			{
				if(cb_getNextOff(&buffers[INDATA], commands[currentCommand][NUM]-1) == 'e') // check if terminator is present
					return 1;
				cb_deleteN(&buffers[INDATA], commands[currentCommand][NUM]); // otherwise delete data from input buffer
				sendString(SEND_STATUS_INVALID);
			}
		}
		else // received command is not in command-list
		{
			sendString(SEND_STATUS_UNKNOWN);
			if(sendAnswer) // echo the command for debugging
			{
				sendString("\r\n");
				cb_put(&buffers[OUTDATA], cb_getNext(&buffers[INDATA]));
				sendString("\r\n");
			}
			cb_delete(&buffers[INDATA]);
		}
	}
	return 0;
}

void sendString(char *data)
{
	cb_putString(&buffers[OUTDATA], (uint8_t *) data);
	if(!isSending) // trigger interrupt-based sending
	{
		isSending = 1;
		UDR = cb_getNext(&buffers[OUTDATA]);
		cb_delete(&buffers[OUTDATA]);
	}
}

void loadFromMemory(void)
{
	cli(); // while reading from EEPROM, disable interrupts
	for(uint8_t i=0; i < sizeof(kombiData); i++)
	{
		while(readBit(&EECR, EEWE)); // wait for possible writing to finish
		EEARL = i; // write target address
		EEARH = 0;
		setBit(&EECR, EERE, 1); // enable read operation
		pkdCache[i] = EEDR; // load target data to cache
		setBit(&EECR, EERE, 0); // disable read operation
	}
	EECR = 0; // clear any operation bits
	sei(); // re-enable interrupts
}

void saveToMemory(void)
{
	cli(); // while writing, disable interrupts
	for(uint8_t i=0; i < sizeof(kombiData); i++)
	{
		while(readBit(&EECR, EEWE)); // wait for possible writing to finish
		EEARL = i; // write target address
		EEARH = 0;
		EEDR = pkdCache[i]; // write target data
		EECR = (1 << EEMWE);
		// the write enable bit has to be set within 4 cycles after master write enable
		// setBit(...) is too slow!
		EECR |= (1 << EEWE);
	}
	while(readBit(&EECR, EEWE)); // wait for possible writing to finish
	EECR = 0; // clear any operation bits
	sei(); // re-enable interrupts
}

void loadFromCache(void)
{
	cli();
	for(uint8_t i=0; i < sizeof(kombiData); i++)
		pkdActive[i] = pkdCache[i];
	dimActive = kdActive.dimActive;
	dimEnabled = kdActive.dimEnabled;
	breakActive = kdActive.breakActive;
	calculateBreakpoint();
	sei();
}

void loadDemoData(void)
{
	kdCache.breakpoints[0].rpm = 0;
	kdCache.breakpoints[0].dutyRed = 0;
	kdCache.breakpoints[0].dutyGre = 0;
	kdCache.breakpoints[0].dutyBlu = 100;
	kdCache.breakpoints[1].rpm = 600;
	kdCache.breakpoints[1].dutyRed = 0;
	kdCache.breakpoints[1].dutyGre = 0;
	kdCache.breakpoints[1].dutyBlu = 100;
	kdCache.breakpoints[2].rpm = 800;
	kdCache.breakpoints[2].dutyRed = 0;
	kdCache.breakpoints[2].dutyGre = 100;
	kdCache.breakpoints[2].dutyBlu = 0;
	kdCache.breakpoints[3].rpm = 1500;
	kdCache.breakpoints[3].dutyRed = 0;
	kdCache.breakpoints[3].dutyGre = 100;
	kdCache.breakpoints[3].dutyBlu = 0;
	kdCache.breakpoints[4].rpm = 3000;
	kdCache.breakpoints[4].dutyRed = 100;
	kdCache.breakpoints[4].dutyGre = 0;
	kdCache.breakpoints[4].dutyBlu = 0;

	kdCache.dimmers[0].rpmLow = 1000;
	kdCache.dimmers[0].rpmHigh = 8000;
	kdCache.dimmers[0].tRise = 50000;
	kdCache.dimmers[0].tHigh = 1000;
	kdCache.dimmers[0].tFall = 50000;
	kdCache.dimmers[0].tLow = 2000;

	kdCache.breakHyst = 20;
	kdCache.dimHyst = 20;
	
	kdCache.breakActive = 0;
	kdCache.dimActive = 0;
	kdCache.dimEnabled = 1;

	kdCache.filter = 1;
}

void resetTimer(uint8_t index) // resets the time for the given timer
{
	if(index < NUM_TIMERS)
		timers[index] = timer;
}

uint32_t getTimeDiff(uint8_t index) // returns the stored time for the given timer
{
	if(index < NUM_TIMERS)
		return timer-timers[index];
	return 0;
}

void determineActiveEffects(void)
{
	if(breakActive > NUM_BREAK)
		breakActive = NUM_BREAK;
	if(dimActive > NUM_DIM)
		dimActive = NUM_DIM;
	
	if(breakActive > 0)
	{
		uint16_t rpmMin = kdActive.breakpoints[breakActive-1].rpm;
		if(rpmMin > kdActive.breakHyst) // prevent underflow
			rpmMin -= kdActive.breakHyst;
		else
			rpmMin = 0;
		if(rpm < rpmMin)
		{
			breakActive--;
			calculateBreakpoint();
		}
	}
	if(breakActive < (NUM_BREAK - 1))
	{
		uint16_t rpmMax = kdActive.breakpoints[breakActive].rpm;
		if(65535 - rpmMax > kdActive.breakHyst)
			rpmMax += kdActive.breakHyst;
		else
			rpmMax = 65535;
		if(rpm > rpmMax)
		{
			breakActive++;
			calculateBreakpoint();
		}
	}

	uint8_t change = 0;
	if(dimActive >= NUM_DIM)
		change = 1;
	else
	{
		uint16_t rpmMin, rpmMax;
		rpmMin = kdActive.dimmers[dimActive].rpmLow;
		rpmMax = kdActive.dimmers[dimActive].rpmHigh;
		if(rpmMin > kdActive.dimHyst)
			rpmMin -= kdActive.dimHyst;
		else
			rpmMin = 0;
		if(65535 - rpmMax > kdActive.dimHyst)
			rpmMax += kdActive.dimHyst;
		else
			rpmMax = 65535;

		if(rpm < rpmMin || rpm > rpmMax)
			change = 1;
	}

	if(change)
	{
		dimActive = 0;
		while(dimActive < NUM_DIM)
		{
			if(rpm >= kdActive.dimmers[dimActive].rpmLow && rpm <= kdActive.dimmers[dimActive].rpmHigh)
				break;
			dimActive++;
		}
		dimPhase = PH_HIGH;
		resetTimer(T_DIMMER);
		dimEnabled = 1;
		if(dimActive == NUM_DIM)
			dimEnabled = 0;
	}
}

void calculateBreakpoint(void)
{
	if(breakActive >= NUM_BREAK) // if no breakpoint is set, set LEDs off
	{
		breakSlopes[RED] = 0;
		breakSlopes[GRE] = 0;
		breakSlopes[BLU] = 0;
		breakOffset[RED] = 0;
		breakOffset[GRE] = 0;
		breakOffset[BLU] = 0;
	}
	else if(breakActive == 0 || breakActive == (NUM_BREAK - 1)) // the first breakpoint has no breakpoint before, therefore, no slopes
	{
		breakSlopes[RED] = 0;
		breakSlopes[GRE] = 0;
		breakSlopes[BLU] = 0;
		breakOffset[RED] = kdActive.breakpoints[breakActive].dutyRed;
		breakOffset[GRE] = kdActive.breakpoints[breakActive].dutyGre;
		breakOffset[BLU] = kdActive.breakpoints[breakActive].dutyBlu;
	}
	else
	{
		double cacheA, cacheB;
		cacheB = kdActive.breakpoints[breakActive].rpm - kdActive.breakpoints[breakActive-1].rpm;
		cacheA = kdActive.breakpoints[breakActive].dutyRed - kdActive.breakpoints[breakActive-1].dutyRed;
		breakSlopes[RED] = cacheA / cacheB;
		cacheA = kdActive.breakpoints[breakActive].dutyGre - kdActive.breakpoints[breakActive-1].dutyGre;
		breakSlopes[GRE] = cacheA / cacheB;
		cacheA = kdActive.breakpoints[breakActive].dutyBlu - kdActive.breakpoints[breakActive-1].dutyBlu;
		breakSlopes[BLU] = cacheA / cacheB;
		breakOffset[RED] = kdActive.breakpoints[breakActive].dutyRed - kdActive.breakpoints[breakActive].rpm * breakSlopes[RED];
		breakOffset[GRE] = kdActive.breakpoints[breakActive].dutyGre - kdActive.breakpoints[breakActive].rpm * breakSlopes[GRE];
		breakOffset[BLU] = kdActive.breakpoints[breakActive].dutyBlu - kdActive.breakpoints[breakActive].rpm * breakSlopes[BLU];
	}
}

void calculateEffects(void)
{
	//breakpoints
	for(uint8_t i=0; i < 3; i++)
		breakValues[i] = rpm * breakSlopes[i] + breakOffset[i];

	//dimmer
	if(dimEnabled)
	{
		if(dimPhase == PH_RISE)
		{
			dimValue = (double) getTimeDiff(T_DIMMER) / (double) kdActive.dimmers[dimActive].tRise;
			if(getTimeDiff(T_DIMMER) >= kdActive.dimmers[dimActive].tRise)
			{
				dimPhase = PH_HIGH;
				resetTimer(T_DIMMER);
			}
		}
		else if(dimPhase == PH_HIGH)
		{
			dimValue = 1.0;
			if(getTimeDiff(T_DIMMER) >= kdActive.dimmers[dimActive].tHigh)
			{
				dimPhase = PH_FALL;
				resetTimer(T_DIMMER);
			}
		}
		else if(dimPhase == PH_FALL)
		{
			dimValue = 1.0 - (double) getTimeDiff(T_DIMMER) / (double) kdActive.dimmers[dimActive].tFall;
			if(dimValue < 0)
				dimValue = 0;
			if(getTimeDiff(T_DIMMER) >= kdActive.dimmers[dimActive].tFall)
			{
				dimPhase = PH_LOW;
				resetTimer(T_DIMMER);
			}
		}
		else if(dimPhase == PH_LOW)
		{
			dimValue = 0;
			if(getTimeDiff(T_DIMMER) >= kdActive.dimmers[dimActive].tLow)
			{
				dimPhase = PH_RISE;
				resetTimer(T_DIMMER);
			}
		}
		
		for(uint8_t i=0; i < 3; i++)
			breakValues[i] = breakValues[i] * dimValue;
	}

	for(uint8_t i=0; i < 3; i++)
		dutyCyclesBuffer[i] = (uint8_t) breakValues[i];

	if(rpm >= kdActive.rpmStarterOff)
		dutyCyclesBuffer[DT_STARTER] = 0;
	else if(rpm <= kdActive.rpmStarterOn)
		dutyCyclesBuffer[DT_STARTER] = PWM_PERIOD;
}

void handlePWM(void)
{
	if(getTimeDiff(T_PWM) > PWM_PERIOD)
	{
		for(uint8_t i=0; i < NUM_DT; i++)
			dutyCycles[i] = dutyCyclesBuffer[i];
		if(dutyCycles[DT_RED] > 0)
			setBit(LED_RED, 1);
		if(dutyCycles[DT_GRE] > 0)
			setBit(LED_GRE, 1);
		if(dutyCycles[DT_BLU] > 0)
			setBit(LED_BLU, 1);
		resetTimer(T_PWM);

		if(dutyCycles[DT_STARTER] > 0)
		{
			setBit(STARTER1, 1);
			setBit(STARTER2, 1);
		}
		else
		{
			setBit(STARTER1, 0);
			setBit(STARTER2, 0);
		}
	}
	if(dutyCycles[DT_RED] < PWM_PERIOD && getTimeDiff(T_PWM) >= dutyCycles[DT_RED])
		setBit(LED_RED, 0);
	if(dutyCycles[DT_GRE] < PWM_PERIOD && getTimeDiff(T_PWM) >= dutyCycles[DT_GRE])
		setBit(LED_GRE, 0);
	if(dutyCycles[DT_BLU] < PWM_PERIOD && getTimeDiff(T_PWM) >= dutyCycles[DT_BLU])
		setBit(LED_BLU, 0);
}

ISR(INT0_vect)
{
	newRpm = RPM_TO_NUM/getTimeDiff(T_RPM);
	resetTimer(T_RPM);
}

ISR(TIMER2_COMP_vect)
{
	timer++;
	filterStep++;
	if(filterStep >= kdActive.filter)
	{
		if(newRpm > rpm)
			rpm++;
		else if(newRpm < rpm)
			rpm--;
		filterStep = 0;
	}
	handlePWM();
}

ISR(USART_RXC_vect) // RX complete
{
	uint8_t cache = UDR;
	cb_put(&buffers[INDATA], cache);
}

ISR(USART_TXC_vect)
{
	if(cb_hasNext(&buffers[OUTDATA]))
	{
		UDR = cb_getNext(&buffers[OUTDATA]);
		cb_delete(&buffers[OUTDATA]);
	}
	else
		isSending = 0;
}
