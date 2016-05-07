// Ping.c
// Written by Allen Wang and Alvin Tung
// Used to interface with various pinging modules.

#include "Ping.h"
#include "../inc/tm4c123gh6pm.h"
#include "OS.h"
#include "Fifo.h"
#include <stdint.h>

// ***************************************
// This interface assumes the usage of an HC-SR04
// Ping))) 1
// PB7 - Trig
// PB6 - Echo

// Ping))) 2
// PB5 - Trig
// PB4 - Echo

// Ping))) 3
// PB3 - Trig
// PB2 - Echo

// Ping))) 4
// PC5 - Trig
// PC4 - Echo

#define PB7 ((volatile uint32_t *)0x4000501C)
#define PB6 ((volatile uint32_t *)0x40005018)
#define PB5 ((volatile uint32_t *)0x40005014)
#define PB4 ((volatile uint32_t *)0x40005010)
#define PB3 ((volatile uint32_t *)0x4000500C)
#define PB2 ((volatile uint32_t *)0x40005008)

#define PC5 ((volatile uint32_t *)0x40006014)
#define PC4 ((volatile uint32_t *)0x40006010)

long OS_StartCritical(void);
void OS_EndCritical(long sr);

long lastFlag[4];
long lastTime[4];

// ******* Ping_Init *****************
void Ping_Init(void) {
  Ping1_Init();
  Ping2_Init();
  Ping3_Init();
  Ping4_Init();
}

//********* Ping1_Init ***************
// Initialize ping 1 interface.
void Ping1_Init(void) {
  long status = OS_StartCritical();
  SYSCTL_RCGCGPIO_R |= 0x02;          // 1) Port B clock
  status = status;
//  GPIO_PORTB_AMSEL_R &= ~0xC0;     // 3) disable analog for PB7 and PB6
//  GPIO_PORTB_PCTL_R &= ~0xFF000000;// 4) configure as GPIO 
  GPIO_PORTB_DIR_R |= TRIGGER1;        // 5) PB7 output
  GPIO_PORTB_DIR_R &= ~ECHO1;       //    PB6 input
    
  GPIO_PORTB_AFSEL_R &= ~ECHO1|TRIGGER1;     // 6) normal function
  GPIO_PORTB_DEN_R |= ECHO1|TRIGGER1;        // 7) digital I/O on PB7-6
  GPIO_PORTB_IS_R &= ~ECHO1;        // 8) PB6 is edge sensitive
  GPIO_PORTB_IBE_R |= ECHO1;       // 9) PB6 is not both edges
                                   // set interrupt priority
 // NVIC_PRI0_R = (NVIC_PRI0_R & 0xFFFF00FFF) | (PING_PRIORITY << 13);
  NVIC_PRI0_R = (NVIC_PRI0_R & 0xFFFF00FFF) | (0x00000000);
  GPIO_PORTB_ICR_R |= ECHO1;
  GPIO_PORTB_IM_R |= ECHO1;
  
  NVIC_EN0_R |= 2;          // Enable in NVIC_EN0
  OS_EndCritical(status);
  GPIO_PORTB_DATA_R = 0;
  lastFlag[0] = 0;
  Ping_Fifo1_Init(8);                // link ping))) 1 to OS fifo 1
}

//********* Ping2_Init ***************
// Initialize ping 2 interface.
void Ping2_Init(void) {
  long status = OS_StartCritical();
  SYSCTL_RCGCGPIO_R |= 0x02;          // 1) Port B clock
  status = status;
//  GPIO_PORTB_AMSEL_R &= ~0x30;     // 3) disable analog for PB5 and PB4
//  GPIO_PORTB_PCTL_R &= ~0x00FF0000;// 4) configure as GPIO 
  GPIO_PORTB_DIR_R |= TRIGGER2;        // 5) PB5 output
  GPIO_PORTB_DIR_R &= ~ECHO2;       //    PB4 input
    
  GPIO_PORTB_AFSEL_R &= ~ECHO2|TRIGGER2;     // 6) normal function
  GPIO_PORTB_DEN_R |= ECHO2|TRIGGER2;        // 7) digital I/O on PB5-4
  GPIO_PORTB_IS_R &= ~ECHO2;        // 8) PB4 is edge sensitive
  GPIO_PORTB_IBE_R |= ECHO2;       // 9) PB4 is not both edges
                                   // set interrupt priority
//  NVIC_PRI0_R = (NVIC_PRI0_R & 0xFFFF00FFF) | (PING_PRIORITY << 13);
  GPIO_PORTB_ICR_R |= ECHO2;
  GPIO_PORTB_IM_R |= ECHO2;
  NVIC_PRI0_R = (NVIC_PRI0_R & 0xFFFF00FF) | (0x00000000);
  NVIC_EN0_R |= 2;          // Enable in NVIC_EN0
  OS_EndCritical(status);
  GPIO_PORTB_DATA_R = 0;
  lastFlag[1] = 0;
  Ping_Fifo2_Init(8);                // link ping))) 2 to OS fifo 2
}

//********* Ping3_Init ***************
// Initialize ping 3 interface.
void Ping3_Init(void) {
  long status = OS_StartCritical();
  SYSCTL_RCGCGPIO_R |= 0x02;          // 1) Port B clock
  status = status;
//  GPIO_PORTB_AMSEL_R &= ~0x0C;     // 3) disable analog for PB3 and PB2
//  GPIO_PORTB_PCTL_R &= ~0x0000FF00;// 4) configure as GPIO 
  GPIO_PORTB_DIR_R |= TRIGGER3;        // 5) PB3 output
  GPIO_PORTB_DIR_R &= ~ECHO3;       //    PB2 input
    
  GPIO_PORTB_AFSEL_R &= ~ECHO3|TRIGGER3;     // 6) normal function
  GPIO_PORTB_DEN_R |= ECHO3|TRIGGER3;        // 7) digital I/O on PB5-4
  GPIO_PORTB_IS_R &= ~ECHO3;        // 8) PB2 is edge sensitive
  GPIO_PORTB_IBE_R |= ECHO3;        // 9) PB2 is both edges
                                   // set interrupt priority
//  NVIC_PRI0_R = (NVIC_PRI0_R & 0xFFFFF00FF) | (PING_PRIORITY << 13);
  NVIC_PRI0_R = (NVIC_PRI0_R & 0xFFFFF00FF) | (0x00000000);
  GPIO_PORTB_ICR_R |= ECHO3;
  GPIO_PORTB_IM_R |= ECHO3;
  
  NVIC_EN0_R |= 2;          // Enable in NVIC_EN0
  OS_EndCritical(status);
  GPIO_PORTB_DATA_R = 0;
  lastFlag[2] = 0;
  Ping_Fifo3_Init(8);                // link ping))) 3 to OS fifo 3
}


//********* Ping4_Init ***************
// Initialize ping 4 interface.
void Ping4_Init(void) {
  long status = OS_StartCritical();
  SYSCTL_RCGCGPIO_R |= 0x04;          // 1) Port C clock
  SYSCTL_RCGCGPIO_R |= 0x20;          // 1.5) Port F clock
  status = status;
  GPIO_PORTC_DIR_R |= TRIGGER4;        // 5) PC5 output
  GPIO_PORTF_DIR_R &= ~ECHO4;       //    PF4 input
    
  GPIO_PORTC_AFSEL_R &= ~TRIGGER4;     // 6) normal function
  GPIO_PORTF_AFSEL_R &= ~ECHO4;
  GPIO_PORTC_DEN_R |= TRIGGER4;        // 7) digital I/O on PC5, PF4
  GPIO_PORTF_DEN_R |= ECHO4;
  GPIO_PORTF_IS_R &= ~ECHO4;        // 8) PC4 is edge sensitive
  GPIO_PORTF_IBE_R |= ECHO4;       // 9) PC4 is not both edges
  GPIO_PORTF_ICR_R |= ECHO4;
  GPIO_PORTF_IM_R |= ECHO4;
//  NVIC_PRI0_R = (NVIC_PRI0_R & 0xFF00FFFF) | (PING_PRIORITY << 21);
                                     // set interrupt priority
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|(0x00000000); 
  NVIC_EN0_R |= 1<<30; // enable interrupt 4 in NVIC
  OS_EndCritical(status);
  GPIO_PORTC_DATA_R = 0;
  lastFlag[3] = 0;
  Ping_Fifo4_Init(8);                // link ping))) 4 to OS fifo 4
}

#define PING_FIFO_SIZE   8       // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS    1         // return value on success
#define FIFOFAIL       0         // return value on failure
static Sema4Type Fifo1_Mutex;
static Sema4Type Fifo1_CurrentSize;
static Sema4Type Fifo2_Mutex;
static Sema4Type Fifo2_CurrentSize;
static Sema4Type Fifo3_Mutex;
static Sema4Type Fifo3_CurrentSize;
static Sema4Type Fifo4_Mutex;
static Sema4Type Fifo4_CurrentSize;

AddIndexFifo(Ping1, PING_FIFO_SIZE, uint32_t, FIFOSUCCESS, FIFOFAIL)
AddIndexFifo(Ping2, PING_FIFO_SIZE, uint32_t, FIFOSUCCESS, FIFOFAIL)
AddIndexFifo(Ping3, PING_FIFO_SIZE, uint32_t, FIFOSUCCESS, FIFOFAIL)
AddIndexFifo(Ping4, PING_FIFO_SIZE, uint32_t, FIFOSUCCESS, FIFOFAIL)

// ******** Ping_Fifo1_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void Ping_Fifo1_Init(unsigned long size) {
  long status = OS_StartCritical();
  Ping1Fifo_Init();
  OS_InitSemaphore(&Fifo1_CurrentSize, 0);
  OS_InitSemaphore(&Fifo1_Mutex, 1);
  OS_EndCritical(status);
}

// ******** Ping_Fifo1_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int Ping_Fifo1_Put(unsigned long data) {
  int status = 0;
  status = Ping1Fifo_Put(data);
  OS_Signal(&Fifo1_CurrentSize);
	return status;
}

// ******** Ping_Fifo1_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long Ping_Fifo1_Get(void) {
  uint32_t retValue = 0;
  OS_Wait(&Fifo1_CurrentSize);
  OS_Wait(&Fifo1_Mutex);
  Ping1Fifo_Get(&retValue);
  OS_Signal(&Fifo1_Mutex);
	return (long)retValue;
}

// ******** Ping_Fifo1_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long Ping_Fifo1_Size(void) {
	return PING_FIFO_SIZE;
}

// ******** Ping_Fifo2_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void Ping_Fifo2_Init(unsigned long size) {
  long status = OS_StartCritical();
  Ping2Fifo_Init();
  OS_InitSemaphore(&Fifo2_CurrentSize, 0);
  OS_InitSemaphore(&Fifo2_Mutex, 1);
  OS_EndCritical(status);
}

// ******** Ping_Fifo2_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int Ping_Fifo2_Put(unsigned long data) {
  int status = 0;
  status = Ping2Fifo_Put(data);
  OS_Signal(&Fifo2_CurrentSize);
	return status;
}

// ******** Ping_Fifo2_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long Ping_Fifo2_Get(void) {
  uint32_t retValue = 0;
  OS_Wait(&Fifo2_CurrentSize);
  OS_Wait(&Fifo2_Mutex);
  Ping2Fifo_Get(&retValue);
  OS_Signal(&Fifo2_Mutex);
	return (long)retValue;
}

// ******** Ping_Fifo2_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long Ping_Fifo2_Size(void) {
	return PING_FIFO_SIZE;
}

// ******** Ping_Fifo3_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void Ping_Fifo3_Init(unsigned long size) {
  long status = OS_StartCritical();
  Ping3Fifo_Init();
  OS_InitSemaphore(&Fifo3_CurrentSize, 0);
  OS_InitSemaphore(&Fifo3_Mutex, 1);
  OS_EndCritical(status);
}

// ******** Ping_Fifo3_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int Ping_Fifo3_Put(unsigned long data) {
  int status = 0;
  status = Ping3Fifo_Put(data);
  OS_Signal(&Fifo3_CurrentSize);
	return status;
}

// ******** Ping_Fifo3_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long Ping_Fifo3_Get(void) {
  uint32_t retValue = 0;
  OS_Wait(&Fifo3_CurrentSize);
  OS_Wait(&Fifo3_Mutex);
  Ping3Fifo_Get(&retValue);
  OS_Signal(&Fifo3_Mutex);
	return (long)retValue;
}

// ******** Ping_Fifo3_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long Ping_Fifo3_Size(void) {
	return PING_FIFO_SIZE;
}

// ******** Ping_Fifo4_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void Ping_Fifo4_Init(unsigned long size) {
  long status = OS_StartCritical();
  Ping4Fifo_Init();
  OS_InitSemaphore(&Fifo4_CurrentSize, 0);
  OS_InitSemaphore(&Fifo4_Mutex, 1);
  OS_EndCritical(status);
}

// ******** Ping_Fifo4_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int Ping_Fifo4_Put(unsigned long data) {
  int status = 0;
  status = Ping4Fifo_Put(data);
  OS_Signal(&Fifo4_CurrentSize);
	return status;
}

// ******** Ping_Fifo4_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long Ping_Fifo4_Get(void) {
  uint32_t retValue = 0;
  OS_Wait(&Fifo4_CurrentSize);
  OS_Wait(&Fifo4_Mutex);
  Ping4Fifo_Get(&retValue);
  OS_Signal(&Fifo4_Mutex);
	return (long)retValue;
}

// ******** Ping_Fifo4_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long Ping_Fifo4_Size(void) {
	return PING_FIFO_SIZE;
}

#define BLIND_USEC_5    420       // ayy lmao (do a little over 5 usec just in case)
void OS_DisableInterrupts(void);
void OS_EnableInterrupts(void);
void EnableInterrupts(void);
void DisableInterrupts(void);
static void wait5uSec(void) {
  OS_DisableInterrupts();
  int i = 0;
  while (i < BLIND_USEC_5) {
    ++i;
  }
  OS_EnableInterrupts();
}



//********* Ping1_PollThread *********
// Use this thread to poll ping interface 1
void Ping1_PollThread(void) {
  lastTime[0] = 0;
  while(1) {
    GPIO_PORTB_DATA_R &= ~(TRIGGER1);
    GPIO_PORTB_DATA_R |= TRIGGER1;
    wait5uSec();
    OS_DisableInterrupts();
    GPIO_PORTB_DATA_R &= ~TRIGGER1;
    lastTime[0] = OS_Time();
    OS_EnableInterrupts();
    OS_Sleep(POLL_RATE);
  }
}

//********* Ping2_PollThread *********
// Use this thread to poll ping interface 2
void Ping2_PollThread(void) {
  lastTime[1] = 0;
  while(1) {
    GPIO_PORTB_DATA_R &= ~TRIGGER2;
    GPIO_PORTB_DATA_R |= TRIGGER2;
    wait5uSec();
    OS_DisableInterrupts();
    GPIO_PORTB_DATA_R &= ~TRIGGER2;
    lastTime[1] = OS_Time();
    OS_EnableInterrupts();
    OS_Sleep(POLL_RATE);
  }
}

//********* Ping3_PollThread *********
// Use this thread to poll ping interface 3
void Ping3_PollThread(void) {
  lastTime[2] = 0;
  while(1) {
    GPIO_PORTB_DATA_R &= ~TRIGGER3;
    GPIO_PORTB_DATA_R |= TRIGGER3;
    wait5uSec();
    OS_DisableInterrupts();
    GPIO_PORTB_DATA_R &= ~TRIGGER3;
    lastTime[2] = OS_Time();
    OS_EnableInterrupts();
    OS_Sleep(POLL_RATE);
  }
}

//********* Ping4_PollThread *********
// Use this thread to poll ping interface 4
void Ping4_PollThread(void) {
  lastTime[3] = 0;
  while(1) {
    GPIO_PORTC_DATA_R &= ~(TRIGGER4);
    GPIO_PORTC_DATA_R |= (TRIGGER4);
    wait5uSec();
    OS_DisableInterrupts();
    GPIO_PORTC_DATA_R &= ~(TRIGGER4);
    lastTime[3] = OS_Time();
    OS_EnableInterrupts();
    OS_Sleep(POLL_RATE);  // wait 100 ms
  }
}

long processPingData(uint8_t pingNum, int flag) {
  if (!lastFlag[pingNum]) {
    lastTime[pingNum] = OS_Time();
    lastFlag[pingNum] = flag;
    return -1;
  } else {
    long thisTime = OS_Time();
    long timeDifference = OS_TimeDifference(thisTime, lastTime[pingNum]);
    long us = (timeDifference * 13 / 1000);
    long cm = (us + 29)/58;
    lastFlag[pingNum] = flag;
    return cm;
  }
}

void GPIOPortB_Handler(void){
  if (GPIO_PORTB_RIS_R&ECHO1) {
    int flag = GPIO_PORTB_DATA_R&ECHO1;
    long data = processPingData(0, flag);
    if (data != -1) {
      Ping_Fifo1_Put(data);
    }
    GPIO_PORTB_ICR_R |= ECHO1;
  } 
  if (GPIO_PORTB_RIS_R&ECHO2) {
    int flag = GPIO_PORTB_DATA_R&ECHO2;
    long data = processPingData(1, flag);
    if (data != -1) {
      Ping_Fifo2_Put(data);
    }
    GPIO_PORTB_ICR_R |= ECHO2;
  }
  if (GPIO_PORTB_RIS_R&ECHO3) {
    int flag = GPIO_PORTB_DATA_R&ECHO3;
    long data = processPingData(2, flag);
    if (data != -1) {
      Ping_Fifo3_Put(data);
    }
    GPIO_PORTB_ICR_R |= ECHO3;
  }
}

//void GPIOPortF_Handler() {
//  if (GPIO_PORTF_RIS_R&ECHO4) {
//    long data = processPingData(3);
//    if (data != -1) {
//      Ping_Fifo4_Put(data);
//    }
//    GPIO_PORTF_ICR_R |= ECHO4;
//  } 
//}

/* 40.64 cm
* 45
* 46
* 47
* 45
* 46
*
* 50.8 cm
* 53
* 53
* 53
* 53
* 53
* 
* 63.5 cm
* 66
* 67
* 68
* 67
* 66
* 
* 76.2 cm
* 80
* 81
* 81
* 80
* 81
*
* 91.44 cm
* 96
* 95
* 97
* 96
* 95
*
*

*/
