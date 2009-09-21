#ifndef __MAPLE_LIB_H
#define __MAPLE_LIB_H

#include "stm32f10x_type.h" /* defines simple types (u32, vu16, etc) */
#include "cortexm3_macro.h" /* provides asm instruction macros */

void setPin    (u8 bank, u8 pin);
void resetPin  (u8 bank, u8 pin);
void strobePin (u8 bank, u8 pin, u32 rate);

void setupPFB      (void);
void setupPLL      (u8 mult);
void setupUSB      (void);
void setupLED      (void);
bool checkUserCode (u32 jmpAddr);
void jumpToUser    (u32 jmpAddr);

void flashErasePage (u16 pageNum);
s8   flashWritePage (u8 pageNum, u8 *srcBuf);
void flashEraseAll  (void);
void flashUnlock    (void);
void flashBlock     (void);  /* waits until flash is ready */
s8   flashGetStatus (void);

/* macro'd register and peripheral definitions */


#endif
