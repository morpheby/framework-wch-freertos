/*
 * FreeRTOS Kernel V10.4.6
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * The FreeRTOS kernel's RISC-V port is split between the the code that is
 * common across all currently supported RISC-V chips (implementations of the
 * RISC-V ISA), and code which tailors the port to a specific RISC-V chip:
 *
 * + The code that is common to all RISC-V chips is implemented in
 *   FreeRTOS\Source\portable\GCC\RISC-V-RV32\portASM.S.  There is only one
 *   portASM.S file because the same file is used no matter which RISC-V chip is
 *   in use.
 *
 * + The code that tailors the kernel's RISC-V port to a specific RISC-V
 *   chip is implemented in freertos_risc_v_chip_specific_extensions.h.  There
 *   is one freertos_risc_v_chip_specific_extensions.h that can be used with any
 *   RISC-V chip that both includes a standard CLINT and does not add to the
 *   base set of RISC-V registers.  There are additional
 *   freertos_risc_v_chip_specific_extensions.h files for RISC-V implementations
 *   that do not include a standard CLINT or do add to the base set of RISC-V
 *   registers.
 *
 * CARE MUST BE TAKEN TO INCLDUE THE CORRECT
 * freertos_risc_v_chip_specific_extensions.h HEADER FILE FOR THE CHIP
 * IN USE.  To include the correct freertos_risc_v_chip_specific_extensions.h
 * header file ensure the path to the correct header file is in the assembler's
 * include path.
 *
 * This freertos_risc_v_chip_specific_extensions.h is for use on RISC-V chips
 * that include a standard CLINT and do not add to the base set of RISC-V
 * registers.
 *
 */
#if __riscv_xlen == 64
	#define portWORD_SIZE 8
	#define store_x sd
	#define load_x ld
#elif __riscv_xlen == 32
	#define store_x sw
	#define load_x lw
	#define portWORD_SIZE 4
#else
	#error Assembler did not define __riscv_xlen
#endif

/* we define float registers at specific_extensions.h, V30x support hardware floating point unit
can be enable by defined ARCH_FPU*/
#include "freertos_risc_v_chip_specific_extensions.h"


/* Check the freertos_risc_v_chip_specific_extensions.h and/or command line
definitions. */
#if defined( portasmHAS_CLINT ) && defined( portasmHAS_MTIME )
	#error The portasmHAS_CLINT constant has been deprecated.  Please replace it with portasmHAS_MTIME.  portasmHAS_CLINT and portasmHAS_MTIME cannot both be defined at once.  See https://www.FreeRTOS.org/Using-FreeRTOS-on-RISC-V.html
#endif

#ifdef portasmHAS_CLINT
	#warning The portasmHAS_CLINT constant has been deprecated.  Please replace it with portasmHAS_MTIME and portasmHAS_SIFIVE_CLINT.  For now portasmHAS_MTIME and portasmHAS_SIFIVE_CLINT are derived from portasmHAS_CLINT.  See https://www.FreeRTOS.org/Using-FreeRTOS-on-RISC-V.html
	#define portasmHAS_MTIME portasmHAS_CLINT
	#define portasmHAS_SIFIVE_CLINT portasmHAS_CLINT
#endif

#ifndef portasmHAS_MTIME
	#error freertos_risc_v_chip_specific_extensions.h must define portasmHAS_MTIME to either 1 (MTIME clock present) or 0 (MTIME clock not present).  See https://www.FreeRTOS.org/Using-FreeRTOS-on-RISC-V.html
#endif

#ifndef portasmHANDLE_INTERRUPT /* don't need this define */
#	#error portasmHANDLE_INTERRUPT must be defined to the function to be called to handle external/peripheral interrupts.  portasmHANDLE_INTERRUPT can be defined on the assembler command line or in the appropriate freertos_risc_v_chip_specific_extensions.h header file.  https://www.FreeRTOS.org/Using-FreeRTOS-on-RISC-V.html
#endif

#ifndef portasmHAS_SIFIVE_CLINT
	#define portasmHAS_SIFIVE_CLINT 0
#endif

/* Only the standard core registers are stored by default.  Any additional
registers must be saved by the portasmSAVE_ADDITIONAL_REGISTERS and
portasmRESTORE_ADDITIONAL_REGISTERS macros - which can be defined in a chip
specific version of freertos_risc_v_chip_specific_extensions.h.  See the notes
at the top of this file. */
#define portCONTEXT_SIZE ( 30 * portWORD_SIZE )

.global xPortStartFirstTask
.global SW_Handler
.global pxPortInitialiseStack

.extern pxCurrentTCB
.extern vTaskSwitchContext
.extern xISRStackTop


#if defined( FREERTOS_USE_ISP ) && ( FREERTOS_USE_ISP == 1 )
.global Global_IRQ_Handler
.extern __MCU_Vectors
.extern __MCU_Exceptions
#endif


/*-----------------------------------------------------------*/
.align 8
.func
SW_Handler:
	addi sp, sp, -portCONTEXT_SIZE
	store_x x1, 1 * portWORD_SIZE( sp )
	store_x x5, 2 * portWORD_SIZE( sp )
	store_x x6, 3 * portWORD_SIZE( sp )
	store_x x7, 4 * portWORD_SIZE( sp )
	store_x x8, 5 * portWORD_SIZE( sp )
	store_x x9, 6 * portWORD_SIZE( sp )
	store_x x10, 7 * portWORD_SIZE( sp )
	store_x x11, 8 * portWORD_SIZE( sp )
	store_x x12, 9 * portWORD_SIZE( sp )
	store_x x13, 10 * portWORD_SIZE( sp )
	store_x x14, 11 * portWORD_SIZE( sp )
	store_x x15, 12 * portWORD_SIZE( sp )
	store_x x16, 13 * portWORD_SIZE( sp )
	store_x x17, 14 * portWORD_SIZE( sp )
	store_x x18, 15 * portWORD_SIZE( sp )
	store_x x19, 16 * portWORD_SIZE( sp )
	store_x x20, 17 * portWORD_SIZE( sp )
	store_x x21, 18 * portWORD_SIZE( sp )
	store_x x22, 19 * portWORD_SIZE( sp )
	store_x x23, 20 * portWORD_SIZE( sp )
	store_x x24, 21 * portWORD_SIZE( sp )
	store_x x25, 22 * portWORD_SIZE( sp )
	store_x x26, 23 * portWORD_SIZE( sp )
	store_x x27, 24 * portWORD_SIZE( sp )
	store_x x28, 25 * portWORD_SIZE( sp )
	store_x x29, 26 * portWORD_SIZE( sp )
	store_x x30, 27 * portWORD_SIZE( sp )
	store_x x31, 28 * portWORD_SIZE( sp )

	csrr t0, mstatus					/* Required for MPIE bit. */
	store_x t0, 29 * portWORD_SIZE( sp )

	portasmSAVE_ADDITIONAL_REGISTERS	/* Defined in freertos_risc_v_chip_specific_extensions.h to save any registers unique to the RISC-V implementation. */

	load_x  t0, pxCurrentTCB			/* Load pxCurrentTCB. */
	store_x  sp, 0( t0 )				/* Write sp to first TCB member. */

	csrr a1, mepc
	store_x a1, 0( sp )					/* Save updated exception return address. */

	csrr	t1, mscratch				/* Get known stack pointer for ISRs */
	csrw	mscratch, sp				/* Swap to ISR stack */
	mv		sp, t1

	jal vTaskSwitchContext

	csrw	mscratch, sp				/* Revert stack */

processed_source:
	load_x  t1, pxCurrentTCB			/* Load pxCurrentTCB. */
	load_x  sp, 0( t1 )				 	/* Read sp from first TCB member. */

	/* Load mret with the address of the next instruction in the task to run next. */
	load_x t0, 0( sp )
	csrw mepc, t0

	portasmRESTORE_ADDITIONAL_REGISTERS	/* Defined in freertos_risc_v_chip_specific_extensions.h to restore any registers unique to the RISC-V implementation. */

#if defined( WCH_HPE_ENABLED ) && ( WCH_HPE_ENABLED != 0 )
	addi a1, x0, 0x20                   /* Disable HPE for this mret. */
	csrs 0x804, a1						/* This causes the CPU to not pop last HPE stack, instead just erasing in */
#endif

	/* Load mstatus with the interrupt enable bits used by the task. */
	load_x  t0, 29 * portWORD_SIZE( sp )
	csrw mstatus, t0						/* Required for MPIE bit. */

	load_x  x1, 1 * portWORD_SIZE( sp )
	load_x  x5, 2 * portWORD_SIZE( sp )		/* t0 */
	load_x  x6, 3 * portWORD_SIZE( sp )		/* t1 */
	load_x  x7, 4 * portWORD_SIZE( sp )		/* t2 */
	load_x  x8, 5 * portWORD_SIZE( sp )		/* s0/fp */
	load_x  x9, 6 * portWORD_SIZE( sp )		/* s1 */
	load_x  x10, 7 * portWORD_SIZE( sp )	/* a0 */
	load_x  x11, 8 * portWORD_SIZE( sp )	/* a1 */
	load_x  x12, 9 * portWORD_SIZE( sp )	/* a2 */
	load_x  x13, 10 * portWORD_SIZE( sp )	/* a3 */
	load_x  x14, 11 * portWORD_SIZE( sp )	/* a4 */
	load_x  x15, 12 * portWORD_SIZE( sp )	/* a5 */
	load_x  x16, 13 * portWORD_SIZE( sp )	/* a6 */
	load_x  x17, 14 * portWORD_SIZE( sp )	/* a7 */
	load_x  x18, 15 * portWORD_SIZE( sp )	/* s2 */
	load_x  x19, 16 * portWORD_SIZE( sp )	/* s3 */
	load_x  x20, 17 * portWORD_SIZE( sp )	/* s4 */
	load_x  x21, 18 * portWORD_SIZE( sp )	/* s5 */
	load_x  x22, 19 * portWORD_SIZE( sp )	/* s6 */
	load_x  x23, 20 * portWORD_SIZE( sp )	/* s7 */
	load_x  x24, 21 * portWORD_SIZE( sp )	/* s8 */
	load_x  x25, 22 * portWORD_SIZE( sp )	/* s9 */
	load_x  x26, 23 * portWORD_SIZE( sp )	/* s10 */
	load_x  x27, 24 * portWORD_SIZE( sp )	/* s11 */
	load_x  x28, 25 * portWORD_SIZE( sp )	/* t3 */
	load_x  x29, 26 * portWORD_SIZE( sp )	/* t4 */
	load_x  x30, 27 * portWORD_SIZE( sp )	/* t5 */
	load_x  x31, 28 * portWORD_SIZE( sp )	/* t6 */
	addi sp, sp, portCONTEXT_SIZE

	mret
	.endfunc
/*-----------------------------------------------------------*/

.align 8
.func
xPortStartFirstTask:

	lw t0, xISRStackTop

#if defined( FREERTOS_USE_ISP ) && ( FREERTOS_USE_ISP == 0 )
	addi t0, t0, -16
#else
	/* if it is an assembly entry code, the SP offset value is determined by the assembly code,
	but the C code is determined by the compiler, so we subtract 512 here as a reservation.
	When entering the interrupt function of C code, the compiler automatically presses the stack
	into the task stack. We can only change the SP value used by the calling function after switching
	the interrupt stack.This problem can be solved by modifying the interrupt to the assembly entry,
	and there is no need to reserve 512 bytes. You only need to switch the interrupt stack at the
	beginning of the interrupt function */
	addi t0, t0, -512
#endif

    csrw mscratch,t0

	load_x  sp, pxCurrentTCB			/* Load pxCurrentTCB. */
	load_x  sp, 0( sp )				 	/* Read sp from first TCB member. */

	load_x  x1, 0( sp ) /* Note for starting the scheduler the exception return address is used as the function return address. */

	portasmRESTORE_ADDITIONAL_REGISTERS	/* Defined in freertos_risc_v_chip_specific_extensions.h to restore any registers unique to the RISC-V implementation. */

	load_x  x6, 3 * portWORD_SIZE( sp )		/* t1 */
	load_x  x7, 4 * portWORD_SIZE( sp )		/* t2 */
	load_x  x8, 5 * portWORD_SIZE( sp )		/* s0/fp */
	load_x  x9, 6 * portWORD_SIZE( sp )		/* s1 */
	load_x  x10, 7 * portWORD_SIZE( sp )	/* a0 */
	load_x  x11, 8 * portWORD_SIZE( sp )	/* a1 */
	load_x  x12, 9 * portWORD_SIZE( sp )	/* a2 */
	load_x  x13, 10 * portWORD_SIZE( sp )	/* a3 */
	load_x  x14, 11 * portWORD_SIZE( sp )	/* a4 */
	load_x  x15, 12 * portWORD_SIZE( sp )	/* a5 */
	load_x  x16, 13 * portWORD_SIZE( sp )	/* a6 */
	load_x  x17, 14 * portWORD_SIZE( sp )	/* a7 */
	load_x  x18, 15 * portWORD_SIZE( sp )	/* s2 */
	load_x  x19, 16 * portWORD_SIZE( sp )	/* s3 */
	load_x  x20, 17 * portWORD_SIZE( sp )	/* s4 */
	load_x  x21, 18 * portWORD_SIZE( sp )	/* s5 */
	load_x  x22, 19 * portWORD_SIZE( sp )	/* s6 */
	load_x  x23, 20 * portWORD_SIZE( sp )	/* s7 */
	load_x  x24, 21 * portWORD_SIZE( sp )	/* s8 */
	load_x  x25, 22 * portWORD_SIZE( sp )	/* s9 */
	load_x  x26, 23 * portWORD_SIZE( sp )	/* s10 */
	load_x  x27, 24 * portWORD_SIZE( sp )	/* s11 */
	load_x  x28, 25 * portWORD_SIZE( sp )	/* t3 */
	load_x  x29, 26 * portWORD_SIZE( sp )	/* t4 */
	load_x  x30, 27 * portWORD_SIZE( sp )	/* t5 */
	load_x  x31, 28 * portWORD_SIZE( sp )	/* t6 */

	load_x  x5, 29 * portWORD_SIZE( sp )	/* Initial mstatus into x5 (t0) */
	addi x5, x5, 0x08						/* Set MIE bit so the first task starts with interrupts enabled - required as returns with ret not eret. */
	csrrw  x0, mstatus, x5					/* Interrupts enabled from here! */
	load_x  x5, 2 * portWORD_SIZE( sp )		/* Initial x5 (t0) value. */

	addi	sp, sp, portCONTEXT_SIZE
	ret
	.endfunc
/*-----------------------------------------------------------*/

/*
 * Unlike other ports pxPortInitialiseStack() is written in assembly code as it
 * needs access to the portasmADDITIONAL_CONTEXT_SIZE constant.  The prototype
 * for the function is as per the other ports:
 * StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters );
 *
 * As per the standard RISC-V ABI pxTopcOfStack is passed in in a0, pxCode in
 * a1, and pvParameters in a2.  The new top of stack is passed out in a0.
 *
 * RISC-V maps registers to ABI names as follows (X1 to X31 integer registers
 * for the 'I' profile, X1 to X15 for the 'E' profile, currently I assumed).
 *
 * Register		ABI Name	Description						Saver
 * x0			zero		Hard-wired zero					-
 * x1			ra			Return address					Caller
 * x2			sp			Stack pointer					Callee
 * x3			gp			Global pointer					-
 * x4			tp			Thread pointer					-
 * x5-7			t0-2		Temporaries						Caller
 * x8			s0/fp		Saved register/Frame pointer	Callee
 * x9			s1			Saved register					Callee
 * x10-11		a0-1		Function Arguments/return values Caller
 * x12-17		a2-7		Function arguments				Caller
 * x18-27		s2-11		Saved registers					Callee
 * x28-31		t3-6		Temporaries						Caller
 *
 * The RISC-V context is saved t FreeRTOS tasks in the following stack frame,
 * where the global and thread pointers are currently assumed to be constant so
 * are not saved:
 *
 * mstatus
 * x31
 * x30
 * x29
 * x28
 * x27
 * x26
 * x25
 * x24
 * x23
 * x22
 * x21
 * x20
 * x19
 * x18
 * x17
 * x16
 * x15
 * x14
 * x13
 * x12
 * x11
 * pvParameters
 * x9
 * x8
 * x7
 * x6
 * x5
 * portTASK_RETURN_ADDRESS
 * [chip specific registers go here]
 * pxCode
 */
.align 8
.func
pxPortInitialiseStack:

	csrr t0, mstatus					/* Obtain current mstatus value. */
	andi t0, t0, ~0x8					/* Ensure interrupts are disabled when the stack is restored within an ISR.  Required when a task is created after the schedulre has been started, otherwise interrupts would be disabled anyway. */
	addi t1, x0, 0x188					/* Generate the value 0x7880, which are the MPIE, MPP and FS bits to set in mstatus. */
	slli t1, t1, 4
	or t0, t0, t1						/* Set MPIE and MPP bits in mstatus value. */

	addi a0, a0, -portWORD_SIZE
	store_x t0, 0(a0)					/* mstatus onto the stack. */
	addi a0, a0, -(22 * portWORD_SIZE)	/* Space for registers x11-x31. */
	store_x a2, 0(a0)					/* Task parameters (pvParameters parameter) goes into register X10/a0 on the stack. */
	addi a0, a0, -(6 * portWORD_SIZE)	/* Space for registers x5-x9. */
	store_x x0, 0(a0)					/* Return address onto the stack, could be portTASK_RETURN_ADDRESS */
	addi t0, x0, portasmADDITIONAL_CONTEXT_SIZE /* The number of chip specific additional registers. */
chip_specific_stack_frame:				/* First add any chip specific registers to the stack frame being created. */
	beq t0, x0, 1f						/* No more chip specific registers to save. */
	addi a0, a0, -portWORD_SIZE			/* Make space for chip specific register. */
	store_x x0, 0(a0)					/* Give the chip specific register an initial value of zero. */
	addi t0, t0, -1						/* Decrement the count of chip specific registers remaining. */
	j chip_specific_stack_frame			/* Until no more chip specific registers. */
1:
	addi a0, a0, -portWORD_SIZE
	store_x a1, 0(a0)					/* mret value (pxCode parameter) onto the stack. */
	ret
	.endfunc

#if defined( FREERTOS_USE_ISP ) && ( FREERTOS_USE_ISP == 1 )
.align 8
.cfi_sections .debug_frame
.func Global_IRQ_Handler
Global_IRQ_Handler:
	.cfi_startproc

	addi	sp, sp, -4 * portWORD_SIZE	/* Reserve space on current stack. Alignment is 16 bytes */
	.cfi_def_cfa_offset 4 * portWORD_SIZE
	
	store_x fp, 0( sp )					/* Save fp on the stack. */
	store_x t0, portWORD_SIZE( sp )		/* Save t0 on the stack. */
	store_x t1, 2 * portWORD_SIZE( sp )	/* Save t1 on the stack. */
	store_x t2, 3 * portWORD_SIZE( sp )	/* Save t2 on the stack. */

	.cfi_rel_offset fp,  0*portWORD_SIZE
	.cfi_rel_offset t0,  1*portWORD_SIZE
	.cfi_rel_offset t1,  2*portWORD_SIZE
	.cfi_rel_offset t2,  3*portWORD_SIZE

	csrr	t2, mcause					/* Read MCAUSE */
	li		t1, 0x8000000E				/* SW interrupt */
	bne		t2, t1, process_irq			/* If not SW interrupt, process normally */

	load_x	t0, portWORD_SIZE( sp )		/* Restore t0, t1, t2 back */
	load_x	t1, 2 * portWORD_SIZE( sp )
	load_x	t2, 3 * portWORD_SIZE( sp )
	addi	sp, sp, 4 * portWORD_SIZE	/* Restore SP back */
	j		SW_Handler					/* Continue with SW_Handler */

process_irq:
	li 		t1, 0x80000000				/* Highest bit set means interrupt, unset is exception */
	and		t0, t1, t2					/* t0 == 0 means exception, t0 != 0 means interrupt */
	beq		t0, x0, exception			/* Go to exception handling */

	li      t1, 0x7FFFFFFF				/* Mask to get interrupt number */
	and     t0, t2, t1
	slli    t0, t0, 2					/* Each entry is 4 bytes */
	la      t2, __MCU_Vectors
	add     t2, t0, t2
	lw		t2, 0( t2 )					/* Load interrupt handler address */

	j		call_irq					/* Jump to common handler */

exception:
	and     t0, t2, t1
	slli    t0, t0, 2					/* Each entry is 4 bytes */
	la      t2, __MCU_Exceptions
	add     t2, t0, t2
	lw		t2, 0( t2 )					/* Load exception handler address */
	
	j		call_irq					/* Jump to common handler */

endirq:

#if !(defined( WCH_HPE_ENABLED ) && ( WCH_HPE_ENABLED != 0 ))
	load_x	t0, portWORD_SIZE( sp )		/* Restore t0, t1, t2 back */
	load_x	t1, 2 * portWORD_SIZE( sp )
	load_x	t2, 3 * portWORD_SIZE( sp )
#endif

	load_x	fp, 0( sp )					/* Load fp from the stack. */

	addi	sp, sp, 4 * portWORD_SIZE	/* Restore SP back */

	mret

.cfi_endproc
.endfunc

.cfi_sections .debug_frame
.func call_irq
call_irq:
	.cfi_startproc
	.cfi_val_encoded_addr ra, 0, endirq

	mv		t0, x0						/* Zero t0 */
	csrr	t1, mscratch				/* Get known stack pointer for ISRs */
	bgt		sp, t1, no_swap				/* If current SP > ISR SP, no need to swap.
										   ISR stack is always above normal stack, so if SP > ISR SP,
										   it means we are in a nested interrupt */
	
	csrw	mscratch, sp				/* Swap to ISR stack */
	mv		sp, t1
	li		t0, 1						/* Indicate we did a stack swap */
	
	.cfi_rel_offset sp,  15*portWORD_SIZE

no_swap:

#if defined( WCH_HPE_ENABLED ) && ( WCH_HPE_ENABLED != 0 )
	addi	sp, sp, -4 * portWORD_SIZE	/* Reserve space on ISR stack */
	.cfi_def_cfa_offset 4 * portWORD_SIZE
	addi	fp, sp, 4 * portWORD_SIZE	/* Store frame pointer */
	store_x x1, 3 * portWORD_SIZE( sp )	/* Save return address */

	store_x	t0, 0 * portWORD_SIZE( sp )	/* Save stack swap flag */
#else
	addi	sp, sp, -16 * portWORD_SIZE	/* Reserve space on ISR stack */
	.cfi_def_cfa_offset 16 * portWORD_SIZE
	addi	fp, sp, 16 * portWORD_SIZE	/* Store frame pointer */
	.cfi_undefined fp
	store_x x1, 14 * portWORD_SIZE( sp )/* Save return address */
	.cfi_rel_offset x1,  14*portWORD_SIZE
	csrr	x1, mepc
	store_x x1, 13 * portWORD_SIZE( sp )/* Save exception return address */
	.cfi_rel_offset x1,  13*portWORD_SIZE
	csrr    t0, mscratch
	addi	t0, t0, 4 * portWORD_SIZE
	store_x t0, 15 * portWORD_SIZE( sp )/* Save old stack pointer */

	store_x	t3, 0 * portWORD_SIZE( sp )	/* Save t3 */
	.cfi_rel_offset t3,  0*portWORD_SIZE
	store_x	t4, 1 * portWORD_SIZE( sp )	/* Save t4 */
	.cfi_rel_offset t4,  1*portWORD_SIZE
	store_x	t5, 2 * portWORD_SIZE( sp )	/* Save t5 */
	.cfi_rel_offset t5,  2*portWORD_SIZE
	store_x	t6, 3 * portWORD_SIZE( sp )	/* Save t6 */
	.cfi_rel_offset t6,  3*portWORD_SIZE
	store_x	a0, 4 * portWORD_SIZE( sp )	/* Save a0 */
	.cfi_rel_offset a0,  4*portWORD_SIZE
	store_x	a1, 5 * portWORD_SIZE( sp )	/* Save a1 */
	.cfi_rel_offset a1,  5*portWORD_SIZE
	store_x	a2, 6 * portWORD_SIZE( sp )	/* Save a2 */
	.cfi_rel_offset a2,  6*portWORD_SIZE
	store_x	a3, 7 * portWORD_SIZE( sp )	/* Save a3 */
	.cfi_rel_offset a3,  7*portWORD_SIZE
	store_x	a4, 8 * portWORD_SIZE( sp )	/* Save a4 */
	.cfi_rel_offset a4,  8*portWORD_SIZE
	store_x	a5, 9 * portWORD_SIZE( sp )	/* Save a5 */
	.cfi_rel_offset a5,  9*portWORD_SIZE
	store_x	a6, 10 * portWORD_SIZE( sp )/* Save a6 */
	.cfi_rel_offset a6,  10*portWORD_SIZE
	store_x	a7, 11 * portWORD_SIZE( sp )/* Save a7 */
	.cfi_rel_offset a7,  11*portWORD_SIZE
	store_x	t0, 12 * portWORD_SIZE( sp )/* Save stack swap flag */
#endif

	jalr	t2							/* Jump to interrupt/exception handler */

#if defined( WCH_HPE_ENABLED ) && ( WCH_HPE_ENABLED != 0 )
	load_x	t0, 0 * portWORD_SIZE( sp )	/* Load stack swap flag */
	load_x	x1, 3 * portWORD_SIZE( sp )	/* Load return address */
#else
	load_x	t3, 0 * portWORD_SIZE( sp )	/* Load t3 */
	.cfi_restore t3
	load_x	t4, 1 * portWORD_SIZE( sp )	/* Load t4 */
	.cfi_restore t4
	load_x	t5, 2 * portWORD_SIZE( sp )	/* Load t5 */
	.cfi_restore t5
	load_x	t6, 3 * portWORD_SIZE( sp )	/* Load t6 */
	.cfi_restore t6
	load_x	a0, 4 * portWORD_SIZE( sp )	/* Load a0 */
	.cfi_restore a0
	load_x	a1, 5 * portWORD_SIZE( sp )	/* Load a1 */
	.cfi_restore a1
	load_x	a2, 6 * portWORD_SIZE( sp )	/* Load a2 */
	.cfi_restore a2
	load_x	a3, 7 * portWORD_SIZE( sp )	/* Load a3 */
	.cfi_restore a3
	load_x	a4, 8 * portWORD_SIZE( sp )	/* Load a4 */
	.cfi_restore a4
	load_x	a5, 9 * portWORD_SIZE( sp )	/* Load a5 */
	.cfi_restore a5
	load_x	a6, 10 * portWORD_SIZE( sp )/* Load a6 */
	.cfi_restore a6
	load_x	a7, 11 * portWORD_SIZE( sp )/* Load a7 */
	.cfi_restore a7
	load_x	t0, 12 * portWORD_SIZE( sp )/* Load stack swap flag */
	load_x	x1, 14 * portWORD_SIZE( sp )/* Load return address */
	.cfi_restore x1
#endif

	mv		sp, fp						/* Restore SP from frame pointer */

	beq		t0, x0, endirq				/* If no stack swap was done, skip the swap back */

	csrr	t0, mscratch				/* Save current stack pointer to t0 */
	csrw	mscratch, sp				/* Swap back to normal stack */
	mv		sp, t0

	j		endirq

.cfi_endproc
.endfunc
#endif
