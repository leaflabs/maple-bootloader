#include "maple_lib.h"

#define EXC_RETURN 0xFFFFFFF9
#define DEFAULT_CPSR 0x61000000
#define STACK_TOP 0x20005000

void checkFlashLoop();

/* overflow the weak definition in c_only_startup.c */
void USB_LP_CAN1_RX0_IRQHandler(void) {
    unsigned int target;
    usbISTR();

    /* could smash the stack when we absolutely needed to, but
     for now, do this whenever the dfu state is not appIDLE */
    if (dfuGetState() == appDETACH) {
      /* disable any interfering interrupts (not USB though!) */
      nvicDisableInterrupts();
      usbEnbISR();

      target =  (unsigned int)checkFlashLoop | 0x1;
      asm volatile("mov r0, %[stack_top]      \n\t"             // Reset the stack
		   "mov sp, r0                \n\t"
		   "mov r0, #1                \n\t"
		   "mov r1, %[target_addr]    \n\t"
		   "mov r2, %[cpsr]           \n\t"
		   "push {r2}                 \n\t"             // Fake xPSR
		   "push {r1}                 \n\t"             // Target address for PC
		   "push {r0}                 \n\t"             // Fake LR
		   "push {r0}                 \n\t"             // Fake R12
		   "push {r0}                 \n\t"             // Fake R3
		   "push {r0}                 \n\t"             // Fake R2
		   "push {r0}                 \n\t"             // Fake R1
		   "push {r0}                 \n\t"             // Fake R0
		   "mov lr, %[exc_return]     \n\t"
		   "bx lr"
		   :
		   : [stack_top] "r" (STACK_TOP),
		     [target_addr] "r" (target),
		     [exc_return] "r" (EXC_RETURN),
		     [cpsr] "r" (DEFAULT_CPSR)
		   : "r0", "r1", "r2");
      // Should never get here.
    }
}

void NMI_Handler(void) {
//    while (1) {
//        strobePin(GPIOA,5,50,0x40000);
//        strobePin(GPIOA,5,1,0xF0000);
//    }
}

void HardFault_Handler(void) {
//    while (1) {
//        strobePin(GPIOA,5,50,0x40000);
//        strobePin(GPIOA,5,2,0xF0000);
//    }
}

void MemManage_Handler(void) {
//    while (1) {
//        strobePin(GPIOA,5,50,0x40000);
//        strobePin(GPIOA,5,3,0xF0000);
//    }
}

void BusFault_Handler(void) {
//    while (1) {
//        strobePin(GPIOA,5,50,0x40000);
//        strobePin(GPIOA,5,4,0xF0000);
//    }
}

void UsageFault_Handler(void) {
//    while (1) {
//        strobePin(GPIOA,5,50,0x40000);
//        strobePin(GPIOA,5,5,0xF0000);
//    }
}

void checkFlashLoop() {
  /* trap ourselves in a loop that checks the 
     global flash operation flag */
  while (1) {
    if (code_copy_lock == BEGINNING) {
      code_copy_lock = MIDDLE;
      strobePin(GPIOA,5,2,0x1000);
      dfuCopyBufferToExec();
      strobePin(GPIOA,5,2,0x500);
      code_copy_lock = END;
    }
  }
}

int main (void) {
    systemReset();
    setupCLK();
    setupLED();
    setupUSB();
    setupFLASH(); /* here for debug, move to dfu */

    strobePin (GPIOA,5,5,0x50000); /* start indicator */


#if COM_ENB
    char* testMsg = "0123456789ABCDE\n";
    UserToPMABufferCopy(testMsg,ENDP1_TXADDR,0xF);
    SetEPTxCount(ENDP1,0xF);
    SetEPTxValid(ENDP1);
#endif

#if 1
    if (checkUserCode(USER_CODE_RAM)) {
      char* ramMsg = "Jumping to Ram!\n";
      UserToPMABufferCopy(ramMsg,ENDP1_TXADDR,0x10);
      SetEPTxCount(ENDP1,0x10);
      SetEPTxValid(ENDP1);
      jumpToUser(USER_CODE_RAM);
    }

    /* consider adding a long pause to allow for escaping a potentially unescapable junmp */
    if (checkUserCode(USER_CODE_FLASH)) {
      strobePin (GPIOA,5,3,0x100000); /* start indicator */

      char* flashMsg = "Jumping to Flash\n";
      UserToPMABufferCopy(flashMsg,ENDP1_TXADDR,0x11);
      SetEPTxCount(ENDP1,0x10);
      SetEPTxValid(ENDP1);

      jumpToUser(USER_CODE_FLASH);
    }
#endif

    while(1) {
        asm volatile("nop");
    }
//    checkFlashLoop();
}
