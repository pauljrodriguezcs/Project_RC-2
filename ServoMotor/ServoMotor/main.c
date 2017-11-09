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

unsigned char right = 0;	// A0
//unsigned char center = 0;	// A1
unsigned char left = 0;		// A2
unsigned char max_servo = 0;
unsigned char min_servo = 0;
unsigned char servo_counter = 0;
unsigned char needs_centering  = 0;
unsigned char timeline = 1;	// left 0, center = 1, right = 2

enum BUTTONState {release,push} button_state;

void BUTTON_Init(){
	button_state = release;
}

void BUTTON_Tick(){
	//Actions
	switch(button_state){
		case release:
			break;
		case push:
			break;
			
		default:
			break;
	}
	//Transitions
	switch(button_state){
		case release:
			if(GetBit(~PINA,0) && !GetBit(~PINA,2)){
				if(timeline < 2){
					right = 1;
					left = 0;
					max_servo = 2;
					++timeline;
										
					if(timeline != 1){
						needs_centering = 1;
					}
					
					else{
						needs_centering = 0;
					}
					
					button_state = push;
				}
			}
			
			else if(!GetBit(~PINA,0) && GetBit(~PINA,2)){
				if(0 < timeline){
					right = 0;
					left = 1;
					max_servo = 1;
					--timeline;
					
					if(timeline != 1){
						needs_centering = 1;
					}
					
					else{
						needs_centering = 0;
					}
					button_state = push;
				}
			}
			
			else{
				if(timeline < 1){
					right = 1;
					left = 0;
					max_servo = 2;
					++timeline;
					needs_centering = 1;
				}
				
				else if(1 < timeline){
					right = 0;
					left = 1;
					max_servo = 1;
					--timeline;
					needs_centering = 1;
				}
				
				else{
					right = 0;
					left = 0;
					max_servo = 0;
					needs_centering = 0;
				}
				
				button_state = release;
			}
			
			break;
		
		case push:
			if(GetBit(~PINA,0) && !GetBit(~PINA,2)){
				if(timeline < 2){
					right = 1;
					left = 0;
					max_servo = 2;
					++timeline;
					
					if(timeline != 1){
						needs_centering = 1;
					}
					
					else{
						needs_centering = 0;
					}
				}
				
				button_state = push;
			}
	
			else if(!GetBit(~PINA,0) && GetBit(~PINA,2)){
				if(0 < timeline){
					right = 0;
					left = 1;
					max_servo = 1;
					--timeline;
					
					if(timeline != 1){
						needs_centering = 1;
					}
					
					else{
						needs_centering = 0;
					}
					
				}
				button_state = push;
			}
		
			else{
				button_state = release;
			}
			break;
		
		default:
			button_state = release;
			break;
	}
}

void BUTTONSecTask()
{
	BUTTON_Init();
   for(;;) 
   { 	
	BUTTON_Tick();
	vTaskDelay(300); 
   } 
}

void BUTTONSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(BUTTONSecTask, (signed portCHAR *)"BUTTONSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}	
 
 enum SERVOState {servo_init,drive_low, drive_high} servo_state;

 void SERVO_Init(){
	 servo_state = servo_init;
 }

void SERVO_Tick(){
	//Actions
	switch(servo_state){
		case servo_init:
			break;
		  
		case drive_high:
			break;

		case drive_low:
			break;
		  
		default:
			break;
	}
	
	//Transitions
	switch(servo_state){
		case servo_init:
			if(left || right){
				servo_state = drive_high;
				servo_counter = 0;
				min_servo = 20 - max_servo;
				PORTD = 0x01;
			}
		  
			else{
				PORTD = 0x00;
				servo_state = servo_init;
			}
			break;
		case drive_high:
			if((servo_counter < max_servo) && (left || right)){
				 ++servo_counter;
				servo_state = drive_high;
			}
		  
			else if(!(servo_counter < max_servo) && (left || right)){
				servo_counter = 0;
				PORTD = 0x00;
				servo_state = drive_low;
			}
		  
			else{
				 left = 0;
				right = 0;
				PORTD = 0x00;
				servo_state = servo_init;
			}
			break;
		  
		case drive_low:
			if((servo_counter < min_servo) && (left || right)){
				++servo_counter;
				servo_state = drive_low;
			}
		  
			 else{
				left = 0;
				right = 0;
				PORTD = 0x00;
				servo_state = servo_init;
			}
		  
			break;
		  
		default:
			servo_state = servo_init;
			break;
	  }
  }


 void SERVOSecTask()
 {
	 SERVO_Init();
	 for(;;)
	 {
		 SERVO_Tick();
		 vTaskDelay(1);
	 }
 }

 void SERVOSecPulse(unsigned portBASE_TYPE Priority)
 {
	 xTaskCreate(SERVOSecTask, (signed portCHAR *)"SERVOSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
 }
 
int main(void) 
{ 
   DDRA = 0x00; PORTA = 0xFF;
   DDRD = 0xFF; PORTD = 0x00;
   
   //Start Tasks  
   BUTTONSecPulse(1);
   SERVOSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}