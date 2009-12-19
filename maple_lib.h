#ifndef __MAPLE_LIB_H
#define __MAPLE_LIB_H

/* ST Libs (stripped) */
#include "stm32f10x_type.h" /* defines simple types (u32, vu16, etc) */
#include "maple_types.h"
#include "cortexm3_macro.h" /* provides asm instruction macros */
#include "usb_lib.h"

/* Local Includes */
#include "maple_usb.h"
#include "maple_dfu.h"
#include "maple_regs.h"


/* any global setting macros */
#define F_SUSPEND_ENABLED 1
#define USER_CODE_RAM     ((u32)0x20000C00)
#define USER_CODE_FLASH   ((u32)0x08005000)

#define MAJOR_REV_NUMBER  0
#define MINOR_REV_NUMBER  1

extern MAPLE_VectorTable mapleVectTable;

void setPin    (u32 bank, u8 pin);
void resetPin  (u32 bank, u8 pin);
void strobePin (u32 bank, u8 pin, u8 count, u32 rate);
void strobeCode (u32 bank, u8 pin, u8 val);

void systemHardReset(void);
void systemReset   (void);
void setupCLK      (void);
void setupUSB      (void);
void setupLED      (void);
void setupFLASH    (void);
void setupTimer    (void);
bool checkUserCode (u32 usrAddr);
void jumpToUser    (u32 usrAddr);

bool flashWriteWord (u32 addr, u32 word);
bool flashErasePage (u32 addr);
bool flashErasePages (u32 addr, u16 n);
void flashLock      (void);
void flashUnlock    (void);
void nvicInit(NVIC_InitTypeDef*);
void nvicDisableInterrupts();







#endif
