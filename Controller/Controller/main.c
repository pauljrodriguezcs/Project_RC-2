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
#include "usart_ATmega1284.h"

// variables for throttle
unsigned short joystick_value = 0;

// variable for steering
unsigned short steering = 0;

// variable for lights
unsigned char light = 0;

unsigned char data_to_transmit = 0x00;
//	7		6		5		4		3		2			1		0
//	nothing	nothing	lights	left	right	direction	speed	speed
//	
//	lights: 0 = off, 1 = on
//	left: 0 = off, 1 = left
//	right: 0 = off, 1 = right
//	direction: 0 = forward/none, 1 = reverse
//	[1:0]: 01 = 1, 10 = 2, 11 = 3


//-------------------------------------------------- Enumeration of States --------------------------------------------------//

enum JOYState {joy_read} joy_state;
enum STEERINGState {steering_read} steering_state;
enum BUTTONState {button_off, button_on} button_state;
enum TRANSMITState {wait_to_transmit, transmit_data} transmit_state;

//-------------------------------------------------- Start Joystick SM --------------------------------------------------//

void JOY_Init(){
	joy_state = joy_read;
}

void JOY_Tick(){
	//Actions
	switch(joy_state){
		case joy_read:
			joystick_value = adc_read(0);
		
			if(580 < joystick_value && joystick_value < 720){	// sets "going forward" and throttle speed "1"
				data_to_transmit = data_to_transmit & 0xF8;
				data_to_transmit = data_to_transmit | 0x01;
			}
		
			else if(720 < joystick_value && joystick_value < 860){	// sets "going forward" and throttle speed "2"
				data_to_transmit = data_to_transmit & 0xF8;
				data_to_transmit = data_to_transmit | 0x02;
			}
		
			else if(860 < joystick_value){	// sets "going forward" and throttle speed "3"
				data_to_transmit = data_to_transmit & 0xF8;
				data_to_transmit = data_to_transmit | 0x03;
			}
		
			else if(334 < joystick_value && joystick_value < 500){	// sets "going reverse" and throttle speed "1"
				data_to_transmit = data_to_transmit & 0xF8;
				data_to_transmit = data_to_transmit | 0x05;
			}
		
			else if(167 < joystick_value && joystick_value < 334){	// sets "going reverse" and throttle speed "2"
				data_to_transmit = data_to_transmit & 0xF8;
				data_to_transmit = data_to_transmit | 0x06;
			}
			
			else if(joystick_value < 167){	// sets "going reverse" and throttle speed "3"
				data_to_transmit = data_to_transmit & 0xF8;
				data_to_transmit = data_to_transmit | 0x07;
			}
		
			else{	// disables "going forward" and "going reverse" and sets throttle speed "0"
				data_to_transmit = data_to_transmit & 0xF8;
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
				data_to_transmit = data_to_transmit & 0xE7;
				data_to_transmit = data_to_transmit | 0x10;
			}
		
			else if(steering < 500){
				data_to_transmit = data_to_transmit & 0xE7;
				data_to_transmit = data_to_transmit | 0x08;
			}
		
			else{
				data_to_transmit = data_to_transmit & 0xE7;
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

//-------------------------------------------------- Start Button SM --------------------------------------------------//
void BUTTON_Init(){
	button_state = button_off;
}

void BUTTON_Tick(){
	//Transitions
	switch(button_state){
		case button_off:
			if(GetBit(~PINB, 1)){
				if(light){
					light = 0;
					data_to_transmit = data_to_transmit & 0xDF;
				}
				
				else{
					light = 1;
					data_to_transmit = data_to_transmit & 0xDF;
					data_to_transmit = data_to_transmit | 0x20;
				}
				
				if(light){
					PORTC = 0xFF;
				}
				
				else{
					PORTC = 0x00;
				}
				
				button_state = button_on;
			}
			
			else{
				
				button_state = button_off;
			}
			
			break;
		
		case button_on:
			if(GetBit(~PINB, 1)){
				button_state = button_on;
			}
			
			else{
				
				button_state = button_off;
			}
			
			break;
		
		default:
			button_state = button_off;
			break;
	}
	
	//Actions
	switch(button_state){
		case button_off:
			break;
		
		case button_on:
			break;
		
		default:
			break;
	}
	
}



void BUTTONSecTask()
{
	BUTTON_Init();
	for(;;)
	{
		BUTTON_Tick();
		vTaskDelay(15);
	}
}

void BUTTONSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(BUTTONSecTask, (signed portCHAR *)"BUTTONSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

//-------------------------------------------------- End Button SM --------------------------------------------------//

//-------------------------------------------------- Start Transmission SM --------------------------------------------------//

void TRANSMIT_Init(){
	transmit_state = wait_to_transmit;
}

void TRANSMIT_Tick(){
	//transitions
	switch(transmit_state){
		case wait_to_transmit:
			if(USART_IsSendReady(0)){
				transmit_state = transmit_data;
			}
			
			else{
				transmit_state = wait_to_transmit;
			}
			break;
			
		case transmit_data:
			if(USART_HasTransmitted(0)){
				transmit_state = wait_to_transmit;
			}
			else{
				transmit_state = transmit_data;
			}
			break;
		
		default:
			transmit_state = wait_to_transmit;
			break;
	}
	
	//actions
	switch(transmit_state){
		case wait_to_transmit:
			break;
		
		case transmit_data:
			USART_Send(data_to_transmit,0);
			break;
		
		default:
			break;
	}
}
	
void TRANSMITSecTask(){
	TRANSMIT_Init();
	for(;;)
	{
		TRANSMIT_Tick();
		vTaskDelay(5);
	}
}
	
void TRANSMITSecPulse(unsigned portBASE_TYPE Priority){
	xTaskCreate(TRANSMITSecTask, (signed portCHAR *)"TRANSMITSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}
	
	

//-------------------------------------------------- End Transmission SM --------------------------------------------------//


int main(void)
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0x00; PORTB = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	//DDRD = 0xFF; PORTD = 0x00;
	adc_init();
	initUSART(0);
	//Start Tasks
	JOYSecPulse(1);
	STEERINGSecPulse(1);
	BUTTONSecPulse(1);
	TRANSMITSecPulse(1);
	//RunSchedular
	vTaskStartScheduler();
	
	return 0;
}