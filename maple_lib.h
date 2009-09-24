#ifndef __MAPLE_LIB_H
#define __MAPLE_LIB_H

#include "stm32f10x_type.h" /* defines simple types (u32, vu16, etc) */
#include "cortexm3_macro.h" /* provides asm instruction macros */
#include "usb_lib.h"

void setPin    (u32 bank, u8 pin);
void resetPin  (u32 bank, u8 pin);
void strobePin (u32 bank, u8 pin, u8 count, u32 rate);

void systemReset   (void);
void setupCLK      (void);
void setupUSB      (void);
void setupLED      (void);
bool checkUserCode (u32 usrAddr);
void jumpToUser    (u32 usrAddr);

void flashErasePage (u16 pageNum);
s8   flashWritePage (u8 pageNum, u8 *srcBuf);
void flashEraseAll  (void);
void flashUnlock    (void);
void flashBlock     (void);  /* waits until flash is ready */
s8   flashGetStatus (void);

/* todo, wrap in do whiles for the natural ; */
#define SET_REG(addr,val) *(vu32*)(addr)=val
#define GET_REG(addr)     *(vu32*)(addr)


/* macro'd register and peripheral definitions */
#define RCC   0x40021000
#define FLASH 0x40022000
#define GPIOA 0x40010800
#define CPIOC 0x40011000

#define RCC_CR      RCC
#define RCC_CFGR    RCC + 0x04
#define RCC_CIR     RCC + 0x08
#define RCC_AHBENR  RCC + 0x14
#define RCC_APB2ENR RCC + 0x18
#define RCC_APB1ENR RCC + 0x16

#define FLASH_KEYR FLASH + 0x04
#define FLASH_ACR  FLASH + 0x00

#define GPIO_CRL(port)  port
#define GPIO_CRH(port)  port+0x04
#define GPIO_ODR(port)  port+0x0c
#define GPIO_BSRR(port) port+0x10



#endif
