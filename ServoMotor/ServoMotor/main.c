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

//code modified from http://maxembedded.com/2011/06/the-adc-of-the-avr/----------------------------
void adc_init()
{
	// AREF = AVcc
	ADMUX = (1<<REFS0);
	
	// ADC Enable and prescaler of 128
	// 16000000/128 = 125000
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

uint16_t adc_read(uint8_t ch)
{
	// select the corresponding channel 0~7
	// ANDing with ’7? will always keep the value
	// of ‘ch’ between 0 and 7
	ch &= 0b00000111;  // AND operation with 7
	ADMUX = (ADMUX & 0xF8)|ch; // clears the bottom 3 bits before ORing
	
	// start single convertion
	// write ’1? to ADSC
	ADCSRA |= (1<<ADSC);
	
	// wait for conversion to complete
	// ADSC becomes ’0? again
	// till then, run loop continuously
	while(ADCSRA & (1<<ADSC));
	
	return (ADC);
}
//end of code modified from http://maxembedded.com/2011/06/the-adc-of-the-avr/---------------------


unsigned char right = 0;	// A0
unsigned char left = 0;		// A2
unsigned char max_servo = 0;
unsigned char min_servo = 0;
unsigned char servo_counter = 0;
unsigned char needs_centering  = 0;
unsigned char timeline = 1;	// left 0, center = 1, right = 2
unsigned short steering = 0;

enum STEERINGState {steering_read} steering_state;

void STEERING_Init(){
	steering_state = steering_read;
}

void STEERING_Tick(){
	//Actions
	switch(steering_state){
		case steering_read:
			steering = adc_read(1);
		
			if(580 < steering){
				//PORTB = 0x01;
				
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
				
			}
		
			else if(steering < 500){
				//PORTB = 0x02;
				
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
				
			}
		
			else{
				//PORTB = 0x03;
				
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
				
			}
		
			break;
		
		default:
			break;
	}
	//Transitions
	switch(steering_state){
		case steering_read:
			steering_state = steering_read;
			break;
		
		default:
			steering_state = steering_read;
			break;
	}
}

void STEERINGSecTask()
{
	STEERING_Init();
	for(;;)
	{
		STEERING_Tick();
		vTaskDelay(15);
	}
}

void STEERINGSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(STEERINGSecTask, (signed portCHAR *)"STEERINGSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
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
				PORTB = 0x01;
			}
		  
			else{
				PORTB = 0x00;
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
				PORTB = 0x00;
				servo_state = drive_low;
			}
		  
			else{
				 left = 0;
				right = 0;
				PORTB = 0x00;
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
				PORTB = 0x00;
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
   DDRB = 0xFF; PORTB = 0x00;
   
   adc_init();
   
   //Start Tasks  
   STEERINGSecPulse(1);
   SERVOSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}