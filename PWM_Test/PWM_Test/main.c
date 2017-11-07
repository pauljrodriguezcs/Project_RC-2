/*
 * PWM_Test.c
 *
 * Created: 11/6/2017 7:29:32 PM
 * Author : Paul
 */ 

#include <avr/io.h>


int main(void)
{
    /* Replace with your application code */
    while (1) 
    {
    }
}

void set_PWM(double frequency) {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB6 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM32: When counter (TCNT3) matches OCR3A, reset counter
	// CS31 & CS30: Set a prescaler of 64
	// set OCR3A based on desired frequency
	
	OCR3A = (short)(8000000 / (128 * frequency)) - 1; 
	
	TCNT3 = 0; // resets counter
	
}