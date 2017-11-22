#include <stdint.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <string.h> 
#include <math.h> 
#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <avr/eeprom.h> 
#include <avr/portpins.h> 
#include <avr/pgmspace.h> 
#include <util/delay.h>
 
//FreeRTOS include files 
#include "FreeRTOS.h" 
#include "task.h" 
#include "croutine.h" 
#include "bit.h"

#define F_CPU 8000000UL

enum SONARState {init, pulse, waitEcho, display} sonar_state;

unsigned long duration = 0;
unsigned long distance = 0;


void SONAR_Init(){
	sonar_state = init;
}

void SONAR_Tick(){
	//Actions
	switch(sonar_state){
		case init:
			PORTC = 0x00;
			break;
		
		case pulse:
			PORTB = 0x00;
			_delay_us(2);
			PORTB = 0xFF;
			_delay_us(10);
			PORTB = 0x00;
			break;
			
		case waitEcho:
			++duration;
			if(!GetBit(PINA,0)){
				distance = (duration * 1000)/58;
			}
			break;
			
		case display:
			PORTC = distance;
			break;
		
		default:
			break;
	}
	//Transitions
	switch(sonar_state){
		case init:
			sonar_state = pulse;
			break;
			
		case pulse:
			if(GetBit(PINA,0)){
				duration = 0;
				sonar_state = waitEcho;	
			}
			else{
				sonar_state = pulse;
			}
			
			break;
			
		case waitEcho:
			if(!GetBit(PINA,0)){
				sonar_state = display;
			}
			
			else{
				sonar_state = waitEcho;
			}
			break;
			
		case display:
			sonar_state = pulse;
			break;
		
		default:
			sonar_state = pulse;
			break;
	}
}

void SONARSecTask()
{
	SONAR_Init();
   for(;;) 
   { 	
	SONAR_Tick();
	vTaskDelay(1); 
   } 
}

void SONARSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(SONARSecTask, (signed portCHAR *)"SONARSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}	


int main(void) 
{ 
   DDRA = 0x00; PORTA = 0xFF;	// input button and echo
   DDRB = 0xFF; PORTB = 0x00;	// output trigger
   DDRC = 0xFF; PORTC = 0x00;	// output state LED;
   DDRD = 0xFF; PORTD = 0x00;	// output led banks
   //Start Tasks  
   SONARSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}