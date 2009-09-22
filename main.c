#include "maple_lib.h"


int main (void) {
  systemReset();
  setupCLK();
  setupLED();

  u32 c = 0;
  while (1) {
    for (c=0;c<1000000;c++){}
    SET_REG(GPIO_BSRR(GPIOA),0x00200000);
    for (c=0;c<1000000;c++){}
    SET_REG(GPIO_BSRR(GPIOA),0x00000020);
  }
}
