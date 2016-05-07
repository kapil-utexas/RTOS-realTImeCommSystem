// Heartbeat.c
// Allen Wang and Alvin Tung
// Library used for LED toggles.
#include "Heartbeat.h"
#include "../inc/tm4c123gh6pm.h"
#include <stdint.h>

#define PF1 (*((volatile unsigned long *)0x40025008))
#define PF2 (*((volatile unsigned long *)0x40025010))
#define PF4 (*((volatile unsigned long *)0x40025040))

//************* Heartbeat_Init *******************
// Heartbeat initializations. Uses PF2.
// Input - none
// Output - none
void HeartBeat_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x20;       // activate port F
	int delay = SYSCTL_RCGCGPIO_R; 
	if(delay){}
	GPIO_PORTF_DIR_R |= 0x00000014;  // make PF2 out
	GPIO_PORTF_AFSEL_R &= ~(0x00000014);// disable alt funct on PF2 
	GPIO_PORTF_DEN_R |= 0x00000014;   // enable digital I/O on PF2
															// configure PF2 as GPIO (default setting)
	GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&(0xFFFFFFFF) & ~(0xF0F00))+0x00000000;  // TODO - is this right?
	GPIO_PORTF_AMSEL_R &= ~(0x00000014);// disable analog functionality on PE4 (default setting)
}

//************* Heartbeat_Toggle *******************
// Toggles PF2
// Input - none
// Output - none
void HeartBeat_Toggle(void){
	PF2 ^= 0x04;
	PF4 ^= 0x10;
}

//************* Hardbeat_Init *******************
// Hardbeat initializations. Uses PF1.
// Input - none
// Output - none
void HardBeat_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x20;       // activate port F
	int delay = SYSCTL_RCGCGPIO_R; 
	if(delay){}
	GPIO_PORTF_DIR_R |= 0x00000002;  // make PF1 out
	GPIO_PORTF_AFSEL_R &= ~(0x00000002);// disable alt funct on PF1 
	GPIO_PORTF_DEN_R |= 0x00000002;   // enable digital I/O on PF1
															// configure PF2 as GPIO (default setting)
	GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&(0xFFFFFFFF) & ~(0x000F0))+0x00000000;  // TODO - is this right?
	GPIO_PORTF_AMSEL_R &= ~(0x00000002);// disable analog functionality on PE1 (default setting)
}

//************* Hardbeat_Toggle *******************
// Toggles PF1
// Input - none
// Output - none
void HardBeat_Toggle(void){
	PF1 ^= 0x02;
}
