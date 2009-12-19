#include "maple_lib.h"

#define EXC_RETURN 0xFFFFFFF9
#define DEFAULT_CPSR 0x61000000
#define STACK_TOP 0x20005000

MAPLE_VectorTable mapleVectTable = 
{
  NULL, /* serial tx cb */
  NULL, /* serial rx cb */
  NULL, /* serial linecoding cb */
  &vcom_count_in,
  &vcom_count_out,
  vcom_buffer_out,
  &linecoding,
  (u8) MAJOR_REV_NUMBER,
  (u8) MINOR_REV_NUMBER,
  (void*)(&usb_master_device),
  NULL, /* reserved 0 */
  NULL, /* reserved 1 */
  NULL, /* reserved 2 */
  NULL  /* reserved 3 */
};

void checkFlashLoop();

/* overflow the weak definition in c_only_startup.c */
void USB_LP_CAN1_RX0_IRQHandler(void) {
    unsigned int target;
    usbISTR();

    /* could smash the stack when we absolutely needed to, but
     for now, do this whenever the dfu state is not appIDLE */
    if (dfuGetState() == appDETACH) {
      /* move the vector table back! */

      nvicDisableInterrupts();
      SET_REG(SCB_VTOR,0x0);

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
      code_copy_lock=MIDDLE;
      strobePin(GPIOA,5,2,0x1000);
      dfuCopyBufferToExec();
      strobePin(GPIOA,5,2,0x500);
      code_copy_lock = END;
    }
  }
}

int main (void) {
#if COMM_ENB
  char* waitMsg = "Waiting for Code...\n";
  char* jumpRam = "Jumping to Ram\n";
  char* jumpFlash = "Jumping to Flash\n";
#endif

  u32 count=0;

    systemReset();
    setupCLK();
    setupLED();
    setupUSB();
    setupFLASH(); /* here for debug, move to dfu */

    strobePin (GPIOA,5,5,0x50000); /* start indicator */


    if (checkUserCode(USER_CODE_RAM)) {
#if COMM_ENB      
      UserToPMABufferCopy((u8*)jumpRam,ENDP1_TXADDR,0xF);
      SetEPTxCount(ENDP1,0xF);
      SetEPTxValid(ENDP1);
#endif
      strobePin (GPIOA,5,3,0x100000); /* start indicator */
      jumpToUser(USER_CODE_RAM);
    }

    /* consider adding a long pause to allow for escaping a potentially unescapable junmp */
    if (checkUserCode(USER_CODE_FLASH)) {
#if COMM_ENB
      UserToPMABufferCopy((u8*)jumpFlash,ENDP1_TXADDR,0x11);
      SetEPTxCount(ENDP1,0x11);
      SetEPTxValid(ENDP1);
#endif
      strobePin (GPIOA,5,5,0x100000); /* start indicator */
      jumpToUser(USER_CODE_FLASH);

    }

#if COMM_ENB
    if (count++%10000000 == 0) {
      UserToPMABufferCopy((u8*)waitMsg,ENDP1_TXADDR,0x14);
      SetEPTxCount(ENDP1,0x14);
      SetEPTxValid(ENDP1);
    } 
#endif 
     
    while(1) {
      asm volatile("nop");
    }
//    checkFlashLoop();
}
