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
#include "usart_ATmega1284.h"

unsigned char data_recieved = 0x00;

enum TRANSMISSIONState {transmit_wait, transmit_read} transmission_state;


void TRANSMISSION_Init(){
	transmission_state = transmit_wait;
}

void TRANSMISSION_Tick(){
	//state actions
	switch(transmission_state){
		case transmit_wait:
			break;
		
		case transmit_read:
			data_recieved = USART_Receive(0); //store received value
			USART_Flush(0); //delete received val from register
			PORTC = data_recieved;
			break;
		
		default:
			break;
	}
	
	//state transitions
	switch(transmission_state){
		case transmit_wait:
			if(USART_HasReceived(0)){
				transmission_state = transmit_read;
			}
			else{
				transmission_state = transmit_wait;
			}
			break;
		
		case transmit_read:
			transmission_state = transmit_wait;
			break;
		
		default:
			transmission_state = transmit_wait;
			break;
	}
}

void TRANSMISSIONSecTask(){
	TRANSMISSION_Init();
	for(;;){
		TRANSMISSION_Tick();
		vTaskDelay(3);
	}
}

void TRANSMISSIONSecPulse(unsigned portBASE_TYPE Priority){
	xTaskCreate(TRANSMISSIONSecTask, (signed portCHAR *)"TRANSMISSIONSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

	
 
int main(void) 
{ 
   DDRC = 0xFF; PORTC = 0x00;
   initUSART(0);
   //Start Tasks
   TRANSMISSIONSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}