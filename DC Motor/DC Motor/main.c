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

// Addition files
#include "bit.h"

// Shared Variables
unsigned char going_forward = 0;
unsigned char going_reverse = 0;

//-------------------------------------------------- Enumeration of States --------------------------------------------------//

enum BUTTONState {b_release, b_push} button_state;
enum FORWARDState {forward_off, forward_on} forward_state;
enum REVERSEState {reverse_off, reverse_on} reverse_state;

//-------------------------------------------------- Start Button SM --------------------------------------------------//

void BUTTONS_Init(){
	button_state = b_release;
}

void BUTTONS_Tick(){
	// State Actions
	switch(button_state){
		case b_release:
			break;
		case b_push:
			break;
		default:
			break;
	}
	
	// State Transitions
	switch(button_state){
		case b_release:
			if((GetBit(~PINA,0) == 1) && !(GetBit(~PINA,1) == 1)){
				going_forward = 1;
				going_reverse = 0;		
				button_state = b_push;
			}
			
			else if(!(GetBit(~PINA,0) == 1) && (GetBit(~PINA,1) == 1)){
				going_forward = 0;
				going_reverse = 1;
				button_state = b_push;
			}
			
			else{
				going_forward = 0;
				going_reverse = 0;
				button_state = b_release;	
			}
			
			break;
		
		case b_push:
			if(!(GetBit(~PINA,0) == 1) && !(GetBit(~PINA,1) == 1)){
				going_forward = 0;
				going_reverse = 0;
				button_state = b_release;
			}
			
			else{
				button_state = b_push;
			}
			
			break;
		
		default:
			button_state = b_release;
			break;
	}
}

void ButtonSecTask(){
	BUTTONS_Init();
	for(;;){
		BUTTONS_Tick();
		vTaskDelay(100);
	}
}

void ButtonSecPulse(unsigned portBASE_TYPE Priority){
	xTaskCreate(ButtonSecTask, (signed portCHAR *)"ButtonSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

//-------------------------------------------------- End Button SM --------------------------------------------------//

//-------------------------------------------------- Start Motor SM (Forward) --------------------------------------------------//

void FORWARD_Init(){
	forward_state = forward_off;
}

void FORWARD_Tick(){
	//State Actions
	switch(forward_state){
		case forward_off:
			break;
		case forward_on:
			break;
		default:
			break;
	}
	
	//State Transitions
	switch(forward_state){
		case forward_off:
			if(going_forward){
				forward_state = forward_on;
				//PORTC = 0xFF;
				PORTC = 0x01;
				PORTD = 0x01;
			}
			
			else{
				forward_state = forward_off;
			}
			
			break;
			
		case forward_on:
			if(going_forward){
				forward_state = forward_on;
			}
			
			else{
				forward_state = forward_off;
				PORTC = 0x00;
				PORTD = 0x00;
			}
			break;
			
		default:
			break;
	}
	
}

void ForwardSecTask(){
	FORWARD_Init();
	for(;;){
		FORWARD_Tick();
		vTaskDelay(100);
	}
}

void ForwardSecPulse(unsigned portBASE_TYPE Priority){
	xTaskCreate(ForwardSecTask, (signed portCHAR *)"ForwardSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

//-------------------------------------------------- End Motor SM (Forward) --------------------------------------------------//

//-------------------------------------------------- Start Motor SM (Reverse) --------------------------------------------------//

void REVERSE_Init(){
	reverse_state = reverse_off;
}

void REVERSE_Tick(){
	//State Actions
	switch(reverse_state){
		case reverse_off:
			break;
		case reverse_on:
			break;
		default:
			break;
	}
	
	//State Transitions
	switch(reverse_state){
		case reverse_off:
			if(going_reverse){
				reverse_state = reverse_on;
				//PORTD = 0xFF;
				PORTC = 0x02;
				PORTD = 0x02;
			}
			
			else{
				reverse_state = reverse_off;
			}
			break;
			
		case reverse_on:
			if(going_reverse){
				reverse_state = reverse_on;
			}
		
			else{
				reverse_state = reverse_off;
				PORTC = 0x00;
				PORTD = 0x00;
			}
			break;
			
		default:
			break;
	}
}

void ReverseSecTask(){
	REVERSE_Init();
	for(;;){
		REVERSE_Tick();
		vTaskDelay(100);
	}
}

void ReverseSecPulse(unsigned portBASE_TYPE Priority){
	xTaskCreate(ReverseSecTask, (signed portCHAR *)"ReverseSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

//-------------------------------------------------- End Motor SM (Reverse) --------------------------------------------------//

 
int main(void) 
{ 
   DDRA = 0x00; PORTA=0xFF;
   DDRC = 0xFF; PORTC = 0x00;
   DDRD = 0xFF; PORTD = 0x00;

   //Start Tasks  
   ButtonSecPulse(1);
   ForwardSecPulse(1);
   ReverseSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}