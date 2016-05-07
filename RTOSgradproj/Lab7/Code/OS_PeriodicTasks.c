// OS_PeriodicTasks.c
// Allen Wang and Alvin Tung
// This file is used for all periodic task related functions for the OS
#include <stdint.h>
#include "OS_PeriodicTasks.h"
#include "OS.h"
#include "UART.h"
#include "OS_Time.h"
#include "inc/tm4c123gh6pm.h"
#include "ST7735.h"
struct periodicTaskStruct{
  void (*task)(void);
  uint32_t priority;
  uint32_t period;
  uint32_t lastTimeRan;
  uint8_t started;
};

typedef struct periodicTaskStruct periodicTask;

// Used to calculate the jitter of the periodic tasks
#define OS_JitterSIZE 64
unsigned long const OS_JitterSize=OS_JitterSIZE;
unsigned long OS_JitterHistogram[OS_JitterSIZE]={0,};
unsigned long MaxJitter;

// Allocation for periodic tasks
#define MAX_PERIODIC_TASKS      2
periodicTask periodicTasks[MAX_PERIODIC_TASKS];
uint8_t numPeriodicTasks;

// External functions
void OS_DisableInterrupts(void);
void OS_EnableInterrupts(void);
uint32_t OS_StartCritical(void);
void OS_EndCritical(uint32_t status);


// ***************** TIMER1_Init ****************
// Activate TIMER1 interrupts to run user task periodically
// Inputs:  periodicTask struct pointer
// Outputs: none
void Timer1_Init(periodicTask * pt) {
  long status = OS_StartCritical();
  SYSCTL_RCGCTIMER_R |= 0x02;   // 0) activate TIMER1
  TIMER1_CTL_R &= ~0x00000001;  // 1) disable TIMER1A during setup
  TIMER1_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER1_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER1_TAILR_R = pt->period - 1;       // 4) Just put gibberish period for now
  TIMER1_TAPR_R = 0;            // 5) bus clock resolution
  TIMER1_ICR_R = 0x00000001;    // 6) clear TIMER1A timeout flag
  TIMER1_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|((pt->priority&0x7) << 13);  // set priority
// interrupts enabled in the main program after all devices initialized
// vector number 37, interrupt number 21
  NVIC_EN0_R = 1<<21;           // 9)  enable IRQ 21 in NVIC
  TIMER1_CTL_R |= 0x00000001;
  MaxJitter = 0;
  OS_EndCritical(status);
}

//TODO - Fix this initialization to be correct for Timer3B. Can't use timer3A because of CAN...
// ***************** Timer3B_Init ****************
// Activate Timer3 interrupts to run user task periodically
// Inputs:  periodicTask struct pointer
// Outputs: none
void Timer3B_Init(periodicTask * pt){
  long status = OS_StartCritical();
  SYSCTL_RCGCTIMER_R |= 0x08;   // 0) activate timer3
  TIMER3_CTL_R = 0x00000000;    // 1) disable timer3B during setup
  TIMER3_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER3_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER3_TAILR_R = pt->period-1;    // 4) reload value
  TIMER3_TAPR_R = 0;            // 5) bus clock resolution
  TIMER3_ICR_R = 0x00000001;    // 6) clear timer3A timeout flag
  TIMER3_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI9_R = (NVIC_PRI9_R&0xFFFFF00F)|((pt->priority & 0x7) << 5); // set priority
// interrupts enabled in the main program after all devices initialized
// vector number 39, interrupt number 23
  NVIC_EN1_R = 1<<(35 - 32);           // 9) enable IRQ 23 in NVIC
  TIMER3_CTL_R |= 0x00000001;    // 10) enable timer3A
  OS_EndCritical(status);
}

static void calculateJitter(periodicTask * pt) {
    unsigned long thisTime = OS_Time();
    unsigned long lastTime = pt->lastTimeRan;
    unsigned long diff = OS_TimeDifference(thisTime, lastTime);
    if (lastTime > thisTime && pt->started) { 
      pt->lastTimeRan = thisTime; return; 
    }
    unsigned long jitter;
    if (pt->started) {
        if (diff > pt->period) {
            jitter = (diff - pt->period + 4) / 8;   // in 0.1 usec
        } else {
            jitter = (pt->period - diff + 4) / 8;   // in 0.1 usec
        }
        if (jitter > MaxJitter) {
            MaxJitter = jitter;
        }
        if (jitter >= OS_JitterSIZE) {
            jitter = OS_JitterSIZE - 1;
        }
        OS_JitterHistogram[jitter]++;
    } else {
        pt->started = 1;
    }
    pt->lastTimeRan = thisTime;
}

void Timer1A_Handler(void) {
  TIMER1_ICR_R = TIMER_ICR_TATOCINT;    // acknowledge TIMER1A timeout
  periodicTask * thisTask = &periodicTasks[0];
  calculateJitter(thisTask);
  (thisTask->task)();                // execute task
}

void Timer3B_Handler(void) {
  TIMER3_ICR_R = TIMER_ICR_TATOCINT;
  periodicTask * thisTask = &periodicTasks[1];
  calculateJitter(thisTask);
  (thisTask->task)();
}

//******** OS_EnablePeriodicThread **************
// Enables periodic threads
void OS_EnablePeriodicThread(uint8_t taskNum) {
  if (taskNum == 1) {
    TIMER1_CTL_R |= 0x00000001;
  } else if (taskNum == 2) {
    TIMER3_CTL_R |= 0x00000001;
  }
}

//******** OS_DisablePeriodicThread **************
// Enables periodic threads
void OS_DisablePeriodicThread(uint8_t taskNum) {
  if (taskNum == 1) {
    TIMER1_CTL_R &= ~0x00000001;
  } else if (taskNum == 2) {
    TIMER3_CTL_R &= ~0x00000001;
  }
}

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
// The task must call OS_Kill
int OS_AddPeriodicThread(void(*task)(void), 
   unsigned long period, unsigned long priority) {
  if (numPeriodicTasks >= MAX_PERIODIC_TASKS) {
    // Cannot support this many periodic threads
    return 0;
  }
  periodicTask * thisTask = &periodicTasks[numPeriodicTasks];
  thisTask->period = period;
  thisTask->task = task;
  ++numPeriodicTasks;

  if (numPeriodicTasks == 1) {
    Timer1_Init(thisTask);
    OS_EnablePeriodicThread(1);
  } else if (numPeriodicTasks == 2) {
    Timer3B_Init(thisTask);
    OS_EnablePeriodicThread(2); 
  }
  
  thisTask->lastTimeRan = 0;
  thisTask->started = 0;
  return 1;
}

//******** OS_ST7735PrintJitterInfo *************** 
// Outputs information to the ST7735 about calculated jitter.
// Inputs: none
// Outputs: none
void OS_ST7735PrintJitterInfo(void) {
//  ST7735_SClear(1,3);
//  ST7735_SDrawString(1,3,"Jitter 0.1us = "); ST7735_SDrawValue(1,3,MaxJitter);
}


//******** OS_UARTPrintJitterInfo *************** 
// Outputs information over UART about calculated jitter.
// Inputs: none
// Outputs: none
void OS_UARTPrintJitterInfo(void) {
  unsigned long sum = 0;
  unsigned long count = 0;
  for (int i = 0; i < OS_JitterSIZE; ++i) {
    UART_printf("%d : %d", i, OS_JitterHistogram[i]); 
    sum += OS_JitterHistogram[i] * i;
    count += OS_JitterHistogram[i];
    UART_NewLine();
  }
  unsigned long avg = sum / count;
  UART_printf("Max jitter = %d", MaxJitter);
  UART_NewLine();
  UART_printf("Average jitter = %d", avg);
}
