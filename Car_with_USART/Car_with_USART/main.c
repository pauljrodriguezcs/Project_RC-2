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

#include "usart_ATmega1284.h"
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
unsigned short steering = 0;
unsigned char b_output = 0x00;

// variables for lights
unsigned char lights = 0; //0 = lights are currently off; 1 = lights are currently on


//variables for data transmitted
unsigned char bit_val_0 = 0;
unsigned char bit_val_1 = 0;
unsigned char data_recieved = 0x00;
//	7		6		5		4		3		2			1		0
//	nothing	nothing	lights	left	right	direction	speed	speed
//
//	lights: 0 = off, 1 = on
//	left: 0 = off, 1 = left
//	right: 0 = off, 1 = right
//	direction: 0 = forward/none, 1 = reverse
//	[1:0]: 01 = 1, 10 = 2, 11 = 3

//-------------------------------------------------- Enumeration of States --------------------------------------------------//

enum TRANSMISSIONState {transmit_wait, transmit_read} transmission_state;
enum IRState {ir_read} ir_state;
enum FORWARDState {forward_off, forward_on_high,forward_on_low} forward_state;
enum REVERSEState {reverse_off, reverse_on_high,reverse_on_low} reverse_state;
enum SERVOState {servo_init, drive_high} servo_state;

//-------------------------------------------------- Start Transmission Read SM --------------------------------------------------//

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
			
			bit_val_0 = GetBit(data_recieved,2);	// direction bit
			
			if(bit_val_0){	//reverse
				if(!GetBit(~PINA,1)){
					going_reverse = 1;
				}
				
				else{
					going_reverse = 0;
				}
				bit_val_0 = GetBit(data_recieved,0);	// speed bit 0 
				bit_val_1 = GetBit(data_recieved,1);	// speed bit 1
				
				if(!bit_val_1 && bit_val_0){	//01
					reverse = 1;
				}
				
				else if(bit_val_1 && !bit_val_0){	//10
					reverse = 2;
				}
				
				else if(bit_val_1 && bit_val_0){	//11
					reverse = 3;
				}
				
				else {	//00
					reverse = 0;
				}
			}
			
			else{	//forward or none
				if(!GetBit(~PINA,0)){
					going_forward = 1;
				}
				
				else{
					going_forward = 0;
				}
				
				bit_val_0 = GetBit(data_recieved,0);	// speed bit 0
				bit_val_1 = GetBit(data_recieved,1);	// speed bit 1
				
				if(!bit_val_1 && bit_val_0){	//01
					throttle = 1;
				}
				
				else if(bit_val_1 && !bit_val_0){	//10
					throttle = 2;
				}
				
				else if(bit_val_1 && bit_val_0){	//11
					throttle = 3;

				}
				
				else {	//00
					going_forward = 0;
					going_reverse = 0;
					throttle = 0;
					reverse = 0;
				}
			}
			
			bit_val_0 = GetBit(data_recieved,3);	// right
			bit_val_1 = GetBit(data_recieved,4);	// left
			
			if(!bit_val_1 && bit_val_0){		// turns right
				if(!GetBit(~PINA,3)){
					right = 1;
					left = 0;
					max_servo = 2;
				}				
			}
			
			else if(bit_val_1 && !bit_val_0){	// turns left
				if(!GetBit(~PINA,2)){
					right = 0;
					left = 1;
					max_servo = 1;
				}
			}
			
			else{
				right = 0;
				left = 0;
				max_servo = 0;
			}
	
			bit_val_0 = GetBit(data_recieved,5);	// lights
			
			if(bit_val_0){
				if(!lights){
					lights = 1;
					b_output = b_output & 0xEF;
					b_output = b_output | 0x10;
					PORTB = b_output;
				}
		
			}
			
			else{
				if(lights){
					lights = 0;
					b_output = b_output & 0xEF;
					PORTB = b_output;
				}
			}
			
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
	

//-------------------------------------------------- End Transmission Read SM --------------------------------------------------//

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
				PORTC = 0x11;
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
			}
		
			else{
				forward_state = forward_off;
				PORTC = 0x00;
			
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
				PORTC = 0x11;
			}
			
			else{
				forward_state = forward_off;
				PORTC = 0x00;
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
				PORTC = 0x22;
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
			}
			
			else{
				reverse_state = reverse_off;
				PORTC = 0x00;
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
				PORTC = 0x22;
			}
		
			else{
				reverse_state = reverse_off;
				PORTC = 0x00;
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
		
		default:
			break;
	}
	
	//Transitions
	switch(servo_state){
		case servo_init:
			if(left || right){
				servo_state = drive_high;
				servo_counter = 0;
				b_output = b_output & 0xFE;
				b_output = b_output | 0x01;
				PORTB = b_output;
			}
		
			else{
				b_output = b_output & 0xFE;
				PORTB = b_output;
				servo_state = servo_init;
			}
			break;
		
		case drive_high:
			if((servo_counter < max_servo) && (left || right)){
				++servo_counter;
				servo_state = drive_high;
			}
		
			else{
				left = 0;
				right = 0;
				b_output = b_output & 0xFE;
				PORTB = b_output;
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

//-------------------------------------------------- End ServoMotor SM --------------------------------------------------//

int main(void)
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	//DDRD = 0xFF; PORTD = 0x00;
	initUSART(0);
	//Start Tasks
	TRANSMISSIONSecPulse(1);
	ForwardSecPulse(1);
	ReverseSecPulse(1);
	SERVOSecPulse(1);
	//RunSchedular
	vTaskStartScheduler();
	
	return 0;
}