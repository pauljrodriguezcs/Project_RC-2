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


void pwm_init()
{
	// initialize TCCR0 as per requirement, say as follows
	TCCR3A = (1<<COM3A0);
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	OCR3A = (short)(1250);
	
	// make sure to make OC0 pin (pin PB3 for atmega32) as output pin
}

unsigned char right = 0;	// A0
unsigned char center = 0;	// A1
unsigned char left = 0;		// A2

uint8_t duty = 19;

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
			if(GetBit(~PINA,0) && !GetBit(~PINA,1) && !GetBit(~PINA,2)){
				right = 1;
				center = 0;
				left = 0;
				duty = 26;
				button_state = push;
			}
			
			else if(!GetBit(~PINA,0) && GetBit(~PINA,1) && !GetBit(~PINA,2)){
				right = 0;
				center = 1;
				left = 0;
				duty = 19;
				button_state = push;
			}
			
			else if(!GetBit(~PINA,0) && !GetBit(~PINA,1) && GetBit(~PINA,2)){
				right = 0;
				center = 0;
				left = 1;
				duty = 13;
				button_state = push;
			}
			
			else{
				right = 0;
				center = 1;
				left = 0;
				duty = 13;
				button_state = release;
			}
			
			break;
		
		case push:
			if(GetBit(~PINA,0) && !GetBit(~PINA,1) && !GetBit(~PINA,2)){
				button_state = push;
			}
			
			else if(!GetBit(~PINA,0) && GetBit(~PINA,1) && !GetBit(~PINA,2)){
				button_state = push;
			}
			
			else if(!GetBit(~PINA,0) && !GetBit(~PINA,1) && GetBit(~PINA,2)){
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
	vTaskDelay(10); 
   } 
}

void BUTTONSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(BUTTONSecTask, (signed portCHAR *)"BUTTONSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}	
 
 enum SERVOState {drive} servo_state;

 void SERVO_Init(){
	 servo_state = drive;
 }

 void SERVO_Tick(){
	 //Actions
	 switch(servo_state){
		 case drive:
			if(right){
				//OCR0A = 1250;
				PORTB = 0x40;
				PORTD = 0x01;
			}
			
			else if(center){
				//OCR0A = 1250;
				// PORTB = 0x00;
				PORTD = 0x02;
			}
			
			else if(left){
				//OCR0A = duty;
				PORTB = 0x40;
				PORTD = 0x04;
			}
			
			else{
				//OCR0A = duty;
				// PORTB = 0x00;
				PORTD = 0x00;
			}
		 
			break;
		 
		 default:
		 
			break;
	 }
	 //Transitions
	 switch(servo_state){
		 case drive:
			servo_state = drive;
			break;
		 default:
			servo_state = drive;
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
   DDRB = 0xFF; PORTB = 0x00;
   DDRD = 0xFF; PORTD = 0x00;
   
   pwm_init();
   //Start Tasks  
   BUTTONSecPulse(1);
   SERVOSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}