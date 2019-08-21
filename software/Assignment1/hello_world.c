#include <stdio.h>
#include "sys/alt_alarm.h"
#include <system.h>
#include <altera_avalon_pio_regs.h>

// #Defines
#define LIGHT_TRANSITION_TIME 1000
// ENUMS
enum OpperationMode {Mode1 = 1, Mode2 = 2, Mode3 = 3, Mode4 = 4};
enum LightColour {Red = 1, Yellow = 2, Green = 3};

// Function declarations
void UpdateMode(enum OpperationMode *currentMode);
alt_u32 tlc_timer_isr(void* context);
void button_interrupts_function(void* context, alt_u32 id);
void lcd_set_mode(enum OpperationMode currentMode);
void simple_tlc();
void pedestrian_tlc(void);
void init_buttons_pio(void);
void NSEW_ped_isr(void* context, alt_u32 id);


// Global variables
volatile alt_alarm timer;
volatile int timer_has_started = 0;
// Mode variables
volatile int mode1State = 0;
volatile int mode2State = 0;
// Different timers
volatile int t0 = 500;
volatile int t1 = 6000;
volatile int t2 = 2000;
volatile int t3 = 500;
volatile int t4 = 6000;
volatile int t5 = 2000;
volatile int currentTimeOut = 0;
// Pedestrian flags
volatile int EW_Ped = 0;
volatile int NS_Ped = 0;


alt_u32 tlc_timer_isr(void* context) {
	enum OpperationMode *currentMode = (unsigned int*) context;

	UpdateMode(currentMode);

	switch ((*currentMode)) {
	case Mode1:
		simple_tlc();
		break;
	case Mode2:
		pedestrian_tlc();
		break;
	case Mode3:
		break;
	case Mode4:
		break;
	}

	return currentTimeOut;
}



int main() {

	enum OpperationMode currentMode = Mode1;
	lcd_set_mode(currentMode);
	void* timerContext = (void*) &currentMode;
	alt_alarm_start(&timer, LIGHT_TRANSITION_TIME, tlc_timer_isr, timerContext); //Start the timer
	init_buttons_pio();
	//printf("currentMode %d\n", *currentMode);

	while(1){

	}
	return 0;
}


void UpdateMode(enum OpperationMode *currentMode){

	if (InSafeState()) { //Only change mode when in a safe state.
		unsigned int modeSwitchValue = IORD_ALTERA_AVALON_PIO_DATA(SWITCHES_BASE);
		if ((modeSwitchValue & 1<<0)) {
			if (*currentMode != Mode1){
				lcd_set_mode(Mode1);
			}

			(*currentMode) = Mode1;

		} else if ((modeSwitchValue & 1<<1)) {
			if (*currentMode != Mode2){
				lcd_set_mode(Mode2);
			}
			(*currentMode) = Mode2;
		} else if ((modeSwitchValue & 1<<2)) {
			if (*currentMode != Mode3){
				lcd_set_mode(Mode3);
			}
			(*currentMode) = Mode3;
		} else if ((modeSwitchValue & 1<<3)) {
			if (*currentMode != Mode4){
				lcd_set_mode(Mode4);
			}
			(*currentMode) = Mode4;
		}
	}
	return;
}

int InSafeState (void) {
	if(mode1State == 0 || mode1State == 3){
		printf("Mode 1 state %d\n", mode1State);
		return 1;
	}
	else {
		printf("Not safe N \n");
		return 0;
	}
}

void lcd_set_mode(enum OpperationMode currentMode) {
	// Write the current mode to the lcd
	#define ESC 27
	#define CLEAR_LCD_STRING "[2J"
	FILE *lcd;
	lcd = fopen(LCD_NAME, "w");
	if(lcd != NULL){
		fprintf(lcd, "%c%s", ESC, CLEAR_LCD_STRING);
		fprintf(lcd, "MODE: %d\n", currentMode);
	}
	return;
}

void simple_tlc() {
	printf("IN SIMPLE TLC\n");

	switch ((mode1State)) {
	case 0:
		IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00100100); //NS:Red EW:Red (safe state)
		currentTimeOut = t0;

		break;
	case 1:
		IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00001100); //NS:Green EW:Red
		currentTimeOut = t1;
		break;
	case 2:
		IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00010100); //NS:Yellow EW:Red
		currentTimeOut = t2;

		break;
	case 3:
		IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00100100); //NS:Red EW:Red (safe state)
		currentTimeOut = t3;
		break;
	case 4:
		IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00100001); //NS:Red EW:Green
		currentTimeOut = t4;
		break;
	case 5:
		IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00100010); //NS:Red EW:Yellow
		currentTimeOut = t5;
		break;
	}
	mode1State++;
	mode1State = mode1State%6;
}

void pedestrian_tlc(void) {
	switch ((mode2State)) {
		case 0:
			IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00100100); //NS:Red EW:Red (safe state)
			currentTimeOut = t0;
			break;
		case 1:
			if (NS_Ped == 1){
				IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b10001100); //NS:Green EW:Red
				NS_Ped = 0;
			}else {
				IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00001100); //NS:Green EW:Red
			}
			currentTimeOut = t1;
			break;
		case 2:
			IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00010100); //NS:Yellow EW:Red
			currentTimeOut = t2;
			break;
		case 3:
			IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00100100); //NS:Red EW:Red (safe state)
			currentTimeOut = t3;
			break;
		case 4:
			if (EW_Ped == 1) {
				IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b01100001); //NS:Red EW:Green
				EW_Ped = 0;
			}else{
				IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00100001); //NS:Red EW:Green
			}
			currentTimeOut = t4;
			break;
		case 5:
			IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE, 0b00100010); //NS:Red EW:Yellow
			currentTimeOut = t5;
			break;


		}
		mode2State++;
		mode2State = mode1State%6;
}

void init_buttons_pio(void) {
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0); // enable interrupts for buttons
	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(KEYS_BASE, 0x3); // enable interrupts for two right buttons.
	alt_irq_register(KEYS_IRQ,0, NSEW_ped_isr);
}

void NSEW_ped_isr(void* context, alt_u32 id) {
	unsigned int buttonValue = IORD_ALTERA_AVALON_PIO_DATA(KEYS_BASE);

	if (!(buttonValue & 1<<0)) {
		printf("EW Pressed\n");
		EW_Ped = 1;

	} else if (!(buttonValue & 1<<1)) {
		printf("NS Pressed\n");
		NS_Ped = 1;
	}
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(KEYS_BASE, 0);
}
