/*
    This file is part of AutoQuad.

    AutoQuad is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AutoQuad is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with AutoQuad.  If not, see <http://www.gnu.org/licenses/>.

    Copyright © 2011, 2012  Bill Nesbitt
*/

#include "fpu.h"
#include <CoOS.h>
#include <OsTask.h>

uint32_t fpuRegisters[CFG_MAX_USER_TASKS*33] __attribute__((section(".ccm")));
#ifdef FPU_LAZY_SWITCH
uint8_t fpuCurrentTask;
#endif

// setup system to do lazy FPU context switching triggered by the NOCP UsageFault exception
void fpuInit(void) {
    FPU->FPCCR &= ~FPU_FPCCR_ASPEN_Msk;			    // turn off FP context save
    FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk;			    // turn off lazy save
#ifdef FPU_LAZY_SWITCH
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;		    // turn on usage fault interrupt
    CoreDebug->DEMCR &= ~ CoreDebug_DEMCR_VC_NOCPERR_Msk;   // disable debug halt on UsageFault caused by an access to the FPU Coprocessor
    __fpu_disable();					    // disable FPU access
#endif
}

#ifdef FPU_LAZY_SWITCH
void UsageFault_Handler(void) {
    // test USGFAULTACT flag && Usage Fault Status Register == NOCP
//    if ((SCB->SHCSR & SCB_SHCSR_USGFAULTACT_Msk) && (SCB->CFSR & SCB_CFSR_USGFAULTSR_Msk) == 0x80000) {
	__fpu_enable();
	// only swap FPU context if taskID has changed
	if (fpuCurrentTask != TCBRunning->taskID) {
	    __vfp_store(&fpuRegisters[33*fpuCurrentTask]);
	    fpuCurrentTask = TCBRunning->taskID;
	    __vfp_restore(&fpuRegisters[33*fpuCurrentTask]);
	}
	SCB->CFSR = 0x80000;		// clear NOCP flag
//    }
}
#endif
