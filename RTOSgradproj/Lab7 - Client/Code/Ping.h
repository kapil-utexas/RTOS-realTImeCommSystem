// Ping.h
// Written by Allen Wang and Alvin Tung
// Used to interface with various pinging modules.

// ***************************************
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

#ifndef _PING_H_
#define _PING_H_

#include <stdint.h>

#define PING_PRIORITY (int)1

#define ECHO1 0x40
#define ECHO2 0x10
#define ECHO3 0x04
#define ECHO4 0x10

#define TRIGGER1 0x80
#define TRIGGER2 0x20
#define TRIGGER3 0x08
#define TRIGGER4 0x20

// ******* Ping_Init *****************
void Ping_Init(void);

//********* Ping1_Init ***************
// Initialize ping 1 interface.
void Ping1_Init(void);

//********* Ping2_Init ***************
// Initialize ping 2 interface.
void Ping2_Init(void);

//********* Ping3_Init ***************
// Initialize ping 3 interface.
void Ping3_Init(void);

//********* Ping4_Init ***************
// Initialize ping 4 interface.
void Ping4_Init(void);

//********* Ping1_PollThread *********
// Use this thread to poll ping interface 1
void Ping1_PollThread(void);

//********* Ping1_PollThread *********
// Use this thread to poll ping interface 2
void Ping2_PollThread(void);

//********* Ping1_PollThread *********
// Use this thread to poll ping interface 3
void Ping3_PollThread(void);

//********* Ping1_PollThread *********
// Use this thread to poll ping interface 4
void Ping4_PollThread(void);
 
// ******** Ping_Fifo1_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void Ping_Fifo1_Init(unsigned long size);

// ******** Ping_Fifo1_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int Ping_Fifo1_Put(unsigned long data);  

// ******** Ping_Fifo1_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long Ping_Fifo1_Get(void);

// ******** Ping_Fifo1_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to Ping_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to Ping_Fifo_Get will spin or block
long Ping_Fifo1_Size(void);

// ******** Ping_Fifo2_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void Ping_Fifo2_Init(unsigned long size);

// ******** Ping_Fifo2_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int Ping_Fifo2_Put(unsigned long data);  

// ******** Ping_Fifo2_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long Ping_Fifo2_Get(void);

// ******** Ping_Fifo2_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to Ping_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to Ping_Fifo_Get will spin or block
long Ping_Fifo2_Size(void);

// ******** Ping_Fifo3_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void Ping_Fifo3_Init(unsigned long size);

// ******** Ping_Fifo3_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int Ping_Fifo3_Put(unsigned long data);  

// ******** Ping_Fifo3_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long Ping_Fifo3_Get(void);

// ******** Ping_Fifo3_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to Ping_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to Ping_Fifo_Get will spin or block
long Ping_Fifo3_Size(void);

// ******** Ping_Fifo4_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void Ping_Fifo4_Init(unsigned long size);

// ******** Ping_Fifo4_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int Ping_Fifo4_Put(unsigned long data);  

// ******** Ping_Fifo4_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long Ping_Fifo4_Get(void);

// ******** Ping_Fifo4_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to Ping_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to Ping_Fifo_Get will spin or block
long Ping_Fifo4_Size(void);

long processPingData(uint8_t pingNum, int flag);
#endif /* _PING_H_ */
