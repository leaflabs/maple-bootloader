#include "maple_lib.h"


int main (void) {
  systemReset();
  setupCLK();
  setupLED();

  
  strobePin (GPIOA,5,3,0x50000); /* start indicator */

/*   u32 testUsrSP = 0xFFFFFFFF; */
/*   u32 *testUsrAddr = &testUsrSP; */

/*   if (checkUserCode(testUsrAddr)) { */
/*     strobePin (GPIOA,5,3,0x100000); */
/*     setPin (GPIOA,5); */
/*   } else { */
/*     strobePin (GPIOA,5,3,0x300000); */
/*   } */

/*   testUsrSP = 0x20000500; */
/*   if (checkUserCode(0x00)) { */
/*     strobePin (GPIOA,5,6,0x50000); */
/*     setPin (GPIOA,5); */
/*     jumpToUser(0x00); */
/*   } */

  while (1) {}
  


}
