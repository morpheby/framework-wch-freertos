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
 * Converted from portASM.S to C with top-level inline assembly for
 * GCC + Clang compatibility.
 *
 * This file implements the RISC-V FreeRTOS context switch, first-task
 * startup, initial stack setup, and (when FREERTOS_USE_ISP == 1) the
 * common interrupt dispatcher for CH32V20x (RV32).
 *
 * Chip-specific FPU register context save/restore is NOT supported.
 * If ARCH_FPU is enabled, a compile-time error is raised.
 *
 * Required compiler defines (passed via -D):
 *   FREERTOS_USE_ISP  — 0 or 1  (interrupt service provider mode)
 *   WCH_HPE_ENABLED   — 0 or 1  (hardware prologue/epilogue)
 */

/* ------------------------------------------------------------------ *
 * Chip-specific configuration
 * (mirrors freertos_risc_v_chip_specific_extensions.h)
 * ------------------------------------------------------------------ */

#ifndef portasmHAS_MTIME
#define portasmHAS_MTIME 0
#endif

#ifndef portasmHAS_SIFIVE_CLINT
#define portasmHAS_SIFIVE_CLINT 0
#endif

#ifndef ARCH_FPU
#define ARCH_FPU 0
#endif

#if ARCH_FPU
#error "portASM.c does not support FPU context save/restore. " \
       "Implement portasmSAVE/RESTORE_ADDITIONAL_REGISTERS manually."
#endif

/* ================================================================== *
 * Assembler constants and symbol declarations
 * ================================================================== */

__asm__(
    ".equ portWORD_SIZE, 4\n"
    ".equ portCONTEXT_SIZE, 30 * portWORD_SIZE\n"
    ".equ portasmADDITIONAL_CONTEXT_SIZE, 0\n"
    "\n"
    ".global xPortStartFirstTask\n"
    ".global SW_Handler\n"
    ".global pxPortInitialiseStack\n"
    "\n"
    ".extern pxCurrentTCB\n"
    ".extern vTaskSwitchContext\n"
    ".extern xISRStackTop\n"
);

#if defined(FREERTOS_USE_ISP) && (FREERTOS_USE_ISP == 1)
__asm__(
    ".global Global_IRQ_Handler\n"
    ".extern __MCU_Vectors\n"
    ".extern __MCU_Exceptions\n"
);
#endif

/* ================================================================== *
 * SW_Handler — FreeRTOS context switch (software interrupt handler)
 * ================================================================== */

__asm__(
    "\n"
    ".align 8\n"
    "SW_Handler:\n"

    /* ---- Save all general-purpose registers onto current task stack ---- */
    "    addi sp, sp, -portCONTEXT_SIZE\n"
    "    sw x1,  1 * portWORD_SIZE(sp)\n"     /* ra  */
    "    sw x5,  2 * portWORD_SIZE(sp)\n"     /* t0  */
    "    sw x6,  3 * portWORD_SIZE(sp)\n"     /* t1  */
    "    sw x7,  4 * portWORD_SIZE(sp)\n"     /* t2  */
    "    sw x8,  5 * portWORD_SIZE(sp)\n"     /* s0/fp */
    "    sw x9,  6 * portWORD_SIZE(sp)\n"     /* s1  */
    "    sw x10, 7 * portWORD_SIZE(sp)\n"     /* a0  */
    "    sw x11, 8 * portWORD_SIZE(sp)\n"     /* a1  */
    "    sw x12, 9 * portWORD_SIZE(sp)\n"     /* a2  */
    "    sw x13, 10 * portWORD_SIZE(sp)\n"    /* a3  */
    "    sw x14, 11 * portWORD_SIZE(sp)\n"    /* a4  */
    "    sw x15, 12 * portWORD_SIZE(sp)\n"    /* a5  */
    "    sw x16, 13 * portWORD_SIZE(sp)\n"    /* a6  */
    "    sw x17, 14 * portWORD_SIZE(sp)\n"    /* a7  */
    "    sw x18, 15 * portWORD_SIZE(sp)\n"    /* s2  */
    "    sw x19, 16 * portWORD_SIZE(sp)\n"    /* s3  */
    "    sw x20, 17 * portWORD_SIZE(sp)\n"    /* s4  */
    "    sw x21, 18 * portWORD_SIZE(sp)\n"    /* s5  */
    "    sw x22, 19 * portWORD_SIZE(sp)\n"    /* s6  */
    "    sw x23, 20 * portWORD_SIZE(sp)\n"    /* s7  */
    "    sw x24, 21 * portWORD_SIZE(sp)\n"    /* s8  */
    "    sw x25, 22 * portWORD_SIZE(sp)\n"    /* s9  */
    "    sw x26, 23 * portWORD_SIZE(sp)\n"    /* s10 */
    "    sw x27, 24 * portWORD_SIZE(sp)\n"    /* s11 */
    "    sw x28, 25 * portWORD_SIZE(sp)\n"    /* t3  */
    "    sw x29, 26 * portWORD_SIZE(sp)\n"    /* t4  */
    "    sw x30, 27 * portWORD_SIZE(sp)\n"    /* t5  */
    "    sw x31, 28 * portWORD_SIZE(sp)\n"    /* t6  */
    "\n"
    "    csrr t0, mstatus\n"                  /* Required for MPIE bit */
    "    sw t0, 29 * portWORD_SIZE(sp)\n"
    "\n"
    "    /* portasmSAVE_ADDITIONAL_REGISTERS — no-op (ARCH_FPU == 0) */\n"
    "\n"

    /* ---- Save stack pointer to current TCB ---- */
    "    lw  t0, pxCurrentTCB\n"              /* Load pxCurrentTCB */
    "    sw  sp, 0(t0)\n"                     /* Write sp to first TCB member */
    "\n"

    /* ---- Save exception return address ---- */
    "    csrr a1, mepc\n"
    "    sw a1, 0(sp)\n"                      /* Save updated exception return address */
    "\n"

    /* ---- Switch to ISR stack ---- */
    "    csrr  t1, mscratch\n"                /* Get known stack pointer for ISRs */
    "    csrw  mscratch, sp\n"                /* Swap to ISR stack */
    "    mv    sp, t1\n"
    "\n"
    "    jal vTaskSwitchContext\n"
    "\n"
    "    csrw  mscratch, sp\n"                /* Revert stack */
    "\n"

    /* ---- Restore context of the newly selected task ---- */
    "processed_source:\n"
    "    lw  t1, pxCurrentTCB\n"              /* Load pxCurrentTCB */
    "    lw  sp, 0(t1)\n"                     /* Read sp from first TCB member */
    "\n"
    "    lw t0, 0(sp)\n"                      /* Load mret address */
    "    csrw mepc, t0\n"
    "\n"
    "    /* portasmRESTORE_ADDITIONAL_REGISTERS — no-op (ARCH_FPU == 0) */\n"
    "\n"
#if defined(WCH_HPE_ENABLED) && (WCH_HPE_ENABLED != 0)
    "    addi a1, x0, 0x20\n"                /* Disable HPE for this mret */
    "    csrs 0x804, a1\n"                    /* Causes CPU to not pop last HPE stack */
#endif
    "\n"
    /* ---- Restore mstatus ---- */
    "    lw  t0, 29 * portWORD_SIZE(sp)\n"
    "    csrw mstatus, t0\n"                  /* Required for MPIE bit */
    "\n"

    /* ---- Restore all general-purpose registers ---- */
    "    lw  x1,  1 * portWORD_SIZE(sp)\n"    /* ra  */
    "    lw  x5,  2 * portWORD_SIZE(sp)\n"    /* t0  */
    "    lw  x6,  3 * portWORD_SIZE(sp)\n"    /* t1  */
    "    lw  x7,  4 * portWORD_SIZE(sp)\n"    /* t2  */
    "    lw  x8,  5 * portWORD_SIZE(sp)\n"    /* s0/fp */
    "    lw  x9,  6 * portWORD_SIZE(sp)\n"    /* s1  */
    "    lw  x10, 7 * portWORD_SIZE(sp)\n"    /* a0  */
    "    lw  x11, 8 * portWORD_SIZE(sp)\n"    /* a1  */
    "    lw  x12, 9 * portWORD_SIZE(sp)\n"    /* a2  */
    "    lw  x13, 10 * portWORD_SIZE(sp)\n"   /* a3  */
    "    lw  x14, 11 * portWORD_SIZE(sp)\n"   /* a4  */
    "    lw  x15, 12 * portWORD_SIZE(sp)\n"   /* a5  */
    "    lw  x16, 13 * portWORD_SIZE(sp)\n"   /* a6  */
    "    lw  x17, 14 * portWORD_SIZE(sp)\n"   /* a7  */
    "    lw  x18, 15 * portWORD_SIZE(sp)\n"   /* s2  */
    "    lw  x19, 16 * portWORD_SIZE(sp)\n"   /* s3  */
    "    lw  x20, 17 * portWORD_SIZE(sp)\n"   /* s4  */
    "    lw  x21, 18 * portWORD_SIZE(sp)\n"   /* s5  */
    "    lw  x22, 19 * portWORD_SIZE(sp)\n"   /* s6  */
    "    lw  x23, 20 * portWORD_SIZE(sp)\n"   /* s7  */
    "    lw  x24, 21 * portWORD_SIZE(sp)\n"   /* s8  */
    "    lw  x25, 22 * portWORD_SIZE(sp)\n"   /* s9  */
    "    lw  x26, 23 * portWORD_SIZE(sp)\n"   /* s10 */
    "    lw  x27, 24 * portWORD_SIZE(sp)\n"   /* s11 */
    "    lw  x28, 25 * portWORD_SIZE(sp)\n"   /* t3  */
    "    lw  x29, 26 * portWORD_SIZE(sp)\n"   /* t4  */
    "    lw  x30, 27 * portWORD_SIZE(sp)\n"   /* t5  */
    "    lw  x31, 28 * portWORD_SIZE(sp)\n"   /* t6  */
    "    addi sp, sp, portCONTEXT_SIZE\n"
    "\n"
    "    mret\n"
);

/* ================================================================== *
 * xPortStartFirstTask — launch the very first FreeRTOS task
 * ================================================================== */

__asm__(
    "\n"
    ".align 8\n"
    "xPortStartFirstTask:\n"
    "\n"
    "    lw t0, xISRStackTop\n"
    "\n"
#if defined(FREERTOS_USE_ISP) && (FREERTOS_USE_ISP == 0)
    "    addi t0, t0, -16\n"
#else
    /*
     * If it is an assembly entry code, the SP offset value is determined
     * by the assembly code, but the C code is determined by the compiler,
     * so we subtract 512 here as a reservation.  When entering the
     * interrupt function of C code, the compiler automatically presses
     * the stack into the task stack.  We can only change the SP value
     * used by the calling function after switching the interrupt stack.
     * This problem can be solved by modifying the interrupt to the
     * assembly entry, and there is no need to reserve 512 bytes.  You
     * only need to switch the interrupt stack at the beginning of the
     * interrupt function.
     */
    "    addi t0, t0, -512\n"
#endif
    "\n"
    "    csrw mscratch, t0\n"
    "\n"
    "    lw  sp, pxCurrentTCB\n"              /* Load pxCurrentTCB */
    "    lw  sp, 0(sp)\n"                     /* Read sp from first TCB member */
    "\n"
    /* Note: for starting the scheduler the exception return address is
     * used as the function return address. */
    "    lw  x1, 0(sp)\n"
    "\n"
    "    /* portasmRESTORE_ADDITIONAL_REGISTERS — no-op (ARCH_FPU == 0) */\n"
    "\n"
    "    lw  x6,  3 * portWORD_SIZE(sp)\n"    /* t1  */
    "    lw  x7,  4 * portWORD_SIZE(sp)\n"    /* t2  */
    "    lw  x8,  5 * portWORD_SIZE(sp)\n"    /* s0/fp */
    "    lw  x9,  6 * portWORD_SIZE(sp)\n"    /* s1  */
    "    lw  x10, 7 * portWORD_SIZE(sp)\n"    /* a0  */
    "    lw  x11, 8 * portWORD_SIZE(sp)\n"    /* a1  */
    "    lw  x12, 9 * portWORD_SIZE(sp)\n"    /* a2  */
    "    lw  x13, 10 * portWORD_SIZE(sp)\n"   /* a3  */
    "    lw  x14, 11 * portWORD_SIZE(sp)\n"   /* a4  */
    "    lw  x15, 12 * portWORD_SIZE(sp)\n"   /* a5  */
    "    lw  x16, 13 * portWORD_SIZE(sp)\n"   /* a6  */
    "    lw  x17, 14 * portWORD_SIZE(sp)\n"   /* a7  */
    "    lw  x18, 15 * portWORD_SIZE(sp)\n"   /* s2  */
    "    lw  x19, 16 * portWORD_SIZE(sp)\n"   /* s3  */
    "    lw  x20, 17 * portWORD_SIZE(sp)\n"   /* s4  */
    "    lw  x21, 18 * portWORD_SIZE(sp)\n"   /* s5  */
    "    lw  x22, 19 * portWORD_SIZE(sp)\n"   /* s6  */
    "    lw  x23, 20 * portWORD_SIZE(sp)\n"   /* s7  */
    "    lw  x24, 21 * portWORD_SIZE(sp)\n"   /* s8  */
    "    lw  x25, 22 * portWORD_SIZE(sp)\n"   /* s9  */
    "    lw  x26, 23 * portWORD_SIZE(sp)\n"   /* s10 */
    "    lw  x27, 24 * portWORD_SIZE(sp)\n"   /* s11 */
    "    lw  x28, 25 * portWORD_SIZE(sp)\n"   /* t3  */
    "    lw  x29, 26 * portWORD_SIZE(sp)\n"   /* t4  */
    "    lw  x30, 27 * portWORD_SIZE(sp)\n"   /* t5  */
    "    lw  x31, 28 * portWORD_SIZE(sp)\n"   /* t6  */
    "\n"
    "    lw  x5, 29 * portWORD_SIZE(sp)\n"    /* Initial mstatus into x5 (t0) */
    "    addi x5, x5, 0x08\n"                /* Set MIE bit — first task starts with interrupts enabled */
    "    csrrw x0, mstatus, x5\n"            /* Interrupts enabled from here! */
    "    lw  x5, 2 * portWORD_SIZE(sp)\n"     /* Initial x5 (t0) value */
    "\n"
    "    addi sp, sp, portCONTEXT_SIZE\n"
    "    ret\n"
);

/* ================================================================== *
 * pxPortInitialiseStack — set up the initial stack frame for a new task
 *
 * StackType_t *pxPortInitialiseStack(
 *     StackType_t   *pxTopOfStack,   // a0
 *     TaskFunction_t pxCode,          // a1
 *     void          *pvParameters );  // a2
 *
 * Returns new top of stack in a0.
 *
 * RISC-V register → ABI-name mapping (RV32I):
 *   x0  zero    x1  ra    x2  sp    x3  gp    x4  tp
 *   x5-7  t0-2          x8  s0/fp     x9  s1
 *   x10-11 a0-1          x12-17 a2-7
 *   x18-27 s2-11         x28-31 t3-6
 *
 * Stack frame layout (top = low address):
 *   pxCode (mepc)
 *   [chip-specific registers — zero for portasmADDITIONAL_CONTEXT_SIZE == 0]
 *   portTASK_RETURN_ADDRESS (x0 here)
 *   x5  x6  x7  x8  x9
 *   pvParameters (→ a0/x10)
 *   x11 .. x31
 *   mstatus
 * ================================================================== */

__asm__(
    "\n"
    ".align 8\n"
    "pxPortInitialiseStack:\n"
    "\n"
    "    csrr t0, mstatus\n"                  /* Obtain current mstatus value */
    "    andi t0, t0, ~0x8\n"                 /* Clear MIE — interrupts disabled when stack is restored within an ISR */
    "    addi t1, x0, 0x188\n"               /* Generate the MPIE | MPP bits to set in mstatus */
    "    slli t1, t1, 4\n"
    "    or t0, t0, t1\n"                     /* Set MPIE and MPP bits in mstatus value */
    "\n"
    "    addi a0, a0, -portWORD_SIZE\n"
    "    sw t0, 0(a0)\n"                      /* mstatus onto the stack */
    "    addi a0, a0, -(22 * portWORD_SIZE)\n" /* Space for registers x11-x31 */
    "    sw a2, 0(a0)\n"                      /* pvParameters → x10/a0 on the stack */
    "    addi a0, a0, -(6 * portWORD_SIZE)\n" /* Space for registers x5-x9 */
    "    sw x0, 0(a0)\n"                      /* Return address (could be portTASK_RETURN_ADDRESS) */
    "    addi t0, x0, portasmADDITIONAL_CONTEXT_SIZE\n"
    "chip_specific_stack_frame:\n"            /* Add any chip-specific registers to the stack frame */
    "    beq t0, x0, 1f\n"                    /* No more chip-specific registers to save */
    "    addi a0, a0, -portWORD_SIZE\n"       /* Make space for chip-specific register */
    "    sw x0, 0(a0)\n"                      /* Give the register an initial value of zero */
    "    addi t0, t0, -1\n"                   /* Decrement the count */
    "    j chip_specific_stack_frame\n"
    "1:\n"
    "    addi a0, a0, -portWORD_SIZE\n"
    "    sw a1, 0(a0)\n"                      /* mret value (pxCode parameter) onto the stack */
    "    ret\n"
);

/* ================================================================== *
 * Global_IRQ_Handler + call_irq  (only when FREERTOS_USE_ISP == 1)
 *
 * Common interrupt entry point that dispatches to the appropriate
 * handler from __MCU_Vectors (interrupts) or __MCU_Exceptions.
 * SW interrupt (mcause == 0x8000000E) is fast-pathed to SW_Handler.
 * ================================================================== */

#if defined(FREERTOS_USE_ISP) && (FREERTOS_USE_ISP == 1)

__asm__(
    "\n"
    ".align 8\n"
    ".cfi_sections .debug_frame\n"
    "Global_IRQ_Handler:\n"
    "    .cfi_startproc\n"
    "\n"
    "    addi  sp, sp, -4 * portWORD_SIZE\n"  /* Reserve space on current stack (16-byte aligned) */
    "    .cfi_def_cfa_offset 4 * portWORD_SIZE\n"
    "\n"
    "    sw fp, 0(sp)\n"                      /* Save fp  */
    "    sw t0, portWORD_SIZE(sp)\n"          /* Save t0  */
    "    sw t1, 2 * portWORD_SIZE(sp)\n"      /* Save t1  */
    "    sw t2, 3 * portWORD_SIZE(sp)\n"      /* Save t2  */
    "\n"
    "    .cfi_rel_offset fp,  0 * portWORD_SIZE\n"
    "    .cfi_rel_offset t0,  1 * portWORD_SIZE\n"
    "    .cfi_rel_offset t1,  2 * portWORD_SIZE\n"
    "    .cfi_rel_offset t2,  3 * portWORD_SIZE\n"
    "\n"
    "    csrr  t2, mcause\n"                  /* Read MCAUSE */
    "    li    t1, 0x8000000E\n"              /* SW interrupt */
    "    bne   t2, t1, process_irq\n"         /* If not SW interrupt, process normally */
    "\n"
    /* SW interrupt fast-path: restore saved regs and delegate to SW_Handler */
    "    lw  t0, portWORD_SIZE(sp)\n"         /* Restore t0 */
    "    lw  t1, 2 * portWORD_SIZE(sp)\n"     /* Restore t1 */
    "    lw  t2, 3 * portWORD_SIZE(sp)\n"     /* Restore t2 */
    "    addi sp, sp, 4 * portWORD_SIZE\n"    /* Restore SP */
    "    j   SW_Handler\n"                    /* Continue with SW_Handler */
    "\n"
    "process_irq:\n"
    "    li    t1, 0x80000000\n"              /* Highest bit set → interrupt, unset → exception */
    "    and   t0, t1, t2\n"                  /* t0 == 0 → exception, t0 != 0 → interrupt */
    "    beq   t0, x0, exception\n"           /* Go to exception handling */
    "\n"
    "    li    t1, 0x7FFFFFFF\n"              /* Mask to get interrupt number */
    "    and   t0, t2, t1\n"
    "    slli  t0, t0, 2\n"                   /* Each entry is 4 bytes */
    "    la    t2, __MCU_Vectors\n"
    "    add   t2, t0, t2\n"
    "    lw    t2, 0(t2)\n"                   /* Load interrupt handler address */
    "\n"
    "    j     call_irq\n"                    /* Jump to common handler */
    "\n"
    "exception:\n"
    "    and   t0, t2, t1\n"
    "    slli  t0, t0, 2\n"                   /* Each entry is 4 bytes */
    "    la    t2, __MCU_Exceptions\n"
    "    add   t2, t0, t2\n"
    "    lw    t2, 0(t2)\n"                   /* Load exception handler address */
    "\n"
    "    j     call_irq\n"                    /* Jump to common handler */
    "\n"
    "endirq:\n"
    "\n"
#if !(defined(WCH_HPE_ENABLED) && (WCH_HPE_ENABLED != 0))
    "    lw  t0, portWORD_SIZE(sp)\n"         /* Restore t0 */
    "    lw  t1, 2 * portWORD_SIZE(sp)\n"     /* Restore t1 */
    "    lw  t2, 3 * portWORD_SIZE(sp)\n"     /* Restore t2 */
#endif
    "\n"
    "    lw  fp, 0(sp)\n"                     /* Restore fp */
    "\n"
    "    addi sp, sp, 4 * portWORD_SIZE\n"    /* Restore SP back */
    "\n"
    "    mret\n"
    "\n"
    "    .cfi_endproc\n"
);

/* ------------------------------------------------------------------ *
 * call_irq — common IRQ dispatch (reached from Global_IRQ_Handler)
 * Handles ISR stack swap and calls the actual interrupt handler.
 * ------------------------------------------------------------------ */

__asm__(
    "\n"
    ".cfi_sections .debug_frame\n"
    "call_irq:\n"
    "    .cfi_startproc\n"
    "\n"
    "    mv    t0, x0\n"                      /* Zero t0 (no-swap flag) */
    "    csrr  t1, mscratch\n"                /* Get known stack pointer for ISRs */
    "    bgt   sp, t1, no_swap\n"             /* If current SP > ISR SP, nested interrupt — no swap */
    "\n"
    "    csrw  mscratch, sp\n"                /* Swap to ISR stack */
    "    mv    sp, t1\n"
    "    li    t0, 1\n"                       /* Indicate we did a stack swap */
    "\n"
    "    .cfi_rel_offset sp,  15 * portWORD_SIZE\n"
    "\n"
    "no_swap:\n"
    "\n"
#if defined(WCH_HPE_ENABLED) && (WCH_HPE_ENABLED != 0)

    /* ---- HPE path ---- */
    "    addi  sp, sp, -4 * portWORD_SIZE\n"  /* Reserve space on ISR stack */
    "    .cfi_def_cfa_offset 4 * portWORD_SIZE\n"
    "    addi  fp, sp, 4 * portWORD_SIZE\n"   /* Store frame pointer */
    "    sw x1, 3 * portWORD_SIZE(sp)\n"      /* Save return address */
    "\n"
    "    sw t0, 0 * portWORD_SIZE(sp)\n"      /* Save stack swap flag */

#else /* !WCH_HPE_ENABLED */

    /* ---- Non-HPE path: save all caller-saved registers ---- */
    "    addi  sp, sp, -16 * portWORD_SIZE\n" /* Reserve space on ISR stack */
    "    .cfi_def_cfa_offset 16 * portWORD_SIZE\n"
    "    addi  fp, sp, 16 * portWORD_SIZE\n"  /* Store frame pointer */
    "    .cfi_undefined fp\n"
    "    sw x1, 14 * portWORD_SIZE(sp)\n"     /* Save return address */
    "    .cfi_rel_offset x1, 14 * portWORD_SIZE\n"
    "    csrr  x1, mepc\n"
    "    sw x1, 13 * portWORD_SIZE(sp)\n"     /* Save exception return address */
    "    .cfi_rel_offset x1, 13 * portWORD_SIZE\n"
    "    csrr  t0, mscratch\n"
    "    addi  t0, t0, 4 * portWORD_SIZE\n"
    "    sw t0, 15 * portWORD_SIZE(sp)\n"     /* Save old stack pointer */
    "\n"
    "    sw t3, 0 * portWORD_SIZE(sp)\n"      /* Save t3 */
    "    .cfi_rel_offset t3,  0 * portWORD_SIZE\n"
    "    sw t4, 1 * portWORD_SIZE(sp)\n"      /* Save t4 */
    "    .cfi_rel_offset t4,  1 * portWORD_SIZE\n"
    "    sw t5, 2 * portWORD_SIZE(sp)\n"      /* Save t5 */
    "    .cfi_rel_offset t5,  2 * portWORD_SIZE\n"
    "    sw t6, 3 * portWORD_SIZE(sp)\n"      /* Save t6 */
    "    .cfi_rel_offset t6,  3 * portWORD_SIZE\n"
    "    sw a0, 4 * portWORD_SIZE(sp)\n"      /* Save a0 */
    "    .cfi_rel_offset a0,  4 * portWORD_SIZE\n"
    "    sw a1, 5 * portWORD_SIZE(sp)\n"      /* Save a1 */
    "    .cfi_rel_offset a1,  5 * portWORD_SIZE\n"
    "    sw a2, 6 * portWORD_SIZE(sp)\n"      /* Save a2 */
    "    .cfi_rel_offset a2,  6 * portWORD_SIZE\n"
    "    sw a3, 7 * portWORD_SIZE(sp)\n"      /* Save a3 */
    "    .cfi_rel_offset a3,  7 * portWORD_SIZE\n"
    "    sw a4, 8 * portWORD_SIZE(sp)\n"      /* Save a4 */
    "    .cfi_rel_offset a4,  8 * portWORD_SIZE\n"
    "    sw a5, 9 * portWORD_SIZE(sp)\n"      /* Save a5 */
    "    .cfi_rel_offset a5,  9 * portWORD_SIZE\n"
    "    sw a6, 10 * portWORD_SIZE(sp)\n"     /* Save a6 */
    "    .cfi_rel_offset a6,  10 * portWORD_SIZE\n"
    "    sw a7, 11 * portWORD_SIZE(sp)\n"     /* Save a7 */
    "    .cfi_rel_offset a7,  11 * portWORD_SIZE\n"
    "    sw t0, 12 * portWORD_SIZE(sp)\n"     /* Save stack swap flag */

#endif /* WCH_HPE_ENABLED */

    "\n"
    "    jalr  t2\n"                          /* Jump to interrupt/exception handler */
    "\n"

#if defined(WCH_HPE_ENABLED) && (WCH_HPE_ENABLED != 0)

    /* ---- HPE path: restore ---- */
    "    lw t0, 0 * portWORD_SIZE(sp)\n"      /* Load stack swap flag */
    "    lw x1, 3 * portWORD_SIZE(sp)\n"      /* Load return address */

#else /* !WCH_HPE_ENABLED */

    /* ---- Non-HPE path: restore caller-saved registers ---- */
    "    lw t3, 0 * portWORD_SIZE(sp)\n"      /* Load t3 */
    "    .cfi_restore t3\n"
    "    lw t4, 1 * portWORD_SIZE(sp)\n"      /* Load t4 */
    "    .cfi_restore t4\n"
    "    lw t5, 2 * portWORD_SIZE(sp)\n"      /* Load t5 */
    "    .cfi_restore t5\n"
    "    lw t6, 3 * portWORD_SIZE(sp)\n"      /* Load t6 */
    "    .cfi_restore t6\n"
    "    lw a0, 4 * portWORD_SIZE(sp)\n"      /* Load a0 */
    "    .cfi_restore a0\n"
    "    lw a1, 5 * portWORD_SIZE(sp)\n"      /* Load a1 */
    "    .cfi_restore a1\n"
    "    lw a2, 6 * portWORD_SIZE(sp)\n"      /* Load a2 */
    "    .cfi_restore a2\n"
    "    lw a3, 7 * portWORD_SIZE(sp)\n"      /* Load a3 */
    "    .cfi_restore a3\n"
    "    lw a4, 8 * portWORD_SIZE(sp)\n"      /* Load a4 */
    "    .cfi_restore a4\n"
    "    lw a5, 9 * portWORD_SIZE(sp)\n"      /* Load a5 */
    "    .cfi_restore a5\n"
    "    lw a6, 10 * portWORD_SIZE(sp)\n"     /* Load a6 */
    "    .cfi_restore a6\n"
    "    lw a7, 11 * portWORD_SIZE(sp)\n"     /* Load a7 */
    "    .cfi_restore a7\n"
    "    lw t0, 12 * portWORD_SIZE(sp)\n"     /* Load stack swap flag */
    "    lw x1, 14 * portWORD_SIZE(sp)\n"     /* Load return address */
    "    .cfi_restore x1\n"

#endif /* WCH_HPE_ENABLED */

    "\n"
    "    mv    sp, fp\n"                      /* Restore SP from frame pointer */
    "\n"
    "    beq   t0, x0, endirq\n"             /* If no stack swap was done, skip the swap back */
    "\n"
    "    csrr  t0, mscratch\n"                /* Save current stack pointer to t0 */
    "    csrw  mscratch, sp\n"                /* Swap back to normal stack */
    "    mv    sp, t0\n"
    "\n"
    "    j     endirq\n"
    "\n"
    "    .cfi_endproc\n"
);

#endif /* FREERTOS_USE_ISP */
