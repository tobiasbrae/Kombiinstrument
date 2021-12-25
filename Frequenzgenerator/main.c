// ================ [main.c (kombiinstrument frequency generator)] ========================
/*
*	A simple frequency generator program for an Atmel ATmega (L) running @8MHz
*	Configuration via UART (19200 baud/s, 8 data bits, no parity, 1 stop bit)
*
*	The output pin is PORTD7.
*	The analog input pin is PINC0.
*
*	Implemented commands:
*	- "fxxxxxe" -> sets the frequency to xxxxx Hz
*	- "rxxxxxe" -> sets the frequency to fit a xxxxx RPM signal for 4-cylinder Engines
*	- "mxe" -> sets the running mode (0 = running; 1 = control over analog in; 2 = perm. on; 3 = perm. off)
*
*	In analog in mode, the frequency/rpm value is controlled between one and the value given with the
*	"f-" or "r-" command.
*
*	Author: Tobias Brächter
*	Last update: 2019-05-13
*
*/

// ==================================== [includes] =========================================

#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>

#include "charBuffer.h"
#include "bitOperation.h"

// ==================================== [defines] ==========================================

#define MIN_FREQ 1
#define MAX_FREQ 10000
#define MIN_RPM 1
#define MAX_RPM 15000

#define NUM_TIMERS 2
#define T_FREQ 0
#define T_CHECK 1

#define CHECK_PERIOD 500

#define HZ_TO_NUM 5000 // time-base 100us, one period=2 ticks -> 5000 cycles per tick for 1 Hz
#define RPM_TO_NUM 150000 // time-base 100us, 4 signals per round, 2 ticks per period -> 7500 cycles per tick for 1 RPM

#define NUM_BUFFERS 2
#define INDATA 0
#define OUTDATA 1

#define IO_BUFFER_SIZE 50

#define MODE_RUNNING 0
#define MODE_AIN 1
#define MODE_ON 2
#define MODE_OFF 3

#define MODE_FREQ 0
#define MODE_RPM 1

#define NUM_COMMANDS 4
#define COM_F 1	// frequency
#define COM_R 2 // RPM
#define COM_M 3 // mode

#define COM 0
#define NUM 1

// ==================================== [variables] ==========================================

volatile uint32_t timer; // gets incremented via timer-interrupt
uint32_t timers[NUM_TIMERS]; // stores the different timer values
uint32_t waitTimeRaw; // stores the raw value to be generated
uint32_t waitTimeBuffer; // buffer for waitTime
uint32_t waitTime; // stores the time to wait for given frequency
uint8_t runningMode; // stores the current running mode
uint8_t valueMode; // stores if the current mode is rpm or frequency

cb_charBuffer buffers[NUM_BUFFERS];
uint8_t ioBuffers[NUM_BUFFERS][IO_BUFFER_SIZE];

uint8_t commands[NUM_COMMANDS][2];
uint8_t currentCommand;

uint8_t isSending; // indicates if the output buffer is currently being emptied

// ==================================== [function declaration] ==========================================


void initialize(void); // setting the timers, uart, etc.
void createCommands(void); // filling the array of allowed commands
void handleData(void); // checks the received data for valid commands
uint8_t hasNextCommand(void); // checks for next valid command
void sendString(char* data); // send a string via uart
void handleOutput(void); // control the output pin (gets called by time interrupt)
void resetTimer(uint8_t index); // resets the time for the given timer
uint32_t getTimeDiff(uint8_t index); // returns the stored time for the given timer

// ==================================== [program start] ==========================================

int main(void)
{
	initialize();
	createCommands();
	
	while(1)
	{
		handleData();
		if(runningMode == MODE_AIN && getTimeDiff(T_CHECK) > CHECK_PERIOD)
		{
			float newWaitTime;
			uint16_t adcResult = ADC;
			newWaitTime = (float) adcResult / 1024.0;
			newWaitTime = newWaitTime * (float) waitTimeRaw;
			if(newWaitTime < 1.0)
				newWaitTime = 1.0;
	
			if(valueMode == MODE_FREQ)
				waitTimeBuffer = HZ_TO_NUM / (uint32_t) newWaitTime;
			else if(valueMode == MODE_RPM)
				waitTimeBuffer = RPM_TO_NUM / (uint32_t) newWaitTime;

			resetTimer(T_CHECK);
		}
	}
}

void initialize(void)
{
	cli(); // disable global interrupts
  
	setBit(&DDRD, 7, 1); // set the output pin
	setBit(&DDRC, 0, 0); // PC0 as input or adc
	setBit(&DDRD, 0, 0); // RX0 as input
	setBit(&DDRD, 1, 1); // TX0 as output

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

	// adc setup
	setBit(&ADMUX, REFS0, 0); // set external aref as reference voltage
	setBit(&ADMUX, REFS1, 0);
	setBit(&ADMUX, ADLAR, 0); // right-adjust result
	setBit(&ADMUX, MUX0, 0); // select PC0 as adc port
	setBit(&ADMUX, MUX1, 0);
	setBit(&ADMUX, MUX2, 0);
	setBit(&ADMUX, MUX3, 0);
	setBit(&ADCSRA, ADPS0, 1); // set prescaler to 64 -> 125kHz conversion speed
	setBit(&ADCSRA, ADPS1, 1);
	setBit(&ADCSRA, ADPS2, 1);
	setBit(&ADCSRA, ADEN, 1); // switch adc on
	setBit(&ADCSRA, ADFR, 1); // enable free running mode
	setBit(&ADCSRA, ADSC, 1); // start conversion

	//running mode	
	waitTimeRaw = 8000;
	valueMode = MODE_RPM;
	runningMode = MODE_AIN;

	cb_initBuffer(&buffers[INDATA], &ioBuffers[INDATA][0], IO_BUFFER_SIZE);
	cb_initBuffer(&buffers[OUTDATA], &ioBuffers[OUTDATA][0], IO_BUFFER_SIZE);

	sei(); // enable global interrupts
}

void createCommands(void)
{
	commands[COM_F][0] = 'f';
	commands[COM_F][1] = 7;
	commands[COM_R][0] = 'r';
	commands[COM_R][1] = 7;
	commands[COM_M][0] = 'm';
	commands[COM_M][1] = 3;
}

void handleData(void)
{
	if(hasNextCommand())
	{
		if(currentCommand == COM_F || currentCommand == COM_R)
		{
			waitTimeRaw = 0;
			waitTimeRaw += 10000 * (cb_getNextOff(&buffers[INDATA], 1) - 48);
			waitTimeRaw += 1000  * (cb_getNextOff(&buffers[INDATA], 2) - 48);
			waitTimeRaw += 100   * (cb_getNextOff(&buffers[INDATA], 3) - 48);
			waitTimeRaw += 10    * (cb_getNextOff(&buffers[INDATA], 4) - 48);
			waitTimeRaw += 1     * (cb_getNextOff(&buffers[INDATA], 5) - 48);
			if(currentCommand == COM_F && waitTimeRaw >= MIN_FREQ && waitTimeRaw <= MAX_FREQ)
			{
				valueMode = MODE_FREQ;
				if(runningMode != MODE_AIN)
					waitTimeBuffer = HZ_TO_NUM / waitTimeRaw;
				sendString("New frequency set!\r\n");
			}
			else if(currentCommand == COM_R && waitTimeRaw >= MIN_RPM && waitTimeRaw <= MAX_RPM)
			{
				valueMode = MODE_RPM;
				if(runningMode != MODE_AIN)
					waitTimeBuffer = RPM_TO_NUM / waitTimeRaw;
				sendString("New RPM set!\r\n");
			}
			else
				sendString("Value not in allowed range!\r\n");
		}
		else if(currentCommand == COM_M)
		{
			if(cb_getNextOff(&buffers[INDATA],1)-48 >= MODE_RUNNING && cb_getNextOff(&buffers[INDATA],1)-48 <= MODE_OFF)
			{
				runningMode = cb_getNextOff(&buffers[INDATA],1)-48;
				if(runningMode == MODE_RUNNING)
				{
					if(valueMode == MODE_FREQ)
						waitTimeBuffer = HZ_TO_NUM / waitTimeRaw;
					else if(valueMode == MODE_RPM)
						waitTimeBuffer = RPM_TO_NUM / waitTimeRaw;
				}
				sendString("New mode set successfully!\r\n");
			}
			else
				sendString("Invalid mode!\r\n");
		}
		cb_deleteN(&buffers[INDATA], commands[currentCommand][NUM]);
	}
}

uint8_t hasNextCommand(void)
{
	currentCommand = 0;
	if(cb_hasNext(&buffers[INDATA]))
	{
		for(uint8_t i = 1; i < NUM_COMMANDS; i++)
			if(commands[i][COM] == cb_getNext(&buffers[INDATA]))
				currentCommand = i;
		if(currentCommand)
		{
			if(cb_hasNext(&buffers[INDATA]) >= commands[currentCommand][NUM])
			{
				if(cb_getNextOff(&buffers[INDATA], commands[currentCommand][NUM]-1) == 'e')
					return 1;
				cb_deleteN(&buffers[INDATA], commands[currentCommand][NUM]);
				sendString("Error! Invalid command!\r\n");
			}
		}
		else
		{
			sendString("Error! Unknown command!\r\n");
			cb_delete(&buffers[INDATA]);
		}
	}
	return 0;
}

void sendString(char *data)
{
	cb_putString(&buffers[OUTDATA], (uint8_t *) data);
	if(!isSending)
	{
		isSending = 1;
		UDR = cb_getNext(&buffers[OUTDATA]);
		cb_delete(&buffers[OUTDATA]);
	}
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

void handleOutput(void)
{
	if(runningMode == MODE_OFF)
	{
		timer = 0;
		if(readBit(&PORTD, 7))
			setBit(&PORTD, 7, 0);
	}
	else if(runningMode == MODE_ON)
	{
		timer = 0;
		if(!readBit(&PORTD, 7))
			setBit(&PORTD, 7, 1);
	}
	else
	{
		if(getTimeDiff(T_FREQ) >= waitTime)
		{
			waitTime = waitTimeBuffer;
			toggleBit(&PORTD, 7);
			resetTimer(T_FREQ);
		}
	}
}

ISR(TIMER2_COMP_vect)
{
	timer++;
	handleOutput();
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
