/*
 * SonarTest.c
 *
 * Created: 11/20/2017 9:31:16 PM
 * Author : Paul
 */ 
#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

#include "bit.h"

int main(void)
{
	DDRA = 0x00; PORTA = 0xFF; //Input
	DDRB = 0xFF; PORTB = 0x00; //Output
	DDRC = 0xFF; PORTC = 0x00; //Output
	DDRD = 0xFF; PORTD = 0x00; //Output
	unsigned long duration;
	unsigned long new_distance;
    /* Replace with your application code */
    while (1) 
    {
		duration = 0;
		new_distance = 0;
		PORTB = 0x00;
		_delay_us(10);
		PORTB = 0xFF;
		_delay_us(20);
		PORTB = 0x00;
		while(GetBit(PINA,0)){
			++duration;	
		}
		new_distance = duration * 170 ;
		PORTC = (new_distance);
		/*
		if(4<=new_distance){
			PORTC = 0xFF;
		}
		else{
			PORTC = 0x00;
		}
		*/
		_delay_ms(1);
    }
}

