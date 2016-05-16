// DCMotor.c
// Runs on LM4F120 or TM4C123
// Use SysTick interrupts to implement a software PWM to drive
// a DC motor at a given duty cycle.  The built-in button SW1
// increases the speed, and SW2 decreases the speed.
// Daniel Valvano, Jonathan Valvano
// August 6, 2013

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013
 Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// PA5 connected to DC motor interface

#include "PLL.h"
#include "PWM.h"
#include "DCMotor.h"

#define GPIO_PORTA_DATA_R       (*((volatile unsigned long *)0x400043FC))
#define GPIO_PORTA_DIR_R        (*((volatile unsigned long *)0x40004400))
#define GPIO_PORTA_AFSEL_R      (*((volatile unsigned long *)0x40004420))
#define GPIO_PORTA_DR8R_R       (*((volatile unsigned long *)0x40004508))
#define GPIO_PORTA_DEN_R        (*((volatile unsigned long *)0x4000451C))
#define GPIO_PORTA_AMSEL_R      (*((volatile unsigned long *)0x40004528))
#define GPIO_PORTA_PCTL_R       (*((volatile unsigned long *)0x4000452C))
#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_IS_R         (*((volatile unsigned long *)0x40025404))
#define GPIO_PORTF_IBE_R        (*((volatile unsigned long *)0x40025408))
#define GPIO_PORTF_IEV_R        (*((volatile unsigned long *)0x4002540C))
#define GPIO_PORTF_IM_R         (*((volatile unsigned long *)0x40025410))
#define GPIO_PORTF_RIS_R        (*((volatile unsigned long *)0x40025414))
#define GPIO_PORTF_ICR_R        (*((volatile unsigned long *)0x4002541C))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_PUR_R        (*((volatile unsigned long *)0x40025510))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_LOCK_R       (*((volatile unsigned long *)0x40025520))
#define GPIO_LOCK_KEY           0x4C4F434B  // Unlocks the GPIO_CR register
#define GPIO_PORTF_CR_R         (*((volatile unsigned long *)0x40025524))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
#define NVIC_EN0_R              (*((volatile unsigned long *)0xE000E100))
#define NVIC_PRI7_R             (*((volatile unsigned long *)0xE000E41C))
#define NVIC_SYS_PRI3_R         (*((volatile unsigned long *)0xE000ED20))


unsigned long H,L;
void Motor_Init(void){ // this should initialize both motors
 // SYSCTL_RCGC2_R |= 0x00000001; // activate clock for port A
  //H = L = 40000;                // 50%
  //GPIO_PORTA_AMSEL_R &= ~0x20;      // disable analog functionality on PA5
  //GPIO_PORTA_PCTL_R &= ~0x00F00000; // configure PA5 as GPIO
  //GPIO_PORTA_DIR_R |= 0x20;     // make PA5 out
  //GPIO_PORTA_DR8R_R |= 0x20;    // enable 8 mA drive on PA5
  //GPIO_PORTA_AFSEL_R &= ~0x20;  // disable alt funct on PA5
  //GPIO_PORTA_DEN_R |= 0x20;     // enable digital I/O on PA5
  //GPIO_PORTA_DATA_R &= ~0x20;   // make PA5 low
  //NVIC_ST_CTRL_R = 0;           // disable SysTick during setup
  //NVIC_ST_RELOAD_R = L-1;       // reload value for 500us
  //NVIC_ST_CURRENT_R = 0;        // any write to current clears it
  //NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
  //NVIC_ST_CTRL_R = 0x00000007;  // enable with core clock and interrupts
	PWM0A_Init(MAXSPEED, 0); //PB6
	PWM0B_Init(MAXSPEED, 0); //PB7
	
	// Output on PB6/M0PWM0
	PWM0_1A_Init(MAXSPEED, 0);  //PB4       // initialize PWM0, 0 Hz, 0% duty
  //Output on PB7/M0PWM1
	PWM0_1B_Init(MAXSPEED, 0);   //PB5       // initialize PWM1, 1000 Hz, 75% duty
	
	PWM0_3A_Init(MAXANGLE,0);  //PD0
	//PWM0_3B_Init(10000,0 );  //PD1
}

//motorID
void Motor_Run(uint8_t motorID, uint8_t speed, uint8_t direction){
	//use motor ID later on, right now it is just a placeholder
	//Right now both motor are controlled at once, need to know whether we need individual motor control
	uint16_t duty = (MAXSPEED*speed)/100 ;
	if(direction == forward) {
    PWM0B_Duty(MAXSPEED);//PB7
    PWM0_1A_Duty(MAXSPEED);//PB4

    PWM0A_Duty(MAXSPEED - duty);//PB6
    PWM0_1B_Duty(MAXSPEED - duty);//PB5
    //	PWM0A_Duty(duty);//PB6
    //  PWM0_1B_Duty(duty);//PB5
	} else if(direction == reverse){ 	
    PWM0A_Duty(0); //PB6
    PWM0_1B_Duty(0);//PB5
      
    PWM0B_Duty(MAXSPEED - duty);//PB7
    PWM0_1A_Duty(MAXSPEED - duty); //PB4
	}
}	

#define TOTALPERCENTAGE 200 // large the percentage the finer the turning will be
void Motor_Direction(uint8_t angle){//for the servo
	uint16_t duty =  (MAXANGLE*angle)/TOTALPERCENTAGE;
	PWM0_3A_Duty(duty); //PD0
	//PWM0_3B_Duty(duty); //PD1
}
