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

//Extra Files
#include "bit.h"

enum IRState {ir_read} ir_state;
enum LEDState {led_output} led_state;
	
	
unsigned char front = 0;
unsigned char back = 0;
unsigned char left = 0;
unsigned char right = 0;
unsigned char led_value = 0x00;

void IR_Init(){
	ir_state = ir_read;
}

void IR_Tick(){
	//Actions
	switch(ir_state){
		case ir_read:
			if(GetBit(~PINA,0)){
				front = 1;
			}
			
			if(!GetBit(~PINA,0)){
				front = 0;
			}
			
			if(GetBit(~PINA,1)){
				back = 1;
			}
			
			if(!GetBit(~PINA,1)){
				back = 0;
			}
			
			if(GetBit(~PINA,2)){
				left = 1;
			}
			
			if(!GetBit(~PINA,2)){
				left = 0;
			}
			
			if(GetBit(~PINA,3)){
				right = 1;
			}
			
			if(!GetBit(~PINA,3)){
				right = 0;
			}
			
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

void LED_Init(){
	led_state = led_output;
}

void LED_Tick(){
	//Actions
	switch(led_state){
		case led_output:
			if(front){
				led_value = led_value | 0x01;
				PORTD = led_value;
			}
			
			if(!front){
				led_value = led_value & 0xFE;
				PORTD = led_value;
			}
			
			if(back){
				led_value = led_value | 0x02;
				PORTD = led_value;
			}
			
			if(!back){
				led_value = led_value & 0xFD;
				PORTD = led_value;
			}
			
			if(left){
				led_value = led_value | 0x04;
				PORTD = led_value;
			}
			
			if(!left){
				led_value = led_value & 0xFB;
				PORTD = led_value;
			}
			
			if(right){
				led_value = led_value | 0x08;
				PORTD = led_value;
			}
			
			if(!right){
				led_value = led_value & 0xF7;
				PORTD = led_value;
			}
			
			break;
		
		default:
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
		vTaskDelay(10);
	}
}

void LEDSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(LEDSecTask, (signed portCHAR *)"LEDSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}	
 
int main(void) 
{ 
   DDRA = 0x00; PORTA=0xFF;
   DDRC = 0xFF; PORTC = 0x00;
   DDRD = 0xFF; PORTD = 0x00;
   //Start Tasks  
   IRSecPulse(1);
   LEDSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}