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

unsigned char throttle = 0; // ranges 1, 2, or 3
unsigned char reverse = 0;	// ranges 1, 2, or 3
unsigned char steering_left = 0;	// 1 is left; 0 is straight
unsigned char steering_right = 0;	// 1 is right; 0 is straight
unsigned char led_value = 0x00;
unsigned short joystick_value = 0;
unsigned short steering = 0;
unsigned char obstacle_front = 0;
unsigned char obstacle_back = 0;
unsigned char obstacle_left = 0;
unsigned char obstacle_right = 0;

//variables for data transmitted
unsigned char bit_val_0 = 0;
unsigned char bit_val_1 = 0;
unsigned char data_recieved = 0x00;

enum TRANSMISSIONState {transmit_wait, transmit_read} transmission_state;
enum JOYState {joy_read} joy_state;
enum STEERINGState {steering_read} steering_state;
enum LEDState {led_output} led_state;
enum IRState {ir_read} ir_state;
	
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
			//going_reverse = 1;
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
			////going_forward = 1;
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
				//going_forward = 0;
				//going_reverse = 0;
				throttle = 0;
				reverse = 0;
			}
		}
		
		bit_val_0 = GetBit(data_recieved,3);	// steering_right
		bit_val_1 = GetBit(data_recieved,4);	// left
		
		if(!bit_val_1 && bit_val_0){
			steering_right = 1;
			steering_left = 0;
			//max_servo = 2;
			
		}
		
		else if(bit_val_1 && !bit_val_0){
			steering_right = 0;
			steering_left = 1;
			//max_servo = 1;
		}
		
		else{
			steering_right = 0;
			steering_left = 0;
			//max_servo = 0;
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

	
//--------------------------------------------- Joystick Start ----------------------------------//

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

//--------------------------------------------- Joystick End ----------------------------------//

//--------------------------------------------- Steering Start ----------------------------------//

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

//--------------------------------------------- Steering End ----------------------------------//

//--------------------------------------------- LED Start ----------------------------------//

void LED_Init(){
	led_state = led_output;
}

void LED_Tick(){
	//Actions
	switch(led_state){
		case led_output:
			if(throttle == 1 && !obstacle_front){
				led_value = led_value & 0xC0;
				led_value = led_value | 0x04;
				PORTC = led_value;
			}
			
			else if(throttle == 2 && !obstacle_front){
				led_value = led_value & 0xC0;
				led_value = led_value | 0x06;
				PORTC = led_value;

			}
			
			else if(throttle == 3 && !obstacle_front){
				led_value = led_value & 0xC0;
				led_value = led_value | 0x07;
				PORTC = led_value;
			}
			
			else if(reverse == 1 && !obstacle_back){
				led_value = led_value & 0xC0;
				led_value = led_value | 0x08;
				PORTC = led_value;
			}
			
			else if(reverse == 2 && !obstacle_back){
				led_value = led_value & 0xC0;
				led_value = led_value | 0x18;
				PORTC = led_value;
			}
			
			else if(reverse == 3 && !obstacle_back){
				led_value = led_value & 0xC0;
				led_value = led_value | 0x38;
				PORTC = led_value;
			}
			
			else{
				led_value = led_value & 0xC0;
			}
			
			if(steering_left == 1 && !obstacle_left){
				led_value = led_value & 0x3F;
				led_value = led_value | 0x40;
				PORTC = led_value;
			}
			
			else if(steering_right && !obstacle_right){
				led_value = led_value & 0x3F;
				led_value = led_value | 0x80;
				PORTC = led_value;
			}
			
			else{
				led_value = led_value & 0x3F;
				PORTC = led_value;
			}
			
			break;
		
		default:
			PORTC = 0x00;
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
 
 //--------------------------------------------- LED End ----------------------------------//
 
 //--------------------------------------------- Sensor Start ----------------------------------//
 void IR_Init(){
	 ir_state = ir_read;
 }

 void IR_Tick(){
	 //Actions
	 switch(ir_state){
		 case ir_read:
			/*
			if(GetBit(~PINA,2)){
				obstacle_front = 1;
			}
		 
			if(!GetBit(~PINA,2)){
				obstacle_front = 0;
			}
			*/
			
			/*
			if(GetBit(~PINA,3)){
				obstacle_back = 1;
			}
		 
			if(!GetBit(~PINA,3)){
				obstacle_back = 0;
			}
			*/
			
			/*
			if(GetBit(~PINA,4)){
				obstacle_left = 1;
			}
		 
			if(!GetBit(~PINA,4)){
				obstacle_left = 0;
			}
			*/
			
			/*
			if(GetBit(~PINA,5)){
				obstacle_right = 1;
			}
		 
			if(!GetBit(~PINA,5)){
				obstacle_right = 0;
			}
			*/
			break;
		 
		 default:
			break;
	 }
	 //Transitions
	 switch(ir_state){
		 case ir_read:
			ir_state = ir_read;
			break;
		 
		 default:
			ir_state = ir_read;
			break;
	 }
 }

 void IRSecTask()
 {
	 IR_Init();
	 for(;;)
	 {
		 IR_Tick();
		 vTaskDelay(10);
	 }
 }

 void IRSecPulse(unsigned portBASE_TYPE Priority)
 {
	 xTaskCreate(IRSecTask, (signed portCHAR *)"IRSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
 }
 
 //--------------------------------------------- Sensor End ----------------------------------//
 
int main(void) 
{ 
   DDRA = 0x00; PORTA=0xFF;
   DDRC = 0xFF; PORTC = 0x00;
   //adc_init();
   initUSART(0);

   //Start Tasks
   TRANSMISSIONSecPulse(1);   
   //JOYSecPulse(1);
   //STEERINGSecPulse(1);
   //IRSecPulse(10);
   LEDSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}