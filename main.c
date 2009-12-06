#include "maple_lib.h"

/* overflow the weak definition in c_only_startup.c */
void USB_LP_CAN1_RX0_IRQHandler(void) {
    usbISTR();  

    /* perry stub */
    if (code_copy_lock == BEGINNING) {
      /* dont return from this ISR to user code! jump to 
	 the checkFlashLoop!
	 - either access LR and PC and MSP directly or
	 simply modify the return address that was pushed
	 on the stack at exception entry and let the nvic
	 do the right thing.
      */
    }
}

void NMI_Handler(void) {
    while (1) {
        strobePin(GPIOA,5,50,0x40000);
        strobePin(GPIOA,5,1,0xF0000);
    }
}

void HardFault_Handler(void) {
    while (1) {
        strobePin(GPIOA,5,50,0x40000);
        strobePin(GPIOA,5,2,0xF0000);
    }
}

void MemManage_Handler(void) {
    while (1) {
        strobePin(GPIOA,5,50,0x40000);
        strobePin(GPIOA,5,3,0xF0000);
    }
}

void BusFault_Handler(void) {
    while (1) {
        strobePin(GPIOA,5,50,0x40000);
        strobePin(GPIOA,5,4,0xF0000);
    }
}

void UsageFault_Handler(void) {
    while (1) {
        strobePin(GPIOA,5,50,0x40000);
        strobePin(GPIOA,5,5,0xF0000);
    }
}

void checkFlashLoop() {
  /* trap ourselves in a loop that checks the 
     global flash operation flag */
  while (1) {
    if (code_copy_lock==BEGINNING) {
      code_copy_lock = MIDDLE;
      dfuCopyBufferToExec();
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

    if (checkUserCode(USER_CODE_RAM)) {
      jumpToUser(USER_CODE_RAM);
    }

    /* consider adding a long pause to allow for escaping a potentially unescapable junmp */
    if (checkUserCode(USER_CODE_FLASH)) {
      strobePin (GPIOA,5,3,0x100000); /* start indicator */
      jumpToUser(USER_CODE_FLASH);
    }

    checkFlashLoop();
}
