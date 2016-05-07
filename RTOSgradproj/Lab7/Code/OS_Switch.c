// OS_Switch.c
// Runs on LM4F120/TM4C123
// Provide functions that initialize a GPIO as an input pin and
// allow reading of two negative logic switches on PF0 and PF4
// and an external switch on PA5.
// Use bit-banded I/O.
// Daniel and Jonathan Valvano
// September 12, 2013

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2015
   Section 4.2    Program 4.2

  "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
   Example 2.3, Program 2.9, Figure 2.36

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

// negative logic switches connected to PF0 and PF4 on the Launchpad
// red LED connected to PF1 on the Launchpad
// blue LED connected to PF2 on the Launchpad
// green LED connected to PF3 on the Launchpad
// NOTE: The NMI (non-maskable interrupt) is on PF0.  That means that
// the Alternate Function Select, Pull-Up Resistor, Pull-Down Resistor,
// and Digital Enable are all locked for PF0 until a value of 0x4C4F434B
// is written to the Port F GPIO Lock Register.  After Port F is
// unlocked, bit 0 of the Port F GPIO Commit Register must be set to
// allow access to PF0's control registers.  On the LM4F120, the other
// bits of the Port F GPIO Commit Register are hard-wired to 1, meaning
// that the rest of Port F can always be freely re-configured at any
// time.  Requiring this procedure makes it unlikely to accidentally
// re-configure the JTAG and NMI pins as GPIO, which can lock the
// debugger out of the processor and make it permanently unable to be
// debugged or re-programmed.
#include "OS_Switch.h"
#include "OS.h"
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "Ping.h"

#define GPIO_LOCK_KEY           0x4C4F434B  // Unlocks the GPIO_CR register
#define PF0                     (*((volatile uint32_t *)0x40025004))
#define PF4                     (*((volatile uint32_t *)0x40025040))
#define SWITCHES                (*((volatile uint32_t *)0x40025044))
//#define SW1       0x10                      // on the left side of the Launchpad board
#define SW2       0x01                      // on the right side of the Launchpad board
#define SYSCTL_RCGC2_GPIOF      0x00000020  // port F Clock Gating Control

void OS_DisableInterrupts(void); // Disable interrupts
void OS_EnableInterrupts(void);  // Enable interrupts

long OS_StartCritical(void);
void OS_EndCritical(long status);
void (*SW1Task)(void);   // user function
void (*SW2Task)(void);
//void static DebounceTaskSW1(void);
void static DebounceTaskSW2(void);

unsigned long LastSW1;
unsigned long LastSW2;

unsigned long SW1Priority;
unsigned long SW2Priority;
//------------OS_SwitchInit------------
// Initialize GPIO Port F for negative logic switches on PF0 and
// PF4 as the Launchpad is wired.  Weak internal pull-up
// resistors are enabled, and the NMI functionality on PF0 is
// disabled.
// Input: task, priority 
// Output: none
void OS_SwitchInit(){ volatile long delay;
  long status = OS_StartCritical();
  SYSCTL_RCGCGPIO_R |= 0x20;     // 1) activate Port F
  delay = SYSCTL_RCGCGPIO_R;     // ready?
                                 // 2a) unlock GPIO Port F Commit Register
  GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;
//  GPIO_PORTF_CR_R |= (SW1|SW2);  // 2b) enable commit for PF4 and PF0
//                                 // 3) disable analog functionality on PF4 and PF0
//  GPIO_PORTF_AMSEL_R &= ~(SW1|SW2);
//                                 // 4) configure PF0 and PF4 as GPIO
//  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFF0FFF0)+0x00000000;
//  GPIO_PORTF_DIR_R &= ~(SW1|SW2);// 5) make PF0 and PF4 in (built-in buttons)
//                                 // 6) disable alt funct on PF0 and PF4
//  GPIO_PORTF_AFSEL_R &= ~(SW1|SW2);
////  delay = SYSCTL_RCGC2_R;        // put a delay here if you are seeing erroneous NMI
//  GPIO_PORTF_PUR_R |= (SW1|SW2); // enable weak pull-up on PF0 and PF4
//  GPIO_PORTF_DEN_R |= (SW1|SW2); // 7) enable digital I/O on PF0 and PF4
//  // Edge Triggered Interrupt Init()
//  GPIO_PORTF_IS_R &= ~(SW1|SW2);   // SW1|SW2 is edge-sensitive 
//  GPIO_PORTF_IBE_R |= (SW1|SW2);   // SW1|SW2 is both edges 
//  //GPIO_PORTF_IEV_R |= (SW1|SW2);  // SW1|SW2 both edge event
//  GPIO_PORTF_ICR_R = (SW1|SW2);    // clear flags
//  GPIO_PORTF_IM_R |= (SW1|SW2);    // enable interrupt on SW1|SW2
//                                   // GPIO PortF=priority 2

  GPIO_PORTF_CR_R |= (SW2);  // 2b) enable commit for PF4 and PF0
                                 // 3) disable analog functionality on PF4 and PF0
  GPIO_PORTF_AMSEL_R &= ~(SW2);
                                 // 4) configure PF0 and PF4 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFF0FFF0)+0x00000000;
  GPIO_PORTF_DIR_R &= ~(SW2);// 5) make PF0 and PF4 in (built-in buttons)
                                 // 6) disable alt funct on PF0 and PF4
  GPIO_PORTF_AFSEL_R &= ~(SW2);
//  delay = SYSCTL_RCGC2_R;        // put a delay here if you are seeing erroneous NMI
  GPIO_PORTF_PUR_R |= (SW2); // enable weak pull-up on PF0 and PF4
  GPIO_PORTF_DEN_R |= (SW2); // 7) enable digital I/O on PF0 and PF4
  // Edge Triggered Interrupt Init()
  GPIO_PORTF_IS_R &= ~(SW2);   // SW1|SW2 is edge-sensitive 
  GPIO_PORTF_IBE_R |= (SW2);   // SW1|SW2 is both edges 
  //GPIO_PORTF_IEV_R |= (SW1|SW2);  // SW1|SW2 both edge event
  GPIO_PORTF_ICR_R = (SW2);    // clear flags
  GPIO_PORTF_IM_R |= (SW2);    // enable interrupt on SW1|SW2
                                   // GPIO PortF=priority 2

  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|(7<<21); 
  NVIC_EN0_R |= 1<<30; // enable interrupt 4 in NVIC
  SW1Priority = 10;   // bogus value
  SW2Priority = 10;   // bogus value
  OS_DisableSWTasks();
  OS_EndCritical(status);
}

//void OS_AddSwitchTask(void(*task)(void), unsigned long priority, uint8_t switchNum) {
//  long status = OS_StartCritical();
//  // For now we're just setting the switch priority to whichever is highest
//  if (switchNum == 1) {
//    SW1Task = task;
//    LastSW1 = SW1;
//    if (SW2Priority != 10 && priority < SW2Priority) {  // if true, SW2 has been initialized and the priority is higher than SW2
//      NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|(priority<<21); 
//    }
//    SW1Priority = priority;
//    //OS_EnableSWTask(1);
//    GPIO_PORTF_IM_R |= SW1;
//  } else if (switchNum == 2) {
//    SW2Task = task;
//    LastSW2 = SW2;
//    if (SW1Priority != 10 && priority < SW1Priority) {
//      NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|(priority<<21); 
//    }
//    SW2Priority = priority;
//    //OS_EnableSWTask(2);
//    GPIO_PORTF_IM_R |= SW2;
//  }
//  OS_EndCritical(status);
//}

void OS_AddSwitchTask(void(*task)(void), unsigned long priority) {
  long status = OS_StartCritical();
  // For now we're just setting the switch priority to whichever is highest
  SW2Task = task;
  LastSW2 = SW2;
  if (SW1Priority != 10 && priority < SW1Priority) {
    NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|(priority<<21); 
  }
  SW2Priority = priority;
  //OS_EnableSWTask(2);
  GPIO_PORTF_IM_R |= SW2;
  OS_EndCritical(status);
}

// TODO - figure out how to have separate priorities
void GPIOPortF_Handler(void){
  if(GPIO_PORTF_RIS_R&SW2){  // poll SW2
    if (OS_AddThread(&DebounceTaskSW2, STACKSIZE, 0) != -1) {
      if (LastSW2 == SW2)
        SW2Task();
      GPIO_PORTF_IM_R &= ~SW2;  // acknowledge flag SW2
    } else {
      GPIO_PORTF_ICR_R |= SW2; // acknowledge flag SW1
    }
  }
  if (GPIO_PORTF_RIS_R&ECHO4) {   // use this for ping))) 4
    int flag = GPIO_PORTF_DATA_R&ECHO4;
    long data = processPingData(3, flag);
    if (data != -1) {
      Ping_Fifo4_Put(data);
    }
    GPIO_PORTF_ICR_R |= ECHO4;
  }
}

//void GPIOPortF_Handler(void){
//  if(GPIO_PORTF_RIS_R&SW1){  // poll SW1
//    if (OS_AddThread(&DebounceTaskSW1, STACKSIZE, 0) != -1) {
//      if(LastSW1 == SW1)         // if previous was high, this is a falling edge 
//        SW1Task();             // execute user task;
//      GPIO_PORTF_IM_R &= ~SW1; // disarm interrupt on SW1
//    } else {
//      GPIO_PORTF_ICR_R |= SW1; // acknowledge flag SW1
//    }
//  }
//  if(GPIO_PORTF_RIS_R&SW2){  // poll SW2
//    if (OS_AddThread(&DebounceTaskSW2, STACKSIZE, 0) != -1) {
//      if (LastSW2 == SW2)
//        SW2Task();
//      GPIO_PORTF_IM_R &= ~SW2;  // acknowledge flag SW2
//    } else {
//      GPIO_PORTF_ICR_R |= SW2; // acknowledge flag SW1
//    }
//  }
//  if (GPIO_PORTF_RIS_R&ECHO4) {   // use this for ping))) 4
//    long data = processPingData(3);
//    if (data != -1) {
//      Ping_Fifo4_Put(data);
//    }
//    GPIO_PORTF_ICR_R |= ECHO4;
//  }
//}

//void static DebounceTaskSW1(void){
//  OS_Sleep(10);           // foreground sleeping, must run within 50ms
//  LastSW1 = PF4;
//  GPIO_PORTF_ICR_R |= SW1; // acknowledge flag SW1
//  GPIO_PORTF_IM_R |= SW1; // enable interrupt on SW1|SW2
//  OS_Kill();
//}

void static DebounceTaskSW2(void) {
  OS_Sleep(10);           // foreground sleeping, must run within 50ms
  LastSW2 = PF0;
  GPIO_PORTF_ICR_R |= SW2; // acknowledge flag SW1
  GPIO_PORTF_IM_R |= SW2; // enable interrupt on SW1|SW2
  OS_Kill();
}

//------------OS_SwitchInput------------
// Read and return the status of the switches.
// Input: none
// Output: 0x01 if only Switch 1 is pressed
//         0x10 if only Switch 2 is pressed
//         0x00 if both switches are pressed
//         0x11 if no switches are pressed
uint32_t OS_SwitchInput(void){
  return SWITCHES;
}

//******** OS_AddSW1Task *************** 
// add a background task to run whenever the SW1 (PF4) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
//int OS_AddSW1Task(void(*task)(void), unsigned long priority) {
//  OS_DisableInterrupts();
//  OS_AddSwitchTask(task, priority, 1);
//  OS_EnableInterrupts();
//	return 1;
//}

//******** OS_AddSW2Task *************** 
// add a background task to run whenever the SW2 (PF0) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed user task will run to completion and return
// This task can not spin block loop sleep or kill
// This task can call issue OS_Signal, it can call OS_AddThread
// This task does not have a Thread ID
// In lab 2, this function can be ignored
// In lab 3, this command will be called will be called 0 or 1 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSWTask(void(*task)(void), unsigned long priority) {
  OS_DisableInterrupts();
//  OS_AddSwitchTask(task, priority, 2);
  OS_AddSwitchTask(task, priority);
  OS_EnableInterrupts();
	return 1;
}

//********* OS_DisableSWTasks **************
// Disables switch tasks.
// Inputs:  None
// Outputs: None
void OS_DisableSWTasks(void) {
//  GPIO_PORTF_IM_R &= ~(SW1|SW2);    // enable interrupt on SW1|SW2
  GPIO_PORTF_IM_R &= ~(SW2);    // enable interrupt on SW1|SW2

}

//********* OS_DisableSWTask **************
// Disables a switch task.
// Inputs:  switchNum
// Outputs: None
void OS_DisableSWTask(uint8_t switchNum) {
  if (switchNum == 1) {
//    GPIO_PORTF_IM_R &= ~SW1;
  } else if (switchNum == 2) {
    GPIO_PORTF_IM_R &= ~SW2;
  }
}

//********* OS_EnableSWTasks **************
// Enables switch tasks.
// Inputs:  None
// Outputs: None
void OS_EnableSWTasks(void) {
//  GPIO_PORTF_IM_R |= (SW1|SW2);    // enable interrupt on SW1|SW2
  GPIO_PORTF_IM_R |= (SW2);    // enable interrupt on SW1|SW2
}

//********* OS_EnableSWTask **************
// Enables a switch task.
// Inputs:  switchNum
// Outputs: None
void OS_EnableSWTask(uint8_t switchNum) {
  if (switchNum == 1) {
//    GPIO_PORTF_IM_R |= SW1;
  } else if (switchNum == 2) {
    GPIO_PORTF_IM_R |= SW2;
  }
}


