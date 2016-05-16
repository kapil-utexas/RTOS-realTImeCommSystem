// IR.h
#ifndef _IR_H_
#define _IR_H_

//********* IR_Init ***************
// Initialize ADC interface.
void IR_Init(void);

//********* IRSensor0 ***************
// Thread to continously sample sensor 0.
void IRSensor0Thread(void);

//********* IRSensor1 ***************
// Thread to continously sample sensor 1.
void IRSensor1Thread(void);

//********* IRSensor2 ***************
//Thread to continously sample sensor 2.
void IRSensor2Thread(void);

//********* IRSensor3 ***************
// Thread to continously sample sensor 3.
void IRSensor3Thread(void);

// ******** IR_Fifo0_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void IR_Fifo0_Init(unsigned long size);

// ******** IR_Fifo0_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int IR_Fifo0_Put(unsigned long data);

// ******** IR_Fifo0_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long IR_Fifo0_Get(void);

// ******** IR_Fifo0_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long IR_Fifo0_Size(void);

// ******** IR_Fifo1_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void IR_Fifo1_Init(unsigned long size);
// ******** IR_Fifo1_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int IR_Fifo1_Put(unsigned long data);

// ******** IR_Fifo1_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long IR_Fifo1_Get(void);
// ******** IR_Fifo1_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long IR_Fifo1_Size(void);

// ******** IR_Fifo2_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void IR_Fifo2_Init(unsigned long size);

// ******** IR_Fifo2_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int IR_Fifo2_Put(unsigned long data);

// ******** IR_Fifo2_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long IR_Fifo2_Get(void);

// ******** IR_Fifo2_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long IR_Fifo2_Size(void);

// ******** IR_Fifo3_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
void IR_Fifo3_Init(unsigned long size);

// ******** IR_Fifo3_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
// this function can not disable or enable interrupts
int IR_Fifo3_Put(unsigned long data);

// ******** IR_Fifo3_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long IR_Fifo3_Get(void);

// ******** IR_Fifo3_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long IR_Fifo3_Size(void);

#endif /* _IR_H_ */
