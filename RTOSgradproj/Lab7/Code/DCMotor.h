// PWM.h
// Runs on TM4C123
// Use PWM0A/PB6 and PWM0B/PB7 to generate pulse-width modulated outputs.
// Daniel Valvano
// March 28, 2014

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
   Program 6.8, section 6.3.2

   "Embedded Systems: Real-Time Operating Systems for ARM Cortex M Microcontrollers",
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2015
   Program 8.4, Section 8.3

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// Speed constants
#define DCM_MAX_SPEED  30
#define DCM_SLOW_SPEED 75
#define DCM_MED_SPEED  60
//#define DCM_MAX_SPEED 100
//#define DCM_SLOW_SPEED 30
#define DCM_OFF_SPEED  0
// Direction constants
#define DCM_FORWARD   1
#define DCM_REVERSE   0

#define SOFT_LEFT                     10
#define SOFT_RIGHT                    20
#define STRAIGHT                      14

#define HARD_LEFT                     6
#define HARD_RIGHT                    26 

#include <stdint.h>


// This function should initializes both motors
// by initialization it means that it should initialize the PWM pins in the correct way
void Motor_Init(void);
void Motor_Run(uint8_t motorID, uint8_t speed, uint8_t direction);
void Motor_Direction(uint8_t angle);
enum MotorId{
	leftM,
	rightM
};

enum direction{
	forward,
	reverse,
	left,
	right
};
#define MAXSPEED 64000
#define MAXANGLE 50000
