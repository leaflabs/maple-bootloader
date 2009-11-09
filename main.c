#include "maple_lib.h"

/* overflow the weak definition in c_only_startup.c */
void USB_LP_CAN1_RX0_IRQHandler(void) {
    usbISTR();  
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

int main (void) {
    systemReset();
    setupCLK();
    setupLED();
    setupUSB();
    
    strobePin (GPIOA,5,5,0x50000); /* start indicator */

    if (checkUserCode(USER_CODE_RAM)) {
        jumpToUser(USER_CODE_RAM);
    }

    while (1) {
      /* hack to perform the dfu write operation AFTER weve responded 
	 to the host on the bus */
      if (copyLock) {
	dfuCopyBufferToExec();
	copyLock = FALSE;
	dfuAppStatus.bwPollTimeout0 = 0x00;
	dfuAppStatus.bState = dfuDNLOAD_SYNC;
      }
    }

}
