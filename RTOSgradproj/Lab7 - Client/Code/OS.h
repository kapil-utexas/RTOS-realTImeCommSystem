// filename **********OS.H***********
// Real Time Operating System for Labs 2 and 3 
// Jonathan W. Valvano 1/27/14, valvano@mail.utexas.edu
// Modified by Allen Wang and Alvin Tung
// EE445M/EE380L.6 
// You may use, edit, run or distribute this file 
// You are free to change the syntax/organization of this file

#ifndef __OS_H
#define __OS_H  1

// edit these depending on your clock        
#define TIME_1MS    80000          
#define TIME_2MS    (2*TIME_1MS)  
#define TIME_500US  (TIME_1MS/2)  
#define TIME_250US  (TIME_1MS/5)  

#define MAXTHREADS  12       // maximum number of threads
#define STACKSIZE   512      // number of 32-bit words in stack

#define BLOCKED     1
#define UNBLOCKED   0

#include "OS_PeriodicTasks.h"
#include "OS_Time.h"
#include "SensorInstructions.h"
#include <stdint.h>

struct tcb{
  uint32_t *sp;      // pointer to stack (valid for threads not running
  struct tcb *next;  // linked-list pointer
  struct tcb *prev;
	uint32_t id;
	uint32_t priority;
  uint8_t status;
  uint32_t sleepCounter;  
};

typedef struct tcb tcbType;

// feel free to change the type of semaphore, there are lots of good solutions
struct  Sema4 {
  long Value;   // >0 means free, otherwise means busy        
  tcbType * BlockedThreads;
// add other components here, if necessary to implement blocking
};
typedef struct Sema4 Sema4Type;

// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: serial, ADC, systick, LaunchPad I/O and timers 
// input:  none
// output: none
void OS_Init(void); 

// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, long value); 

// ******** OS_Wait ************
// decrement semaphore 
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt); 

// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt); 

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
//void OS_bWait(Sema4Type *semaPt); 

// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
//void OS_bSignal(Sema4Type *semaPt); 

//******** OS_AddThread *************** 
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
int OS_AddThread(void(*task)(void), 
   unsigned long stackSize, unsigned long priority);

//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
unsigned long OS_Id(void);

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
//int OS_AddSW1Task(void(*task)(void), unsigned long priority);

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
//int OS_AddSW2Task(void(*task)(void), unsigned long priority);
int OS_AddSWTask(void(*task)(void), unsigned long priority);

//********* OS_DisableSWTasks **************
// Disables switch tasks.
// Inputs:  None
// Outputs: None
void OS_DisableSWTasks(void);

//********* OS_DisableSWTask **************
// Disables a switch  task.
// Inputs:  switchNum
// Outputs: None
void OS_DisableSWTask(uint8_t switchNum);

//********* OS_EnableSWTasks **************
// Enables switch tasks.
// Inputs:  None
// Outputs: None
void OS_EnableSWTasks(void);

//********* OS_EnableSWTask **************
// Enables a switch task.
// Inputs:  switchNum
// Outputs: None
void OS_EnableSWTask(uint8_t switchNum);

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(unsigned long sleepTime); 

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void); 

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void);

// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(unsigned long size);

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(unsigned long data);  

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void);

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void);

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void);

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(SensorInstruction data);

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
SensorInstruction OS_MailBox_Recv(void);

//******** OS_Launch *************** 
// start the scheduler, enable interrupts
// Inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
void OS_Launch(unsigned long theTimeSlice);

//******** OS_AddPeriodicThread *************** 
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units (12.5ns)
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// You are free to select the time resolution for this function
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
// When called, the function will enable periodic threads immediately
//int OS_AddPeriodicThread(void(*task)(void), 
//   unsigned long period, unsigned long priority);

// NOTE - DON'T UNCOMMENT THIS OUT UNTIL ADDING SECOND PERIODIC TASK IS DONE!!! 
// We need to make Timer3B initialization work, or at least use another timer.


//******** OS_ST7735PrintJitterInfo *************** 
// Outputs information to the ST7735 about calculated jitter.
// Inputs: none
// Outputs: none
void OS_ST7735PrintJitterInfo(void);

//******** OS_UARTPrintJitterInfo *************** 
// Outputs information over UART about calculated jitter.
// Inputs: none
// Outputs: none
void OS_UARTPrintJitterInfo(void);

//******** OS_EnablePeriodicThread **************
// Enables periodic threads
void OS_EnablePeriodicThread(uint8_t taskNum);

//******** OS_DisablePeriodicThread **************
// Enables periodic threads
void OS_DisablePeriodicThread(uint8_t taskNum);

// ********* OS_GetNextThread ************
// Returns the next thread with the highest priority
// Generally used in case of removing a thread from active.
// Use this after you remove it from the active list.
// Works for finding the next thread for context switches
// ****** THIS IS ONLY USED FOR SYSTICK ********
// Input:  none
// Output: priority, -1 if there are no threads running
tcbType * OS_GetNextThread(void);

void HardFault_Handler(void);

#endif
