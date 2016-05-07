// IR.c
#include <stdint.h>
#include "UART.h"
#include "OS.h"
#include "Fifo.h"
#include "IR.h"
#include "inc/tm4c123gh6pm.h"
#include "ADC.h"
#include "LinearInterpolation.h"

static Sema4Type mutex;

// Non-recursive, 3-point median filter
static unsigned short Median(unsigned short u1,unsigned short u2,unsigned short u3) {
	if(u1>u2) {
	if(u2>u3) return u2; // u1>u2,u2>u3 u1>u2>u3
	if(u1>u3) return u3; // u1>u2,u3>u2,u1>u3 u1>u3>u2
	return u1; // u1>u2,u3>u2,u3>u1 u3>u1>u2
	}
	else {
	if(u3>u2) return u2; // u2>u1,u3>u2 u3>u2>u1
	if(u1>u3) return u1; // u2>u1,u2>u3,u1>u3 u2>u1>u3
	return u3; // u2>u1,u2>u3,u3>u1 u2>u3>u1
	}
}
//need to sample at 12.5hz since max frequency is 25hz
uint8_t ADCData0[4];

void IR_Init() {
	ADC_Init(0);
  OS_InitSemaphore(&mutex, 1);
  IR_Fifo0_Init(16);
  IR_Fifo1_Init(16);
  IR_Fifo2_Init(16);
  IR_Fifo3_Init(16);
}

void IRSensor0Thread(void) {
	//ADC_Init(0);
	unsigned short sample1;
	unsigned short sample2;
	unsigned short sample3;
	unsigned short filteredSample;
	while(1) {
    OS_Wait(&mutex);
		ADC_Open(0);
		sample1 = ADC_In();
		sample2 = ADC_In();
		sample3 = ADC_In();
    OS_Signal(&mutex);
		filteredSample = Median(sample1, sample2, sample3);		
		filteredSample = ADC_ConvertToVolts(filteredSample);
		filteredSample = Linear_Interpolation(filteredSample);
    IR_Fifo0_Put(filteredSample);
    OS_Sleep(POLL_RATE);
	}
}

uint8_t ADCData1[4];
void IRSensor1Thread(void) {
	unsigned short sample1;
	unsigned short sample2;
	unsigned short sample3;
	unsigned short filteredSample;
	while(1) {
    OS_Wait(&mutex);
		ADC_Open(1);
		sample1 = ADC_In();
		sample2 = ADC_In();
		sample3 = ADC_In();
    OS_Signal(&mutex);
		filteredSample = Median(sample1, sample2, sample3);		
		filteredSample = ADC_ConvertToVolts(filteredSample);
		filteredSample = Linear_Interpolation(filteredSample);
    IR_Fifo1_Put(filteredSample);
    OS_Sleep(POLL_RATE);
	}
}
uint8_t ADCData2[4];
void IRSensor2Thread(void) {
	unsigned short sample1;
	unsigned short sample2;
	unsigned short sample3;
	unsigned short filteredSample;
	while(1) {
    OS_Wait(&mutex);
		ADC_Open(2);
		sample1 = ADC_In();
		sample2 = ADC_In();
		sample3 = ADC_In();
    OS_Signal(&mutex);
		filteredSample = Median(sample1, sample2, sample3);		
		filteredSample = ADC_ConvertToVolts(filteredSample);
		filteredSample = Linear_Interpolation(filteredSample);
    IR_Fifo2_Put(filteredSample);
    OS_Sleep(POLL_RATE);
	}
}
uint8_t ADCData3[4];
void IRSensor3Thread(void) {
	unsigned short sample1;
	unsigned short sample2;
	unsigned short sample3;
	unsigned short filteredSample;
	while(1) {
    OS_Wait(&mutex);
		ADC_Open(3);
		sample1 = ADC_In();
		sample2 = ADC_In();
		sample3 = ADC_In();
    OS_Signal(&mutex);
		filteredSample = Median(sample1, sample2, sample3);		
		filteredSample = ADC_ConvertToVolts(filteredSample);
		filteredSample = Linear_Interpolation(filteredSample);
    IR_Fifo3_Put(filteredSample);
    OS_Sleep(POLL_RATE);
	}
}

#define IR_FIFO_SIZE   8         // size of the FIFOs (must be power of 2)
#define FIFOSUCCESS    1         // return value on success
#define FIFOFAIL       0         // return value on failure
static Sema4Type Fifo0_Mutex;
static Sema4Type Fifo0_CurrentSize;
static Sema4Type Fifo1_Mutex;
static Sema4Type Fifo1_CurrentSize;
static Sema4Type Fifo2_Mutex;
static Sema4Type Fifo2_CurrentSize;
static Sema4Type Fifo3_Mutex;
static Sema4Type Fifo3_CurrentSize;

AddIndexFifo(IR0, IR_FIFO_SIZE, uint32_t, FIFOSUCCESS, FIFOFAIL)
AddIndexFifo(IR1, IR_FIFO_SIZE, uint32_t, FIFOSUCCESS, FIFOFAIL)
AddIndexFifo(IR2, IR_FIFO_SIZE, uint32_t, FIFOSUCCESS, FIFOFAIL)
AddIndexFifo(IR3, IR_FIFO_SIZE, uint32_t, FIFOSUCCESS, FIFOFAIL)

// ******** IR_Fifo0_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void IR_Fifo0_Init(unsigned long size) {
  long status = OS_StartCritical();
  IR0Fifo_Init();
  OS_InitSemaphore(&Fifo0_CurrentSize, 0);
  OS_InitSemaphore(&Fifo0_Mutex, 1);
  OS_EndCritical(status);
}

// ******** IR_Fifo0_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int IR_Fifo0_Put(unsigned long data) {
  int status = 0;
  status = IR0Fifo_Put(data);
  OS_Signal(&Fifo0_CurrentSize);
	return status;
}

// ******** IR_Fifo0_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long IR_Fifo0_Get(void) {
  uint32_t retValue = 0;
  OS_Wait(&Fifo0_CurrentSize);
  OS_Wait(&Fifo0_Mutex);
  IR0Fifo_Get(&retValue);
  OS_Signal(&Fifo0_Mutex);
	return (long)retValue;
}

// ******** IR_Fifo0_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long IR_Fifo0_Size(void) {
	return IR_FIFO_SIZE;
}

// ******** IR_Fifo1_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void IR_Fifo1_Init(unsigned long size) {
  long status = OS_StartCritical();
  IR1Fifo_Init();
  OS_InitSemaphore(&Fifo1_CurrentSize, 0);
  OS_InitSemaphore(&Fifo1_Mutex, 1);
  OS_EndCritical(status);
}

// ******** IR_Fifo1_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int IR_Fifo1_Put(unsigned long data) {
  int status = 0;
  status = IR1Fifo_Put(data);
  OS_Signal(&Fifo1_CurrentSize);
	return status;
}

// ******** IR_Fifo1_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long IR_Fifo1_Get(void) {
  uint32_t retValue = 0;
  OS_Wait(&Fifo1_CurrentSize);
  OS_Wait(&Fifo1_Mutex);
  IR1Fifo_Get(&retValue);
  OS_Signal(&Fifo1_Mutex);
	return (long)retValue;
}

// ******** IR_Fifo1_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long IR_Fifo1_Size(void) {
	return IR_FIFO_SIZE;
}

// ******** IR_Fifo2_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void IR_Fifo2_Init(unsigned long size) {
  long status = OS_StartCritical();
  IR2Fifo_Init();
  OS_InitSemaphore(&Fifo2_CurrentSize, 0);
  OS_InitSemaphore(&Fifo2_Mutex, 1);
  OS_EndCritical(status);
}

// ******** IR_Fifo2_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int IR_Fifo2_Put(unsigned long data) {
  int status = 0;
  status = IR2Fifo_Put(data);
  OS_Signal(&Fifo2_CurrentSize);
	return status;
}

// ******** IR_Fifo2_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long IR_Fifo2_Get(void) {
  uint32_t retValue = 0;
  OS_Wait(&Fifo2_CurrentSize);
  OS_Wait(&Fifo2_Mutex);
  IR2Fifo_Get(&retValue);
  OS_Signal(&Fifo2_Mutex);
	return (long)retValue;
}

// ******** IR_Fifo2_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long IR_Fifo2_Size(void) {
	return IR_FIFO_SIZE;
}

// ******** IR_Fifo3_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void IR_Fifo3_Init(unsigned long size) {
  long status = OS_StartCritical();
  IR3Fifo_Init();
  OS_InitSemaphore(&Fifo3_CurrentSize, 0);
  OS_InitSemaphore(&Fifo3_Mutex, 1);
  OS_EndCritical(status);
}

// ******** IR_Fifo3_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int IR_Fifo3_Put(unsigned long data) {
  int status = 0;
  status = IR3Fifo_Put(data);
  OS_Signal(&Fifo3_CurrentSize);
	return status;
}

// ******** IR_Fifo3_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long IR_Fifo3_Get(void) {
  uint32_t retValue = 0;
  OS_Wait(&Fifo3_CurrentSize);
  OS_Wait(&Fifo3_Mutex);
  IR3Fifo_Get(&retValue);
  OS_Signal(&Fifo3_Mutex);
	return (long)retValue;
}

// ******** IR_Fifo3_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long IR_Fifo3_Size(void) {
	return IR_FIFO_SIZE;
}
