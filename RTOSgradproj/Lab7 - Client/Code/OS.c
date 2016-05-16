// os.c
// Runs on LM4F120/TM4C123
// A very simple real time operating system with minimal features.
// Daniel Valvano
// January 29, 2015

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

   Programs 4.4 through 4.12, section 4.2

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

#include <stdint.h>
#include "OS.h"
#include "OS_Switch.h"
#include "PLL.h"
#include "inc/tm4c123gh6pm.h"
#include "FIFO.h"
#include "Heartbeat.h"
#include "OS_Shared.h"
#include "ST7735.h"
#include "UART.h"
#include "esp8266.h"

#define NVIC_ST_CTRL_R          (*((volatile uint32_t *)0xE000E010))
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_R        (*((volatile uint32_t *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile uint32_t *)0xE000E018))
#define NVIC_INT_CTRL_R         (*((volatile uint32_t *)0xE000ED04))
#define NVIC_INT_CTRL_PENDSTSET 0x04000000  // Set pending SysTick interrupt
#define NVIC_SYS_PRI3_R         (*((volatile uint32_t *)0xE000ED20))  // Sys. Handlers 12 to 15 Priority

void EnableInterrupts(void);  // Enable interrupts
void DisableInterrupts(void); // Disable interrupts

// function definitions in osasm.s
void OS_DisableInterrupts(void); // Disable interrupts
void OS_EnableInterrupts(void);  // Enable interrupts

void StartOS(void);

// Allocation/pointers used for running threads
#define NUM_PRIORITIES  8     
tcbType tcbs[MAXTHREADS];
tcbType *ActiveThreads[NUM_PRIORITIES];
tcbType *SleepingThreads;
tcbType *DeadThreads; 


tcbType *RunPt;
uint32_t Stacks[MAXTHREADS][STACKSIZE];
uint32_t NumThreads; 
uint32_t NumActiveThreads;
uint32_t NumSleepingThreads;
uint32_t NumDeadThreads;

tcbType * NextThread;

// Extern variable - check OS_Shared.h
unsigned long SystemTimeMS = 0;
/*************************************************** Data structure related code ********************************/
// Append to a regular linked list
// Input: thisThread
//        newHead
static void AppendToList(tcbType * thisThread, tcbType ** newHead) {
  long status = OS_StartCritical();
  thisThread->next = 0;
  if (*newHead == 0) {    // list did not exist
    *newHead = thisThread;
    thisThread->prev = 0;
  } else {
    tcbType * traverseThread = *newHead;
    while (traverseThread->next) {
      traverseThread = traverseThread->next;
    }
    traverseThread->next = thisThread;
    thisThread->prev = traverseThread;
  }
  OS_EndCritical(status);
}

// Append to a regular linked list with priority
// Input: thisThread
//        newHead
static void AppendToPriorityList(tcbType * thisThread, tcbType ** newHead) {
  long status = OS_StartCritical();
  thisThread->next = 0;
  if (*newHead == 0) {    // list did not exist
    *newHead = thisThread;
    thisThread->prev = 0;
  } else {
    tcbType * traverseThread = *newHead;
    while (traverseThread->next) {
      if (traverseThread->priority > thisThread->priority) {
        HardFault_Handler();
          //while(1);
//          thisThread->next = traverseThread->next;
//          thisThread->next->prev = thisThread;
//          break;
      }
      traverseThread = traverseThread->next;
    }
    traverseThread->next = thisThread;
    thisThread->prev = traverseThread;
  }
  OS_EndCritical(status);
}

// Append to a circularly linked list
// Input: thisThread
//        newHead
static void AppendToCircularList(tcbType * thisThread, tcbType ** newHead) {
  long status = OS_StartCritical();
  if (*newHead == 0) {  // list did not exist
    *newHead = thisThread;
    thisThread->prev = thisThread;
    thisThread->next = thisThread;
  } else {
    // Insert it between head and its previous
    tcbType * head = *newHead;
    thisThread->next = head;
    thisThread->prev = head->prev;
    head->prev = thisThread;
    thisThread->prev->next = thisThread;
  }
  OS_EndCritical(status);
}

// Remove from a regular linked list
// Input: thisThread
//        originalHead
static void RemoveFromList(tcbType * thisThread, tcbType ** originalHead) {
  long status = OS_StartCritical();
  if (*originalHead == 0) {
    HardFault_Handler();
//    while(1);
//    OS_EndCritical(status);
//    return;
  }   // There is something really wrong here...
  tcbType * before = thisThread->prev;
  tcbType * after = thisThread->next;
  
  if (before == thisThread || after == thisThread) {    // there is only one thread in this list
    before = 0;
    after = 0;
    *originalHead = 0;
  }
  
  if (before) before->next = after;
  if (after) after->prev = before;
  if (thisThread == *originalHead) {
    *originalHead = after; // issue?
  }
  thisThread->next = 0;
  thisThread->prev = 0;
  OS_EndCritical(status);
}


// ******** OS_SwitchThread ************
// Switches threads using PendSV Handler. Used in the case of removing a node
// from the round robin scheduler
static void OS_SwitchThread(tcbType * newThread) {
  OS_DisableInterrupts();
  if(newThread == 0) { 
    HardFault_Handler();
  } // Something really wrong here hahahahahahaha
  NextThread = newThread;
  NVIC_INT_CTRL_R |= 0x10000000;
  OS_EnableInterrupts();
}

/************************OS Sleep Related code*****************************/
//---------------OS_MSTimerEnable---------------
// Enable Timer2 
// Ouput: none
// Input: none
//static void OS_MSTimerEnable(void){
//  TIMER2_CTL_R |= TIMER_CTL_TAEN; // enable timer2A
//}

//---------------OS_MSTimerDisable---------------
// Disable Timer0 
// Ouput: none
// Input: none
//static void OS_MSTimerDisable(void){
//  TIMER2_CTL_R &= ~TIMER_CTL_TAEN; // disable timer2A
//}

// ***************** OS_MSTimerInit ****************
// Activate Timer2 interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in units (in ms)
// Outputs: none
void OS_MSTimerInit(unsigned long period){
  SYSCTL_RCGCTIMER_R |= 0x04;   // 0) activate timer2
  TIMER2_CTL_R = 0x00000000;    // 1) disable timer2A during setup
  TIMER2_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER2_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER2_TAILR_R = period * (80000) - 1;    // 4) reload value
  TIMER2_TAPR_R = 0;            // 5) bus clock resolution
  TIMER2_ICR_R = 0x00000001;    // 6) clear timer2A timeout flag
  TIMER2_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 39, interrupt number 23
  NVIC_EN0_R = 1<<23;           // 9) enable IRQ 23 in NVIC
  TIMER2_CTL_R = 0x00000001;    // 10) enable timer2A
}

// ***************** OS_Wake ****************
// Wakes up a thread from sleep.
// Inputs:  thread to wake up
// Outputs: none
static tcbType * OS_Wake(tcbType * thisThread) {
  long status = OS_StartCritical();
  tcbType * nextThread = thisThread->next;
  thisThread->sleepCounter = 0; // safe check to make sure it is 0 
  RemoveFromList(thisThread, &SleepingThreads);
  AppendToCircularList(thisThread, &ActiveThreads[thisThread->priority]);
  --NumSleepingThreads;
  ++NumActiveThreads;
  OS_EndCritical(status);
  return nextThread;
}

void Timer2A_Handler(void){
  TIMER2_ICR_R = TIMER_ICR_TATOCINT; // acknowledge TIMER2A timeout
  long status = OS_StartCritical();
  SystemTimeMS++;
  
  //Sleeping Check
  tcbType * traversePt = SleepingThreads;
  // Traverse sleeping threads
  while(traversePt) {
    traversePt->sleepCounter -= 1;
    if(traversePt->sleepCounter == 0) {
      traversePt = OS_Wake(traversePt);
    } else {
      traversePt = traversePt->next;
    }
  }
  if (SystemTimeMS % 100 == 0) {
    HeartBeat_Toggle();
  }
  OS_EndCritical(status);
}

// ********************************************************** OS Code ********************************************

void WaitForInterrupt(void);
// *********** OS_BackupThread *****************
// This thread runs in case no other threads are active.
// It will have the lowest priority and should be the only thread
// with this priority.
static void OS_BackupThread(void) {
  while(1) {
    WaitForInterrupt();
  }
}

unsigned long long disableTime;
unsigned long long enableTime;
unsigned long maxDisableTime;
unsigned long lastDisableTime;
unsigned long lastEnableTime;
uint32_t interruptsEnabled;
uint32_t continueMeasuring;
uint32_t OS_IsRunning;

#define PERIODIC_FREQUENCY 1   // ms
// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: systick, 50 MHz PLL
// input:  none
// output: none
void OS_Init(void){
  DisableInterrupts(); //OS_ not used for timing
  OS_IsRunning = 0;
  OS_MSTimerInit(PERIODIC_FREQUENCY); //ms
  disableTime = 0; enableTime = 0; maxDisableTime = 0;
  lastDisableTime = 0; lastEnableTime = 0;
  interruptsEnabled = 0;      // starts in disabled mode
  continueMeasuring = 1;
	NumThreads = 0;
  NumActiveThreads = 0;
  NumSleepingThreads = 0;
  SleepingThreads = 0;
  for (int i = 0; i < NUM_PRIORITIES; ++i) {
    ActiveThreads[i] = 0;
  }
		
  PLL_Init(Bus80MHz);         // set processor clock to 80 MHz
  UART_Init();
	OS_SwitchInit();
  HeartBeat_Init();
  HardBeat_Init();
	//init wifi module  
	ESP8266_Init(115200);  // connect to access point, set up as client
  ESP8266_GetVersionNumber();
		//ESP8266_Init(115200);

  //ST7735_SInit();
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0x00FFFFFF)|0xE0000000; // priority 7
  OS_AddThread(&OS_BackupThread, STACKSIZE, NUM_PRIORITIES - 1);
  OS_EnableInterrupts(); //OS_ 
}


static void SetInitialStack(int i){
  tcbs[i].sp = &Stacks[i][STACKSIZE-16]; // thread stack pointer
  Stacks[i][STACKSIZE-1] = 0x01000000;   // thumb bit
  Stacks[i][STACKSIZE-3] = 0x14141414;   // R14
  Stacks[i][STACKSIZE-4] = 0x12121212;   // R12
  Stacks[i][STACKSIZE-5] = 0x03030303;   // R3
  Stacks[i][STACKSIZE-6] = 0x02020202;   // R2
  Stacks[i][STACKSIZE-7] = 0x01010101;   // R1
  Stacks[i][STACKSIZE-8] = 0x00000000;   // R0
  Stacks[i][STACKSIZE-9] = 0x11111111;   // R11
  Stacks[i][STACKSIZE-10] = 0x10101010;  // R10
  Stacks[i][STACKSIZE-11] = 0x09090909;  // R9
  Stacks[i][STACKSIZE-12] = 0x08080808;  // R8
  Stacks[i][STACKSIZE-13] = 0x07070707;  // R7
  Stacks[i][STACKSIZE-14] = 0x06060606;  // R6
  Stacks[i][STACKSIZE-15] = 0x05050505;  // R5
  Stacks[i][STACKSIZE-16] = 0x04040404;  // R4
}


// ********* OS_GetHighestPriorityNum ************
// Returns the highest priority thread that's active
// Input:  none
// Output: priority, -1 if there are no threads running
static int OS_GetHighestPriorityNum(void) {
  for (int i = 0; i < NUM_PRIORITIES; ++i) {
    if (ActiveThreads[i] != 0) {
      return i;
    }
  }
  OS_DisableInterrupts();
  HardFault_Handler();                // THIS SHOULD NEVER HAPPEN. THIS WILL TELL YOU THAT
  return 0;
}

// ********* OS_GetNextThread ************
// Returns the next thread with the highest priority
// Generally used in case of removing a thread from active.
// Use this after you remove it from the active list.
// Works for finding the next thread for context switches
// Input:  none
// Output: priority, -1 if there are no threads running
tcbType * OS_GetNextThread(void) {
  tcbType * thisThread = RunPt;
  int highestPri = OS_GetHighestPriorityNum();
  if (thisThread->next && highestPri == thisThread->priority) {
    return thisThread->next;
  } else {
    return ActiveThreads[highestPri];
  }
}

///******** OS_Launch ***************
// start the scheduler, enable interrupts
// Inputs: number of 20ns clock cycles for each time slice
//         (maximum of 24 bits)
// Outputs: none (does not return)
void OS_Launch(unsigned long theTimeSlice){
  tcbType * firstThread = ActiveThreads[OS_GetHighestPriorityNum()];
  OS_IsRunning = 1;
  RunPt = firstThread; // TODO check if it belongs here or add thread?
  NVIC_ST_RELOAD_R = theTimeSlice - 1; // reload value
  NVIC_ST_CTRL_R |= 0x00000007; // enable, core clock and interrupt arm
  StartOS();                   // start on the first task
}

// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, long value) {
	OS_DisableInterrupts();
	semaPt->Value = value;
  semaPt->BlockedThreads = 0;
	OS_EnableInterrupts();
}

static volatile uint32_t waitCalls = 0;
// ******** OS_Wait ************
// decrement semaphore 
// DON'T USE THIS IN A CRITICAL SECTION
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt) {
  long status = OS_StartCritical();
  uint8_t contextSwitch = 0;
  tcbType * nextThread = 0; 
  --(semaPt->Value);
  if (semaPt->Value < 0) {    // block this thread
    ++waitCalls;
    tcbType * thisThread = RunPt;
    thisThread->status = BLOCKED;
    RemoveFromList(thisThread, &(ActiveThreads[thisThread->priority]));
    AppendToPriorityList(thisThread, &(semaPt->BlockedThreads));
    nextThread = OS_GetNextThread();
    --NumActiveThreads;
    contextSwitch = 1;
  }
  OS_EndCritical(status);
  if (contextSwitch) { 
    OS_SwitchThread(nextThread); 
  }
}

// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt) {
	long status = OS_StartCritical();
	semaPt->Value = semaPt->Value + 1;
  if (semaPt->Value <= 0) {
    // Unblock the highest priority thread
    tcbType * unblockThisThread = semaPt->BlockedThreads;
    RemoveFromList(unblockThisThread, &semaPt->BlockedThreads);
    unblockThisThread->status = UNBLOCKED;
    AppendToCircularList(unblockThisThread, &ActiveThreads[unblockThisThread->priority]);
    ++NumActiveThreads;
  }
	OS_EndCritical(status);
}

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt) {
  long status = OS_StartCritical();
  uint8_t contextSwitch = 0;
  tcbType * nextThread = 0; 
  --(semaPt->Value);
  if (semaPt->Value < 0) {    // block this thread
    ++waitCalls;
    tcbType * thisThread = RunPt;
    thisThread->status = BLOCKED;
    RemoveFromList(thisThread, &(ActiveThreads[thisThread->priority]));
    AppendToPriorityList(thisThread, &(semaPt->BlockedThreads));
    nextThread = OS_GetNextThread();
    --NumActiveThreads;
    contextSwitch = 1;
  }
  OS_EndCritical(status);
  if (contextSwitch) { 
    OS_SwitchThread(nextThread); 
  }
}

// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt) { 
	int32_t status = OS_StartCritical();
  if (semaPt->Value < 0) {
    // Unblock sleeping threads
    tcbType * unblockThisThread = semaPt->BlockedThreads;
    RemoveFromList(unblockThisThread, &semaPt->BlockedThreads);
    unblockThisThread->status = UNBLOCKED;
    AppendToCircularList(unblockThisThread, &ActiveThreads[unblockThisThread->priority]);
    ++NumActiveThreads;
  }
  semaPt->Value = 1;
	OS_EndCritical(status);
}

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
   unsigned long stackSize, unsigned long priority) {
	long status = OS_StartCritical();
  tcbType * thisThread;
  int id;
  if (DeadThreads && NumDeadThreads >= 1) {    // Recycle a killed thread
    thisThread = DeadThreads;
    RemoveFromList(thisThread, &DeadThreads);
    --NumDeadThreads;
    id = thisThread->id;
  } else {
    if (NumThreads >= MAXTHREADS) { 
      OS_EndCritical(status);
      return 0; 
    }    
    thisThread = &tcbs[NumThreads];
    (*thisThread).id = NumThreads;
    id = NumThreads;
    ++NumThreads;
  }
    
  SetInitialStack(id); Stacks[id][STACKSIZE-2] = (int32_t)(task); 
  thisThread->status = UNBLOCKED;
  thisThread->priority = priority;
  thisThread->sleepCounter = 0;
  AppendToCircularList(thisThread, &(ActiveThreads[priority]));
  ++NumActiveThreads;
  
  // Set Initial Stack sets tcb stack pointer
  
	OS_EndCritical(status);
  return 1;
}

//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
unsigned long OS_Id(void) {
	return RunPt->id;
}

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(unsigned long sleepTime) {
  long status = OS_StartCritical();
  tcbType * thisThread = RunPt; 
  tcbType * nextThread = 0;

//  if (thisThread) nextThread  = thisThread->next;    // We need knowledge of this for the context switch (Must be from active thread circular)
//  if (!nextThread) {
//    nextThread = ActiveThreads[OS_GetHighestPriorityNum()];
//  }

  RemoveFromList(thisThread, &(ActiveThreads[thisThread->priority]));
  AppendToList(thisThread, &SleepingThreads);
  
  // find next thread to run if none in priority   
  nextThread = ActiveThreads[OS_GetHighestPriorityNum()];

  thisThread->sleepCounter = sleepTime;
  ++NumSleepingThreads;
  --NumActiveThreads;
  OS_EndCritical(status);
  OS_SwitchThread(nextThread);
}

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void) {
  long status = OS_StartCritical();
  tcbType * thisThread = RunPt;
  RemoveFromList(thisThread, &ActiveThreads[thisThread->priority]);
  AppendToList(thisThread, &DeadThreads);
  ++NumDeadThreads;
  --NumActiveThreads; // Thread 
  tcbType * nextThread;
  
  // find next thread to run if none in priority   
  nextThread = ActiveThreads[OS_GetHighestPriorityNum()];
  OS_EndCritical(status);
  OS_SwitchThread(nextThread);
}


// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void) {
	NVIC_ST_CURRENT_R = 0;					// clear counter
	NVIC_INT_CTRL_R |= 0x04000000; 	// trigger SysTick interrupt (context switch)
}
 
//*******************************************OS_Fifos**********************************************

#define OS_FIFO_SIZE   8         // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS    1         // return value on success
#define FIFOFAIL       0         // return value on failure
static Sema4Type Fifo_Mutex;
static Sema4Type Fifo_CurrentSize;
AddIndexFifo(OS, OS_FIFO_SIZE, uint32_t, FIFOSUCCESS, FIFOFAIL)

// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void OS_Fifo_Init(unsigned long size) {
  long status = OS_StartCritical();
  OSFifo_Init();
  OS_InitSemaphore(&Fifo_CurrentSize, 0);
  OS_InitSemaphore(&Fifo_Mutex, 1);
  OS_EndCritical(status);
}

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int OS_Fifo_Put(unsigned long data) {
  int status = 0;
  status = OSFifo_Put(data);
  OS_Signal(&Fifo_CurrentSize);
	return status;
}

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void) {
  uint32_t retValue = 0;
  OS_Wait(&Fifo_CurrentSize);
  OS_Wait(&Fifo_Mutex);
  OSFifo_Get(&retValue);
  OS_Signal(&Fifo_Mutex);
	return (long)retValue;
}

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void) {
	return OS_FIFO_SIZE;
}

//****************************** OS_Mailbox *********************************
static Sema4Type OS_Mailbox_Send;
static Sema4Type OS_Mailbox_Ack;
SensorInstruction Mailbox_Mail;
// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void) {
  long status = OS_StartCritical();
  OS_InitSemaphore(&OS_Mailbox_Send, 0);
  OS_InitSemaphore(&OS_Mailbox_Ack, 0);
  SensorInstruction s;
  Mailbox_Mail = s;
  OS_EndCritical(status);
}

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(SensorInstruction data) {
  Mailbox_Mail = data;
  OS_Signal(&OS_Mailbox_Send);
  OS_Wait(&OS_Mailbox_Ack);
}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
SensorInstruction OS_MailBox_Recv(void) {
  SensorInstruction data;
  OS_Wait(&OS_Mailbox_Send);
  data = Mailbox_Mail;
  OS_Signal(&OS_Mailbox_Ack);
	return data;
}


void HardFault_Handler(void){
  int i = 0;
  while(1){ 
    i++; 
    if(i == 4000000){ 
      HardBeat_Toggle();
      i = 0;
    }
  }
}

