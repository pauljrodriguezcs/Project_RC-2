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


//FreeRTOS include files 
#include "FreeRTOS.h" 
#include "task.h" 
#include "croutine.h" 

#include "bit.h"

unsigned char throttle = 0; // ranges 1, 2, or 3
unsigned char reverse = 0;	// ranges 1, 2, or 3
unsigned char steering_left = 0;	// 1 is left; 0 is straight
unsigned char steering_right = 0;	// 1 is right; 0 is straight
unsigned char led_value = 0x00;
unsigned short joystick_value = 0;
unsigned short steering = 0;

enum JOYState {joy_read} joy_state;

void JOY_Init(){
	joy_state = joy_read;
}

void JOY_Tick(){
	//Actions
	switch(joy_state){
		case joy_read:
			joystick_value = adc_read(0);
			
			if(580 < joystick_value && joystick_value < 720){
				throttle = 1;
			}
			
			else if(720 < joystick_value && joystick_value < 860){
				throttle = 2;
			}
			
			else if(860 < joystick_value){
				throttle = 3;
			}
			
			else if(334 < joystick_value && joystick_value < 500){
				reverse = 1;
			}
			
			else if(167 < joystick_value && joystick_value < 334){
				reverse = 2;
			}
			else if(joystick_value < 167){
				reverse = 3;
			}
			
			else{
				throttle = 0;
				reverse = 0;
			}
			
			break;
		
		default:
			break;
	}
	//Transitions
	switch(joy_state){
		case joy_read:
			joy_state = joy_read;
		break;
		
		default:
			joy_state = joy_read;
		break;
	}
}

void JOYSecTask()
{
	JOY_Init();
   for(;;) 
   { 	
	JOY_Tick();
	vTaskDelay(15); 
   } 
}

void JOYSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(JOYSecTask, (signed portCHAR *)"JOYSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

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
				steering_left = 1;
			}
		
			else if(steering < 500){
				steering_right = 1;
			}
		
			else{
				steering_left = 0;
				steering_right = 0;
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

enum LEDState {led_output} led_state;

void LED_Init(){
	led_state = led_output;
}

void LED_Tick(){
	//Actions
	switch(led_state){
		case led_output:
			if(throttle == 1){
				led_value = led_value & 0xC0;
				led_value = led_value | 0x04;
				PORTD = led_value;
			}
			
			else if(throttle == 2){
				led_value = led_value & 0xC0;
				led_value = led_value | 0x06;
				PORTD = led_value;

			}
			
			else if(throttle == 3){
				led_value = led_value & 0xC0;
				led_value = led_value | 0x07;
				PORTD = led_value;
			}
			
			else if(reverse == 1){
				led_value = led_value & 0xC0;
				led_value = led_value | 0x08;
				PORTD = led_value;
			}
			
			else if(reverse == 2){
				led_value = led_value & 0xC0;
				led_value = led_value | 0x18;
				PORTD = led_value;
			}
			
			else if(reverse == 3){
				led_value = led_value & 0xC0;
				led_value = led_value | 0x38;
				PORTD = led_value;
			}
			
			else{
				led_value = led_value & 0xC0;
			}
			
			if(steering_left == 1){
				led_value = led_value & 0x3F;
				led_value = led_value | 0x40;
				PORTD = led_value;
			}
			
			else if(steering_right){
				led_value = led_value & 0x3F;
				led_value = led_value | 0x80;
				PORTD = led_value;
			}
			
			else{
				led_value = led_value & 0x3F;
				PORTD = led_value;
			}
			
			break;
		
		default:
			PORTD = 0x00;
			break;
	}
	//Transitions
	switch(led_state){
		case led_output:
			led_state = led_output;
			break;
		
		default:
			led_state = led_output;
			break;
	}
}

void LEDSecTask()
{
	LED_Init();
	for(;;)
	{
		LED_Tick();
		vTaskDelay(15);
	}
}

void LEDSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(LEDSecTask, (signed portCHAR *)"LEDSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}
 
int main(void) 
{ 
   DDRA = 0x00; PORTA=0xFF;
   DDRD = 0xFF; PORTD = 0x00;
   adc_init();
   //Start Tasks  
   JOYSecPulse(1);
   STEERINGSecPulse(1);
   LEDSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}