// ADC.c
// Runs on LM4F120/TM4C123
// Provide a function that initializes Timer0A to trigger ADC
// SS3 conversions and request an interrupt when the conversion
// is complete.
// Allen Wang and Alvin Tung
// 23 Jan 2016

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

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
#include "ADC.h"
#include <stdint.h>
#include <stdbool.h>
#include "../inc/tm4c123gh6pm.h"
#include "OS.h"
#define NVIC_EN0_INT17          0x00020000  // Interrupt 17 enable

#define TIMER_CFG_16_BIT        0x00000004  // 16-bit timer configuration,
                                            // function is controlled by bits
                                            // 1:0 of GPTMTAMR and GPTMTBMR
#define TIMER_CFG_32_BIT        0x00000000
#define TIMER_TAMR_TACDIR       0x00000010  // GPTM Timer A Count Direction
#define TIMER_TAMR_TAMR_PERIOD  0x00000002  // Periodic Timer mode
#define TIMER_CTL_TAOTE         0x00000020  // GPTM TimerA Output Trigger
                                            // Enable
#define TIMER_CTL_TAEN          0x00000001  // GPTM TimerA Enable
#define TIMER_CTL_TADA          0x00000000  // GPTM TimerA Disable
#define TIMER_IMR_TATOIM        0x00000001  // GPTM TimerA Time-Out Interrupt
                                            // Mask
#define TIMER_TAILR_TAILRL_M    0x0000FFFF  // GPTM TimerA Interval Load
                                            // Register Low

#define ADC_ACTSS_ASEN3         0x00000008  // ADC SS3 Enable
#define ADC_RIS_INR3            0x00000008  // SS3 Raw Interrupt Status
#define ADC_IM_MASK3            0x00000008  // SS3 Interrupt Mask
#define ADC_ISC_IN3             0x00000008  // SS3 Interrupt Status and Clear
#define ADC_EMUX_EM3_M          0x0000F000  // SS3 Trigger Select mask
#define ADC_EMUX_EM3_TIMER      0x00005000  // Timer
#define ADC_EMUX_EM3_PROCESSOR  0x00000000  // Processor
#define ADC_SSPRI_SS3_4TH       0x00003000  // fourth priority
#define ADC_SSPRI_SS2_3RD       0x00000200  // third priority
#define ADC_SSPRI_SS1_2ND       0x00000010  // second priority
#define ADC_SSPRI_SS0_1ST       0x00000000  // first priority
#define ADC_PSSI_SS3            0x00000008  // SS3 Initiate
#define ADC_SSCTL3_TS0          0x00000008  // 1st Sample Temp Sensor Select
#define ADC_SSCTL3_IE0          0x00000004  // 1st Sample Interrupt Enable
#define ADC_SSCTL3_END0         0x00000002  // 1st Sample is End of Sequence
#define ADC_SSCTL3_D0           0x00000001  // 1st Sample Diff Input Select
#define ADC_SSCTL3_M            0x0000000F  // 1st Sample Mask
#define ADC_SSFIFO3_DATA_M      0x00000FFF  // Conversion Result Data mask
#define ADC_PC_SR_M             0x0000000F  // ADC Sample Rate
#define ADC_PC_SR_125K          0x00000001  // 125 ksps
#define SYSCTL_RCGCGPIO_R4      0x00000010  // GPIO Port E Run Mode Clock
                                            // Gating Control
#define SYSCTL_RCGCGPIO_R3      0x00000008  // GPIO Port D Run Mode Clock
                                            // Gating Control
#define SYSCTL_RCGCGPIO_R1      0x00000002  // GPIO Port B Run Mode Clock
                                            // Gating Control

void OS_DisableInterrupts(void); // Disable interrupts
void OS_EnableInterrupts(void);  // Enable interrupts
long OS_StartCritical (void);    // previous I bit, disable interrupts
void OS_EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

// There are many choices to make when using the ADC, and many
// different combinations of settings will all do basically the
// same thing.  For simplicity, this function makes some choices
// for you.  When calling this function, be sure that it does
// not conflict with any other software that may be running on
// the microcontroller.  Particularly, ADC0 sample sequencer 3
// is used here because it only takes one sample, and only one
// sample is absolutely needed.  Sample sequencer 3 generates a
// raw interrupt when the conversion is complete, and it is then
// promoted to an ADC0 controller interrupt.  Hardware Timer0A
// triggers the ADC0 conversion at the programmed interval, and
// software handles the interrupt to process the measurement
// when it is complete.

#define MAX_ADC_VALUE ((1<<12) - 1)
#define ROUND_ADC_VALUE (1<<11)
#define VREF 333

static volatile bool collect_complete;
//static volatile unsigned short* collect_buffer;
static volatile unsigned int collect_numberOfSamples, collect_sampleNum;

//---------------ADC_ChannelInit---------------
// Initializes port for ADC channel
// Ouput: none
// Input: unsigned int, 'channelNum' [0:11]
static void ADC_ChannelInit(uint8_t channelNum){
  volatile uint32_t delay;
  // **** GPIO pin initialization ****
  switch(channelNum){             // 1) activate clock
    case 0:
    case 1:
    case 2:
    case 3:
    case 8:
    case 9:                       //    these are on GPIO_PORTE
      SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4; break;
    case 4:
    case 5:
    case 6:
    case 7:                       //    these are on GPIO_PORTD
      SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3; break;
    case 10:
    case 11:                      //    these are on GPIO_PORTB
      SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1; break;
    default: return;              //    0 to 11 are valid channels on the LM4F120
  }
  delay = SYSCTL_RCGCGPIO_R;      // 2) allow time for clock to stabilize
  delay = SYSCTL_RCGCGPIO_R;
  switch(channelNum){
    case 0:                       //      Ain0 is on PE3
      GPIO_PORTE_DIR_R &= ~0x08;  // 3.0) make PE3 input
      GPIO_PORTE_AFSEL_R |= 0x08; // 4.0) enable alternate function on PE3
      GPIO_PORTE_DEN_R &= ~0x08;  // 5.0) disable digital I/O on PE3
      GPIO_PORTE_AMSEL_R |= 0x08; // 6.0) enable analog functionality on PE3
      break;
    case 1:                       //      Ain1 is on PE2
      GPIO_PORTE_DIR_R &= ~0x04;  // 3.1) make PE2 input
      GPIO_PORTE_AFSEL_R |= 0x04; // 4.1) enable alternate function on PE2
      GPIO_PORTE_DEN_R &= ~0x04;  // 5.1) disable digital I/O on PE2
      GPIO_PORTE_AMSEL_R |= 0x04; // 6.1) enable analog functionality on PE2
      break;
    case 2:                       //      Ain2 is on PE1
      GPIO_PORTE_DIR_R &= ~0x02;  // 3.2) make PE1 input
      GPIO_PORTE_AFSEL_R |= 0x02; // 4.2) enable alternate function on PE1
      GPIO_PORTE_DEN_R &= ~0x02;  // 5.2) disable digital I/O on PE1
      GPIO_PORTE_AMSEL_R |= 0x02; // 6.2) enable analog functionality on PE1
      break;
    case 3:                       //      Ain3 is on PE0
      GPIO_PORTE_DIR_R &= ~0x01;  // 3.3) make PE0 input
      GPIO_PORTE_AFSEL_R |= 0x01; // 4.3) enable alternate function on PE0
      GPIO_PORTE_DEN_R &= ~0x01;  // 5.3) disable digital I/O on PE0
      GPIO_PORTE_AMSEL_R |= 0x01; // 6.3) enable analog functionality on PE0
      break;
    case 4:                       //      Ain4 is on PD3
      GPIO_PORTD_DIR_R &= ~0x08;  // 3.4) make PD3 input
      GPIO_PORTD_AFSEL_R |= 0x08; // 4.4) enable alternate function on PD3
      GPIO_PORTD_DEN_R &= ~0x08;  // 5.4) disable digital I/O on PD3
      GPIO_PORTD_AMSEL_R |= 0x08; // 6.4) enable analog functionality on PD3
      break;
    case 5:                       //      Ain5 is on PD2
      GPIO_PORTD_DIR_R &= ~0x04;  // 3.5) make PD2 input
      GPIO_PORTD_AFSEL_R |= 0x04; // 4.5) enable alternate function on PD2
      GPIO_PORTD_DEN_R &= ~0x04;  // 5.5) disable digital I/O on PD2
      GPIO_PORTD_AMSEL_R |= 0x04; // 6.5) enable analog functionality on PD2
      break;
    case 6:                       //      Ain6 is on PD1
      GPIO_PORTD_DIR_R &= ~0x02;  // 3.6) make PD1 input
      GPIO_PORTD_AFSEL_R |= 0x02; // 4.6) enable alternate function on PD1
      GPIO_PORTD_DEN_R &= ~0x02;  // 5.6) disable digital I/O on PD1
      GPIO_PORTD_AMSEL_R |= 0x02; // 6.6) enable analog functionality on PD1
      break;
    case 7:                       //      Ain7 is on PD0
      GPIO_PORTD_DIR_R &= ~0x01;  // 3.7) make PD0 input
      GPIO_PORTD_AFSEL_R |= 0x01; // 4.7) enable alternate function on PD0
      GPIO_PORTD_DEN_R &= ~0x01;  // 5.7) disable digital I/O on PD0
      GPIO_PORTD_AMSEL_R |= 0x01; // 6.7) enable analog functionality on PD0
      break;
    case 8:                       //      Ain8 is on PE5
      GPIO_PORTE_DIR_R &= ~0x20;  // 3.8) make PE5 input
      GPIO_PORTE_AFSEL_R |= 0x20; // 4.8) enable alternate function on PE5
      GPIO_PORTE_DEN_R &= ~0x20;  // 5.8) disable digital I/O on PE5
      GPIO_PORTE_AMSEL_R |= 0x20; // 6.8) enable analog functionality on PE5
      break;
    case 9:                       //      Ain9 is on PE4
      GPIO_PORTE_DIR_R &= ~0x10;  // 3.9) make PE4 input
      GPIO_PORTE_AFSEL_R |= 0x10; // 4.9) enable alternate function on PE4
      GPIO_PORTE_DEN_R &= ~0x10;  // 5.9) disable digital I/O on PE4
      GPIO_PORTE_AMSEL_R |= 0x10; // 6.9) enable analog functionality on PE4
      break;
    case 10:                      //       Ain10 is on PB4
      GPIO_PORTB_DIR_R &= ~0x10;  // 3.10) make PB4 input
      GPIO_PORTB_AFSEL_R |= 0x10; // 4.10) enable alternate function on PB4
      GPIO_PORTB_DEN_R &= ~0x10;  // 5.10) disable digital I/O on PB4
      GPIO_PORTB_AMSEL_R |= 0x10; // 6.10) enable analog functionality on PB4
      break;
    case 11:                      //       Ain11 is on PB5
      GPIO_PORTB_DIR_R &= ~0x20;  // 3.11) make PB5 input
      GPIO_PORTB_AFSEL_R |= 0x20; // 4.11) enable alternate function on PB5
      GPIO_PORTB_DEN_R &= ~0x20;  // 5.11) disable digital I/O on PB5
      GPIO_PORTB_AMSEL_R |= 0x20; // 6.11) enable analog functionality on PB5
      break;
  }    
}

//---------------ADC_Init---------------
// Initialization function for T0ATrigger
// Ouput: none
// Input: period between ADC conversions and
//        ADC channel number
// This initialization function sets up the ADC according to the
// following parameters.  Any parameters not explicitly listed
// below are not modified:
// Timer0A: enabled
// Mode: 16-bit, down counting
// One-shot or periodic: default 1000
// Interval value: programmable using 32-bit period
// Sample time is busPeriod*period
// Max sample rate: <=125,000 samples/second
// Sequencer 0 priority: 1st (highest)
// Sequencer 1 priority: 2nd
// Sequencer 2 priority: 3rd
// Sequencer 3 priority: 4th (lowest)
// SS3 triggering event: Timer0A
// SS3 1st sample source: programmable using variable 'channelNum' [0:11]
// SS3 interrupts: enabled and promoted to controller
void ADC_Init(uint8_t channelNum){  
  volatile uint32_t delay;
  ADC_ChannelInit(channelNum);
  OS_DisableInterrupts();
  SYSCTL_RCGCADC_R |= 0x01;     // activate ADC0 
  SYSCTL_RCGCTIMER_R |= 0x01;   // activate timer0 
  delay = SYSCTL_RCGCTIMER_R;   // allow time to finish activating
  TIMER0_CTL_R &= ~TIMER_CTL_TAEN;    // disable timer0A during setup
  TIMER0_CTL_R |= TIMER_CTL_TAOTE;   // enable timer0A trigger to ADC
  TIMER0_CFG_R = TIMER_CFG_16_BIT; // configure for 16-bit timer mode
  TIMER0_TAMR_R = 0x00000002;   // configure for periodic mode, default down-count settings
  TIMER0_TAPR_R = T0A_PRESCALER; // prescale value for trigger
  TIMER0_TAILR_R = (SYSCLK_FREQ/1000); // start value for trigger
  TIMER0_IMR_R &= ~0x01;    // disable all interrupts
  //TIMER0_CTL_R &= TIMER_CTL_TADA; // disable timer0A 16-b, periodic, no interrupts
  ADC0_PC_R = 0x01;         // configure for 125K samples/sec
  ADC0_SSPRI_R = 0x3210;    // sequencer 0 is highest, sequencer 3 is lowest
  ADC0_ACTSS_R &= ~0x08;    // disable sample sequencer 3
  ADC0_EMUX_R = (ADC0_EMUX_R&0xFFFF0FFF)+0x5000; // timer trigger event
  ADC0_SSMUX3_R = channelNum;
  ADC0_SSCTL3_R = 0x06;          // set flag and end                       
  ADC0_IM_R |= ADC_IM_MASK3;     // enable SS3 interrupts
  ADC0_ACTSS_R |= 0x08;          // enable sample sequencer 3
  NVIC_PRI4_R = (NVIC_PRI4_R&0xFFFF00FF)|0x00004000; //priority 2 
  NVIC_EN0_R = 1<<17;             // enable interrupt 17 in NVIC
	OS_EnableInterrupts();
}

//---------------ADC_PeriodT0ATrigger---------------
// Update T0A Timer period
// Ouput: none
// Input: period between ADC conversions
static void ADC_PeriodT0ATrigger(uint32_t period){
  TIMER0_TAILR_R = (period-1) & 0x0000FFFF;    // start value for trigger
}

//---------------ADC_EnableT0ATrigger---------------
// Enable Timer0 for T0ATrigger
// Ouput: none
// Input: none
static void ADC_EnableT0ATrigger(void){
  TIMER0_CTL_R |= (TIMER_CTL_TAOTE | TIMER_CTL_TAEN); // enable timer0A 16-b, periodic, ADC interrupt trigger
}

//---------------ADC_DisableT0ATrigger---------------
// Disable Timer0 for T0ATrigger
// Ouput: none
// Input: none
static void ADC_DisableT0ATrigger(void){
  TIMER0_CTL_R &= ~(TIMER_CTL_TAOTE | TIMER_CTL_TAEN); // disable timer0A 16-b, periodic, no interrupts
}

//---------------ADC_Open---------------
// Changes current ADC channel
// Ouput: none
// Input: unsigned int, 'channelNum' [0:11]
void ADC_Open(unsigned int channelNum){
  ADC_ChannelInit(channelNum);
  ADC0_SSMUX3_R = channelNum;
}

//---------------ADC_In---------------
// Busy-wait captures ADC single sample **(maybe use interrupt?)
// Ouput: 16 bit ADC data value 
// Input: none
unsigned short ADC_In(void){uint16_t result;

  // Setup ADC for single sample
  ADC0_EMUX_R = (ADC0_EMUX_R&0xFFFF0FFF)+ADC_EMUX_EM3_PROCESSOR; // change to processor	
  ADC0_IM_R &= ~ADC_IM_MASK3; // disable SS3 interrupt
	
  // Start and capture single sample  
  ADC0_PSSI_R = 0x0008;            // 1) initiate SS3
  while((ADC0_RIS_R&0x08)==0){};   // 2) wait for conversion done
  result = ADC0_SSFIFO3_R&0xFFF;   // 3) read result
  ADC0_ISC_R = 0x0008;             // 4) acknowledge completion
    
  // Restore Timer0A Trigger ADC settings		
  ADC0_EMUX_R = (ADC0_EMUX_R&0xFFFF0FFF)+0x5000; // timer trigger event
  ADC0_IM_R |= ADC_IM_MASK3; // enable SS3 interrupts
    
  return result;
}
	
//---------------ADC_Status--------------
// Status of ADC if collection is complete
// Ouput: 0 for complete, 1 for in progress 
// Input: none
uint8_t ADC_Status(void){
  return collect_complete;
}

void (*ADCTask)(unsigned long);   // user function

//---------------ADC_Collect---------------
// Uses T0A interrupt to collect ADC samples
// Ouput: ADC_Status
// Input: The channel, frequency, and size to 
//        sample and where to store values 
int ADC_Collect(unsigned int channelNum, unsigned int fs, 
  void(*task)(unsigned long)){
  
  //Setup ADC for collection using parameters
  ADC_DisableT0ATrigger(); // turn off any previous conversions
		
  ADC_Open(channelNum); // change channel 
  ADC_PeriodT0ATrigger(SYSCLK_FREQ/((fs)*(T0A_PRESCALER+1))); // change timer period
  
  ADCTask = task;
    
  // Initialize collect variables for interrupt handler
  collect_complete = 1; 
  //collect_buffer = buffer; 
  //collect_numberOfSamples = numberOfSamples;
  collect_sampleNum = 0;
		
  // Restart collection process 
  ADC_EnableT0ATrigger(); 

  return ADC_Status();
}

//---------------ADC_ConvertToVolts--------------
// Converts ADC sample to Volts
// Ouput: decimal fixed point value for volts
// Input: ADC conversion value
uint16_t ADC_ConvertToVolts(uint16_t value){ uint32_t fp_volts;
  fp_volts = (uint32_t)(((value * VREF) + ROUND_ADC_VALUE) / MAX_ADC_VALUE);
  return fp_volts;
}

//---------------ADCSeq3_Handler---------------
// ADCSeq3 ISR to store conversions for ADC collect**(maybe use semaphore?)
// Ouput: none
// Input: none
void ADC0Seq3_Handler(void){
 	ADC0_ISC_R = 0x08; // acknowledge ADC sequence 3 completion  
  (*ADCTask)(ADC0_SSFIFO3_R);
}