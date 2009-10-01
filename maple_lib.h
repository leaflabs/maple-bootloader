#ifndef __MAPLE_LIB_H
#define __MAPLE_LIB_H

/* ST Libs (stripped) */
#include "stm32f10x_type.h" /* defines simple types (u32, vu16, etc) */
#include "cortexm3_macro.h" /* provides asm instruction macros */
#include "usb_lib.h"

/* Local Includes */
//#include "maple_usb.h"
//#include "maple_dfu.h"

/* macro'd register and peripheral definitions */
#define RCC   0x40021000
#define FLASH 0x40022000
#define GPIOA 0x40010800
#define GPIOC 0x40011000

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

#define SCS  = 0xE000E000
#define NVIC = SCS+0x100
#define SCB  = SCS+0xD00

/* todo, wrap in do whiles for the natural ; */
#define SET_REG(addr,val) *(vu32*)(addr)=val
#define GET_REG(addr)     *(vu32*)(addr)

/* exposed library structs */
typedef struct {
  vu32 ISER[2];
  u32  RESERVED0[30];
  vu32 ICER[2];
  u32  RSERVED1[30];
  vu32 ISPR[2];
  u32  RESERVED2[30];
  vu32 ICPR[2];
  u32  RESERVED3[30];
  vu32 IABR[2];
  u32  RESERVED4[62];
  vu32 IPR[15];
} NVIC_Typedef;

typedef struct
{
  u8 NVIC_IRQChannel;
  u8 NVIC_IRQChannelPreemptionPriority;
  u8 NVIC_IRQChannelSubPriority;
  bool NVIC_IRQChannelCmd; /* TRUE for enable */
} NVIC_InitTypeDef;

typedef struct
{
  vuc32 CPUID;
  vu32 ICSR;
  vu32 VTOR;
  vu32 AIRCR;
  vu32 SCR;
  vu32 CCR;
  vu32 SHPR[3];
  vu32 SHCSR;
  vu32 CFSR;
  vu32 HFSR;
  vu32 DFSR;
  vu32 MMFAR;
  vu32 BFAR;
  vu32 AFSR;
} SCB_TypeDef;

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

void nvicInit(NVIC_InitTypeDef*);







#endif
