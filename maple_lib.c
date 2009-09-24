#include "maple_lib.h"

void setPin(u32 bank, u8 pin) {
  u32 pinMask = 0x1 << (pin);
  SET_REG(GPIO_BSRR(bank),pinMask);
}

void resetPin(u32 bank, u8 pin) {
  u32 pinMask = 0x1 << (16+pin);
  SET_REG(GPIO_BSRR(bank),pinMask);
}

void strobePin(u32 bank, u8 pin, u8 count, u32 rate) {
  resetPin(bank,pin);

  u32 c;
  while (count-- >0) {
    for (c=rate;c>0;c--);
    setPin(bank,pin);
    for (c=rate;c>0;c--);
    resetPin(bank,pin);
  } 
} 

void systemReset(void) {
  SET_REG(RCC_CR, GET_REG(RCC_CR)     | 0x00000001);
  SET_REG(RCC_CFGR, GET_REG(RCC_CFGR) & 0xF8FF0000);
  SET_REG(RCC_CR, GET_REG(RCC_CR)     & 0xFEF6FFFF);
  SET_REG(RCC_CR, GET_REG(RCC_CR)     & 0xFFFBFFFF);
  SET_REG(RCC_CFGR, GET_REG(RCC_CFGR) & 0xFF80FFFF);

  SET_REG(RCC_CIR, 0x00000000);  /* disable all interrupts */
}

void setupCLK (void) {
  /* enable HSE */
  SET_REG(RCC_CR,GET_REG(RCC_CR) | 0x00010001);
  while ((GET_REG(RCC_CR) & 0x00020000) == 0); /* for it to come on */
  
  /* enable flash prefetch buffer */
  SET_REG(FLASH_ACR, 0x00000012);

  /* unlock the flash */
  SET_REG(FLASH_KEYR,0x45670123);
  SET_REG(FLASH_KEYR,0xCDEF89AB);

  /* Configure PLL */
  SET_REG(RCC_CFGR,GET_REG(RCC_CFGR) | 0x001D0400);  /* pll=72Mhz,APB1=36Mhz,AHB=72Mhz */
  SET_REG(RCC_CR,GET_REG(RCC_CR)     | 0x01000000);  /* enable the pll */
  while ((GET_REG(RCC_CR) & 0x03000000) == 0);         /* wait for it to come on */
  
  /* Set SYSCLK as PLL */
  SET_REG(RCC_CFGR,GET_REG(RCC_CFGR) | 0x00000002);
  while ((GET_REG(RCC_CFGR) & 0x00000008) == 0); /* wait for it to come on */
}

void setupLED (void) {
  /* Setup APB2 (GPIOA) */
  SET_REG(RCC_APB2ENR,0x00000004);

  /* Setup GPIOA Pin 5 as PP Out */
  SET_REG(GPIO_CRL(GPIOA), 0x00100000);
  setPin(GPIOA,5);
}

bool checkUserCode (u32 usrAddr) {
  u32 sp = *(vu32*) usrAddr;

  if ((sp & 0x2FFE0000) == 0x20000000) {
    return (TRUE);
  } else {
    return (FALSE);
  }
}

void jumpToUser (u32 usrAddr) {
  typedef void (*funcPtr)(void);

  u32 jumpAddr = *(vu32*) (usrAddr + 0x04); /* reset ptr in vector table */  
  funcPtr usrMain = (funcPtr) jumpAddr;

  __MSR_MSP(*(vu32*) usrAddr);              /* set the users stack ptr */
  usrMain();                                /* go! */
}
