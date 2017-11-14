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

// variables for throttle
unsigned char throttle = 0; // ranges 1, 2, or 3
unsigned char reverse = 0;	// ranges 1, 2, or 3
unsigned char going_forward = 0;
unsigned char going_reverse = 0;
unsigned char pwm_counter = 0;
unsigned char low_counter = 10;
unsigned short joystick_value = 0;

// variables for steering
unsigned char right = 0;
unsigned char left = 0;	
unsigned char max_servo = 0;
unsigned char min_servo = 0;
unsigned char servo_counter = 0;
unsigned char needs_centering  = 0;
//unsigned char timeline = 1;	// left 0, center = 1, right = 2
unsigned short steering = 0;

//-------------------------------------------------- Enumeration of States --------------------------------------------------//

enum JOYState {joy_read} joy_state;
enum STEERINGState {steering_read} steering_state;
enum FORWARDState {forward_off, forward_on_high,forward_on_low} forward_state;
enum REVERSEState {reverse_off, reverse_on_high,reverse_on_low} reverse_state;
enum SERVOState {servo_init, drive_high} servo_state;

//-------------------------------------------------- Start Joystick SM --------------------------------------------------//

void JOY_Init(){
	joy_state = joy_read;
}

void JOY_Tick(){
	//Actions
	switch(joy_state){
		case joy_read:
		joystick_value = adc_read(0);
		
		if(580 < joystick_value && joystick_value < 720){
			going_forward = 1;
			throttle = 1;
		}
		
		else if(720 < joystick_value && joystick_value < 860){
			going_forward = 1;
			throttle = 2;
		}
		
		else if(860 < joystick_value){
			going_forward = 1;
			throttle = 3;
		}
		
		else if(334 < joystick_value && joystick_value < 500){
			going_reverse = 1;
			reverse = 1;
		}
		
		else if(167 < joystick_value && joystick_value < 334){
			going_reverse = 1;
			reverse = 2;
		}
		else if(joystick_value < 167){
			going_reverse = 1;
			reverse = 3;
		}
		
		else{
			going_forward = 0;
			going_reverse = 0;
			throttle = 0;
			reverse = 0;
			PORTC = 0x00;
			PORTD = 0x00;
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
		vTaskDelay(2);
	}
}

void JOYSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(JOYSecTask, (signed portCHAR *)"JOYSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}


//-------------------------------------------------- Start Steering SM --------------------------------------------------//
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
			
			//if(0 < timeline){
				right = 0;
				left = 1;
				max_servo = 1;
				//--timeline;
				
				//if(timeline != 1){
				//	needs_centering = 1;
				//}
				
				//else{
				//	needs_centering = 0;
				//}
			//}
			
		}
		
		else if(steering < 500){
			//PORTB = 0x02;
			
			//if(timeline < 2){
				right = 1;
				left = 0;
				max_servo = 2;
				//++timeline;
				
				//if(timeline != 1){
				//	needs_centering = 1;
				//}
				
				//else{
				//	needs_centering = 0;
				//}
			//}
			
		}
		
		else{
			//PORTB = 0x03;
			
			//if(timeline < 1){
			//	right = 1;
			//	left = 0;
			//	max_servo = 2;
			//	++timeline;
			//	needs_centering = 1;
			//}
			
			//else if(1 < timeline){
			//	right = 0;
			//	left = 1;
			//	max_servo = 1;
			//	--timeline;
			//	needs_centering = 1;
			//}
			
			//else{
				right = 0;
				left = 0;
				max_servo = 0;
			//	needs_centering = 0;
			//}
			
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

//-------------------------------------------------- End Steering SM --------------------------------------------------//


//-------------------------------------------------- Start DC Motor SM (Forward) --------------------------------------------------//

void FORWARD_Init(){
	forward_state = forward_off;
}

void FORWARD_Tick(){
	//State Actions
	switch(forward_state){
		case forward_off:
			break;
		case forward_on_high:
			break;
		case forward_on_low:
			break;
		default:
			break;
	}
	
	//State Transitions
	switch(forward_state){
		case forward_off:
			if(going_forward){
				forward_state = forward_on_high;
				low_counter = 3 - throttle;
				pwm_counter = 0;
				PORTC = 0x01;
				PORTD = 0x01;
			}
		
			else{
				forward_state = forward_off;
			}
		
			break;
		
		case forward_on_high:
			if(going_forward && (pwm_counter < throttle)){
				forward_state = forward_on_high;
				++pwm_counter;
			}
			
			else if(going_forward && !(pwm_counter < throttle)){
				forward_state = forward_on_low;
				pwm_counter = 0;
				PORTC = 0x00;
				PORTD = 0x00;
			}
		
			else{
				forward_state = forward_off;
				PORTC = 0x00;
				PORTD = 0x00;
			}
			break;
		
		case forward_on_low:
			if(going_forward && (pwm_counter < low_counter)){
				forward_state = forward_on_low;
				++pwm_counter;
			}
			
			else if(going_forward && !(pwm_counter < low_counter)){
				forward_state = forward_on_high;
				pwm_counter = 0;
				PORTC = 0x01;
				PORTD = 0x01;
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
		vTaskDelay(3);
	}
}

void ForwardSecPulse(unsigned portBASE_TYPE Priority){
	xTaskCreate(ForwardSecTask, (signed portCHAR *)"ForwardSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

//-------------------------------------------------- End DC Motor SM (Forward) --------------------------------------------------//

//-------------------------------------------------- Start DC Motor SM (Reverse) --------------------------------------------------//

void REVERSE_Init(){
	reverse_state = reverse_off;
}

void REVERSE_Tick(){
	//State Actions
	switch(reverse_state){
		case reverse_off:
			break;
		case reverse_on_high:
			break;
		case reverse_on_low:
			break;
		default:
			break;
	}
	
	//State Transitions
	switch(reverse_state){
		case reverse_off:
			if(going_reverse){
				reverse_state = reverse_on_high;
				low_counter = 3 - reverse;
				pwm_counter = 0;
				PORTC = 0x02;
				PORTD = 0x02;
			}
		
			else{
				reverse_state = reverse_off;
			}
			break;
		
		case reverse_on_high:
			if(going_reverse && (pwm_counter < reverse)){
				reverse_state = reverse_on_high;
				++pwm_counter;
			}
		
			else if(going_reverse && !(pwm_counter < reverse)){
				reverse_state = reverse_on_low;
				pwm_counter = 0;
				PORTC = 0x00;
				PORTD = 0x00;
			}
			
			else{
				reverse_state = reverse_off;
				PORTC = 0x00;
				PORTD = 0x00;
			}
			break;
			
		case reverse_on_low:
			if(going_reverse && (pwm_counter < low_counter)){
				reverse_state = reverse_on_low;
				++pwm_counter;
			}
		
			else if(going_reverse && !(pwm_counter < low_counter)){
				reverse_state = reverse_on_high;
				pwm_counter = 0;
				PORTC = 0x02;
				PORTD = 0x02;
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
		vTaskDelay(3);
	}
}

void ReverseSecPulse(unsigned portBASE_TYPE Priority){
	xTaskCreate(ReverseSecTask, (signed portCHAR *)"ReverseSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

//-------------------------------------------------- End DC Motor SM (Reverse) --------------------------------------------------//

//-------------------------------------------------- Start ServoMotor SM --------------------------------------------------//


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

		//case drive_low:
		//break;
		
		default:
		break;
	}
	
	//Transitions
	switch(servo_state){
		case servo_init:
		if(left || right){
			servo_state = drive_high;
			servo_counter = 0;
			//min_servo = 20 - max_servo;
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
		/*
		else if(!(servo_counter < max_servo) && (left || right)){
			servo_counter = 0;
			PORTB = 0x00;
			servo_state = drive_low;
		}
		*/
		else{
			left = 0;
			right = 0;
			PORTB = 0x00;
			servo_state = servo_init;
		}
		break;
		/*
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
		*/
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


//-------------------------------------------------- End ServoMotor SM --------------------------------------------------//
int main(void)
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	adc_init();
	//Start Tasks
	JOYSecPulse(1);
	STEERINGSecPulse(1);
	ForwardSecPulse(1);
	ReverseSecPulse(1);
	SERVOSecPulse(1);
	//RunSchedular
	vTaskStartScheduler();
	
	return 0;
}