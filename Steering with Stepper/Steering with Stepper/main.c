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

unsigned char phases[8] = {0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08,  0x09};
unsigned char orientation = 2; //0 = forward, 1 = backward, 2 = do nothing
unsigned char p_index = 0;
uint16_t axle = 499;
unsigned char needs_centering = 0;


enum MotorState {m_init} motor_state;
enum ButtonState {b_release, b_push} button_state;

void Motor_Init(){
	motor_state = m_init;
}

void Motor_Tick(){
	//Actions
	switch(motor_state){
		case m_init:
		if((orientation == 0) && (axle < 998)){
			++axle;
			
			uint16_t old_axle = eeprom_read_word((uint16_t*)6);
			if(old_axle != axle){
				eeprom_update_word((uint16_t*)6, axle);
			}

			if(axle != 499){
				needs_centering = 1;
			}
			
			else{
				needs_centering = 0;
			}
			
			PORTD = phases[p_index];
			
			if(p_index == 7){
				p_index = 0;
			}
			
			else{
				++p_index;
			}
			
		}
		
		else if((orientation == 1) && (0 < axle)){
			--axle;
			
			uint16_t old_axle = eeprom_read_word((uint16_t*)6);
			if(old_axle != axle){
				eeprom_update_word((uint16_t*)6, axle);
			}

			if(axle != 499){
				needs_centering = 1;
			}
			
			else{
				needs_centering = 0;
			}
			
			PORTD = phases[p_index];
			
			if(p_index == 0){
				p_index = 7;
			}
			
			else{
				--p_index;
			}
		}
		
		break;
		
		default:
		break;
	}
	//Transitions
	switch(motor_state){
		case m_init:
		motor_state = m_init;
		break;
		
		default:
		motor_state = m_init;
		break;
	}
}

void MotorSecTask()
{
	Motor_Init();
	for(;;)
	{
		Motor_Tick();
		vTaskDelay(3);
	}
}

void MotorSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(MotorSecTask, (signed portCHAR *)"MotorSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}

void Button_Init(){
	button_state = b_release;
}

void Button_Tick(){
	//Transitions
	switch(button_state){
		case b_release:
		if((GetBit(~PINA, 0) == 1) && !(GetBit(~PINA, 1) == 1)){
			button_state = b_push;
			PORTC = 0x01;
			orientation = 0;
		}
		
		else if((GetBit(~PINA, 1) == 1) && !(GetBit(~PINA, 0) == 1)){
			button_state = b_push;
			PORTC = 0x02;
			orientation = 1;
		}
		
		else{
			button_state = b_release;
			PORTC = 0x00;
			
			if(needs_centering){
				if(axle < 499){
					orientation = 0;
				}
				
				else{
					orientation = 1;
				}
				
			}
			
			else{
				orientation = 2;
			}
			
		}
		
		break;
		
		case b_push:
		
		if((GetBit(~PINA, 0) == 1) && !(GetBit(~PINA, 1) == 1)){
			button_state = b_push;
			PORTC = 0x01;
		}
		
		else if((GetBit(~PINA, 1) == 1) && !(GetBit(~PINA, 0) == 1)){
			button_state = b_push;
			PORTC = 0x02;

		}
		
		else{
			button_state = b_release;
			PORTC = 0x00;
			orientation = 2;
		}
		
		break;
		
		default:
		button_state = b_release;
		break;
	}
	
	//Actions
	switch(button_state){
		case b_release:
		break;
		case b_push:
		break;
		default:
		break;
	}
	
	
}

void ButtonSecTask(){
	
	Button_Init();
	for(;;){
		Button_Tick();
		vTaskDelay(10);
	}
}

void ButtonSecPulse(unsigned portBASE_TYPE Priority){
	xTaskCreate(ButtonSecTask, (signed portCHAR *)"ButtonSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}


int main(void)
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	//eeprom_write_word((uint16_t*)6,499);
	axle = eeprom_read_word((uint16_t*)6);
	
	if(axle != 499){
		needs_centering = 1;
	}
	
	else{
		needs_centering = 0;
	}
	
	//Start Tasks
	MotorSecPulse(1);
	ButtonSecPulse(1);
	//RunSchedular
	vTaskStartScheduler();
	
	return 0;
}