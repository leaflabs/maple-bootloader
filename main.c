#include "maple_lib.h"

/* overflow the weak definition in c_only_startup.c */
void USB_LP_CAN1_RX0_IRQHandler(void) {
  usbISTR();  
}

void TIM2_IRQHandler(void) {
}

int main (void) {

  systemReset();
  setupCLK();
  setupLED();
  setupUSB();

  resetPin (GPIOC,12);

  strobePin (GPIOA,5,5,0x50000); /* start indicator */

  if (checkUserCode(USER_CODE_RAM)) {
    jumpToUser(USER_CODE_RAM);
  }
  
  while (1) {
  }

}
