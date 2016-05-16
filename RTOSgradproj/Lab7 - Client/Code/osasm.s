;/*****************************************************************************/
; OSasm.s: low-level OS commands, written in assembly                       */
; Runs on LM4F120/TM4C123
; A very simple real time operating system with minimal features.
; Daniel Valvano
; January 29, 2015
;
; This example accompanies the book
;  "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
;  ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
;
;  Programs 4.4 through 4.12, section 4.2
;
;Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
;    You may use, edit, run or distribute this file
;    as long as the above copyright notice remains
; THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
; OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
; VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
; OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
; For more information about my classes, my research, and my books, see
; http://users.ece.utexas.edu/~valvano/
; */
PE1   EQU 0x40024008
        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8

        EXTERN  RunPt            ; Currently running thread
		EXTERN  NextThread		 ; Next desired thread (in case of removing from a list)
		EXTERN  OS_GetNextThread ; Gets the next thread to run for SysTick
        

; DISABLETIME CODE
        EXTERN  OS_UpdateTimeInfoOnEnable	; Run this on enable interrupts
        EXTERN  OS_UpdateTimeInfoOnDisable	; Run this on disable interrupts
		EXTERN  UpdateInterruptStatus
; DISABLETIME CODE 

        EXPORT  OS_DisableInterrupts
        EXPORT  OS_EnableInterrupts
		EXPORT  OS_StartCritical
		EXPORT 	OS_EndCritical
        EXPORT  StartOS
        EXPORT  SysTick_Handler
		EXPORT  PendSV_Handler

;*********** OS_StartCritical ************************
; make a copy of previous I bit, disable interrupts
; inputs:  none
; outputs: previous I bit
OS_StartCritical
        MRS    R0, PRIMASK  ; save old status
		PUSH   {R0}
        CPSID  I            ; mask all (except faults)
; DISABLETIME        
        PUSH    {LR}
        BL      OS_UpdateTimeInfoOnDisable
        POP     {LR}
		POP     {R0}
; DISABLETIME           
        BX     LR

;*********** OS_EndCritical ************************
; using the copy of previous I bit, restore I bit to previous value
; inputs:  previous I bit
; outputs: none
OS_EndCritical
		PUSH	{R0}
; DISABLETIME        
        PUSH    {LR}
;		CMP 	R0, #1			  ; Interrupts were already disabled
;		BEQ		SKIP0			  ; We are reenabling interrupts if we don't branch.
		BL 		OS_UpdateTimeInfoOnEnable
;SKIP0	BL      UpdateInterruptStatus
		POP 	{R0}
		PUSH	{R0}
		POP     {LR}		
		POP 	{R0}
		MSR		PRIMASK, R0
; DISABLETIME        
        BX     LR

OS_DisableInterrupts
        CPSID   I
; DISABLETIME        
        PUSH    {LR}
        BL      OS_UpdateTimeInfoOnDisable
        POP     {LR}
; DISABLETIME           
        BX      LR

OS_EnableInterrupts
; DISABLETIME        
        PUSH    {LR}
        BL      OS_UpdateTimeInfoOnEnable
		POP     {LR}
; DISABLETIME        
        CPSIE   I
        BX      LR
		
PendSV_Handler
	CPSID   I
	PUSH    {R4-R11}
	
	LDR		R0, =RunPt		   ;  R0 = &RunPt
	LDR     R1, [R0]		   ;  R1 = RunPt
	STR     SP, [R1]		   ;  Save SP into TCB
	LDR     R1, =NextThread    ;
	LDR     R1, [R1]           ;  R1 = NextThread
	STR		R1, [R0]           ;  RunPt = R1

    LDR     SP, [R1]           ;  new thread SP; SP = RunPt->sp;
    POP     {R4-R11}           ;  restore regs r4-11
    CPSIE   I                  ;  tasks run with interrupts enabled
    BX      LR                 ;  restore R0-R3,R12,LR,PC,PSR

SysTick_Handler
	CPSID 	I
	PUSH	{LR}
	BL		OS_GetNextThread
	POP		{LR}
	PUSH    {R4-R11}
	
	LDR		R1, =RunPt		   ;  R1 = &RunPt
	LDR     R2, [R1]		   ;  R2 = RunPt
	STR     SP, [R2]		   ;  Save SP into TCB
	;LDR     R2, [R0]		   ;  R2 = NextThread
	STR		R0, [R1]           ;  RunPt = R1

    LDR     SP, [R0]           ;  new thread SP; SP = RunPt->sp;
    POP     {R4-R11}           ;  restore regs r4-11
    CPSIE   I                  ;  tasks run with interrupts enabled
    BX      LR                 ;  restore R0-R3,R12,LR,PC,PSR

;SysTick_Handler                ; 1) Saves R0-R3,R12,LR,PC,PSR
    ;CPSID   I                  ; 2) Prevent interrupt during switch
    ;PUSH    {R4-R11}           ; 3) Save remaining regs r4-11
	
	;; Following is used for debug
	;;LDR 	R1, =PE1
	;;LDR	R0, [R1]
	;;EOR    R0, #0x02
	;;STR 	R0, [R1]
	;; enddebug
	
    ;LDR     R0, =RunPt         ; 4) R0=pointer to RunPt, old thread
    ;LDR     R1, [R0]           ;    R1 = RunPt
    ;STR     SP, [R1]           ; 5) Save SP into TCB
    ;LDR     R1, [R1,#4]        ; 6) R1 = RunPt->next
    ;STR     R1, [R0]           ;    RunPt = R1
    ;LDR     SP, [R1]           ; 7) new thread SP; SP = RunPt->sp;
    ;POP     {R4-R11}           ; 8) restore regs r4-11\\Lab2\os.c\Stacks[0][26]
    ;CPSIE   I                  ; 9) tasks run with interrupts enabled
    ;BX      LR                 ; 10) restore R0-R3,R12,LR,PC,PSR

StartOS
    LDR     R0, =RunPt         ; currently running thread
    LDR     R2, [R0]           ; R2 = value of RunPt
    LDR     SP, [R2]           ; new thread SP; SP = RunPt->stackPointer;
    POP     {R4-R11}           ; restore regs r4-11
    POP     {R0-R3}            ; restore regs r0-3
    POP     {R12}
    POP     {LR}               ; discard LR from initial stack
    POP     {LR}               ; start location
    POP     {R1}               ; discard PSR
    CPSIE   I                  ; Enable interrupts at processor level
    BX      LR                 ; start first thread

    ALIGN
    END
