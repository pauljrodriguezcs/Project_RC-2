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
 
//FreeRTOS include files 
#include "FreeRTOS.h" 
#include "task.h" 
#include "croutine.h" 
#include "bit.h"
enum SONARState {release,push,count} sonar_state;
enum OUTPUTState {init} output_state;

unsigned short counter = 0;
unsigned char temp = 0;


void SONAR_Init(){
	sonar_state = release;
}

void SONAR_Tick(){
	//Actions
	switch(sonar_state){
		case release:
			break;
		
		case push:
			break;
			
		case count:
			PORTC = 0xFF;
			break;
		
		default:
			break;
	}
	//Transitions
	switch(sonar_state){
		case release:
			if(GetBit(~PINA,1) == 1){
				sonar_state = push;
			}
			
			else{
				sonar_state = release;
			}
			
			break;
		
		case push:
			if(GetBit(~PINA,1) == 1){
				sonar_state = push;
			}
			
			else{
				sonar_state = count;
				counter = 0;
				PORTB = 0xFF;
			}
			break;
		
		case count:
			if(GetBit(~PINA,0) == 1){
				sonar_state = release;
				PORTB = 0x00;
				PORTC = 0x00;
			}
			
			else{
				sonar_state = count;
				++counter;
			}
			break;
		
		default:
			sonar_state = release;
			break;
	}
}

void SONARSecTask()
{
	SONAR_Init();
   for(;;) 
   { 	
	SONAR_Tick();
	vTaskDelay(10); 
   } 
}

void SONARSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(SONARSecTask, (signed portCHAR *)"SONARSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}	


void OUTPUT_Init(){
	output_state = init;
}

void OUTPUT_Tick(){
	//Actions
	switch(output_state){
		case init:
			temp = (counter*340)*5;
			PORTD = temp;
			break;
		
		default:
			break;
	}
	//Transitions
	switch(output_state){
		case init:
			output_state = init;
			break;
		
		default:
			output_state = init;
			break;
	}
}

void OUTPUTSecTask()
{
	OUTPUT_Init();
	for(;;)
	{
		OUTPUT_Tick();
		vTaskDelay(10);
	}
}

void OUTPUTSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(OUTPUTSecTask, (signed portCHAR *)"OUTPUTSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}
 
int main(void) 
{ 
   DDRA = 0x00; PORTA=0xFF;		// input button and echo
   DDRB = 0xFF; PORTB = 0x00;	// output trigger
   DDRC = 0xFF; PORTC = 0x00;	// output state LED;
   DDRD = 0xFF; PORTD = 0x00;	// output led banks
   //Start Tasks  
   SONARSecPulse(1);
   OUTPUTSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}